#include <editor/views/postProcessing.h>
#include <editor/widgets/scrubbableSpinBox.h>
#include <editor/styling/icons.h>

#include <editor/views/viewport.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QString effectTitle(const QString &type) {
    QString result;
    for (int index = 0; index < type.size(); ++index) {
        const QChar character = type.at(index);
        if (character == '_' || character == '-') {
            result += ' ';
        } else if (index > 0 && character.isUpper() &&
                   type.at(index - 1).isLower()) {
            result += ' ';
            result += character;
        } else {
            result += index == 0 ? character.toUpper() : character;
        }
    }
    return result;
}

QJsonObject effectDefaults(const QString &type) {
    const QString normalized = type.toLower().remove('_').remove('-');
    if (normalized == "blur")
        return {{"type", "blur"}, {"magnitude", 16.0}};
    if (normalized == "colorcorrection")
        return {{"type", "color_correction"},
                {"exposure", 1.0},
                {"contrast", 1.0},
                {"saturation", 1.0},
                {"gamma", 1.0},
                {"temperature", 0.0},
                {"tint", 0.0}};
    if (normalized == "motionblur")
        return {{"type", "motion_blur"}, {"size", 8}, {"separation", 1.0}};
    if (normalized == "chromaticaberration")
        return {{"type", "chromatic_aberration"},
                {"red", 0.01},
                {"green", 0.0},
                {"blue", -0.01},
                {"direction", QJsonArray{1.0, 0.0}}};
    if (normalized == "posterization")
        return {{"type", "posterization"}, {"levels", 8.0}};
    if (normalized == "pixelation")
        return {{"type", "pixelation"}, {"pixelSize", 4}};
    if (normalized == "dilation")
        return {{"type", "dilation"}, {"size", 3}, {"separation", 1.0}};
    if (normalized == "filmgrain")
        return {{"type", "film_grain"}, {"amount", 0.15}};
    return {{"type", type}};
}

QDoubleSpinBox *effectNumber(double value, QWidget *parent) {
    auto *field = new ScrubbableDoubleSpinBox(parent);
    field->setRange(-10000.0, 10000.0);
    field->setDecimals(3);
    field->setSingleStep(0.05);
    field->setValue(value);
    field->setKeyboardTracking(true);
    return field;
}
}

PostProcessingPanel::PostProcessingPanel(ViewportPanel *viewport,
                                         QWidget *parent)
    : QWidget(parent), viewport(viewport) {
    setObjectName("postProcessingPanel");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("postProcessingToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);
    toolbarLayout->setSpacing(4);
    auto *title = new QLabel("Post Processing", toolbar);
    title->setObjectName("postProcessingTitle");
    targetSelector = new QComboBox(toolbar);
    targetSelector->setMinimumWidth(180);
    auto *addTargetButton = new QToolButton(toolbar);
    addTargetButton->setIcon(
        styling::icon(styling::Icon::Plus, "#8498A8"));
    addTargetButton->setText("Target");
    addTargetButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    removeTargetButton = new QToolButton(toolbar);
    removeTargetButton->setText("Remove");
    removeTargetButton->setIcon(
        styling::icon(styling::Icon::Trash, "#A17F7F"));
    statusLabel = new QLabel(toolbar);
    statusLabel->setObjectName("postProcessingStatus");
    toolbarLayout->addWidget(title);
    toolbarLayout->addWidget(targetSelector);
    toolbarLayout->addWidget(addTargetButton);
    toolbarLayout->addWidget(removeTargetButton);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(statusLabel);
    layout->addWidget(toolbar);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("postProcessingScroll");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    body = new QWidget(scroll);
    body->setObjectName("postProcessingBody");
    bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(12, 12, 12, 14);
    bodyLayout->setSpacing(9);
    scroll->setWidget(body);
    layout->addWidget(scroll, 1);

    connect(targetSelector, &QComboBox::currentIndexChanged, this,
            [this](int index) {
                targetIndex = index;
                rebuildEditor();
            });
    connect(addTargetButton, &QToolButton::clicked, this,
            &PostProcessingPanel::addTarget);
    connect(removeTargetButton, &QToolButton::clicked, this,
            &PostProcessingPanel::removeTarget);
    if (viewport != nullptr) {
        connect(viewport, &ViewportPanel::sceneSnapshotChanged, this,
                &PostProcessingPanel::applySceneSnapshot);
        connect(viewport, &ViewportPanel::runtimeAvailabilityChanged, this,
                [this](bool available) {
                    if (available)
                        statusLabel->setText("Preview up to date");
                });
    }
    rebuildTargetList();
}

void PostProcessingPanel::applySceneSnapshot(const QString &snapshot) {
    const QJsonDocument document = QJsonDocument::fromJson(snapshot.toUtf8());
    if (!document.isObject())
        return;
    const QJsonObject root = document.object();
    const QJsonArray nextTargets = root.value("targets").toArray();
    const QJsonObject nextEnvironment = root.value("environment").toObject();
    if (nextTargets == targets && nextEnvironment == environment)
        return;
    targets = nextTargets;
    environment = nextEnvironment;
    if (!applying) {
        rebuildTargetList();
    }
}

void PostProcessingPanel::rebuildTargetList() {
    const int previous = targetIndex;
    targetSelector->blockSignals(true);
    targetSelector->clear();
    for (int index = 0; index < targets.size(); ++index) {
        const QJsonObject target = targets.at(index).toObject();
        targetSelector->addItem(
            target.value("name").toString(
                QStringLiteral("Render Target %1").arg(index + 1)));
    }
    targetIndex = targets.isEmpty()
                      ? -1
                      : std::clamp(previous, 0,
                                   static_cast<int>(targets.size()) - 1);
    targetSelector->setCurrentIndex(targetIndex);
    targetSelector->blockSignals(false);
    removeTargetButton->setEnabled(targetIndex >= 0);
    rebuildEditor();
}

void PostProcessingPanel::rebuildEditor() {
    while (QLayoutItem *item = bodyLayout->takeAt(0)) {
        if (item->widget() != nullptr)
            item->widget()->deleteLater();
        delete item;
    }

    const QJsonObject lightBloom = environment.value("lightBloom").toObject();
    auto *bloom = new QGroupBox("Bloom", body);
    auto *bloomForm = new QFormLayout(bloom);
    auto *threshold =
        effectNumber(lightBloom.value("threshold").toDouble(0.8), bloom);
    threshold->setRange(0.0, 10000.0);
    threshold->setSingleStep(0.05);
    bloomForm->addRow("Threshold", threshold);
    bodyLayout->addWidget(bloom);
    connect(threshold, &QDoubleSpinBox::valueChanged, this,
            [this](double value) { setBloomThreshold(value); });

    if (targetIndex < 0 || targetIndex >= targets.size()) {
        auto *empty = new QLabel(
            "Create a render target to build a post-processing stack.", body);
        empty->setObjectName("postProcessingEmpty");
        empty->setAlignment(Qt::AlignCenter);
        bodyLayout->addWidget(empty, 1);
        return;
    }

    const QJsonObject target = targets.at(targetIndex).toObject();
    auto *settings = new QGroupBox("Render Target", body);
    auto *form = new QFormLayout(settings);
    auto *name = new QLineEdit(target.value("name").toString(), settings);
    auto *type = new QComboBox(settings);
    type->addItems({"scene", "multisampled"});
    type->setCurrentText(target.value("type").toString("scene"));
    auto *render = new QCheckBox(settings);
    render->setChecked(target.value("render").toBool(true));
    auto *display = new QCheckBox(settings);
    display->setChecked(target.value("display").toBool(true));
    form->addRow("Name", name);
    form->addRow("Type", type);
    form->addRow("Render Scene", render);
    form->addRow("Display Output", display);
    bodyLayout->addWidget(settings);
    connect(name, &QLineEdit::editingFinished, this,
            [this, name] { setTargetValue("/name", name->text()); });
    connect(type, &QComboBox::currentTextChanged, this,
            [this](const QString &value) { setTargetValue("/type", value); });
    connect(render, &QCheckBox::toggled, this,
            [this](bool value) { setTargetValue("/render", value); });
    connect(display, &QCheckBox::toggled, this,
            [this](bool value) { setTargetValue("/display", value); });

    auto *effectsHeading = new QWidget(body);
    auto *effectsHeadingLayout = new QHBoxLayout(effectsHeading);
    effectsHeadingLayout->setContentsMargins(0, 4, 0, 0);
    auto *effectsTitle = new QLabel("Effect Stack", effectsHeading);
    effectsTitle->setObjectName("postProcessingSectionTitle");
    auto *addEffectButton = new QToolButton(effectsHeading);
    addEffectButton->setIcon(
        styling::icon(styling::Icon::Plus, "#8498A8"));
    addEffectButton->setText("Add Effect");
    addEffectButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addEffectButton->setPopupMode(QToolButton::InstantPopup);
    auto *effectMenu = new QMenu(addEffectButton);
    const QStringList effects{
        "inversion",          "grayscale",      "sharpen",
        "blur",               "edge_detection", "color_correction",
        "motion_blur",        "chromatic_aberration",
        "posterization",      "pixelation",     "dilation",
        "film_grain"};
    for (const QString &effect : effects) {
        effectMenu->addAction(
            styling::icon(styling::Icon::Sparkle, "#849589"),
            effectTitle(effect), this,
                              [this, effect] { addEffect(effect); });
    }
    addEffectButton->setMenu(effectMenu);
    effectsHeadingLayout->addWidget(effectsTitle);
    effectsHeadingLayout->addStretch();
    effectsHeadingLayout->addWidget(addEffectButton);
    bodyLayout->addWidget(effectsHeading);

    const QJsonArray effectStack = target.value("effects").toArray();
    for (int effectIndex = 0; effectIndex < effectStack.size(); ++effectIndex) {
        const QJsonObject effect = effectStack.at(effectIndex).toObject();
        const QString effectType = effect.value("type").toString("effect");
        auto *group = new QGroupBox(effectTitle(effectType), body);
        auto *groupLayout = new QVBoxLayout(group);
        auto *effectForm = new QFormLayout();
        groupLayout->addLayout(effectForm);
        for (auto iterator = effect.begin(); iterator != effect.end();
             ++iterator) {
            if (iterator.key() == "type")
                continue;
            if (iterator.value().isDouble()) {
                const bool integral = iterator.key() == "size" ||
                                      iterator.key() == "pixelSize";
                if (integral) {
                    auto *field = new ScrubbableSpinBox(group);
                    field->setRange(1, 1024);
                    field->setValue(iterator.value().toInt());
                    effectForm->addRow(effectTitle(iterator.key()), field);
                    connect(field, &QSpinBox::valueChanged, this,
                            [this, effectIndex, key = iterator.key()](int value) {
                                setEffectValue(effectIndex, key, value);
                            });
                } else {
                    auto *field =
                        effectNumber(iterator.value().toDouble(), group);
                    effectForm->addRow(effectTitle(iterator.key()), field);
                    connect(field, &QDoubleSpinBox::valueChanged, this,
                            [this, effectIndex,
                             key = iterator.key()](double value) {
                                setEffectValue(effectIndex, key, value);
                            });
                }
            } else if (iterator.value().isArray() &&
                       iterator.value().toArray().size() == 2) {
                const QJsonArray vector = iterator.value().toArray();
                auto *vectorEditor = new QWidget(group);
                auto *vectorLayout = new QHBoxLayout(vectorEditor);
                vectorLayout->setContentsMargins(0, 0, 0, 0);
                auto *x = effectNumber(vector.at(0).toDouble(), vectorEditor);
                auto *y = effectNumber(vector.at(1).toDouble(), vectorEditor);
                vectorLayout->addWidget(x);
                vectorLayout->addWidget(y);
                effectForm->addRow(effectTitle(iterator.key()), vectorEditor);
                auto commit = [this, effectIndex, key = iterator.key(), x, y] {
                    setEffectValue(effectIndex, key,
                                   QJsonArray{x->value(), y->value()});
                };
                connect(x, &QDoubleSpinBox::valueChanged, this,
                        [commit](double) { commit(); });
                connect(y, &QDoubleSpinBox::valueChanged, this,
                        [commit](double) { commit(); });
            }
        }
        auto *actions = new QWidget(group);
        auto *actionsLayout = new QHBoxLayout(actions);
        actionsLayout->setContentsMargins(0, 0, 0, 0);
        actionsLayout->setSpacing(4);
        auto *moveUp = new QToolButton(actions);
        moveUp->setText("Move Up");
        moveUp->setIcon(
            styling::icon(styling::Icon::CaretUp, "#7E929C"));
        moveUp->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        moveUp->setEnabled(effectIndex > 0);
        auto *moveDown = new QToolButton(actions);
        moveDown->setText("Move Down");
        moveDown->setIcon(
            styling::icon(styling::Icon::CaretDown, "#7E929C"));
        moveDown->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        moveDown->setEnabled(effectIndex + 1 < effectStack.size());
        auto *remove = new QToolButton(actions);
        remove->setText("Remove Effect");
        remove->setIcon(
            styling::icon(styling::Icon::Trash, "#A17F7F"));
        remove->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        actionsLayout->addStretch();
        actionsLayout->addWidget(moveUp);
        actionsLayout->addWidget(moveDown);
        actionsLayout->addWidget(remove);
        groupLayout->addWidget(actions);
        connect(moveUp, &QToolButton::clicked, this,
                [this, effectIndex] { moveEffect(effectIndex, -1); });
        connect(moveDown, &QToolButton::clicked, this,
                [this, effectIndex] { moveEffect(effectIndex, 1); });
        connect(remove, &QToolButton::clicked, this,
                [this, effectIndex] { removeEffect(effectIndex); });
        bodyLayout->addWidget(group);
    }
    bodyLayout->addStretch();
}

void PostProcessingPanel::addTarget() {
    const int number = targets.size() + 1;
    targets.append(QJsonObject{{"name", QStringLiteral("Render Target %1").arg(number)},
                               {"type", "scene"},
                               {"render", true},
                               {"display", targets.isEmpty()},
                               {"effects", QJsonArray{}}});
    targetIndex = targets.size() - 1;
    replaceTargets();
    rebuildTargetList();
}

void PostProcessingPanel::removeTarget() {
    if (targetIndex < 0 || targetIndex >= targets.size())
        return;
    targets.removeAt(targetIndex);
    targetIndex =
        std::min(targetIndex, static_cast<int>(targets.size()) - 1);
    replaceTargets();
    rebuildTargetList();
}

void PostProcessingPanel::addEffect(const QString &type) {
    if (targetIndex < 0 || targetIndex >= targets.size())
        return;
    QJsonObject target = targets.at(targetIndex).toObject();
    QJsonArray effects = target.value("effects").toArray();
    effects.append(effectDefaults(type));
    target.insert("effects", effects);
    targets.replace(targetIndex, target);
    setTargetValue("/effects", effects);
    rebuildEditor();
}

void PostProcessingPanel::removeEffect(int effectIndex) {
    if (targetIndex < 0 || targetIndex >= targets.size())
        return;
    QJsonObject target = targets.at(targetIndex).toObject();
    QJsonArray effects = target.value("effects").toArray();
    if (effectIndex < 0 || effectIndex >= effects.size())
        return;
    effects.removeAt(effectIndex);
    target.insert("effects", effects);
    targets.replace(targetIndex, target);
    setTargetValue("/effects", effects);
    rebuildEditor();
}

void PostProcessingPanel::moveEffect(int effectIndex, int offset) {
    if (targetIndex < 0 || targetIndex >= targets.size())
        return;
    QJsonObject target = targets.at(targetIndex).toObject();
    QJsonArray effects = target.value("effects").toArray();
    const int destination = effectIndex + offset;
    if (effectIndex < 0 || effectIndex >= effects.size() || destination < 0 ||
        destination >= effects.size())
        return;
    const QJsonValue effect = effects.at(effectIndex);
    effects.removeAt(effectIndex);
    effects.insert(destination, effect);
    target.insert("effects", effects);
    targets.replace(targetIndex, target);
    setTargetValue("/effects", effects);
    rebuildEditor();
}

void PostProcessingPanel::setTargetValue(const QString &path,
                                         const QJsonValue &value) {
    if (targetIndex < 0 || targetIndex >= targets.size() || viewport == nullptr)
        return;
    QJsonObject target = targets.at(targetIndex).toObject();
    target.insert(path.mid(1), value);
    targets.replace(targetIndex, target);
    applying = true;
    if (!viewport->setRuntimeSceneProperty("targets", targetIndex, path, value)) {
        applying = false;
        return;
    }
    applying = false;
    statusLabel->setText("Saved · return to Scene to update preview");
    emit settingsChanged();
    if (path == "/name")
        rebuildTargetList();
}

void PostProcessingPanel::setEffectValue(int effectIndex, const QString &key,
                                         const QJsonValue &value) {
    if (targetIndex < 0 || targetIndex >= targets.size())
        return;
    QJsonObject target = targets.at(targetIndex).toObject();
    QJsonArray effects = target.value("effects").toArray();
    if (effectIndex < 0 || effectIndex >= effects.size())
        return;
    QJsonObject effect = effects.at(effectIndex).toObject();
    effect.insert(key, value);
    effects.replace(effectIndex, effect);
    target.insert("effects", effects);
    targets.replace(targetIndex, target);
    setTargetValue("/effects", effects);
}

void PostProcessingPanel::setBloomThreshold(double value) {
    if (viewport == nullptr)
        return;
    QJsonObject lightBloom = environment.value("lightBloom").toObject();
    lightBloom.insert("threshold", value);
    environment.insert("lightBloom", lightBloom);
    applying = true;
    if (!viewport->setRuntimeSceneProperty("environment", -1,
                                           "/lightBloom/threshold", value)) {
        applying = false;
        return;
    }
    applying = false;
    statusLabel->setText("Saved · return to Scene to update preview");
    emit settingsChanged();
}

void PostProcessingPanel::replaceTargets() {
    if (viewport == nullptr)
        return;
    applying = true;
    if (!viewport->setRuntimeSceneProperty("targets", -1, QString(), targets)) {
        applying = false;
        return;
    }
    applying = false;
    statusLabel->setText("Saved · return to Scene to update preview");
    emit settingsChanged();
}
