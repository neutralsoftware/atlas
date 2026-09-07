#include "editor/views/graphiteEditor.h"

#include "editor/styling/icons.h"
#include "editor/views/viewport.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QSplitter>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>

namespace {

QString pathKey(const QList<int> &path) {
    QStringList parts;
    for (int index : path)
        parts.append(QString::number(index));
    return parts.join('/');
}

QColor jsonColor(const QJsonValue &value, const QColor &fallback) {
    const QJsonArray values = value.toArray();
    if (values.size() < 3)
        return fallback;
    double maximum = std::max(
        {values.at(0).toDouble(), values.at(1).toDouble(),
         values.at(2).toDouble(),
         values.size() > 3 ? values.at(3).toDouble() : 1.0});
    const double scale = maximum > 1.0 ? 255.0 : 1.0;
    return QColor::fromRgbF(
        std::clamp(values.at(0).toDouble() / scale, 0.0, 1.0),
        std::clamp(values.at(1).toDouble() / scale, 0.0, 1.0),
        std::clamp(values.at(2).toDouble() / scale, 0.0, 1.0),
        std::clamp((values.size() > 3 ? values.at(3).toDouble() : scale) /
                       scale,
                   0.0, 1.0));
}

QJsonArray colorJson(const QColor &color) {
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

QColor chooseColor(QWidget *parent, const QColor &initial,
                   const QString &title) {
    QColorDialog dialog(initial, parent);
    dialog.setWindowTitle(title);
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    if (dialog.exec() == QDialog::Rejected)
        return {};
    return dialog.selectedColor();
}

bool isGraphiteColorField(const QString &key) {
    return key.compare("background", Qt::CaseInsensitive) == 0 ||
           key.compare("foreground", Qt::CaseInsensitive) == 0 ||
           key.compare("border", Qt::CaseInsensitive) == 0 ||
           key.compare("tint", Qt::CaseInsensitive) == 0 ||
           key.compare("color", Qt::CaseInsensitive) == 0 ||
           key.endsWith("Color", Qt::CaseInsensitive);
}

QJsonValue repairGraphiteColors(const QJsonValue &value, const QString &key,
                                bool &changed) {
    if (value.isObject()) {
        QJsonObject object = value.toObject();
        for (auto iterator = object.begin(); iterator != object.end(); ++iterator)
            iterator.value() =
                repairGraphiteColors(iterator.value(), iterator.key(), changed);
        return object;
    }
    if (!value.isArray())
        return value;
    QJsonArray array = value.toArray();
    if (isGraphiteColorField(key) && array.size() == 4 &&
        std::abs(array.at(3).toDouble() - (1.0 / 255.0)) < 0.00001) {
        array[3] = 1.0;
        changed = true;
    }
    for (int index = 0; index < array.size(); ++index)
        array[index] = repairGraphiteColors(array.at(index), {}, changed);
    return array;
}

QString ensureGraphiteDefaultFont(const QString &projectRoot) {
    QDir root(projectRoot);
    if (!root.mkpath("assets/fonts"))
        return {};
    const QString path = root.filePath("assets/fonts/GraphiteDefault.ttf");
    if (QFileInfo::exists(path))
        return path;
    QFile source(":/editor/assets/Manrope-VariableFont_wght.ttf");
    QSaveFile destination(path);
    if (!source.open(QIODevice::ReadOnly) ||
        !destination.open(QIODevice::WriteOnly))
        return {};
    const QByteArray contents = source.readAll();
    if (destination.write(contents) != contents.size() ||
        !destination.commit())
        return {};
    return path;
}

QPointF jsonPoint(const QJsonValue &value, const QPointF &fallback = {}) {
    const QJsonArray values = value.toArray();
    if (values.size() != 2)
        return fallback;
    return {values.at(0).toDouble(), values.at(1).toDouble()};
}

QSizeF jsonSize(const QJsonObject &element) {
    const QString type = element.value("type").toString();
    const QJsonArray values = element.value("size").toArray();
    if (values.size() == 2)
        return {std::max(1.0, values.at(0).toDouble()),
                std::max(1.0, values.at(1).toDouble())};
    if (type == "text")
        return {std::max(80.0,
                         element.value("content").toString().size() *
                             element.value("fontSize").toDouble(24.0) * 0.58),
                std::max(32.0, element.value("fontSize").toDouble(24.0) *
                                   1.35)};
    if (type == "checkbox")
        return {220, 44};
    if (type == "textField")
        return {320, 48};
    if (type == "image")
        return {160, 120};
    if (type == "column" || type == "row" || type == "stack")
        return {360, 220};
    return {180, 48};
}

QJsonObject replaceAtPath(QJsonObject document, const QList<int> &path,
                          const QJsonObject *replacement, bool remove) {
    if (path.isEmpty())
        return document;
    std::function<QJsonArray(QJsonArray, int)> replaceArray;
    replaceArray = [&](QJsonArray array, int depth) {
        const int index = path.at(depth);
        if (index < 0 || index >= array.size())
            return array;
        if (depth == path.size() - 1) {
            if (remove)
                array.removeAt(index);
            else if (replacement != nullptr)
                array[index] = *replacement;
            return array;
        }
        QJsonObject parent = array.at(index).toObject();
        parent.insert("children",
                      replaceArray(parent.value("children").toArray(),
                                   depth + 1));
        array[index] = parent;
        return array;
    };
    document.insert("elements",
                    replaceArray(document.value("elements").toArray(), 0));
    return document;
}

class GraphiteDocumentCommand : public QUndoCommand {
  public:
    GraphiteDocumentCommand(QJsonObject before, QJsonObject after,
                            std::function<void(const QJsonObject &)> apply)
        : before(std::move(before)), after(std::move(after)),
          apply(std::move(apply)) {}

    void undo() override { apply(before); }
    void redo() override { apply(after); }

  private:
    QJsonObject before;
    QJsonObject after;
    std::function<void(const QJsonObject &)> apply;
};

}

class GraphiteCanvas : public QWidget {
  public:
    explicit GraphiteCanvas(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumSize(460, 320);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
    }

    void setDocument(const QJsonObject &next, const QString &path) {
        document = next;
        baseDir = QFileInfo(path).absolutePath();
        update();
    }

    void setSelectedPath(const QList<int> &path) {
        selected = path;
        update();
    }

    std::function<void(const QList<int> &)> selectionChanged;
    std::function<void(const QList<int> &, const QPointF &)> elementMoved;

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#171716"));
        const QJsonObject canvas = document.value("canvas").toObject();
        const double width = std::max(1.0, canvas.value("width").toDouble(1280));
        const double height =
            std::max(1.0, canvas.value("height").toDouble(720));
        const double scale = std::min((this->width() - 48.0) / width,
                                      (this->height() - 48.0) / height);
        canvasScale = std::max(0.05, scale);
        const QSizeF shown(width * canvasScale, height * canvasScale);
        canvasRect = QRectF((this->width() - shown.width()) * 0.5,
                            (this->height() - shown.height()) * 0.5,
                            shown.width(), shown.height());
        painter.fillRect(
            canvasRect,
            jsonColor(canvas.value("background"), QColor("#20242C")));
        painter.setPen(QPen(QColor(255, 255, 255, 35), 1));
        painter.drawRect(canvasRect);
        painter.save();
        painter.translate(canvasRect.topLeft());
        painter.scale(canvasScale, canvasScale);
        bounds.clear();
        paintOrder.clear();
        const QJsonArray elements = document.value("elements").toArray();
        for (int index = 0; index < elements.size(); ++index)
            drawElement(painter, elements.at(index).toObject(), {index}, {});
        painter.restore();
        painter.setPen(QColor(255, 255, 255, 90));
        painter.drawText(canvasRect.adjusted(8, 6, -8, -6),
                         Qt::AlignRight | Qt::AlignBottom,
                         QStringLiteral("%1 × %2 · %3%")
                             .arg(width, 0, 'f', 0)
                             .arg(height, 0, 'f', 0)
                             .arg(canvasScale * 100.0, 0, 'f', 0));
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton || !canvasRect.contains(event->position()))
            return;
        const QPointF point =
            (event->position() - canvasRect.topLeft()) / canvasScale;
        QList<int> hit;
        for (auto it = paintOrder.crbegin(); it != paintOrder.crend(); ++it) {
            if (bounds.value(pathKey(*it)).contains(point)) {
                hit = *it;
                break;
            }
        }
        selected = hit;
        dragging = !hit.isEmpty() && hit.size() == 1;
        dragStart = point;
        if (dragging) {
            const QJsonArray elements = document.value("elements").toArray();
            dragOrigin = jsonPoint(elements.at(hit.first()).toObject().value("position"));
        }
        if (selectionChanged)
            selectionChanged(hit);
        update();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!dragging || selected.isEmpty() || !elementMoved)
            return;
        const QPointF point =
            (event->position() - canvasRect.topLeft()) / canvasScale;
        elementMoved(selected, dragOrigin + point - dragStart);
    }

    void mouseReleaseEvent(QMouseEvent *) override { dragging = false; }

  private:
    QRectF drawElement(QPainter &painter, const QJsonObject &element,
                       const QList<int> &path, const QRectF &assigned) {
        const QString type = element.value("type").toString();
        const QSizeF size = jsonSize(element);
        const QPointF position = assigned.isValid()
                                     ? assigned.topLeft()
                                     : jsonPoint(element.value("position"));
        QRectF frame(position, assigned.isValid() ? assigned.size() : size);
        bounds.insert(pathKey(path), frame);
        paintOrder.append(path);
        const QJsonObject normal =
            element.value("style").toObject().value("normal").toObject();
        QColor background = jsonColor(normal.value("background"),
                                      QColor(48, 52, 64, 235));
        QColor foreground = jsonColor(
            normal.value("foreground"),
            jsonColor(element.value("color"), QColor("#F5F6F8")));
        const double radius = normal.value("cornerRadius").toDouble(8.0);
        painter.save();
        if (type == "text") {
            QFont font = painter.font();
            font.setPixelSize(std::max(8, element.value("fontSize").toInt(24)));
            painter.setFont(font);
            painter.setPen(foreground);
            painter.drawText(frame, Qt::AlignLeft | Qt::AlignVCenter,
                             element.value("content").toString("Text"));
        } else if (type == "checkbox") {
            painter.setPen(QPen(QColor(255, 255, 255, 55), 1));
            painter.setBrush(QColor(33, 36, 44));
            const double box = std::min(frame.height() - 12.0, 24.0);
            QRectF check(frame.left() + 6, frame.center().y() - box * 0.5, box,
                         box);
            painter.drawRoundedRect(check, 4, 4);
            if (element.value("checked").toBool()) {
                painter.setPen(QPen(QColor("#E39758"), 3));
                painter.drawLine(check.left() + 5, check.center().y(),
                                 check.center().x(), check.bottom() - 5);
                painter.drawLine(check.center().x(), check.bottom() - 5,
                                 check.right() - 4, check.top() + 5);
            }
            painter.setPen(foreground);
            painter.drawText(frame.adjusted(box + 14, 0, 0, 0),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             element.value("label").toString("Checkbox"));
        } else if (type == "image") {
            painter.setPen(QPen(QColor(255, 255, 255, 45), 1));
            painter.setBrush(QColor(37, 42, 52));
            painter.drawRoundedRect(frame, radius, radius);
            const QString source = element.value("source").toString();
            QImage image(QDir(baseDir).filePath(source));
            if (!image.isNull())
                painter.drawImage(frame, image);
            else {
                painter.drawLine(frame.topLeft(), frame.bottomRight());
                painter.drawLine(frame.topRight(), frame.bottomLeft());
                painter.drawText(frame, Qt::AlignCenter, "Image");
            }
        } else if (type == "column" || type == "row" || type == "stack") {
            painter.setPen(QPen(QColor(126, 146, 156, 150), 1,
                                Qt::DashLine));
            painter.setBrush(background);
            painter.drawRoundedRect(frame, radius, radius);
            const QJsonArray children = element.value("children").toArray();
            const QPointF padding = jsonPoint(element.value("padding"), {12, 12});
            const double spacing = element.value("spacing").toDouble(8.0);
            QRectF content = frame.adjusted(padding.x(), padding.y(),
                                            -padding.x(), -padding.y());
            double cursor = type == "row" ? content.left() : content.top();
            for (int index = 0; index < children.size(); ++index) {
                const QJsonObject child = children.at(index).toObject();
                QSizeF childSize = jsonSize(child);
                QRectF childFrame;
                if (type == "row") {
                    childFrame = QRectF(cursor, content.top(), childSize.width(),
                                        std::min(childSize.height(), content.height()));
                    cursor += childSize.width() + spacing;
                } else if (type == "column") {
                    childFrame = QRectF(content.left(), cursor,
                                        std::min(childSize.width(), content.width()),
                                        childSize.height());
                    cursor += childSize.height() + spacing;
                } else {
                    childFrame = QRectF(content.topLeft(), childSize);
                }
                QList<int> childPath = path;
                childPath.append(index);
                drawElement(painter, child, childPath, childFrame);
            }
        } else {
            painter.setPen(QPen(QColor(255, 255, 255, 52), 1));
            painter.setBrush(background);
            painter.drawRoundedRect(frame, radius, radius);
            painter.setPen(foreground);
            const QString text =
                type == "textField"
                    ? element.value("text").toString().isEmpty()
                          ? element.value("placeholder").toString("Text field")
                          : element.value("text").toString()
                    : element.value("label").toString("Button");
            painter.drawText(frame.adjusted(14, 0, -14, 0),
                             Qt::AlignCenter, text);
        }
        if (path == selected) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#B7A4ED"), 2.0 / canvasScale));
            painter.drawRect(frame.adjusted(-2, -2, 2, 2));
        }
        if (!element.value("components").toArray().isEmpty()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#849589"));
            painter.drawEllipse(QPointF(frame.right() - 7, frame.top() + 7), 4,
                                4);
        }
        painter.restore();
        return frame;
    }

    QJsonObject document;
    QString baseDir;
    QList<int> selected;
    QHash<QString, QRectF> bounds;
    QList<QList<int>> paintOrder;
    QRectF canvasRect;
    double canvasScale = 1.0;
    bool dragging = false;
    QPointF dragStart;
    QPointF dragOrigin;
};

GraphiteEditorPanel::GraphiteEditorPanel(ViewportPanel *viewport,
                                         const QString &projectFile,
                                         QWidget *parent)
    : QWidget(parent), viewport(viewport), projectFile(projectFile) {
    setObjectName("graphiteEditorPanel");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QFrame(this);
    header->setObjectName("materialEditorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 9, 12, 9);
    titleLabel = new QLabel("Graphite", header);
    titleLabel->setObjectName("materialEditorTitle");
    statusLabel = new QLabel(header);
    statusLabel->setObjectName("materialEditorStatus");
    auto *attach = new QPushButton("Attach to Scene", header);
    attach->setIcon(styling::icon(styling::Icon::Assign, "#9E897D"));
    auto *preview = new QPushButton("Preview Camera", header);
    preview->setIcon(styling::icon(styling::Icon::MonitorPlay, "#849589"));
    auto *save = new QPushButton("Save", header);
    save->setIcon(styling::icon(styling::Icon::FloppyDisk, "#A1957D"));
    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(statusLabel);
    headerLayout->addWidget(attach);
    headerLayout->addWidget(preview);
    headerLayout->addWidget(save);
    layout->addWidget(header);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    auto *outline = new QWidget(splitter);
    outline->setMinimumWidth(210);
    auto *outlineLayout = new QVBoxLayout(outline);
    outlineLayout->setContentsMargins(8, 8, 4, 8);
    auto *outlineToolbar = new QHBoxLayout();
    auto *add = new QToolButton(outline);
    add->setText("Add");
    add->setIcon(styling::icon(styling::Icon::Plus, "#8498A8"));
    add->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    add->setPopupMode(QToolButton::InstantPopup);
    auto *addMenu = new QMenu(add);
    const QList<QPair<QString, QString>> types{{"Text", "text"},
                                               {"Image", "image"},
                                               {"Button", "button"},
                                               {"Checkbox", "checkbox"},
                                               {"Text Field", "textField"},
                                               {"Column", "column"},
                                               {"Row", "row"},
                                               {"Stack", "stack"}};
    for (const auto &[label, type] : types)
        addMenu->addAction(label, this, [this, type] { addElement(type); });
    add->setMenu(addMenu);
    auto *duplicate = new QToolButton(outline);
    duplicate->setIcon(styling::icon(styling::Icon::SquaresFour, "#7E929C"));
    duplicate->setToolTip("Duplicate selected element");
    auto *remove = new QToolButton(outline);
    remove->setIcon(styling::icon(styling::Icon::Trash, "#A17F7F"));
    remove->setToolTip("Delete selected element");
    outlineToolbar->addWidget(add);
    outlineToolbar->addStretch();
    outlineToolbar->addWidget(duplicate);
    outlineToolbar->addWidget(remove);
    outlineLayout->addLayout(outlineToolbar);
    tree = new QTreeWidget(outline);
    tree->setHeaderHidden(true);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setUniformRowHeights(true);
    outlineLayout->addWidget(tree, 1);

    canvas = new GraphiteCanvas(splitter);

    auto *inspectorScroll = new QScrollArea(splitter);
    inspectorScroll->setWidgetResizable(true);
    inspectorScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    inspectorScroll->setMinimumWidth(285);
    inspectorBody = new QWidget(inspectorScroll);
    inspectorLayout = new QVBoxLayout(inspectorBody);
    inspectorLayout->setContentsMargins(8, 8, 8, 12);
    inspectorLayout->setSpacing(8);
    inspectorScroll->setWidget(inspectorBody);

    splitter->addWidget(outline);
    splitter->addWidget(canvas);
    splitter->addWidget(inspectorScroll);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({230, 760, 310});
    layout->addWidget(splitter, 1);

    undoStack = new QUndoStack(this);
    connect(tree, &QTreeWidget::itemSelectionChanged, this, [this] {
        canvas->setSelectedPath(selectedPath());
        rebuildInspector();
    });
    connect(save, &QPushButton::clicked, this, &GraphiteEditorPanel::saveUI);
    connect(attach, &QPushButton::clicked, this,
            [this] { attachToScene(false); });
    connect(preview, &QPushButton::clicked, this,
            [this] { attachToScene(true); });
    connect(remove, &QToolButton::clicked, this,
            &GraphiteEditorPanel::deleteSelectedElement);
    connect(duplicate, &QToolButton::clicked, this,
            &GraphiteEditorPanel::duplicateSelectedElement);
    canvas->selectionChanged = [this](const QList<int> &path) {
        const QString key = pathKey(path);
        const auto items = tree->findItems("*", Qt::MatchWildcard |
                                                    Qt::MatchRecursive);
        for (QTreeWidgetItem *item : items) {
            if (item->data(0, Qt::UserRole).toString() == key) {
                tree->setCurrentItem(item);
                return;
            }
        }
        tree->clearSelection();
    };
    canvas->elementMoved = [this](const QList<int> &path,
                                  const QPointF &position) {
        QJsonObject element = elementAtPath(path);
        if (element.isEmpty())
            return;
        element.insert("position", QJsonArray{std::round(position.x()),
                                               std::round(position.y())});
        QJsonObject next = replaceAtPath(document, path, &element, false);
        setDocument(next, true);
    };
    showEmptyState();
}

GraphiteEditorPanel::~GraphiteEditorPanel() {
    if (!uiPath.isEmpty())
        saveUI();
}

void GraphiteEditorPanel::showEmptyState() {
    documentDirty = false;
    document = QJsonObject{{"format", "atlas.graphite.ui"},
                           {"version", 1},
                           {"canvas", QJsonObject{{"width", 1280},
                                                  {"height", 720},
                                                  {"background", QJsonArray{
                                                                     0.035,
                                                                     0.04,
                                                                     0.055,
                                                                     1.0}}}},
                           {"elements", QJsonArray{}}};
    titleLabel->setText("Graphite");
    statusLabel->setText("Open a .aui asset");
    rebuildTree();
    rebuildInspector();
    canvas->setDocument(document, {});
}

void GraphiteEditorPanel::openUI(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Graphite", "The UI asset could not be opened.");
        return;
    }
    QJsonParseError error;
    const QJsonDocument parsed = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject()) {
        QMessageBox::warning(this, "Graphite", "The UI asset is not valid JSON.");
        return;
    }
    QJsonObject next = parsed.object();
    if (next.value("format").toString() != "atlas.graphite.ui" ||
        next.value("version").toInt() != 1) {
        QMessageBox::warning(this, "Graphite",
                             "This is not a supported Graphite UI document.");
        return;
    }
    if (!next.value("elements").isArray()) {
        QJsonArray elements;
        if (next.value("root").isObject()) {
            elements.append(next.value("root"));
            next.remove("root");
        }
        next.insert("elements", elements);
    }
    bool repaired = false;
    next = repairGraphiteColors(next, {}, repaired).toObject();
    uiPath = QFileInfo(path).absoluteFilePath();
    QJsonObject defaultFont = next.value("defaultFont").toObject();
    if (defaultFont.value("source").toString().trimmed().isEmpty()) {
        const QString fontPath = ensureGraphiteDefaultFont(
            QFileInfo(projectFile).absolutePath());
        if (!fontPath.isEmpty()) {
            defaultFont.insert(
                "source",
                QDir(QFileInfo(uiPath).absolutePath()).relativeFilePath(fontPath));
            if (!defaultFont.contains("size"))
                defaultFont.insert("size", 24);
            next.insert("defaultFont", defaultFont);
            repaired = true;
        }
    }
    document = next;
    documentDirty = repaired;
    undoStack->clear();
    titleLabel->setText(QFileInfo(uiPath).completeBaseName());
    statusLabel->setText("Ready");
    rebuildTree();
    rebuildInspector();
    canvas->setDocument(document, uiPath);
    if (repaired)
        saveUI();
}

void GraphiteEditorPanel::saveUI() {
    if (uiPath.isEmpty())
        return;
    QSaveFile file(uiPath);
    const QByteArray contents = QJsonDocument(document).toJson(QJsonDocument::Indented);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() ||
        !file.commit()) {
        QMessageBox::warning(this, "Graphite", "The UI asset could not be saved.");
        return;
    }
    documentDirty = false;
    statusLabel->setText("Saved");
    emit documentSaved();
}

void GraphiteEditorPanel::flushPendingSave() {
    if (documentDirty)
        saveUI();
}

void GraphiteEditorPanel::undo() { undoStack->undo(); }

void GraphiteEditorPanel::redo() { undoStack->redo(); }

QList<int> GraphiteEditorPanel::selectedPath() const {
    if (tree->currentItem() == nullptr)
        return {};
    QList<int> path;
    const QStringList values =
        tree->currentItem()->data(0, Qt::UserRole).toString().split('/');
    for (const QString &value : values) {
        bool okay = false;
        const int index = value.toInt(&okay);
        if (okay)
            path.append(index);
    }
    return path;
}

QJsonObject GraphiteEditorPanel::elementAtPath(const QList<int> &path) const {
    if (path.isEmpty())
        return {};
    QJsonArray array = document.value("elements").toArray();
    QJsonObject element;
    for (int depth = 0; depth < path.size(); ++depth) {
        const int index = path.at(depth);
        if (index < 0 || index >= array.size())
            return {};
        element = array.at(index).toObject();
        array = element.value("children").toArray();
    }
    return element;
}

void GraphiteEditorPanel::replaceElement(const QList<int> &path,
                                         const QJsonObject &element) {
    setDocument(replaceAtPath(document, path, &element, false), true);
}

void GraphiteEditorPanel::removeElement(const QList<int> &path) {
    setDocument(replaceAtPath(document, path, nullptr, true), true);
}

void GraphiteEditorPanel::setDocument(const QJsonObject &next, bool recordUndo) {
    if (next == document)
        return;
    const QList<int> path = selectedPath();
    auto apply = [this, path](const QJsonObject &value) {
        document = value;
        rebuildTree();
        const QString key = pathKey(path);
        const auto items = tree->findItems("*", Qt::MatchWildcard |
                                                    Qt::MatchRecursive);
        for (QTreeWidgetItem *item : items) {
            if (item->data(0, Qt::UserRole).toString() == key) {
                tree->setCurrentItem(item);
                break;
            }
        }
        canvas->setDocument(document, uiPath);
        canvas->setSelectedPath(path);
        rebuildInspector();
        documentDirty = true;
        statusLabel->setText("Modified");
    };
    if (recordUndo)
        undoStack->push(new GraphiteDocumentCommand(document, next, apply));
    else
        apply(next);
}

QTreeWidgetItem *GraphiteEditorPanel::appendTreeElement(
    QTreeWidgetItem *parent, const QJsonObject &element,
    const QList<int> &path) {
    auto *item = parent != nullptr ? new QTreeWidgetItem(parent)
                                   : new QTreeWidgetItem(tree);
    const QString type = element.value("type").toString();
    item->setText(0, element.value("name").toString(type));
    item->setData(0, Qt::UserRole, pathKey(path));
    item->setIcon(0, styling::icon(
                         type == "image" ? styling::Icon::Image
                         : type == "column" || type == "row" || type == "stack"
                             ? styling::Icon::Layout
                             : type == "button" || type == "checkbox" ||
                                       type == "textField"
                                 ? styling::Icon::CursorClick
                                 : styling::Icon::File,
                         "#8498A8"));
    const QJsonArray children = element.value("children").toArray();
    for (int index = 0; index < children.size(); ++index) {
        QList<int> childPath = path;
        childPath.append(index);
        appendTreeElement(item, children.at(index).toObject(), childPath);
    }
    item->setExpanded(true);
    return item;
}

void GraphiteEditorPanel::rebuildTree() {
    const QString selectedKey = tree->currentItem() != nullptr
                                    ? tree->currentItem()
                                          ->data(0, Qt::UserRole)
                                          .toString()
                                    : QString();
    tree->clear();
    const QJsonArray elements = document.value("elements").toArray();
    for (int index = 0; index < elements.size(); ++index)
        appendTreeElement(nullptr, elements.at(index).toObject(), {index});
    if (!selectedKey.isEmpty()) {
        const auto items = tree->findItems("*", Qt::MatchWildcard |
                                                    Qt::MatchRecursive);
        for (QTreeWidgetItem *item : items) {
            if (item->data(0, Qt::UserRole).toString() == selectedKey) {
                tree->setCurrentItem(item);
                break;
            }
        }
    }
}

QJsonObject GraphiteEditorPanel::defaultElement(const QString &type) {
    const int number = nextElementNumber++;
    QJsonObject element{{"id", QStringLiteral("%1_%2").arg(type).arg(number)},
                        {"name", QStringLiteral("%1 %2")
                                     .arg(type.left(1).toUpper() + type.mid(1))
                                     .arg(number)},
                        {"type", type},
                        {"position", QJsonArray{48 + number * 8,
                                                48 + number * 8}},
                        {"components", QJsonArray{}}};
    if (type == "text") {
        element.insert("content", "Text");
        element.insert("fontSize", 32);
        element.insert("color", QJsonArray{1, 1, 1, 1});
    } else if (type == "image") {
        element.insert("source", "");
        element.insert("size", QJsonArray{180, 120});
    } else if (type == "button") {
        element.insert("label", "Button");
        element.insert("size", QJsonArray{180, 52});
        element.insert("enabled", true);
    } else if (type == "checkbox") {
        element.insert("label", "Checkbox");
        element.insert("size", QJsonArray{220, 44});
        element.insert("checked", false);
        element.insert("enabled", true);
    } else if (type == "textField") {
        element.insert("text", "");
        element.insert("placeholder", "Text field");
        element.insert("size", QJsonArray{320, 48});
    } else {
        element.insert("size", QJsonArray{360, 220});
        element.insert("padding", QJsonArray{12, 12});
        element.insert("spacing", 8);
        element.insert("alignment", "start");
        element.insert("children", QJsonArray{});
    }
    element.insert(
        "style",
        QJsonObject{{"normal",
                     QJsonObject{{"background",
                                  QJsonArray{0.12, 0.13, 0.17, 0.96}},
                                 {"foreground", QJsonArray{1, 1, 1, 1}},
                                 {"borderWidth", 1},
                                 {"border", QJsonArray{1, 1, 1, 0.16}},
                                 {"cornerRadius", 8}}}});
    return element;
}

void GraphiteEditorPanel::addElement(const QString &type) {
    if (uiPath.isEmpty())
        return;
    QJsonObject next = document;
    QJsonObject element = defaultElement(type);
    const QList<int> path = selectedPath();
    QJsonObject parent = elementAtPath(path);
    const QString parentType = parent.value("type").toString();
    if (!path.isEmpty() &&
        (parentType == "column" || parentType == "row" ||
         parentType == "stack")) {
        QJsonArray children = parent.value("children").toArray();
        children.append(element);
        parent.insert("children", children);
        next = replaceAtPath(next, path, &parent, false);
    } else {
        QJsonArray elements = next.value("elements").toArray();
        elements.append(element);
        next.insert("elements", elements);
    }
    setDocument(next, true);
}

void GraphiteEditorPanel::deleteSelectedElement() {
    const QList<int> path = selectedPath();
    if (path.isEmpty())
        return;
    removeElement(path);
    tree->clearSelection();
}

void GraphiteEditorPanel::duplicateSelectedElement() {
    const QList<int> path = selectedPath();
    QJsonObject element = elementAtPath(path);
    if (element.isEmpty())
        return;
    element.insert("id", element.value("id").toString() + "_copy");
    element.insert("name", element.value("name").toString() + " Copy");
    QJsonObject next = document;
    if (path.size() == 1) {
        QJsonArray elements = next.value("elements").toArray();
        elements.insert(path.first() + 1, element);
        next.insert("elements", elements);
    } else {
        QList<int> parentPath = path;
        const int index = parentPath.takeLast();
        QJsonObject parent = elementAtPath(parentPath);
        QJsonArray children = parent.value("children").toArray();
        children.insert(index + 1, element);
        parent.insert("children", children);
        next = replaceAtPath(next, parentPath, &parent, false);
    }
    setDocument(next, true);
}

void GraphiteEditorPanel::rebuildInspector() {
    while (QLayoutItem *item = inspectorLayout->takeAt(0)) {
        if (item->widget() != nullptr)
            item->widget()->deleteLater();
        delete item;
    }
    const QList<int> path = selectedPath();
    QJsonObject element = elementAtPath(path);
    if (element.isEmpty()) {
        auto *documentGroup = new QGroupBox("Document", inspectorBody);
        auto *documentForm = new QFormLayout(documentGroup);
        auto *name =
            new QLineEdit(document.value("name").toString(), documentGroup);
        QJsonObject canvasData = document.value("canvas").toObject();
        auto *width = new QDoubleSpinBox(documentGroup);
        auto *height = new QDoubleSpinBox(documentGroup);
        width->setRange(1, 100000);
        height->setRange(1, 100000);
        width->setDecimals(0);
        height->setDecimals(0);
        width->setValue(canvasData.value("width").toDouble(1280));
        height->setValue(canvasData.value("height").toDouble(720));
        width->setKeyboardTracking(false);
        height->setKeyboardTracking(false);
        documentForm->addRow("Name", name);
        documentForm->addRow("Canvas Width", width);
        documentForm->addRow("Canvas Height", height);
        inspectorLayout->addWidget(documentGroup);

        auto *fontGroup = new QGroupBox("Default Font", inspectorBody);
        auto *fontForm = new QFormLayout(fontGroup);
        QJsonObject fontData = document.value("defaultFont").toObject();
        auto *fontPath =
            new QLineEdit(fontData.value("source").toString(), fontGroup);
        auto *chooseFont = new QPushButton("Choose…", fontGroup);
        auto *fontRow = new QWidget(fontGroup);
        auto *fontRowLayout = new QHBoxLayout(fontRow);
        fontRowLayout->setContentsMargins(0, 0, 0, 0);
        fontRowLayout->addWidget(fontPath, 1);
        fontRowLayout->addWidget(chooseFont);
        auto *fontSize = new QDoubleSpinBox(fontGroup);
        fontSize->setRange(1, 512);
        fontSize->setDecimals(0);
        fontSize->setValue(fontData.value("size").toDouble(24));
        fontSize->setKeyboardTracking(false);
        fontForm->addRow("Source", fontRow);
        fontForm->addRow("Size", fontSize);
        inspectorLayout->addWidget(fontGroup);
        auto updateDocument = [this](const std::function<void(QJsonObject &)> &edit) {
            QJsonObject next = document;
            edit(next);
            setDocument(next, true);
        };
        connect(name, &QLineEdit::editingFinished, this,
                [name, updateDocument] {
                    updateDocument([name](QJsonObject &next) {
                        next.insert("name", name->text().trimmed());
                    });
                });
        auto updateCanvas = [width, height, updateDocument] {
            updateDocument([width, height](QJsonObject &next) {
                QJsonObject canvas = next.value("canvas").toObject();
                canvas.insert("width", width->value());
                canvas.insert("height", height->value());
                next.insert("canvas", canvas);
            });
        };
        connect(width, &QDoubleSpinBox::editingFinished, this, updateCanvas);
        connect(height, &QDoubleSpinBox::editingFinished, this, updateCanvas);
        auto updateFont = [fontPath, fontSize, updateDocument] {
            updateDocument([fontPath, fontSize](QJsonObject &next) {
                next.insert("defaultFont",
                            QJsonObject{{"source", fontPath->text()},
                                        {"size", fontSize->value()}});
            });
        };
        connect(fontPath, &QLineEdit::editingFinished, this, updateFont);
        connect(fontSize, &QDoubleSpinBox::editingFinished, this, updateFont);
        connect(chooseFont, &QPushButton::clicked, this,
                [this, fontPath, updateFont] {
                    const QString selected = QFileDialog::getOpenFileName(
                        this, "Choose Font", QFileInfo(uiPath).absolutePath(),
                        "Fonts (*.ttf *.otf)");
                    if (selected.isEmpty())
                        return;
                    fontPath->setText(
                        QDir(QFileInfo(uiPath).absolutePath())
                            .relativeFilePath(selected));
                    updateFont();
                });
        inspectorLayout->addStretch();
        return;
    }
    auto update = [this, path](const QString &key, const QJsonValue &value) {
        QJsonObject changed = elementAtPath(path);
        if (changed.isEmpty())
            return;
        changed.insert(key, value);
        replaceElement(path, changed);
    };
    auto *identity = new QGroupBox("Element", inspectorBody);
    auto *identityForm = new QFormLayout(identity);
    auto *name = new QLineEdit(element.value("name").toString(), identity);
    auto *id = new QLineEdit(element.value("id").toString(), identity);
    id->setPlaceholderText("Stable script id");
    auto *type = new QLabel(element.value("type").toString(), identity);
    identityForm->addRow("Name", name);
    identityForm->addRow("ID", id);
    identityForm->addRow("Type", type);
    inspectorLayout->addWidget(identity);
    connect(name, &QLineEdit::editingFinished, this,
            [name, update] { update("name", name->text().trimmed()); });
    connect(id, &QLineEdit::editingFinished, this,
            [id, update] { update("id", id->text().trimmed()); });

    auto spin = [](double value, double minimum, double maximum,
                   QWidget *parent) {
        auto *field = new QDoubleSpinBox(parent);
        field->setRange(minimum, maximum);
        field->setDecimals(1);
        field->setValue(value);
        field->setKeyboardTracking(false);
        return field;
    };
    auto *geometry = new QGroupBox("Geometry", inspectorBody);
    auto *geometryForm = new QFormLayout(geometry);
    const QPointF position = jsonPoint(element.value("position"));
    const QSizeF size = jsonSize(element);
    auto *x = spin(position.x(), -100000, 100000, geometry);
    auto *y = spin(position.y(), -100000, 100000, geometry);
    auto *width = spin(size.width(), 1, 100000, geometry);
    auto *height = spin(size.height(), 1, 100000, geometry);
    geometryForm->addRow("X", x);
    geometryForm->addRow("Y", y);
    geometryForm->addRow("Width", width);
    geometryForm->addRow("Height", height);
    inspectorLayout->addWidget(geometry);
    connect(x, &QDoubleSpinBox::editingFinished, this, [x, y, update] {
        update("position", QJsonArray{x->value(), y->value()});
    });
    connect(y, &QDoubleSpinBox::editingFinished, this, [x, y, update] {
        update("position", QJsonArray{x->value(), y->value()});
    });
    connect(width, &QDoubleSpinBox::editingFinished, this,
            [width, height, update] {
                update("size", QJsonArray{width->value(), height->value()});
            });
    connect(height, &QDoubleSpinBox::editingFinished, this,
            [width, height, update] {
                update("size", QJsonArray{width->value(), height->value()});
            });

    const QString elementType = element.value("type").toString();
    if (elementType == "text" || elementType == "button" ||
        elementType == "checkbox" || elementType == "textField" ||
        elementType == "image") {
        auto *contentGroup = new QGroupBox("Content", inspectorBody);
        auto *contentForm = new QFormLayout(contentGroup);
        if (elementType == "text" || elementType == "button" ||
            elementType == "checkbox") {
            const QString key = elementType == "text" ? "content" : "label";
            auto *content = new QLineEdit(element.value(key).toString(),
                                          contentGroup);
            contentForm->addRow(elementType == "text" ? "Text" : "Label",
                                content);
            connect(content, &QLineEdit::editingFinished, this,
                    [content, key, update] { update(key, content->text()); });
        } else if (elementType == "textField") {
            auto *content =
                new QLineEdit(element.value("text").toString(), contentGroup);
            auto *placeholder = new QLineEdit(
                element.value("placeholder").toString(), contentGroup);
            contentForm->addRow("Text", content);
            contentForm->addRow("Placeholder", placeholder);
            connect(content, &QLineEdit::editingFinished, this,
                    [content, update] { update("text", content->text()); });
            connect(placeholder, &QLineEdit::editingFinished, this,
                    [placeholder, update] {
                        update("placeholder", placeholder->text());
                    });
        } else {
            auto *source = new QLineEdit(element.value("source").toString(),
                                         contentGroup);
            auto *choose = new QPushButton("Choose…", contentGroup);
            auto *row = new QWidget(contentGroup);
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->addWidget(source, 1);
            rowLayout->addWidget(choose);
            contentForm->addRow("Image", row);
            connect(source, &QLineEdit::editingFinished, this,
                    [source, update] { update("source", source->text()); });
            connect(choose, &QPushButton::clicked, this,
                    [this, source, update] {
                        const QString selected = QFileDialog::getOpenFileName(
                            this, "Choose Image", QFileInfo(uiPath).absolutePath(),
                            "Images (*.png *.jpg *.jpeg *.bmp *.tga *.hdr *.exr)");
                        if (selected.isEmpty())
                            return;
                        const QString relative =
                            QDir(QFileInfo(uiPath).absolutePath())
                                .relativeFilePath(selected);
                        source->setText(relative);
                        update("source", relative);
                    });
        }
        inspectorLayout->addWidget(contentGroup);
    }

    if (elementType == "column" || elementType == "row" ||
        elementType == "stack") {
        auto *layoutGroup = new QGroupBox("Layout", inspectorBody);
        auto *layoutForm = new QFormLayout(layoutGroup);
        auto *spacing =
            spin(element.value("spacing").toDouble(8), 0, 1000, layoutGroup);
        auto *alignment = new QComboBox(layoutGroup);
        alignment->addItems({"start", "center", "end"});
        alignment->setCurrentText(element.value("alignment").toString("start"));
        layoutForm->addRow("Spacing", spacing);
        layoutForm->addRow("Alignment", alignment);
        inspectorLayout->addWidget(layoutGroup);
        connect(spacing, &QDoubleSpinBox::editingFinished, this,
                [spacing, update] { update("spacing", spacing->value()); });
        connect(alignment, &QComboBox::currentTextChanged, this,
                [update](const QString &value) { update("alignment", value); });
    }

    auto *appearance = new QGroupBox("Appearance", inspectorBody);
    auto *appearanceForm = new QFormLayout(appearance);
    QJsonObject style = element.value("style").toObject();
    auto *state = new QComboBox(appearance);
    state->addItems(
        {"normal", "hovered", "pressed", "focused", "disabled", "checked"});
    state->setCurrentText(styleVariant);
    QJsonObject normal = style.value(styleVariant).toObject();
    const QJsonObject fallbackNormal = style.value("normal").toObject();
    const QColor background =
        jsonColor(normal.value("background"),
                  jsonColor(fallbackNormal.value("background"),
                            QColor("#303440")));
    const QColor foreground =
        jsonColor(normal.value("foreground"),
                  jsonColor(fallbackNormal.value("foreground"),
                            QColor("#F5F6F8")));
    auto *backgroundButton = new QPushButton(appearance);
    backgroundButton->setIcon(styling::colorSwatch(background));
    backgroundButton->setText(background.name(QColor::HexArgb));
    auto *foregroundButton = new QPushButton(appearance);
    foregroundButton->setIcon(styling::colorSwatch(foreground));
    foregroundButton->setText(foreground.name(QColor::HexArgb));
    auto *radius =
        spin(normal.value("cornerRadius")
                 .toDouble(fallbackNormal.value("cornerRadius").toDouble(8)),
             0, 1000, appearance);
    appearanceForm->addRow("State", state);
    appearanceForm->addRow("Background", backgroundButton);
    appearanceForm->addRow("Foreground", foregroundButton);
    appearanceForm->addRow("Corner Radius", radius);
    inspectorLayout->addWidget(appearance);
    auto updateStyle = [this, path](const QString &key, const QJsonValue &value) {
        QJsonObject changed = elementAtPath(path);
        QJsonObject style = changed.value("style").toObject();
        QJsonObject normal = style.value(styleVariant).toObject();
        normal.insert(key, value);
        style.insert(styleVariant, normal);
        changed.insert("style", style);
        replaceElement(path, changed);
    };
    connect(backgroundButton, &QPushButton::clicked, this,
            [this, background, updateStyle] {
                const QColor selected =
                    chooseColor(this, background, "Background");
                if (selected.isValid())
                    updateStyle("background", colorJson(selected));
            });
    connect(foregroundButton, &QPushButton::clicked, this,
            [this, foreground, updateStyle] {
                const QColor selected =
                    chooseColor(this, foreground, "Foreground");
                if (selected.isValid())
                    updateStyle("foreground", colorJson(selected));
            });
    connect(radius, &QDoubleSpinBox::editingFinished, this,
            [radius, updateStyle] {
                updateStyle("cornerRadius", radius->value());
            });
    connect(state, &QComboBox::currentTextChanged, this,
            [this](const QString &value) {
                styleVariant = value;
                rebuildInspector();
            });

    auto *components = new QGroupBox("Components", inspectorBody);
    auto *componentsLayout = new QVBoxLayout(components);
    auto *componentList = new QListWidget(components);
    const QJsonArray componentData = element.value("components").toArray();
    for (const QJsonValue &value : componentData) {
        const QJsonObject component = value.toObject();
        auto *item = new QListWidgetItem(
            styling::icon(styling::Icon::FileCode, "#7E929C"),
            component.value("className")
                .toString(component.value("name").toString("Script")),
            componentList);
        item->setToolTip(component.value("source").toString());
    }
    auto *componentButtons = new QHBoxLayout();
    auto *addScript = new QPushButton("Add Script…", components);
    auto *removeScript = new QPushButton("Remove", components);
    componentButtons->addWidget(addScript);
    componentButtons->addWidget(removeScript);
    componentsLayout->addWidget(componentList);
    auto *componentForm = new QFormLayout();
    auto *componentClass = new QLineEdit(components);
    auto *componentSource = new QLineEdit(components);
    componentSource->setReadOnly(true);
    auto *componentVariables = new QPlainTextEdit(components);
    componentVariables->setPlaceholderText("{}");
    componentVariables->setMaximumHeight(92);
    auto *applyComponent = new QPushButton("Apply Component", components);
    componentForm->addRow("Class", componentClass);
    componentForm->addRow("Source", componentSource);
    componentForm->addRow("Variables", componentVariables);
    componentsLayout->addLayout(componentForm);
    componentsLayout->addWidget(applyComponent);
    componentsLayout->addLayout(componentButtons);
    inspectorLayout->addWidget(components);
    connect(addScript, &QPushButton::clicked, this,
            &GraphiteEditorPanel::addScriptComponent);
    connect(removeScript, &QPushButton::clicked, this,
            [this, componentList] {
                componentList->setProperty("componentIndex",
                                           componentList->currentRow());
                removeScriptComponent();
            });
    auto displayComponent = [componentData, componentClass, componentSource,
                             componentVariables](int index) {
        const bool valid = index >= 0 && index < componentData.size();
        componentClass->setEnabled(valid);
        componentSource->setEnabled(valid);
        componentVariables->setEnabled(valid);
        if (!valid) {
            componentClass->clear();
            componentSource->clear();
            componentVariables->clear();
            return;
        }
        const QJsonObject component = componentData.at(index).toObject();
        componentClass->setText(component.value("className").toString());
        componentSource->setText(component.value("source").toString());
        const QJsonObject variables = component.value("variables").toObject();
        componentVariables->setPlainText(
            QString::fromUtf8(QJsonDocument(variables).toJson(
                QJsonDocument::Indented)));
    };
    connect(componentList, &QListWidget::currentRowChanged, this,
            displayComponent);
    connect(applyComponent, &QPushButton::clicked, this,
            [this, path, componentList, componentClass, componentVariables] {
                const int index = componentList->currentRow();
                QJsonObject changed = elementAtPath(path);
                QJsonArray entries = changed.value("components").toArray();
                if (index < 0 || index >= entries.size())
                    return;
                QJsonParseError error;
                const QJsonDocument variables = QJsonDocument::fromJson(
                    componentVariables->toPlainText().toUtf8(), &error);
                if (error.error != QJsonParseError::NoError ||
                    !variables.isObject()) {
                    QMessageBox::warning(this, "Graphite",
                                         "Component variables must be a JSON object.");
                    return;
                }
                QJsonObject component = entries.at(index).toObject();
                component.insert("className",
                                 componentClass->text().trimmed());
                component.insert("variables", variables.object());
                entries[index] = component;
                changed.insert("components", entries);
                replaceElement(path, changed);
            });
    componentList->setProperty("graphiteComponentList", true);
    if (!componentData.isEmpty())
        componentList->setCurrentRow(0);
    else
        displayComponent(-1);
    inspectorLayout->addStretch();
}

void GraphiteEditorPanel::refreshDocument(bool) {
    rebuildTree();
    rebuildInspector();
    canvas->setDocument(document, uiPath);
}

void GraphiteEditorPanel::addScriptComponent() {
    const QList<int> path = selectedPath();
    QJsonObject element = elementAtPath(path);
    if (element.isEmpty())
        return;
    const QString root = QFileInfo(projectFile).absolutePath();
    const QString source = QFileDialog::getOpenFileName(
        this, "Attach TypeScript Component", root, "TypeScript (*.ts)");
    if (source.isEmpty())
        return;
    QString className = QFileInfo(source).completeBaseName();
    className.remove(QRegularExpression("[^A-Za-z0-9_$]"));
    if (className.isEmpty())
        className = "UIComponent";
    if (className.front().isDigit())
        className.prepend("Component");
    bool accepted = false;
    className = QInputDialog::getText(this, "Script Component", "Class name",
                                      QLineEdit::Normal, className, &accepted)
                    .trimmed();
    if (!accepted || className.isEmpty())
        return;
    QJsonArray components = element.value("components").toArray();
    components.append(QJsonObject{
        {"type", "script"},
        {"source", QDir(QFileInfo(uiPath).absolutePath()).relativeFilePath(source)},
        {"className", className},
        {"variables", QJsonObject{}}});
    element.insert("components", components);
    replaceElement(path, element);
}

void GraphiteEditorPanel::removeScriptComponent() {
    const QList<int> path = selectedPath();
    QJsonObject element = elementAtPath(path);
    if (element.isEmpty())
        return;
    auto lists = inspectorBody->findChildren<QListWidget *>();
    int index = -1;
    for (QListWidget *list : lists) {
        if (list->property("graphiteComponentList").toBool()) {
            index = list->currentRow();
            break;
        }
    }
    QJsonArray components = element.value("components").toArray();
    if (index < 0 || index >= components.size())
        return;
    components.removeAt(index);
    element.insert("components", components);
    replaceElement(path, element);
}

void GraphiteEditorPanel::attachToScene(bool preview) {
    if (uiPath.isEmpty() || viewport == nullptr)
        return;
    saveUI();
    const QString scenePath = viewport->currentRuntimeScene();
    if (scenePath.isEmpty()) {
        QMessageBox::information(this, "Graphite",
                                 "Open a scene before attaching this UI.");
        return;
    }
    QFile input(scenePath);
    if (!input.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Graphite", "The scene could not be opened.");
        return;
    }
    QJsonParseError error;
    QJsonDocument parsed = QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject()) {
        QMessageBox::warning(this, "Graphite", "The scene is not valid JSON.");
        return;
    }
    QJsonObject scene = parsed.object();
    QJsonArray interfaces = scene.value("ui").toArray();
    const QString relative =
        QDir(QFileInfo(scenePath).absolutePath()).relativeFilePath(uiPath);
    bool found = false;
    for (const QJsonValue &value : interfaces) {
        if ((value.isString() && value.toString() == relative) ||
            (value.isObject() &&
             value.toObject().value("source").toString() == relative)) {
            found = true;
            break;
        }
    }
    if (!found)
        interfaces.append(relative);
    scene.insert("ui", interfaces);
    QSaveFile output(scenePath);
    const QByteArray contents =
        QJsonDocument(scene).toJson(QJsonDocument::Indented);
    if (!output.open(QIODevice::WriteOnly) ||
        output.write(contents) != contents.size() || !output.commit()) {
        QMessageBox::warning(this, "Graphite", "The scene could not be updated.");
        return;
    }
    statusLabel->setText("Attached to scene");
    if (preview) {
        auto connection = std::make_shared<QMetaObject::Connection>();
        *connection = connect(
            viewport, &ViewportPanel::runtimeAvailabilityChanged, this,
            [this, connection](bool available) {
                if (!available)
                    return;
                disconnect(*connection);
                if (viewport != nullptr)
                    viewport->setCameraFocused(true);
            });
        emit previewRequested();
    }
}
