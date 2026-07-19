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
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QSet>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QStringList bindingValues() {
    QStringList values;
    for (char letter = 'A'; letter <= 'Z'; ++letter)
        values.append(QString(QChar(letter)));
    values << "Space" << "Enter" << "Escape" << "Tab" << "Backspace"
           << "Up" << "Down" << "Left" << "Right" << "Left Shift"
           << "Right Shift" << "Left Control" << "Right Control"
           << "Left Alt" << "Right Alt" << "MouseLeft" << "MouseRight"
           << "MouseMiddle" << "Mouse4" << "Mouse5";
    return values;
}

QComboBox *bindingCombo(QWidget *parent) {
    auto *combo = new QComboBox(parent);
    combo->setEditable(true);
    combo->addItems(bindingValues());
    return combo;
}

QString kindName(InputActionsDialog::ActionKind kind) {
    if (kind == InputActionsDialog::ActionKind::Axis1D)
        return "1D Axis";
    if (kind == InputActionsDialog::ActionKind::Axis2D)
        return "2D Axis";
    return "Button";
}
}

InputActionsDialog::InputActionsDialog(const QString &projectFile,
                                       QWidget *parent)
    : QDialog(parent), projectFile(QFileInfo(projectFile).absoluteFilePath()),
      actionsFile(
          QFileInfo(projectFile).absoluteDir().filePath("input-actions.json")) {
    setupUi();
    load();
}

void InputActionsDialog::setupUi() {
    setWindowTitle("Project Input Actions");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(940, 680);
    setMinimumSize(780, 560);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *heading = new QLabel("Input Actions", this);
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

    auto *editor = new QWidget(splitter);
    auto *editorLayout = new QVBoxLayout(editor);
    editorLayout->setContentsMargins(12, 0, 0, 0);
    editorLayout->setSpacing(12);

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
        {"Source", "Key / Mouse", "Controller", "Button"});
    buttonBindings->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
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
    auto *directionForm = new QFormLayout();
    positiveXField = bindingCombo(axisPage);
    negativeXField = bindingCombo(axisPage);
    positiveYField = bindingCombo(axisPage);
    negativeYField = bindingCombo(axisPage);
    directionForm->addRow("Positive X", positiveXField);
    directionForm->addRow("Negative X", negativeXField);
    positiveYLabel = new QLabel("Positive Y", axisPage);
    negativeYLabel = new QLabel("Negative Y", axisPage);
    directionForm->addRow(positiveYLabel, positiveYField);
    directionForm->addRow(negativeYLabel, negativeYField);
    axisLayout->addLayout(directionForm);

    mouseAxisField = new QCheckBox("Include mouse movement", axisPage);
    controllerAxisField = new QCheckBox("Include controller axis", axisPage);
    axisLayout->addWidget(mouseAxisField);
    axisLayout->addWidget(controllerAxisField);
    auto *controllerForm = new QFormLayout();
    controllerIdField = new QSpinBox(axisPage);
    controllerIdField->setRange(-1, 15);
    controllerIdField->setSpecialValueText("Any");
    controllerAxisXField = new QSpinBox(axisPage);
    controllerAxisXField->setRange(0, 31);
    controllerAxisYField = new QSpinBox(axisPage);
    controllerAxisYField->setRange(0, 31);
    controllerAxisYLabel = new QLabel("Controller Y axis", axisPage);
    controllerForm->addRow("Controller", controllerIdField);
    controllerForm->addRow("Controller X axis", controllerAxisXField);
    controllerForm->addRow(controllerAxisYLabel, controllerAxisYField);
    axisLayout->addLayout(controllerForm);

    auto *processingForm = new QFormLayout();
    deadzoneField = new QDoubleSpinBox(axisPage);
    deadzoneField->setRange(0.0, 1.0);
    deadzoneField->setSingleStep(0.05);
    scaleXField = new QDoubleSpinBox(axisPage);
    scaleXField->setRange(-100.0, 100.0);
    scaleXField->setSingleStep(0.1);
    scaleYField = new QDoubleSpinBox(axisPage);
    scaleYField->setRange(-100.0, 100.0);
    scaleYField->setSingleStep(0.1);
    scaleYLabel = new QLabel("Y scale", axisPage);
    processingForm->addRow("Controller deadzone", deadzoneField);
    processingForm->addRow("X scale", scaleXField);
    processingForm->addRow(scaleYLabel, scaleYField);
    axisLayout->addLayout(processingForm);
    normalizeField = new QCheckBox("Normalize 2D value", axisPage);
    invertYField = new QCheckBox("Invert controller Y", axisPage);
    clampField = new QCheckBox("Clamp values to -1…1", axisPage);
    axisLayout->addWidget(normalizeField);
    axisLayout->addWidget(invertYField);
    axisLayout->addWidget(clampField);
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
    splitter->addWidget(editor);
    splitter->setSizes({270, 670});

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
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
    for (QCheckBox *field : {mouseAxisField, controllerAxisField,
                             normalizeField, invertYField, clampField})
        connect(field, &QCheckBox::toggled, this, markChanged);
    for (QSpinBox *field :
         {controllerIdField, controllerAxisXField, controllerAxisYField})
        connect(field, &QSpinBox::valueChanged, this, markChanged);
    for (QDoubleSpinBox *field : {deadzoneField, scaleXField, scaleYField})
        connect(field, &QDoubleSpinBox::valueChanged, this, markChanged);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        storeCurrentAction();
        if (save())
            accept();
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
            QMessageBox::warning(this, "Input Actions",
                                 "The existing input-actions.json is not valid "
                                 "JSON and could not be opened.");
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
                            action.controllerAxisX =
                                indexes.isEmpty() ? axis.value("index").toInt(0)
                                                  : indexes.at(0).toInt(0);
                            action.controllerAxisY =
                                indexes.size() > 1 ? indexes.at(1).toInt(1)
                                                   : action.controllerAxisX;
                        } else if (type.compare("custom",
                                                Qt::CaseInsensitive) == 0) {
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
    action.mouseAxis = mouseAxisField->isChecked();
    action.controllerAxis = controllerAxisField->isChecked();
    action.controllerId = controllerIdField->value();
    action.controllerAxisX = controllerAxisXField->value();
    action.controllerAxisY = controllerAxisYField->value();
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
            binding.value = buttonBindings->item(row, 1)->text().trimmed();
            binding.controllerId =
                qobject_cast<QSpinBox *>(buttonBindings->cellWidget(row, 2))
                    ->value();
            binding.controllerButton =
                qobject_cast<QSpinBox *>(buttonBindings->cellWidget(row, 3))
                    ->value();
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
    mouseAxisField->setChecked(action.mouseAxis);
    controllerAxisField->setChecked(action.controllerAxis);
    controllerIdField->setValue(action.controllerId);
    controllerAxisXField->setValue(action.controllerAxisX);
    controllerAxisYField->setValue(action.controllerAxisY);
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
            buttonBindings->setItem(row, 1,
                                    new QTableWidgetItem(binding.value));
            auto *controllerId = new QSpinBox(buttonBindings);
            controllerId->setRange(-1, 15);
            controllerId->setSpecialValueText("Any");
            controllerId->setValue(binding.controllerId);
            buttonBindings->setCellWidget(row, 2, controllerId);
            auto *controllerButton = new QSpinBox(buttonBindings);
            controllerButton->setRange(0, 255);
            controllerButton->setValue(binding.controllerButton);
            buttonBindings->setCellWidget(row, 3, controllerButton);
            connect(source, &QComboBox::currentTextChanged, this,
                    [this] { storeCurrentAction(); });
            connect(controllerId, &QSpinBox::valueChanged, this,
                    [this] { storeCurrentAction(); });
            connect(controllerButton, &QSpinBox::valueChanged, this,
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
                           {"button", binding.controllerButton}};
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
        binding.controllerButton = object.value("button").toInt(0);
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
                bindings.append(serializeButtonBinding(binding));
            }
            entry.insert("triggerButtons", bindings);
        } else {
            if (action.positiveX.isEmpty() || action.negativeX.isEmpty() ||
                (action.kind == ActionKind::Axis2D &&
                 (action.positiveY.isEmpty() || action.negativeY.isEmpty()))) {
                QMessageBox::warning(
                    this, "Input Actions",
                    QString("Complete the directional bindings for “%1”.")
                        .arg(action.name));
                return false;
            }
            QJsonArray triggers;
            QJsonObject custom{{"type", "custom"},
                               {"positiveX", action.positiveX},
                               {"negativeX", action.negativeX}};
            if (action.kind == ActionKind::Axis2D) {
                custom.insert("positiveY", action.positiveY);
                custom.insert("negativeY", action.negativeY);
            }
            triggers.append(custom);
            if (action.mouseAxis)
                triggers.append("mouse");
            if (action.controllerAxis) {
                QJsonObject controller{{"type", "controller"},
                                       {"id", action.controllerId}};
                if (action.kind == ActionKind::Axis2D)
                    controller.insert("indexes",
                                      QJsonArray{action.controllerAxisX,
                                                 action.controllerAxisY});
                else
                    controller.insert("index", action.controllerAxisX);
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

    QSaveFile file(actionsFile);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(QJsonObject{{"actions", entries}})
                       .toJson(QJsonDocument::Indented)) < 0 ||
        !file.commit()) {
        QMessageBox::critical(this, "Input Actions",
                              "Atlas could not save input-actions.json.");
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

    const QRegularExpression sectionExpression(
        QStringLiteral(R"((?m)^\[game\][ \t]*$)"));
    const QRegularExpressionMatch sectionMatch =
        sectionExpression.match(contents);
    if (!sectionMatch.hasMatch()) {
        if (!contents.endsWith('\n'))
            contents.append('\n');
        contents.append("\n[game]\ninput_actions = \"input-actions.json\"\n");
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
            gameSection.replace(valueExpression,
                                "\ninput_actions = \"input-actions.json\"");
        else
            gameSection.prepend("\ninput_actions = \"input-actions.json\"");
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
