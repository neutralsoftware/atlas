#include <editor/views/environmentEditor.h>

#include <editor/views/viewport.h>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QJsonObject environmentDefaults() {
    return {
        {"automaticAmbient", true},
        {"atmosphereSky", true},
        {"lookupTexture", ""},
        {"fog", QJsonObject{{"color", QJsonArray{0.7, 0.78, 1.0}},
                            {"intensity", 0.0}}},
        {"volumetricLighting",
         QJsonObject{{"enabled", false},
                     {"density", 0.35},
                     {"weight", 0.02},
                     {"decay", 0.95},
                     {"exposure", 0.7}}},
        {"lightBloom", QJsonObject{{"radius", 0.01}, {"maxSamples", 6}}},
        {"rimLight", QJsonObject{{"intensity", 0.0},
                                 {"color", QJsonArray{1.0, 0.96, 0.86}}}},
        {"atmosphere",
         QJsonObject{
             {"enabled", true},
             {"cycle", false},
             {"timeOfDay", 12.0},
             {"secondsPerHour", 180.0},
             {"wind", QJsonArray{0.0, 0.0, 0.0}},
             {"sunColor", QJsonArray{1.0, 0.95, 0.84}},
             {"moonColor", QJsonArray{0.59, 0.59, 0.82}},
             {"sunSize", 1.0},
             {"moonSize", 1.0},
             {"sunTintStrength", 0.35},
             {"moonTintStrength", 0.8},
             {"starIntensity", 2.5},
             {"globalLight",
              QJsonObject{{"enabled", true},
                          {"castsShadows", true},
                          {"shadowResolution", 4096}}},
             {"clouds",
              QJsonObject{{"enabled", false},
                          {"frequency", 4},
                          {"divisions", 6},
                          {"position", QJsonArray{0.0, 100.0, 0.0}},
                          {"size", QJsonArray{500.0, 80.0, 500.0}},
                          {"scale", 1.5},
                          {"offset", QJsonArray{0.0, 0.0, 0.0}},
                          {"density", 0.45},
                          {"densityMultiplier", 1.5},
                          {"absorption", 1.1},
                          {"scattering", 0.85},
                          {"phase", 0.55},
                          {"clusterStrength", 0.5},
                          {"primaryStepCount", 12},
                          {"lightStepCount", 6},
                          {"lightStepMultiplier", 1.6},
                          {"minStepLength", 0.05},
                          {"wind", QJsonArray{0.03, 0.0, 0.02}}}},
             {"weather",
              QJsonObject{{"enabled", false},
                          {"condition", "clear"},
                          {"intensity", 0.0},
                          {"wind", QJsonArray{0.0, -0.4, 0.0}}}}}}};
}

QJsonObject mergeObjects(QJsonObject base, const QJsonObject &values) {
    for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
        if (iterator.value().isObject() &&
            base.value(iterator.key()).isObject()) {
            base.insert(iterator.key(),
                        mergeObjects(base.value(iterator.key()).toObject(),
                                     iterator.value().toObject()));
        } else {
            base.insert(iterator.key(), iterator.value());
        }
    }
    return base;
}

void insertValue(QJsonObject &object, const QStringList &parts, int index,
                 const QJsonValue &value) {
    if (index >= parts.size())
        return;
    if (index + 1 == parts.size()) {
        object.insert(parts.at(index), value);
        return;
    }
    QJsonObject child = object.value(parts.at(index)).toObject();
    insertValue(child, parts, index + 1, value);
    object.insert(parts.at(index), child);
}

QJsonValue valueAt(const QJsonObject &object, const QString &path) {
    QJsonValue value(object);
    for (const QString &part : path.split('/', Qt::SkipEmptyParts)) {
        if (!value.isObject())
            return {};
        value = value.toObject().value(part);
    }
    return value;
}

QDoubleSpinBox *numberBox(double value, double minimum, double maximum,
                          double step, int decimals, QWidget *parent) {
    auto *field = new QDoubleSpinBox(parent);
    field->setRange(minimum, maximum);
    field->setDecimals(decimals);
    field->setSingleStep(step);
    field->setKeyboardTracking(true);
    field->setValue(value);
    return field;
}
}

EnvironmentEditorPanel::EnvironmentEditorPanel(ViewportPanel *viewport,
                                               QWidget *parent)
    : QWidget(parent), viewport(viewport), environment(environmentDefaults()) {
    setObjectName("environmentEditor");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("workspaceToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 5, 8, 5);
    toolbarLayout->setSpacing(5);
    auto *title = new QLabel("World", toolbar);
    title->setObjectName("workspaceTitle");
    statusLabel = new QLabel("Ready", toolbar);
    statusLabel->setObjectName("workspaceStatus");
    auto *automatic = new QCheckBox("Auto Apply", toolbar);
    automatic->setObjectName("workspaceAutoApply");
    auto *apply = new QToolButton(toolbar);
    apply->setObjectName("workspaceApplyButton");
    apply->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    apply->setText("Apply");
    apply->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    apply->setToolTip("Reload the runtime with these world settings");
    toolbarLayout->addWidget(title);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(statusLabel);
    toolbarLayout->addWidget(automatic);
    toolbarLayout->addWidget(apply);
    layout->addWidget(toolbar);

    auto *workspace = new QSplitter(Qt::Horizontal, this);
    workspace->setObjectName("environmentWorkspace");
    categories = new QListWidget(workspace);
    categories->setObjectName("environmentCategories");
    categories->setIconSize(QSize(18, 18));
    categories->setFixedWidth(152);
    categories->setSpacing(1);
    pages = new QStackedWidget(workspace);
    pages->setObjectName("environmentPages");
    workspace->addWidget(categories);
    workspace->addWidget(pages);
    workspace->setStretchFactor(1, 1);
    layout->addWidget(workspace, 1);

    previewTimer = new QTimer(this);
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(450);
    connect(previewTimer, &QTimer::timeout, this,
            &EnvironmentEditorPanel::applyPreview);
    connect(automatic, &QCheckBox::toggled, this,
            [this](bool enabled) { autoApply = enabled; });
    connect(apply, &QToolButton::clicked, this,
            &EnvironmentEditorPanel::applyPreview);
    connect(categories, &QListWidget::currentRowChanged, pages,
            &QStackedWidget::setCurrentIndex);
    if (viewport != nullptr) {
        connect(viewport, &ViewportPanel::sceneSnapshotChanged, this,
                &EnvironmentEditorPanel::applySceneSnapshot);
        connect(viewport, &ViewportPanel::runtimeAvailabilityChanged, this,
                [this](bool available) {
                    if (available)
                        statusLabel->setText("Preview up to date");
                });
    }
    rebuildEditor();
}

void EnvironmentEditorPanel::applySceneSnapshot(const QString &snapshot) {
    const QJsonDocument document = QJsonDocument::fromJson(snapshot.toUtf8());
    if (!document.isObject())
        return;
    const QJsonObject next = document.object().value("environment").toObject();
    if (next.isEmpty() || next == environment)
        return;
    environment = mergeObjects(environmentDefaults(), next);
    if (!applying)
        rebuildEditor();
}

QWidget *EnvironmentEditorPanel::createPage(const QString &title,
                                            const QString &subtitle) {
    auto *scroll = new QScrollArea(pages);
    scroll->setObjectName("environmentScroll");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *page = new QWidget(scroll);
    page->setObjectName("environmentPage");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 10, 12, 14);
    layout->setSpacing(7);
    auto *heading = new QLabel(title, page);
    heading->setObjectName("environmentPageTitle");
    auto *description = new QLabel(subtitle, page);
    description->setObjectName("environmentPageSubtitle");
    description->setWordWrap(true);
    layout->addWidget(heading);
    layout->addWidget(description);
    scroll->setWidget(page);
    pages->addWidget(scroll);
    return page;
}

QFormLayout *EnvironmentEditorPanel::addSection(QWidget *page,
                                                const QString &title) {
    auto *group = new QGroupBox(title, page);
    group->setObjectName("environmentSection");
    auto *form = new QFormLayout(group);
    form->setContentsMargins(9, 9, 9, 8);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(5);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto *layout = qobject_cast<QVBoxLayout *>(page->layout());
    layout->addWidget(group);
    return form;
}

void EnvironmentEditorPanel::addBoolean(QFormLayout *form,
                                        const QString &label,
                                        const QString &path) {
    auto *field = new QCheckBox(form->parentWidget());
    field->setChecked(environmentValue(path).toBool());
    connect(field, &QCheckBox::toggled, this,
            [this, path](bool value) { setEnvironmentValue(path, value); });
    form->addRow(label, field);
}

void EnvironmentEditorPanel::addNumber(QFormLayout *form, const QString &label,
                                       const QString &path, double minimum,
                                       double maximum, double step,
                                       int decimals) {
    auto *field =
        numberBox(environmentValue(path).toDouble(), minimum, maximum, step,
                  decimals, form->parentWidget());
    connect(field, &QDoubleSpinBox::valueChanged, this,
            [this, path](double value) { setEnvironmentValue(path, value); });
    form->addRow(label, field);
}

void EnvironmentEditorPanel::addInteger(QFormLayout *form,
                                        const QString &label,
                                        const QString &path, int minimum,
                                        int maximum) {
    auto *field = new QSpinBox(form->parentWidget());
    field->setRange(minimum, maximum);
    field->setValue(environmentValue(path).toInt());
    field->setKeyboardTracking(true);
    connect(field, &QSpinBox::valueChanged, this,
            [this, path](int value) { setEnvironmentValue(path, value); });
    form->addRow(label, field);
}

void EnvironmentEditorPanel::addVector(QFormLayout *form, const QString &label,
                                       const QString &path) {
    QJsonArray values = environmentValue(path).toArray();
    while (values.size() < 3)
        values.append(0.0);
    auto *editor = new QFrame(form->parentWidget());
    editor->setObjectName("compactVectorField");
    auto *layout = new QHBoxLayout(editor);
    layout->setContentsMargins(3, 0, 3, 0);
    layout->setSpacing(2);
    QList<QDoubleSpinBox *> fields;
    const QStringList axes{"X", "Y", "Z"};
    for (int index = 0; index < 3; ++index) {
        auto *axis = new QLabel(axes.at(index), editor);
        axis->setObjectName("compactAxisLabel");
        auto *field = numberBox(values.at(index).toDouble(), -100000.0,
                                100000.0, 0.05, 3, editor);
        field->setButtonSymbols(QAbstractSpinBox::NoButtons);
        fields.append(field);
        layout->addWidget(axis);
        layout->addWidget(field, 1);
    }
    auto commit = [this, path, fields] {
        setEnvironmentValue(path,
                            QJsonArray{fields.at(0)->value(),
                                       fields.at(1)->value(),
                                       fields.at(2)->value()});
    };
    for (QDoubleSpinBox *field : fields) {
        connect(field, &QDoubleSpinBox::valueChanged, this,
                [commit](double) { commit(); });
    }
    form->addRow(label, editor);
}

void EnvironmentEditorPanel::addColor(QFormLayout *form, const QString &label,
                                      const QString &path) {
    QJsonArray values = environmentValue(path).toArray();
    while (values.size() < 3)
        values.append(1.0);
    const bool normalized =
        std::all_of(values.begin(), values.end(), [](const QJsonValue &value) {
            return value.toDouble() <= 1.0;
        });
    const double factor = normalized ? 255.0 : 1.0;
    QColor color(std::clamp(static_cast<int>(values.at(0).toDouble() * factor),
                            0, 255),
                 std::clamp(static_cast<int>(values.at(1).toDouble() * factor),
                            0, 255),
                 std::clamp(static_cast<int>(values.at(2).toDouble() * factor),
                            0, 255));
    auto *button = new QPushButton(color.name(QColor::HexRgb).toUpper(),
                                   form->parentWidget());
    button->setObjectName("compactColorButton");
    button->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    button->setStyleSheet(QStringLiteral("background-color: %1;")
                              .arg(color.name(QColor::HexRgb)));
    connect(button, &QPushButton::clicked, this,
            [this, button, path, color, normalized]() mutable {
                const QColor next = QColorDialog::getColor(color, this,
                                                           "Choose Color");
                if (!next.isValid())
                    return;
                color = next;
                button->setText(color.name(QColor::HexRgb).toUpper());
                button->setStyleSheet(
                    QStringLiteral("background-color: %1;")
                        .arg(color.name(QColor::HexRgb)));
                const double divisor = normalized ? 255.0 : 1.0;
                setEnvironmentValue(
                    path, QJsonArray{color.red() / divisor,
                                     color.green() / divisor,
                                     color.blue() / divisor});
            });
    form->addRow(label, button);
}

void EnvironmentEditorPanel::addText(QFormLayout *form, const QString &label,
                                     const QString &path,
                                     const QStringList &choices) {
    if (!choices.isEmpty()) {
        auto *field = new QComboBox(form->parentWidget());
        field->addItems(choices);
        field->setCurrentText(environmentValue(path).toString());
        connect(field, &QComboBox::currentTextChanged, this,
                [this, path](const QString &value) {
                    setEnvironmentValue(path, value);
                });
        form->addRow(label, field);
        return;
    }
    auto *field = new QLineEdit(environmentValue(path).toString(),
                                form->parentWidget());
    connect(field, &QLineEdit::editingFinished, this,
            [this, field, path] { setEnvironmentValue(path, field->text()); });
    form->addRow(label, field);
}

void EnvironmentEditorPanel::rebuildEditor() {
    const int current = std::max(0, categories->currentRow());
    categories->blockSignals(true);
    categories->clear();
    while (pages->count() > 0) {
        QWidget *page = pages->widget(0);
        pages->removeWidget(page);
        page->deleteLater();
    }

    const QList<QPair<QString, QStyle::StandardPixmap>> entries{
        {"World", QStyle::SP_ComputerIcon},
        {"Atmosphere", QStyle::SP_DesktopIcon},
        {"Lighting", QStyle::SP_MessageBoxInformation},
        {"Clouds", QStyle::SP_DriveNetIcon},
        {"Weather", QStyle::SP_BrowserReload}};
    for (const auto &[label, icon] : entries)
        categories->addItem(
            new QListWidgetItem(style()->standardIcon(icon), label));

    QWidget *world = createPage(
        "World", "Scene-wide ambience, fog and image-based color shaping.");
    QFormLayout *worldMode = addSection(world, "Surface");
    addBoolean(worldMode, "Atmosphere Sky", "/atmosphereSky");
    addBoolean(worldMode, "Automatic Ambient", "/automaticAmbient");
    addText(worldMode, "Lookup Texture", "/lookupTexture");
    QFormLayout *fog = addSection(world, "Fog");
    addColor(fog, "Color", "/fog/color");
    addNumber(fog, "Intensity", "/fog/intensity", 0.0, 10.0, 0.001, 4);
    QFormLayout *volume = addSection(world, "Volumetric Lighting");
    addBoolean(volume, "Enabled", "/volumetricLighting/enabled");
    addNumber(volume, "Density", "/volumetricLighting/density", 0.0, 10.0);
    addNumber(volume, "Weight", "/volumetricLighting/weight", 0.0, 10.0);
    addNumber(volume, "Decay", "/volumetricLighting/decay", 0.0, 1.0);
    addNumber(volume, "Exposure", "/volumetricLighting/exposure", 0.0,
              20.0);
    QFormLayout *bloom = addSection(world, "Bloom & Rim Light");
    addNumber(bloom, "Bloom Radius", "/lightBloom/radius", 0.0, 1.0,
              0.001, 4);
    addInteger(bloom, "Bloom Samples", "/lightBloom/maxSamples", 1, 64);
    addNumber(bloom, "Rim Intensity", "/rimLight/intensity", 0.0, 20.0);
    addColor(bloom, "Rim Color", "/rimLight/color");
    qobject_cast<QVBoxLayout *>(world->layout())->addStretch();

    QWidget *atmosphere = createPage(
        "Atmosphere", "Procedural sky, sun, moon and day-night simulation.");
    QFormLayout *sky = addSection(atmosphere, "Sky");
    addBoolean(sky, "Enabled", "/atmosphere/enabled");
    addBoolean(sky, "Day-Night Cycle", "/atmosphere/cycle");
    addNumber(sky, "Time of Day", "/atmosphere/timeOfDay", 0.0, 24.0, 0.1,
              2);
    addNumber(sky, "Seconds per Hour", "/atmosphere/secondsPerHour", 0.01,
              100000.0, 1.0, 2);
    addVector(sky, "Global Wind", "/atmosphere/wind");
    QFormLayout *celestial = addSection(atmosphere, "Celestial Bodies");
    addColor(celestial, "Sun Color", "/atmosphere/sunColor");
    addColor(celestial, "Moon Color", "/atmosphere/moonColor");
    addNumber(celestial, "Sun Size", "/atmosphere/sunSize", 0.01, 100.0);
    addNumber(celestial, "Moon Size", "/atmosphere/moonSize", 0.01, 100.0);
    addNumber(celestial, "Sun Tint", "/atmosphere/sunTintStrength", 0.0,
              10.0);
    addNumber(celestial, "Moon Tint", "/atmosphere/moonTintStrength", 0.0,
              10.0);
    addNumber(celestial, "Stars", "/atmosphere/starIntensity", 0.0, 100.0);
    qobject_cast<QVBoxLayout *>(atmosphere->layout())->addStretch();

    QWidget *lighting = createPage(
        "Atmosphere Lighting",
        "Directional lighting generated automatically from the sky.");
    QFormLayout *global = addSection(lighting, "Global Light");
    addBoolean(global, "Enabled", "/atmosphere/globalLight/enabled");
    addBoolean(global, "Cast Shadows",
               "/atmosphere/globalLight/castsShadows");
    addInteger(global, "Shadow Resolution",
               "/atmosphere/globalLight/shadowResolution", 128, 16384);
    qobject_cast<QVBoxLayout *>(lighting->layout())->addStretch();

    QWidget *clouds = createPage(
        "Volumetric Clouds", "Raymarched cloud volume and wind controls.");
    QFormLayout *cloudShape = addSection(clouds, "Volume");
    addBoolean(cloudShape, "Enabled", "/atmosphere/clouds/enabled");
    addInteger(cloudShape, "Frequency", "/atmosphere/clouds/frequency", 1,
               64);
    addInteger(cloudShape, "Divisions", "/atmosphere/clouds/divisions", 1,
               16);
    addVector(cloudShape, "Position", "/atmosphere/clouds/position");
    addVector(cloudShape, "Size", "/atmosphere/clouds/size");
    addVector(cloudShape, "Offset", "/atmosphere/clouds/offset");
    addVector(cloudShape, "Wind", "/atmosphere/clouds/wind");
    addNumber(cloudShape, "Scale", "/atmosphere/clouds/scale", 0.001,
              1000.0);
    QFormLayout *cloudDensity = addSection(clouds, "Density & Light");
    addNumber(cloudDensity, "Density", "/atmosphere/clouds/density", 0.0,
              10.0);
    addNumber(cloudDensity, "Density Multiplier",
              "/atmosphere/clouds/densityMultiplier", 0.0, 20.0);
    addNumber(cloudDensity, "Absorption", "/atmosphere/clouds/absorption",
              0.0, 20.0);
    addNumber(cloudDensity, "Scattering", "/atmosphere/clouds/scattering",
              0.0, 20.0);
    addNumber(cloudDensity, "Phase", "/atmosphere/clouds/phase", -1.0, 1.0);
    addNumber(cloudDensity, "Cluster Strength",
              "/atmosphere/clouds/clusterStrength", 0.0, 20.0);
    QFormLayout *cloudQuality = addSection(clouds, "Quality");
    addInteger(cloudQuality, "Primary Steps",
               "/atmosphere/clouds/primaryStepCount", 1, 256);
    addInteger(cloudQuality, "Light Steps",
               "/atmosphere/clouds/lightStepCount", 1, 128);
    addNumber(cloudQuality, "Light Step Multiplier",
              "/atmosphere/clouds/lightStepMultiplier", 0.01, 100.0);
    addNumber(cloudQuality, "Minimum Step Length",
              "/atmosphere/clouds/minStepLength", 0.001, 100.0, 0.01, 4);
    qobject_cast<QVBoxLayout *>(clouds->layout())->addStretch();

    QWidget *weather = createPage(
        "Weather", "Static precipitation state and directional wind.");
    QFormLayout *weatherState = addSection(weather, "Weather State");
    addBoolean(weatherState, "Enabled", "/atmosphere/weather/enabled");
    addText(weatherState, "Condition", "/atmosphere/weather/condition",
            {"clear", "rain", "snow", "storm"});
    addNumber(weatherState, "Intensity", "/atmosphere/weather/intensity",
              0.0, 1.0, 0.01, 2);
    addVector(weatherState, "Wind", "/atmosphere/weather/wind");
    qobject_cast<QVBoxLayout *>(weather->layout())->addStretch();

    categories->setCurrentRow(std::min(current, categories->count() - 1));
    pages->setCurrentIndex(categories->currentRow());
    categories->blockSignals(false);
}

void EnvironmentEditorPanel::setEnvironmentValue(const QString &path,
                                                  const QJsonValue &value) {
    if (viewport == nullptr)
        return;
    insertValue(environment, path.split('/', Qt::SkipEmptyParts), 0, value);
    applying = true;
    const bool saved =
        viewport->setRuntimeSceneProperty("environment", -1, path, value);
    applying = false;
    statusLabel->setText(saved ? "Saved · preview pending" : "Could not save");
    if (saved && autoApply)
        previewTimer->start();
}

QJsonValue EnvironmentEditorPanel::environmentValue(const QString &path) const {
    return valueAt(environment, path);
}

void EnvironmentEditorPanel::applyPreview() {
    if (viewport == nullptr)
        return;
    statusLabel->setText("Applying…");
    viewport->reloadRuntime();
}
