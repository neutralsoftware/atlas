#include "editor/views/animationGraphEditor.h"
#include "atlas/animation.h"

#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStringList>
#include <utility>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {

const QStringList kinds{"float", "bool", "trigger"};
const QStringList operators{"==", "!=", "<", ">", "<=", ">="};

std::filesystem::path filePath(const QString &path) {
    return std::filesystem::path(path.toStdU16String());
}

QString text(const QTableWidget &table, int row, int column) {
    const auto *item = table.item(row, column);
    return item ? item->text().trimmed() : QString{};
}

void writeRow(QTableWidget &table, int row, const QStringList &values) {
    for (int i = 0; i < values.size(); ++i)
        table.setItem(row, i, new QTableWidgetItem(values[i]));
}

float number(const QString &value) {
    bool valid = false;
    const float result = value.toFloat(&valid);
    if (!valid || !std::isfinite(result))
        throw std::invalid_argument("Invalid number: " + value.toStdString());
    return result;
}

bool boolean(const QString &value) {
    if (value == "true") return true;
    if (value == "false") return false;
    throw std::invalid_argument("Enter true or false: " + value.toStdString());
}

class StateNode : public QGraphicsRectItem {
  public:
    explicit StateNode(const QString &label, bool initial)
        : QGraphicsRectItem(0, 0, 184, 52) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setBrush(initial ? QColor("#245e55") : QColor("#303c51"));
        setPen(QPen(initial ? QColor("#74d5b5") : QColor("#7186a5")));
        auto *title = new QGraphicsSimpleTextItem(label, this);
        title->setBrush(QColor("#f4f7ff"));
        title->setPos(9, 6);
    }

    std::function<void(QPointF)> moved;

  protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        if (change == ItemPositionHasChanged && moved)
            moved(pos());
        return QGraphicsRectItem::itemChange(change, value);
    }
};

class AnimationGraphDialog : public QDialog {
  public:
    explicit AnimationGraphDialog(QWidget *parent) : QDialog(parent) {
        setWindowTitle("Animation State Machine");
        resize(1060, 730);
        auto *layout = new QVBoxLayout(this);
        auto *toolbar = new QHBoxLayout;
        auto *open = new QPushButton("Open…", this);
        auto *save = new QPushButton("Save…", this);
        toolbar->addWidget(open);
        toolbar->addWidget(save);
        toolbar->addSpacing(16);
        toolbar->addWidget(new QLabel("Initial state", this));
        initial = new QComboBox(this);
        toolbar->addWidget(initial);
        toolbar->addStretch();
        layout->addLayout(toolbar);

        auto *splitter = new QSplitter(Qt::Vertical, this);
        auto *view = new QGraphicsView(&scene, splitter);
        view->setRenderHint(QPainter::Antialiasing);
        view->setBackgroundBrush(QColor("#1b2230"));
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        auto *tabs = new QTabWidget(splitter);
        states = addTable(*tabs, "States", {"Name", "Clip name", "Loop", "Speed", "X", "Y"},
                         {"State", "Clip", "true", "1", "0", "0"});
        states->setColumnHidden(4, true);
        states->setColumnHidden(5, true);
        connect(states, &QTableWidget::currentCellChanged, this, [this](int row, int, int, int) {
            if (row >= 0 && row < nodes.size()) {
                scene.clearSelection();
                nodes[row]->setSelected(true);
            }
        });
        parameters = addTable(*tabs, "Parameters", {"Name", "Kind (float / bool / trigger)", "Initial value"},
                              {"parameter", "float", "0"});
        transitions = addTable(*tabs, "Transitions",
                               {"From", "To", "Fade (seconds)", "Wait for end", "Conditions"},
                               {"", "", "0.2", "false", ""},
                               "Example: speed > 0.1 && grounded == 1. Put higher priority transitions first.");
        splitter->addWidget(view);
        splitter->addWidget(tabs);
        splitter->setSizes({420, 260});
        layout->addWidget(splitter, 1);

        connect(states, &QTableWidget::cellChanged, this, [this] { redraw(); });
        connect(transitions, &QTableWidget::cellChanged, this, [this] { redraw(); });
        connect(initial, &QComboBox::currentTextChanged, this, [this] { redraw(); });
        connect(open, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, "Open Animation Graph", currentPath, "Animation graphs (*.json)");
            if (path.isEmpty()) return;
            perform([&] {
                populate(AnimationStateMachine::load(filePath(path)));
                currentPath = path;
            });
        });
        connect(save, &QPushButton::clicked, this, [this] {
            QString path = QFileDialog::getSaveFileName(
                this, "Save Animation Graph", currentPath, "Animation graphs (*.json)");
            if (path.isEmpty()) return;
            if (!path.endsWith(".json", Qt::CaseInsensitive))
                path += ".json";
            perform([&] {
                readTables().save(filePath(path));
                currentPath = path;
            });
        });
        redraw();
    }

  private:
    QTableWidget *addTable(QTabWidget &tabs, const QString &name,
                           const QStringList &headers, const QStringList &defaults,
                           const QString &hint = {}) {
        auto *page = new QWidget(&tabs);
        auto *layout = new QVBoxLayout(page);
        auto *table = new QTableWidget(0, static_cast<int>(headers.size()), page);
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        layout->addWidget(table);
        if (!hint.isEmpty())
            layout->addWidget(new QLabel(hint, page));
        auto *buttons = new QHBoxLayout;
        auto *add = new QPushButton("Add", page);
        auto *remove = new QPushButton("Remove", page);
        auto *up = new QPushButton("Move Up", page);
        buttons->addWidget(add);
        buttons->addWidget(remove);
        if (name == "Transitions")
            buttons->addWidget(up);
        else
            up->hide();
        buttons->addStretch();
        layout->addLayout(buttons);
        tabs.addTab(page, name);
        connect(add, &QPushButton::clicked, this, [this, table, defaults] {
            const int row = table->rowCount();
            QStringList values = defaults;
            if (table == states) {
                values[0] = QString("State %1").arg(row + 1);
                values[4] = QString::number((row % 4) * 224);
                values[5] = QString::number((row / 4) * 110);
            }
            {
                const QSignalBlocker blocker(table);
                table->insertRow(row);
                writeRow(*table, row, values);
            }
            redraw();
            table->setCurrentCell(row, 0);
            table->editItem(table->item(row, 0));
        });
        connect(remove, &QPushButton::clicked, this, [this, table] {
            const int row = table->currentRow();
            if (row >= 0) {
                table->removeRow(row);
                redraw();
            }
        });
        connect(up, &QPushButton::clicked, this, [this, table] {
            const int row = table->currentRow();
            if (row < 1) return;
            {
                const QSignalBlocker blocker(table);
                for (int col = 0; col < table->columnCount(); ++col) {
                    auto *previous = table->takeItem(row - 1, col);
                    table->setItem(row - 1, col, table->takeItem(row, col));
                    table->setItem(row, col, previous);
                }
            }
            table->setCurrentCell(row - 1, 0);
            redraw();
        });
        return table;
    }

    void perform(const std::function<void()> &action) {
        try {
            action();
        } catch (const std::exception &error) {
            QMessageBox::warning(this, "Animation State Machine", QString::fromUtf8(error.what()));
        }
    }

    AnimationStateMachine readTables() const {
        AnimationStateMachine result;
        result.initialState = initial->currentText().toStdString();
        for (int row = 0; row < states->rowCount(); ++row) {
            result.states.push_back({text(*states, row, 0).toStdString(),
                text(*states, row, 1).toStdString(), boolean(text(*states, row, 2)),
                number(text(*states, row, 3)), number(text(*states, row, 4)),
                number(text(*states, row, 5))});
        }
        for (int row = 0; row < parameters->rowCount(); ++row) {
            const auto kind = kinds.indexOf(text(*parameters, row, 1));
            if (kind < 0)
                throw std::invalid_argument("Parameter kind must be float, bool, or trigger");
            result.parameters.push_back({text(*parameters, row, 0).toStdString(),
                static_cast<AnimationParameterKind>(kind), number(text(*parameters, row, 2))});
        }
        const QRegularExpression conditionPattern(
            R"(^\s*(.+?)\s*(==|!=|<=|>=|<|>)\s*(\S+)\s*$)");
        for (int row = 0; row < transitions->rowCount(); ++row) {
            AnimationTransition transition{text(*transitions, row, 0).toStdString(),
                text(*transitions, row, 1).toStdString(), number(text(*transitions, row, 2)),
                boolean(text(*transitions, row, 3)), {}};
            const auto conditions = text(*transitions, row, 4);
            if (!conditions.isEmpty()) {
                for (const auto &expression : conditions.split("&&")) {
                    const auto match = conditionPattern.match(expression);
                    if (!match.hasMatch())
                        throw std::invalid_argument("Invalid condition: " + expression.toStdString());
                    transition.conditions.push_back({match.captured(1).trimmed().toStdString(),
                        static_cast<AnimationComparison>(operators.indexOf(match.captured(2))),
                        number(match.captured(3))});
                }
            }
            result.transitions.push_back(std::move(transition));
        }
        return result;
    }

    void populate(const AnimationStateMachine &machine) {
        {
            const QSignalBlocker blockStates(states), blockParameters(parameters),
                blockTransitions(transitions), blockInitial(initial);
            states->setRowCount(static_cast<int>(machine.states.size()));
            parameters->setRowCount(static_cast<int>(machine.parameters.size()));
            transitions->setRowCount(static_cast<int>(machine.transitions.size()));
            int row = 0;
            initial->clear();
            for (const auto &s : machine.states) {
                const auto name = QString::fromStdString(s.name);
                initial->addItem(name);
                writeRow(*states, row++, {name, QString::fromStdString(s.clip),
                    s.loop ? "true" : "false", QString::number(s.speed, 'g', 9),
                    QString::number(s.x, 'g', 9), QString::number(s.y, 'g', 9)});
            }
            initial->setCurrentText(QString::fromStdString(machine.initialState));
            row = 0;
            for (const auto &p : machine.parameters)
                writeRow(*parameters, row++, {QString::fromStdString(p.name),
                    kinds[static_cast<int>(p.kind)], QString::number(p.initialValue, 'g', 9)});
            row = 0;
            for (const auto &t : machine.transitions) {
                QStringList conditions;
                for (const auto &c : t.conditions)
                    conditions << QString::fromStdString(c.parameter) + " " +
                        operators[static_cast<int>(c.comparison)] + " " + QString::number(c.value, 'g', 9);
                writeRow(*transitions, row++, {QString::fromStdString(t.from),
                    QString::fromStdString(t.to), QString::number(t.fadeSeconds, 'g', 9),
                    t.waitForEnd ? "true" : "false", conditions.join(" && ")});
            }
        }
        redraw();
    }

    void redraw() {
        scene.clear();
        nodes.clear();
        edges.clear();
        const QString oldInitial = initial->currentText();
        QStringList names;
        for (int row = 0; row < states->rowCount(); ++row)
            names << text(*states, row, 0);
        {
            const QSignalBlocker blocker(initial);
            initial->clear();
            initial->addItems(names);
            if (names.contains(oldInitial))
                initial->setCurrentText(oldInitial);
        }

        for (int row = 0; row < states->rowCount(); ++row) {
            const auto &name = names[row];
            const QString label = name + "\n" + text(*states, row, 1);
            auto *node = new StateNode(label, name == initial->currentText());
            node->setPos(text(*states, row, 4).toDouble(), text(*states, row, 5).toDouble());
            node->moved = [this, row](QPointF pos) {
                const QSignalBlocker blocker(states);
                states->item(row, 4)->setText(QString::number(pos.x(), 'g', 9));
                states->item(row, 5)->setText(QString::number(pos.y(), 'g', 9));
                updateEdges();
            };
            scene.addItem(node);
            nodes.push_back(node);
        }
        for (int row = 0; row < transitions->rowCount(); ++row) {
            const auto from = names.indexOf(text(*transitions, row, 0));
            const auto to = names.indexOf(text(*transitions, row, 1));
            if (from < 0 || to < 0 || from == to) continue;
            auto *path = scene.addPath({}, QPen(QColor("#95b7ed"), 2));
            path->setZValue(-1);
            path->setToolTip(text(*transitions, row, 4));
            edges.push_back({nodes[from], nodes[to], path});
        }
        updateEdges();
    }

    void updateEdges() {
        for (const auto &edge : edges) {
            const QRectF from = edge.from->sceneBoundingRect();
            const QRectF to = edge.to->sceneBoundingRect();
            const qreal direction = to.center().x() >= from.center().x() ? 1 : -1;
            const QPointF begin(direction > 0 ? from.right() : from.left(), from.center().y() + 8);
            const QPointF end(direction > 0 ? to.left() : to.right(), to.center().y() + 8);
            const qreal bend = std::max<qreal>(35, std::abs(end.x() - begin.x()) * 0.45);
            QPainterPath path(begin);
            path.cubicTo(begin + QPointF(direction * bend, 0),
                         end - QPointF(direction * bend, 0), end);
            path.moveTo(end + QPointF(-direction * 9, -5));
            path.lineTo(end);
            path.lineTo(end + QPointF(-direction * 9, 5));
            edge.path->setPath(path);
        }
        scene.setSceneRect(scene.itemsBoundingRect().united(QRectF(0, 0, 420, 220))
                               .adjusted(-28, -28, 28, 28));
    }

    struct Edge {
        StateNode *from;
        StateNode *to;
        QGraphicsPathItem *path;
    };

    QGraphicsScene scene;
    QList<StateNode *> nodes;
    QList<Edge> edges;
    QComboBox *initial = nullptr;
    QTableWidget *states = nullptr;
    QTableWidget *parameters = nullptr;
    QTableWidget *transitions = nullptr;
    QString currentPath;
};

}

void showAnimationGraphEditor(QWidget *parent) {
    AnimationGraphDialog dialog(parent);
    dialog.exec();
}
