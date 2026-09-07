#include <editor/views/inputActionsDialog.h>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QSet>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QStringList bindingValues() {
    QStringList values;
    for (char letter = 'A'; letter <= 'Z'; ++letter)
        values.append(QString(QChar(letter)));
    for (char digit = '0'; digit <= '9'; ++digit)
        values.append(QString(QChar(digit)));
    for (int functionKey = 1; functionKey <= 12; ++functionKey)
        values.append(QStringLiteral("F%1").arg(functionKey));
    values << "Space" << "Enter" << "Escape" << "Tab" << "Backspace"
           << "Insert" << "Delete" << "Home" << "End" << "Page Up"
           << "Page Down" << "Up" << "Down" << "Left" << "Right"
           << "Left Shift" << "Right Shift" << "Left Control"
           << "Right Control" << "Left Alt" << "Right Alt" << "Left Super"
           << "Right Super" << "MouseLeft" << "MouseRight" << "MouseMiddle"
           << "Mouse4" << "Mouse5";
    return values;
}

QComboBox *bindingCombo(QWidget *parent) {
    auto *combo = new QComboBox(parent);
    combo->setEditable(true);
    combo->addItems(bindingValues());
    return combo;
}

QStringList controllerButtonNames() {
    return {"A",          "B",           "X",          "Y",
            "Left Bumper", "Right Bumper", "Back",       "Start",
            "Guide",      "Left Thumb",  "Right Thumb", "D-Pad Up",
            "D-Pad Right", "D-Pad Down",  "D-Pad Left"};
}

QStringList controllerAxisNames() {
    return {"Left Stick X", "Left Stick Y", "Right Stick X",
            "Right Stick Y", "Left Trigger", "Right Trigger"};
}

QString controllerNameForIndex(const QStringList &names, int index) {
    return index >= 0 && index < names.size() ? names.at(index)
                                              : QString::number(index);
}

int controllerIndexForName(const QStringList &names, const QString &name) {
    for (int index = 0; index < names.size(); ++index) {
        if (names.at(index).compare(name, Qt::CaseInsensitive) == 0)
            return index;
    }
    bool valid = false;
    const int index = name.toInt(&valid);
    return valid ? index : -1;
}

QJsonValue controllerValue(const QStringList &names, const QString &name) {
    return controllerIndexForName(names, name) >= 0 &&
                   names.contains(name, Qt::CaseInsensitive)
               ? QJsonValue(name)
               : QJsonValue(controllerIndexForName(names, name));
}

QComboBox *controllerCombo(const QStringList &names, QWidget *parent) {
    auto *combo = new QComboBox(parent);
    combo->setEditable(true);
    combo->addItems(names);
    return combo;
}

QString kindName(InputActionsDialog::ActionKind kind) {
    if (kind == InputActionsDialog::ActionKind::Axis1D)
        return "1D Axis";
    if (kind == InputActionsDialog::ActionKind::Axis2D)
        return "2D Axis";
    return "Button";
}

QString configuredActionsPath(const QString &projectFile) {
    QFile file(projectFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QString contents = QString::fromUtf8(file.readAll());
    const QRegularExpression gameExpression(
        QStringLiteral(R"((?ms)^[ \t]*\[game\][ \t]*\r?\n(.*?)(?=^[ \t]*\[[^\]]+\][ \t]*\r?$|\z))"));
    const QRegularExpressionMatch gameMatch = gameExpression.match(contents);
    if (!gameMatch.hasMatch())
        return {};
    const QRegularExpression pathExpression(
        QStringLiteral(R"((?m)^[ \t]*input_actions[ \t]*=[ \t]*["']([^"']+)["'][ \t]*$)"));
    const QRegularExpressionMatch pathMatch =
        pathExpression.match(gameMatch.captured(1));
    return pathMatch.hasMatch() ? pathMatch.captured(1).trimmed() : QString();
}
}

QString InputActionsDialog::actionsFileForProject(const QString &projectFile) {
    const QFileInfo projectInfo(projectFile);
    const QString configured = configuredActionsPath(projectFile);
    if (configured.isEmpty())
        return projectInfo.absoluteDir().filePath("input-actions.json");
    const QFileInfo configuredInfo(configured);
    return configuredInfo.isAbsolute()
               ? configuredInfo.absoluteFilePath()
               : projectInfo.absoluteDir().absoluteFilePath(configured);
}

QStringList
InputActionsDialog::actionNamesForProject(const QString &projectFile) {
    QFile file(actionsFileForProject(projectFile));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return {};
    QStringList names;
    for (const QJsonValue &value :
         document.object().value("actions").toArray()) {
        const QString name = value.toObject().value("name").toString().trimmed();
        if (!name.isEmpty() && !names.contains(name, Qt::CaseInsensitive))
            names.append(name);
    }
    names.sort(Qt::CaseInsensitive);
    return names;
}

InputActionsDialog::InputActionsDialog(const QString &projectFile,
                                       QWidget *parent)
    : QDialog(parent), projectFile(QFileInfo(projectFile).absoluteFilePath()),
      actionsFile(actionsFileForProject(projectFile)) {
    setupUi();
    load();
}

void InputActionsDialog::setupUi() {
    setWindowTitle("Actions");
    setWindowFlag(Qt::Window, true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(1120, 760);
    setMinimumSize(900, 620);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *heading = new QLabel("Controller Actions", this);
    heading->setObjectName("dialogHeroTitle");
    QFont headingFont = heading->font();
    headingFont.setPointSizeF(18);
    headingFont.setWeight(QFont::DemiBold);
    heading->setFont(headingFont);
    root->addWidget(heading);
    auto *subheading = new QLabel(
        "Create project-wide names for keyboard, mouse, and controller input. "
        "Scripts can use the names without hard-coding keys.",
        this);
    subheading->setWordWrap(true);
    subheading->setProperty("muted", true);
    root->addWidget(subheading);

    auto *splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);
    root->addWidget(splitter, 1);

    auto *sidebar = new QWidget(splitter);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 8, 0);
    searchField = new QLineEdit(sidebar);
    searchField->setPlaceholderText("Search actions");
    searchField->setClearButtonEnabled(true);
    sidebarLayout->addWidget(searchField);
    actionList = new QListWidget(sidebar);
    actionList->setSelectionMode(QAbstractItemView::SingleSelection);
    sidebarLayout->addWidget(actionList, 1);

    auto *sidebarButtons = new QHBoxLayout();
    auto *addButton = new QPushButton("Add", sidebar);
    auto *addMenu = new QMenu(addButton);
    addMenu->addAction("Button", this,
                       [this] { addAction(ActionKind::Button); });
    addMenu->addAction("1D Axis", this,
                       [this] { addAction(ActionKind::Axis1D); });
    addMenu->addAction("2D Axis", this,
                       [this] { addAction(ActionKind::Axis2D); });
    addButton->setMenu(addMenu);
    duplicateButton = new QPushButton("Duplicate", sidebar);
    removeButton = new QPushButton("Remove", sidebar);
    sidebarButtons->addWidget(addButton);
    sidebarButtons->addWidget(duplicateButton);
    sidebarButtons->addWidget(removeButton);
    sidebarLayout->addLayout(sidebarButtons);

    auto *editorScroll = new QScrollArea(splitter);
    editorScroll->setWidgetResizable(true);
    editorScroll->setFrameShape(QFrame::NoFrame);
    auto *editor = new QWidget(editorScroll);
    auto *editorLayout = new QVBoxLayout(editor);
    editorLayout->setContentsMargins(16, 0, 8, 0);
    editorLayout->setSpacing(12);
    editorScroll->setWidget(editor);

    auto *identity = new QFormLayout();
    nameField = new QLineEdit(editor);
    nameField->setPlaceholderText("Action name");
    kindField = new QComboBox(editor);
    kindField->addItems({"Button", "1D Axis", "2D Axis"});
    identity->addRow("Name", nameField);
    identity->addRow("Type", kindField);
    editorLayout->addLayout(identity);

    bindingPages = new QStackedWidget(editor);
    auto *buttonPage = new QWidget(bindingPages);
    auto *buttonLayout = new QVBoxLayout(buttonPage);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonBindings = new QTableWidget(0, 4, buttonPage);
    buttonBindings->setHorizontalHeaderLabels(
        {"Source", "Key / Mouse", "Controller", "Controller Button"});
    buttonBindings->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    buttonBindings->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    buttonBindings->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    buttonBindings->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Stretch);
    buttonBindings->verticalHeader()->setVisible(false);
    buttonBindings->setSelectionBehavior(QAbstractItemView::SelectRows);
    buttonBindings->setSelectionMode(QAbstractItemView::SingleSelection);
    buttonLayout->addWidget(buttonBindings);
    auto *bindingButtons = new QHBoxLayout();
    auto *addBindingButton = new QPushButton("Add Binding", buttonPage);
    auto *removeBindingButton = new QPushButton("Remove Binding", buttonPage);
    bindingButtons->addWidget(addBindingButton);
    bindingButtons->addWidget(removeBindingButton);
    bindingButtons->addStretch();
    buttonLayout->addLayout(bindingButtons);
    bindingPages->addWidget(buttonPage);

    auto *axisPage = new QWidget(bindingPages);
    auto *axisLayout = new QVBoxLayout(axisPage);
    axisLayout->setContentsMargins(0, 0, 0, 0);
    auto *keyboardGroup = new QGroupBox("Keyboard", axisPage);
    auto *keyboardLayout = new QVBoxLayout(keyboardGroup);
    keyboardAxisField = new QCheckBox("Use keyboard bindings", keyboardGroup);
    keyboardLayout->addWidget(keyboardAxisField);
    auto *directionForm = new QFormLayout();
    positiveXField = bindingCombo(keyboardGroup);
    negativeXField = bindingCombo(keyboardGroup);
    positiveYField = bindingCombo(keyboardGroup);
    negativeYField = bindingCombo(keyboardGroup);
    directionForm->addRow("Positive X", positiveXField);
    directionForm->addRow("Negative X", negativeXField);
    positiveYLabel = new QLabel("Positive Y", axisPage);
    negativeYLabel = new QLabel("Negative Y", axisPage);
    directionForm->addRow(positiveYLabel, positiveYField);
    directionForm->addRow(negativeYLabel, negativeYField);
    keyboardLayout->addLayout(directionForm);
    axisLayout->addWidget(keyboardGroup);

    auto *mouseGroup = new QGroupBox("Mouse", axisPage);
    auto *mouseLayout = new QVBoxLayout(mouseGroup);
    mouseAxisField = new QCheckBox("Use mouse movement", mouseGroup);
    mouseLayout->addWidget(mouseAxisField);
    axisLayout->addWidget(mouseGroup);

    auto *controllerGroup = new QGroupBox("Controller", axisPage);
    auto *controllerLayout = new QVBoxLayout(controllerGroup);
    controllerAxisField =
        new QCheckBox("Use a named controller axis", controllerGroup);
    controllerLayout->addWidget(controllerAxisField);
    auto *controllerForm = new QFormLayout();
    controllerIdField = new QSpinBox(controllerGroup);
    controllerIdField->setRange(-1, 15);
    controllerIdField->setSpecialValueText("Any");
    controllerAxisXField =
        controllerCombo(controllerAxisNames(), controllerGroup);
    controllerAxisYField =
        controllerCombo(controllerAxisNames(), controllerGroup);
    controllerAxisYLabel = new QLabel("Y axis", controllerGroup);
    controllerForm->addRow("Controller", controllerIdField);
    controllerForm->addRow("X axis", controllerAxisXField);
    controllerForm->addRow(controllerAxisYLabel, controllerAxisYField);
    controllerLayout->addLayout(controllerForm);
    axisLayout->addWidget(controllerGroup);

    auto *processingGroup = new QGroupBox("Processing", axisPage);
    auto *processingLayout = new QVBoxLayout(processingGroup);
    auto *processingForm = new QFormLayout();
    deadzoneField = new QDoubleSpinBox(processingGroup);
    deadzoneField->setRange(0.0, 1.0);
    deadzoneField->setSingleStep(0.05);
    scaleXField = new QDoubleSpinBox(processingGroup);
    scaleXField->setRange(-100.0, 100.0);
    scaleXField->setSingleStep(0.1);
    scaleYField = new QDoubleSpinBox(processingGroup);
    scaleYField->setRange(-100.0, 100.0);
    scaleYField->setSingleStep(0.1);
    scaleYLabel = new QLabel("Y scale", processingGroup);
    processingForm->addRow("Controller deadzone", deadzoneField);
    processingForm->addRow("X scale", scaleXField);
    processingForm->addRow(scaleYLabel, scaleYField);
    processingLayout->addLayout(processingForm);
    normalizeField = new QCheckBox("Normalize 2D value", processingGroup);
    invertYField = new QCheckBox("Invert controller Y", processingGroup);
    clampField = new QCheckBox("Clamp values to -1…1", processingGroup);
    processingLayout->addWidget(normalizeField);
    processingLayout->addWidget(invertYField);
    processingLayout->addWidget(clampField);
    axisLayout->addWidget(processingGroup);
    axisLayout->addStretch();
    bindingPages->addWidget(axisPage);
    editorLayout->addWidget(bindingPages, 1);

    auto *exampleLabel = new QLabel("Use in a script", editor);
    QFont exampleFont = exampleLabel->font();
    exampleFont.setWeight(QFont::DemiBold);
    exampleLabel->setFont(exampleFont);
    editorLayout->addWidget(exampleLabel);
    scriptExample = new QPlainTextEdit(editor);
    scriptExample->setReadOnly(true);
    scriptExample->setMaximumHeight(86);
    scriptExample->setLineWrapMode(QPlainTextEdit::NoWrap);
    editorLayout->addWidget(scriptExample);

    splitter->addWidget(sidebar);
    splitter->addWidget(editorScroll);
    splitter->setSizes({290, 830});

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Close | QDialogButtonBox::Save, this);
    buttons->button(QDialogButtonBox::Save)->setText("Save Actions");
    root->addWidget(buttons);

    auto markChanged = [this] {
        if (!updating) {
            storeCurrentAction();
            refreshList();
        }
    };
    connect(searchField, &QLineEdit::textChanged, this,
            [this] { refreshList(); });
    connect(actionList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (updating || row < 0)
            return;
        selectAction(actionList->item(row)->data(Qt::UserRole).toInt());
    });
    connect(duplicateButton, &QPushButton::clicked, this,
            [this] { duplicateAction(); });
    connect(removeButton, &QPushButton::clicked, this,
            [this] { removeAction(); });
    connect(nameField, &QLineEdit::textEdited, this, markChanged);
    connect(kindField, &QComboBox::currentIndexChanged, this, markChanged);
    connect(addBindingButton, &QPushButton::clicked, this,
            [this] { addButtonBinding(); });
    connect(removeBindingButton, &QPushButton::clicked, this, [this] {
        if (currentIndex < 0 || buttonBindings->currentRow() < 0)
            return;
        actions[currentIndex].buttonBindings.removeAt(
            buttonBindings->currentRow());
        refreshBindingTable();
    });
    connect(buttonBindings, &QTableWidget::cellChanged, this, markChanged);
    for (QComboBox *field :
         {positiveXField, negativeXField, positiveYField, negativeYField})
        connect(field, &QComboBox::currentTextChanged, this, markChanged);
    for (QCheckBox *field : {keyboardAxisField, mouseAxisField,
                             controllerAxisField,
                             normalizeField, invertYField, clampField})
        connect(field, &QCheckBox::toggled, this, markChanged);
    connect(controllerAxisXField, &QComboBox::currentTextChanged, this,
            markChanged);
    connect(controllerAxisYField, &QComboBox::currentTextChanged, this,
            markChanged);
    for (QSpinBox *field : {controllerIdField})
        connect(field, &QSpinBox::valueChanged, this, markChanged);
    for (QDoubleSpinBox *field : {deadzoneField, scaleXField, scaleYField})
        connect(field, &QDoubleSpinBox::valueChanged, this, markChanged);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        storeCurrentAction();
        if (save())
            emit actionsSaved();
    });
}

void InputActionsDialog::load() {
    QFile file(actionsFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError ||
            !document.isObject()) {
            QMessageBox::warning(
                this, "Input Actions",
                QStringLiteral("The existing %1 file is not valid JSON and "
                               "could not be opened.")
                    .arg(QFileInfo(actionsFile).fileName()));
        } else {
            const QJsonArray entries =
                document.object().value("actions").toArray();
            for (const QJsonValue &entry : entries) {
                const QJsonObject object = entry.toObject();
                ActionDefinition action;
                action.name = object.value("name").toString();
                if (object.value("triggerButtons").isArray()) {
                    for (const QJsonValue &binding :
                         object.value("triggerButtons").toArray())
                        action.buttonBindings.append(
                            parseButtonBinding(binding));
                } else {
                    action.kind = object.value("singleAxis").toBool(false)
                                      ? ActionKind::Axis1D
                                      : ActionKind::Axis2D;
                    action.keyboardAxis = false;
                    for (const QJsonValue &trigger :
                         object.value("triggerAxes").toArray()) {
                        if (trigger.isString() &&
                            trigger.toString().compare(
                                "mouse", Qt::CaseInsensitive) == 0) {
                            action.mouseAxis = true;
                            continue;
                        }
                        const QJsonObject axis = trigger.toObject();
                        const QString type = axis.value("type").toString();
                        if (type.compare("controller", Qt::CaseInsensitive) ==
                            0) {
                            action.controllerAxis = true;
                            action.controllerId = axis.value("id").toInt(-1);
                            const QJsonArray indexes =
                                axis.value("indexes").toArray();
                            const QJsonValue x = indexes.isEmpty()
                                                     ? axis.value("index")
                                                     : indexes.at(0);
                            const QJsonValue y = indexes.size() > 1
                                                     ? indexes.at(1)
                                                     : axis.contains("indexY")
                                                           ? axis.value(
                                                                 "indexY")
                                                           : x;
                            action.controllerAxisX =
                                x.isString()
                                    ? x.toString()
                                    : controllerNameForIndex(
                                          controllerAxisNames(), x.toInt(0));
                            action.controllerAxisY =
                                y.isString()
                                    ? y.toString()
                                    : controllerNameForIndex(
                                          controllerAxisNames(), y.toInt(1));
                        } else if (type.compare("custom",
                                                Qt::CaseInsensitive) == 0) {
                            action.keyboardAxis = true;
                            const QJsonArray directions =
                                axis.value("triggers").toArray();
                            action.positiveX =
                                directions.isEmpty()
                                    ? axis.value("positiveX")
                                          .toString(axis.value("positive")
                                                        .toString("D"))
                                    : directions.at(0).toString("D");
                            action.negativeX =
                                directions.size() < 2
                                    ? axis.value("negativeX")
                                          .toString(axis.value("negative")
                                                        .toString("A"))
                                    : directions.at(1).toString("A");
                            action.positiveY =
                                directions.size() < 3
                                    ? axis.value("positiveY").toString("W")
                                    : directions.at(2).toString("W");
                            action.negativeY =
                                directions.size() < 4
                                    ? axis.value("negativeY").toString("S")
                                    : directions.at(3).toString("S");
                        }
                    }
                    action.deadzone =
                        object.value("controllerDeadzone").toDouble(0.2);
                    action.scaleX = object.value("axisScaleX").toDouble(1.0);
                    action.scaleY = object.value("axisScaleY").toDouble(1.0);
                    action.normalize =
                        object.value("normalize2D").toBool(false);
                    action.invertY =
                        object.value("invertControllerY").toBool(false);
                    action.clamp = object.value("clampAxis").toBool(true);
                }
                if (!action.name.isEmpty())
                    actions.append(action);
            }
        }
    }
    refreshList();
    if (!actions.isEmpty())
        selectAction(0);
    else
        refreshEditor();
}

void InputActionsDialog::addAction(ActionKind kind) {
    storeCurrentAction();
    ActionDefinition action;
    action.kind = kind;
    action.name = uniqueName(kind == ActionKind::Button ? "NewAction" : "Move");
    if (kind == ActionKind::Button)
        action.buttonBindings.append(ButtonBinding());
    actions.append(action);
    searchField->clear();
    refreshList();
    selectAction(actions.size() - 1);
    nameField->setFocus();
    nameField->selectAll();
}

void InputActionsDialog::duplicateAction() {
    if (currentIndex < 0)
        return;
    storeCurrentAction();
    ActionDefinition copy = actions.at(currentIndex);
    copy.name = uniqueName(copy.name + "Copy");
    actions.insert(currentIndex + 1, copy);
    searchField->clear();
    refreshList();
    selectAction(currentIndex + 1);
}

void InputActionsDialog::removeAction() {
    if (currentIndex < 0)
        return;
    const int next = qMin(currentIndex, actions.size() - 2);
    actions.removeAt(currentIndex);
    currentIndex = -1;
    refreshList();
    if (next >= 0)
        selectAction(next);
    else
        refreshEditor();
}

void InputActionsDialog::selectAction(int index) {
    if (index < 0 || index >= actions.size())
        return;
    if (currentIndex != index)
        storeCurrentAction();
    currentIndex = index;
    refreshEditor();
    for (int row = 0; row < actionList->count(); ++row) {
        if (actionList->item(row)->data(Qt::UserRole).toInt() == index) {
            actionList->setCurrentRow(row);
            break;
        }
    }
}

void InputActionsDialog::storeCurrentAction() {
    if (updating || currentIndex < 0 || currentIndex >= actions.size())
        return;
    ActionDefinition &action = actions[currentIndex];
    action.name = nameField->text().trimmed();
    action.kind = static_cast<ActionKind>(kindField->currentIndex());
    action.positiveX = positiveXField->currentText().trimmed();
    action.negativeX = negativeXField->currentText().trimmed();
    action.positiveY = positiveYField->currentText().trimmed();
    action.negativeY = negativeYField->currentText().trimmed();
    action.keyboardAxis = keyboardAxisField->isChecked();
    action.mouseAxis = mouseAxisField->isChecked();
    action.controllerAxis = controllerAxisField->isChecked();
    action.controllerId = controllerIdField->value();
    action.controllerAxisX = controllerAxisXField->currentText().trimmed();
    action.controllerAxisY = controllerAxisYField->currentText().trimmed();
    action.deadzone = deadzoneField->value();
    action.scaleX = scaleXField->value();
    action.scaleY = scaleYField->value();
    action.normalize = normalizeField->isChecked();
    action.invertY = invertYField->isChecked();
    action.clamp = clampField->isChecked();
    if (action.kind == ActionKind::Button) {
        for (int row = 0; row < buttonBindings->rowCount() &&
                          row < action.buttonBindings.size();
             ++row) {
            ButtonBinding &binding = action.buttonBindings[row];
            binding.source =
                qobject_cast<QComboBox *>(buttonBindings->cellWidget(row, 0))
                    ->currentText();
            binding.value =
                qobject_cast<QComboBox *>(buttonBindings->cellWidget(row, 1))
                    ->currentText()
                    .trimmed();
            binding.controllerId =
                qobject_cast<QSpinBox *>(buttonBindings->cellWidget(row, 2))
                    ->value();
            binding.controllerButton =
                qobject_cast<QComboBox *>(buttonBindings->cellWidget(row, 3))
                    ->currentText();
        }
    }
    bindingPages->setCurrentIndex(action.kind == ActionKind::Button ? 0 : 1);
    const bool is2D = action.kind == ActionKind::Axis2D;
    positiveYLabel->setVisible(is2D);
    positiveYField->setVisible(is2D);
    negativeYLabel->setVisible(is2D);
    negativeYField->setVisible(is2D);
    controllerAxisYLabel->setVisible(is2D);
    controllerAxisYField->setVisible(is2D);
    scaleYLabel->setVisible(is2D);
    scaleYField->setVisible(is2D);
    normalizeField->setVisible(is2D);
    invertYField->setVisible(is2D);
    for (QComboBox *field :
         {positiveXField, negativeXField, positiveYField, negativeYField})
        field->setEnabled(action.keyboardAxis);
    controllerIdField->setEnabled(action.controllerAxis);
    controllerAxisXField->setEnabled(action.controllerAxis);
    controllerAxisYField->setEnabled(action.controllerAxis);
    refreshScriptExample();
}

void InputActionsDialog::refreshList() {
    const QString filter = searchField->text().trimmed();
    const int selected = currentIndex;
    updating = true;
    actionList->clear();
    for (int i = 0; i < actions.size(); ++i) {
        const ActionDefinition &action = actions.at(i);
        if (!filter.isEmpty() &&
            !action.name.contains(filter, Qt::CaseInsensitive))
            continue;
        auto *item = new QListWidgetItem(
            QString("%1\n%2").arg(action.name, kindName(action.kind)),
            actionList);
        item->setData(Qt::UserRole, i);
        item->setSizeHint(QSize(0, 48));
        if (i == selected)
            actionList->setCurrentItem(item);
    }
    updating = false;
}

void InputActionsDialog::refreshEditor() {
    const bool enabled = currentIndex >= 0 && currentIndex < actions.size();
    nameField->setEnabled(enabled);
    kindField->setEnabled(enabled);
    bindingPages->setEnabled(enabled);
    duplicateButton->setEnabled(enabled);
    removeButton->setEnabled(enabled);
    if (!enabled) {
        updating = true;
        nameField->clear();
        scriptExample->setPlainText(
            "Add an action to create a project-wide input name.");
        updating = false;
        return;
    }

    const ActionDefinition &action = actions.at(currentIndex);
    updating = true;
    nameField->setText(action.name);
    kindField->setCurrentIndex(static_cast<int>(action.kind));
    bindingPages->setCurrentIndex(action.kind == ActionKind::Button ? 0 : 1);
    positiveXField->setCurrentText(action.positiveX);
    negativeXField->setCurrentText(action.negativeX);
    positiveYField->setCurrentText(action.positiveY);
    negativeYField->setCurrentText(action.negativeY);
    keyboardAxisField->setChecked(action.keyboardAxis);
    mouseAxisField->setChecked(action.mouseAxis);
    controllerAxisField->setChecked(action.controllerAxis);
    controllerIdField->setValue(action.controllerId);
    controllerAxisXField->setCurrentText(action.controllerAxisX);
    controllerAxisYField->setCurrentText(action.controllerAxisY);
    deadzoneField->setValue(action.deadzone);
    scaleXField->setValue(action.scaleX);
    scaleYField->setValue(action.scaleY);
    normalizeField->setChecked(action.normalize);
    invertYField->setChecked(action.invertY);
    clampField->setChecked(action.clamp);
    const bool is2D = action.kind == ActionKind::Axis2D;
    positiveYLabel->setVisible(is2D);
    positiveYField->setVisible(is2D);
    negativeYLabel->setVisible(is2D);
    negativeYField->setVisible(is2D);
    controllerAxisYLabel->setVisible(is2D);
    controllerAxisYField->setVisible(is2D);
    scaleYLabel->setVisible(is2D);
    scaleYField->setVisible(is2D);
    normalizeField->setVisible(is2D);
    invertYField->setVisible(is2D);
    for (QComboBox *field :
         {positiveXField, negativeXField, positiveYField, negativeYField})
        field->setEnabled(action.keyboardAxis);
    controllerIdField->setEnabled(action.controllerAxis);
    controllerAxisXField->setEnabled(action.controllerAxis);
    controllerAxisYField->setEnabled(action.controllerAxis);
    updating = false;
    refreshBindingTable();
    refreshScriptExample();
}

void InputActionsDialog::refreshBindingTable() {
    updating = true;
    buttonBindings->setRowCount(0);
    if (currentIndex >= 0 && currentIndex < actions.size()) {
        const auto &bindings = actions.at(currentIndex).buttonBindings;
        for (int row = 0; row < bindings.size(); ++row) {
            const ButtonBinding &binding = bindings.at(row);
            buttonBindings->insertRow(row);
            auto *source = new QComboBox(buttonBindings);
            source->addItems({"Keyboard", "Mouse", "Controller"});
            source->setCurrentText(binding.source);
            buttonBindings->setCellWidget(row, 0, source);
            auto *value = bindingCombo(buttonBindings);
            value->setCurrentText(binding.value);
            buttonBindings->setCellWidget(row, 1, value);
            auto *controllerId = new QSpinBox(buttonBindings);
            controllerId->setRange(-1, 15);
            controllerId->setSpecialValueText("Any");
            controllerId->setValue(binding.controllerId);
            buttonBindings->setCellWidget(row, 2, controllerId);
            auto *controllerButton =
                controllerCombo(controllerButtonNames(), buttonBindings);
            controllerButton->setCurrentText(binding.controllerButton);
            buttonBindings->setCellWidget(row, 3, controllerButton);
            auto updateRow = [source, value, controllerId, controllerButton] {
                const bool controller = source->currentText() == "Controller";
                value->setEnabled(!controller);
                controllerId->setEnabled(controller);
                controllerButton->setEnabled(controller);
            };
            updateRow();
            connect(source, &QComboBox::currentTextChanged, this,
                    [this, source, value, updateRow] {
                        if (source->currentText() == "Mouse" &&
                            !value->currentText().startsWith(
                                "Mouse", Qt::CaseInsensitive))
                            value->setCurrentText("MouseLeft");
                        else if (source->currentText() == "Keyboard" &&
                                 value->currentText().startsWith(
                                     "Mouse", Qt::CaseInsensitive))
                            value->setCurrentText("Space");
                        updateRow();
                        storeCurrentAction();
                    });
            connect(controllerId, &QSpinBox::valueChanged, this,
                    [this] { storeCurrentAction(); });
            connect(controllerButton, &QComboBox::currentTextChanged, this,
                    [this] { storeCurrentAction(); });
            connect(value, &QComboBox::currentTextChanged, this,
                    [this] { storeCurrentAction(); });
        }
    }
    updating = false;
}

void InputActionsDialog::refreshScriptExample() {
    if (currentIndex < 0)
        return;
    const ActionDefinition &action = actions.at(currentIndex);
    if (action.kind == ActionKind::Button) {
        scriptExample->setPlainText(
            QString(
                "import { Input } from \"atlas/input\";\n\nif "
                "(Input.isActionTriggered(\"%1\")) {\n    performAction();\n}")
                .arg(action.name));
    } else {
        scriptExample->setPlainText(
            QString("import { Input } from \"atlas/input\";\n\nconst axis = "
                    "Input.getAxisActionValue(\"%1\");\nmove(axis.valueX, "
                    "axis.valueY);")
                .arg(action.name));
    }
}

void InputActionsDialog::addButtonBinding() {
    if (currentIndex < 0)
        return;
    actions[currentIndex].buttonBindings.append(ButtonBinding());
    refreshBindingTable();
    buttonBindings->selectRow(buttonBindings->rowCount() - 1);
}

QJsonValue
InputActionsDialog::serializeButtonBinding(const ButtonBinding &binding) const {
    if (binding.source.compare("Controller", Qt::CaseInsensitive) == 0) {
        return QJsonObject{{"type", "controller"},
                           {"id", binding.controllerId},
                           {"button", controllerValue(controllerButtonNames(),
                                                      binding.controllerButton)}};
    }
    if (binding.source.compare("Mouse", Qt::CaseInsensitive) == 0) {
        return QJsonObject{{"type", "mouse"}, {"button", binding.value}};
    }
    return binding.value;
}

InputActionsDialog::ButtonBinding
InputActionsDialog::parseButtonBinding(const QJsonValue &value) const {
    ButtonBinding binding;
    if (value.isString()) {
        binding.value = value.toString();
        binding.source = binding.value.startsWith("Mouse", Qt::CaseInsensitive)
                             ? "Mouse"
                             : "Keyboard";
        return binding;
    }
    const QJsonObject object = value.toObject();
    const QString type = object.value("type").toString();
    if (type.compare("controller", Qt::CaseInsensitive) == 0) {
        binding.source = "Controller";
        binding.controllerId = object.value("id").toInt(-1);
        const QJsonValue button = object.value("button");
        binding.controllerButton =
            button.isString()
                ? button.toString()
                : controllerNameForIndex(controllerButtonNames(),
                                         button.toInt(0));
    } else if (type.compare("mouse", Qt::CaseInsensitive) == 0) {
        binding.source = "Mouse";
        binding.value = object.value("button").toString("MouseLeft");
    } else {
        binding.source = "Keyboard";
        binding.value = object.value("key").toString("Space");
    }
    return binding;
}

QString InputActionsDialog::uniqueName(const QString &base) const {
    QString candidate = base;
    int suffix = 2;
    auto exists = [this](const QString &name) {
        for (const ActionDefinition &action : actions) {
            if (action.name.compare(name, Qt::CaseInsensitive) == 0)
                return true;
        }
        return false;
    };
    while (exists(candidate))
        candidate = base + QString::number(suffix++);
    return candidate;
}

bool InputActionsDialog::save() {
    QSet<QString> names;
    QJsonArray entries;
    for (const ActionDefinition &action : actions) {
        if (action.name.isEmpty()) {
            QMessageBox::warning(this, "Input Actions",
                                 "Every action needs a name.");
            return false;
        }
        const QString normalized = action.name.toLower();
        if (names.contains(normalized)) {
            QMessageBox::warning(
                this, "Input Actions",
                QString("The action name “%1” is used more than once.")
                    .arg(action.name));
            return false;
        }
        names.insert(normalized);
        QJsonObject entry{{"name", action.name}};
        if (action.kind == ActionKind::Button) {
            if (action.buttonBindings.isEmpty()) {
                QMessageBox::warning(
                    this, "Input Actions",
                    QString("Add at least one binding to “%1”.")
                        .arg(action.name));
                return false;
            }
            QJsonArray bindings;
            for (const ButtonBinding &binding : action.buttonBindings) {
                if ((binding.source != "Keyboard" &&
                     binding.source != "Mouse" &&
                     binding.source != "Controller") ||
                    (binding.source != "Controller" &&
                     binding.value.trimmed().isEmpty())) {
                    QMessageBox::warning(
                        this, "Input Actions",
                        QString("Complete every binding for “%1”.")
                            .arg(action.name));
                    return false;
                }
                if (binding.source == "Controller" &&
                    controllerIndexForName(controllerButtonNames(),
                                           binding.controllerButton) < 0) {
                    QMessageBox::warning(
                        this, "Input Actions",
                        QString("Choose a valid named controller button for "
                                "“%1”.")
                            .arg(action.name));
                    return false;
                }
                bindings.append(serializeButtonBinding(binding));
            }
            entry.insert("triggerButtons", bindings);
        } else {
            if (!action.keyboardAxis && !action.mouseAxis &&
                !action.controllerAxis) {
                QMessageBox::warning(
                    this, "Input Actions",
                    QString("Choose at least one input source for “%1”.")
                        .arg(action.name));
                return false;
            }
            if (action.keyboardAxis &&
                (action.positiveX.isEmpty() || action.negativeX.isEmpty() ||
                 (action.kind == ActionKind::Axis2D &&
                  (action.positiveY.isEmpty() ||
                   action.negativeY.isEmpty())))) {
                QMessageBox::warning(
                    this, "Input Actions",
                    QString("Complete the keyboard bindings for “%1”.")
                        .arg(action.name));
                return false;
            }
            if (action.controllerAxis &&
                (controllerIndexForName(controllerAxisNames(),
                                        action.controllerAxisX) < 0 ||
                 (action.kind == ActionKind::Axis2D &&
                  controllerIndexForName(controllerAxisNames(),
                                         action.controllerAxisY) < 0))) {
                QMessageBox::warning(
                    this, "Input Actions",
                    QString("Choose valid named controller axes for “%1”.")
                        .arg(action.name));
                return false;
            }
            QJsonArray triggers;
            if (action.keyboardAxis) {
                QJsonObject custom{{"type", "custom"},
                                   {"positiveX", action.positiveX},
                                   {"negativeX", action.negativeX}};
                if (action.kind == ActionKind::Axis2D) {
                    custom.insert("positiveY", action.positiveY);
                    custom.insert("negativeY", action.negativeY);
                }
                triggers.append(custom);
            }
            if (action.mouseAxis)
                triggers.append("mouse");
            if (action.controllerAxis) {
                QJsonObject controller{{"type", "controller"},
                                       {"id", action.controllerId}};
                if (action.kind == ActionKind::Axis2D)
                    controller.insert(
                        "indexes",
                        QJsonArray{controllerValue(controllerAxisNames(),
                                                   action.controllerAxisX),
                                   controllerValue(controllerAxisNames(),
                                                   action.controllerAxisY)});
                else
                    controller.insert(
                        "index", controllerValue(controllerAxisNames(),
                                                 action.controllerAxisX));
                triggers.append(controller);
            }
            entry.insert("triggerAxes", triggers);
            entry.insert("singleAxis", action.kind == ActionKind::Axis1D);
            entry.insert("controllerDeadzone", action.deadzone);
            entry.insert("axisScaleX", action.scaleX);
            entry.insert("axisScaleY", action.scaleY);
            entry.insert("normalize2D", action.normalize);
            entry.insert("invertControllerY", action.invertY);
            entry.insert("clampAxis", action.clamp);
        }
        entries.append(entry);
    }

    if (!QDir().mkpath(QFileInfo(actionsFile).absolutePath())) {
        QMessageBox::critical(
            this, "Input Actions",
            QStringLiteral("Atlas could not create the folder for %1.")
                .arg(QFileInfo(actionsFile).fileName()));
        return false;
    }
    QSaveFile file(actionsFile);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(QJsonObject{{"actions", entries}})
                       .toJson(QJsonDocument::Indented)) < 0 ||
        !file.commit()) {
        QMessageBox::critical(
            this, "Input Actions",
            QStringLiteral("Atlas could not save %1.")
                .arg(QFileInfo(actionsFile).fileName()));
        return false;
    }
    QString manifestError;
    if (!updateProjectManifest(&manifestError)) {
        QMessageBox::critical(this, "Input Actions", manifestError);
        return false;
    }
    return true;
}

bool InputActionsDialog::updateProjectManifest(QString *errorMessage) {
    QFile file(projectFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errorMessage = "Atlas could not open project.atlas.";
        return false;
    }
    QString contents = QString::fromUtf8(file.readAll());
    file.close();

    QString manifestPath = QDir(QFileInfo(projectFile).absolutePath())
                               .relativeFilePath(actionsFile);
    if (manifestPath.startsWith("../"))
        manifestPath = actionsFile;
    manifestPath = QDir::fromNativeSeparators(manifestPath);
    manifestPath.replace('\\', "\\\\").replace('"', "\\\"");
    const QString inputLine =
        QStringLiteral("input_actions = \"%1\"").arg(manifestPath);

    const QRegularExpression sectionExpression(
        QStringLiteral(R"((?m)^\[game\][ \t]*$)"));
    const QRegularExpressionMatch sectionMatch =
        sectionExpression.match(contents);
    if (!sectionMatch.hasMatch()) {
        if (!contents.endsWith('\n'))
            contents.append('\n');
        contents.append("\n[game]\n" + inputLine + '\n');
    } else {
        const int sectionStart = sectionMatch.capturedEnd();
        const QRegularExpression nextSectionExpression(
            QStringLiteral(R"((?m)^\[[^\]]+\][ \t]*$)"));
        const QRegularExpressionMatch nextSection =
            nextSectionExpression.match(contents, sectionStart);
        const int sectionEnd = nextSection.hasMatch()
                                   ? nextSection.capturedStart()
                                   : contents.size();
        QString gameSection =
            contents.mid(sectionStart, sectionEnd - sectionStart);
        const QRegularExpression valueExpression(
            QStringLiteral(R"((?m)^[ \t]*input_actions[ \t]*=.*$)"));
        if (gameSection.contains(valueExpression))
            gameSection.replace(valueExpression, inputLine);
        else
            gameSection.prepend('\n' + inputLine);
        contents.replace(sectionStart, sectionEnd - sectionStart, gameSection);
    }

    QSaveFile output(projectFile);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text) ||
        output.write(contents.toUtf8()) < 0 || !output.commit()) {
        *errorMessage = "Atlas could not update project.atlas.";
        return false;
    }
    return true;
}
