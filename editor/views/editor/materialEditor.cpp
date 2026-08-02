#include <editor/views/materialEditor.h>
#include <editor/widgets/scrubbableSpinBox.h>
#include <editor/styling/icons.h>

#include <editor/views/viewport.h>
#include <atlas/runtime/context.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEngine>
#include <QPixmap>
#include <QPushButton>
#include <QPair>
#include <QSaveFile>
#include <QScrollArea>
#include <QSettings>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr int MaterialPreviewPathTracingFrames = 24;
constexpr int MaterialPreviewPathTracingIntervalMs = 50;

QColor jsonColor(const QJsonValue &value, const QColor &fallback) {
    const QJsonArray array = value.toArray();
    if (array.size() < 3) {
        return fallback;
    }
    return QColor::fromRgbF(
        std::clamp(array.at(0).toDouble(), 0.0, 1.0),
        std::clamp(array.at(1).toDouble(), 0.0, 1.0),
        std::clamp(array.at(2).toDouble(), 0.0, 1.0),
        array.size() > 3 ? std::clamp(array.at(3).toDouble(), 0.0, 1.0) : 1.0);
}

QJsonArray colorJson(const QColor &color) {
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

void displayColor(QPushButton *button, const QColor &color) {
    button->setProperty("materialColor", color);
    button->setObjectName("materialColorButton");
    button->setText(
        color.name(color.alpha() < 255 ? QColor::HexArgb : QColor::HexRgb)
            .toUpper());
    button->setIcon(styling::colorSwatch(color, QSize(18, 18)));
    button->setIconSize(QSize(18, 18));
}

QDoubleSpinBox *scalarField(double minimum, double maximum, double step,
                            QWidget *parent) {
    auto *field = new ScrubbableDoubleSpinBox(parent);
    field->setObjectName("materialScalarField");
    field->setRange(minimum, maximum);
    field->setSingleStep(step);
    field->setDecimals(3);
    field->setKeyboardTracking(true);
    return field;
}

QString texturePath(const QJsonValue &value) {
    if (value.isString()) {
        return value.toString();
    }
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        return object.value("path").toString(object.value("source").toString());
    }
    return {};
}

QString resolvedTexturePath(const QString &baseDir, const QJsonValue &value) {
    const QString path = texturePath(value);
    if (path.isEmpty() || QFileInfo(path).isAbsolute()) {
        return path;
    }
    return QDir(baseDir).absoluteFilePath(path);
}

QImage loadTextureImage(const QString &baseDir, const QJsonValue &value) {
    const QString path = resolvedTexturePath(baseDir, value);
    return path.isEmpty() ? QImage() : QImage(path);
}

double arrayValue(const QJsonValue &value, int index, double fallback) {
    const QJsonArray array = value.toArray();
    return array.size() > index ? array.at(index).toDouble(fallback) : fallback;
}

} // namespace

class MaterialPreviewWidget : public QWidget {
  public:
    explicit MaterialPreviewWidget(QString projectFile,
                                   QWidget *parent = nullptr)
        : QWidget(parent), projectFile(std::move(projectFile)) {
        setObjectName("materialPreview");
        setAttribute(Qt::WA_DontCreateNativeAncestors);
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_PaintOnScreen);
        setAutoFillBackground(false);
        setMinimumSize(80, 80);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        frameTimer = new QTimer(this);
        frameTimer->setSingleShot(true);
        connect(frameTimer, &QTimer::timeout, this,
                [this] { renderRuntime(); });
    }

    ~MaterialPreviewWidget() override { shutdownRuntime(); }

    void setMaterial(const QJsonObject &next, const QString &nextBaseDir) {
        materialDefinition =
            QJsonDocument(next).toJson(QJsonDocument::Compact);
        baseDir = nextBaseDir;
        if (runtimeContext != nullptr) {
            runtimeContext->setMaterialPreviewMaterial(
                materialDefinition.toStdString(), baseDir.toStdString());
        }
        scheduleFrame();
    }

    void setEnvironmentMode(int mode) {
        environmentMode = mode;
        if (runtimeContext != nullptr) {
            runtimeContext->setMaterialPreviewEnvironment(environmentMode);
        }
        scheduleFrame();
    }

  protected:
    QPaintEngine *paintEngine() const override { return nullptr; }

    void showEvent(QShowEvent *event) override {
        QWidget::showEvent(event);
        scheduleFrame();
    }

    void hideEvent(QHideEvent *event) override {
        frameTimer->stop();
        QWidget::hideEvent(event);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        scheduleFrame();
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            rotating = true;
            lastPointer = event->position();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (rotating && runtimeContext != nullptr) {
            const QPointF delta = event->position() - lastPointer;
            lastPointer = event->position();
            if (runtimeContext->rotateMaterialPreview(
                    static_cast<float>(delta.x() * 0.55),
                    static_cast<float>(delta.y() * 0.55))) {
                scheduleFrame();
            }
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && rotating) {
            rotating = false;
            unsetCursor();
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

  private:
    void scheduleFrame() {
        const int requestedFrames =
            runtimeContext != nullptr &&
                    runtimeContext->materialPreviewUsesPathTracing()
                ? MaterialPreviewPathTracingFrames
                : 2;
        pendingFrames = std::max(pendingFrames, requestedFrames);
        if (isVisible() && frameTimer != nullptr && !frameTimer->isActive()) {
            frameTimer->start(0);
        }
    }

    void startRuntime() {
        if (runtimeContext != nullptr || projectFile.isEmpty() || width() <= 1 ||
            height() <= 1 || materialDefinition.isEmpty()) {
            return;
        }
#ifdef METAL
        try {
            void *metalView =
                reinterpret_cast<void *>(static_cast<quintptr>(winId()));
            runtimeContext = runtime::makeMaterialPreviewContextForMetalView(
                projectFile.toStdString(), metalView);
            if (!runtimeContext->initializeMaterialPreview(
                    materialDefinition.toStdString(), baseDir.toStdString(),
                    environmentMode)) {
                shutdownRuntime();
                return;
            }
            resizeRuntime();
            if (runtimeContext->materialPreviewUsesPathTracing()) {
                pendingFrames =
                    std::max(pendingFrames, MaterialPreviewPathTracingFrames);
            }
        } catch (const std::exception &error) {
            qWarning().noquote()
                << QStringLiteral("Failed to start runtime material preview: %1")
                       .arg(QString::fromUtf8(error.what()));
            runtimeContext.reset();
        }
#endif
    }

    void resizeRuntime() {
        if (runtimeContext == nullptr) {
            return;
        }
        const float scale =
            std::max(1.0f, static_cast<float>(devicePixelRatioF()));
        const int pixelWidth =
            std::max(1, static_cast<int>(std::round(width() * scale)));
        const int pixelHeight =
            std::max(1, static_cast<int>(std::round(height() * scale)));
        if (pixelWidth == runtimeWidth && pixelHeight == runtimeHeight) {
            return;
        }
        runtimeContext->resize(width(), height(), scale);
        runtimeWidth = pixelWidth;
        runtimeHeight = pixelHeight;
    }

    void renderRuntime() {
        if (runtimeContext == nullptr) {
            startRuntime();
        }
        if (runtimeContext == nullptr) {
            return;
        }
        try {
            resizeRuntime();
            if (!runtimeContext->stepFrame()) {
                shutdownRuntime();
                return;
            }
            pendingFrames = std::max(0, pendingFrames - 1);
            if (pendingFrames > 0 && isVisible()) {
                frameTimer->start(
                    runtimeContext->materialPreviewUsesPathTracing()
                        ? MaterialPreviewPathTracingIntervalMs
                        : 1);
            }
        } catch (const std::exception &error) {
            qWarning().noquote()
                << QStringLiteral("Runtime material preview frame failed: %1")
                       .arg(QString::fromUtf8(error.what()));
            shutdownRuntime();
        }
    }

    void shutdownRuntime() {
        if (frameTimer != nullptr) {
            frameTimer->stop();
        }
        if (runtimeContext == nullptr) {
            return;
        }
        auto context = std::move(runtimeContext);
        try {
            context->end();
        } catch (...) {
        }
        runtimeWidth = 0;
        runtimeHeight = 0;
        pendingFrames = 0;
    }

    QString projectFile;
    QByteArray materialDefinition;
    QString baseDir;
    QTimer *frameTimer = nullptr;
    std::shared_ptr<Context> runtimeContext;
    int runtimeWidth = 0;
    int runtimeHeight = 0;
    int environmentMode = 0;
    int pendingFrames = 0;
    QPointF lastPointer;
    bool rotating = false;
};

MaterialEditorPanel::MaterialEditorPanel(ViewportPanel *viewport,
                                         QWidget *parent)
    : QWidget(parent), viewport(viewport) {
    setObjectName("materialEditorPanel");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName("materialEditorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 4, 8, 4);
    titleLabel = new QLabel("Material Editor", header);
    titleLabel->setObjectName("materialEditorTitle");
    statusLabel = new QLabel(header);
    statusLabel->setObjectName("materialEditorStatus");
    auto *saveButton = new QPushButton("Save", header);
    saveButton->setObjectName("materialSaveButton");
    saveButton->setIcon(styling::icon(styling::Icon::FloppyDisk, "#A1957D"));
    auto *assignButton = new QPushButton("Assign to Selected", header);
    assignButton->setObjectName("materialAssignButton");
    assignButton->setIcon(styling::icon(styling::Icon::Assign, "#9E897D"));
    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(statusLabel);
    headerLayout->addWidget(assignButton);
    headerLayout->addWidget(saveButton);
    layout->addWidget(header);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("materialEditorScroll");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    body = new QWidget(scroll);
    body->setObjectName("materialEditorBody");
    bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(10, 10, 10, 12);
    bodyLayout->setSpacing(9);
    scroll->setWidget(body);
    layout->addWidget(scroll, 1);

    saveTimer = new QTimer(this);
    saveTimer->setSingleShot(true);
    saveTimer->setInterval(260);
    connect(saveTimer, &QTimer::timeout, this,
            &MaterialEditorPanel::saveMaterial);
    connect(saveButton, &QPushButton::clicked, this,
            &MaterialEditorPanel::saveMaterial);
    connect(assignButton, &QPushButton::clicked, this,
            &MaterialEditorPanel::assignToSelectedObject);
    showEmptyState();
}

MaterialEditorPanel::~MaterialEditorPanel() {
    if (saveTimer->isActive()) {
        saveMaterial();
    }
}

void MaterialEditorPanel::flushPendingSave() {
    if (saveTimer->isActive())
        saveMaterial();
}

QJsonObject
MaterialEditorPanel::normalizedMaterial(const QJsonObject &source) const {
    QJsonObject result = source;
    if (!result.value("albedo").isArray())
        result.insert("albedo", QJsonArray{0.8, 0.8, 0.8, 1.0});
    if (!result.value("metallic").isDouble())
        result.insert("metallic", 0.0);
    if (!result.value("roughness").isDouble())
        result.insert("roughness", 0.5);
    if (!result.value("ao").isDouble())
        result.insert("ao", 1.0);
    if (!result.value("reflectivity").isDouble())
        result.insert("reflectivity", 0.0);
    if (!result.value("emissiveColor").isArray())
        result.insert("emissiveColor", QJsonArray{0.0, 0.0, 0.0, 1.0});
    if (!result.value("emissiveIntensity").isDouble())
        result.insert("emissiveIntensity", 0.0);
    if (!result.value("normalMapStrength").isDouble())
        result.insert("normalMapStrength", 1.0);
    if (!result.value("useNormalMap").isBool())
        result.insert("useNormalMap", true);
    if (!result.value("textureScale").isArray())
        result.insert("textureScale", QJsonArray{1.0, 1.0});
    if (!result.value("textureOffset").isArray())
        result.insert("textureOffset", QJsonArray{0.0, 0.0});
    if (!result.value("transmittance").isDouble())
        result.insert("transmittance", 0.0);
    if (!result.value("ior").isDouble())
        result.insert("ior", 1.45);
    return result;
}

void MaterialEditorPanel::openMaterial(const QString &path) {
    if (saveTimer->isActive()) {
        saveMaterial();
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Material Editor",
                             "The material could not be opened.");
        return;
    }
    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, "Material Editor",
                             "The material file is not valid JSON.");
        return;
    }
    materialPath = QFileInfo(path).absoluteFilePath();
    assignedObjectId = -1;
    undoHistory.clear();
    redoHistory.clear();
    const QJsonObject root = document.object();
    material = normalizedMaterial(root.value("material").isObject()
                                      ? root.value("material").toObject()
                                      : root);
    showMaterial();
}

void MaterialEditorPanel::rebuildBody() {
    while (QLayoutItem *item = bodyLayout->takeAt(0)) {
        if (item->widget() != nullptr)
            item->widget()->deleteLater();
        delete item;
    }
    preview = nullptr;
    materialSplitter = nullptr;
    textureFields.clear();
    texturePreviews.clear();
}

void MaterialEditorPanel::showEmptyState() {
    rebuildBody();
    titleLabel->setText("Material Editor");
    statusLabel->clear();
    auto *empty = new QLabel(
        "Double-click a material in the Content Browser to edit it.", body);
    empty->setObjectName("materialEditorEmpty");
    empty->setAlignment(Qt::AlignCenter);
    empty->setWordWrap(true);
    bodyLayout->addWidget(empty, 1);
}

void MaterialEditorPanel::showMaterial() {
    rebuildBody();
    loading = true;
    titleLabel->setText(QFileInfo(materialPath).completeBaseName());
    statusLabel->setText("Ready");

    materialSplitter = new QSplitter(Qt::Horizontal, body);
    materialSplitter->setChildrenCollapsible(false);
    auto *previewPane = new QWidget(materialSplitter);
    previewPane->setMinimumWidth(180);
    previewPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *previewLayout = new QVBoxLayout(previewPane);
    previewLayout->setContentsMargins(0, 0, 5, 0);
    previewLayout->setSpacing(8);
    preview = new MaterialPreviewWidget(
        viewport != nullptr ? viewport->runtimeProjectFile() : QString(),
        previewPane);
    preview->setMaterial(material, QFileInfo(materialPath).absolutePath());
    auto *previewOptions = new QWidget(previewPane);
    auto *previewOptionsLayout = new QHBoxLayout(previewOptions);
    previewOptionsLayout->setContentsMargins(0, 0, 0, 0);
    auto *previewLabel = new QLabel("Preview Environment", previewOptions);
    auto *environment = new QComboBox(previewOptions);
    environment->addItems({"Studio", "Sky", "Empty"});
    previewOptionsLayout->addWidget(previewLabel);
    previewOptionsLayout->addStretch();
    previewOptionsLayout->addWidget(environment);
    previewLayout->addWidget(preview, 1);
    previewLayout->addWidget(previewOptions);
    connect(environment, &QComboBox::currentIndexChanged, preview,
            &MaterialPreviewWidget::setEnvironmentMode);

    auto *propertiesScroll = new QScrollArea(materialSplitter);
    propertiesScroll->setObjectName("materialPropertiesScroll");
    propertiesScroll->setWidgetResizable(true);
    propertiesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *properties = new QWidget(propertiesScroll);
    propertiesScroll->setMinimumWidth(280);
    properties->setMinimumWidth(260);
    properties->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *propertiesLayout = new QVBoxLayout(properties);
    propertiesLayout->setContentsMargins(5, 0, 0, 0);
    propertiesLayout->setSpacing(9);
    propertiesScroll->setWidget(properties);
    materialSplitter->addWidget(previewPane);
    materialSplitter->addWidget(propertiesScroll);
    materialSplitter->setStretchFactor(0, 2);
    materialSplitter->setStretchFactor(1, 3);
    bodyLayout->addWidget(materialSplitter, 1);

    QSettings settings("Neutral Software", "Atlas Engine");
    const QByteArray splitterState =
        settings.value("materialEditor/splitterState").toByteArray();
    if (splitterState.isEmpty() ||
        !materialSplitter->restoreState(splitterState)) {
        QTimer::singleShot(0, materialSplitter, [this] {
            if (materialSplitter == nullptr)
                return;
            const int available = std::max(materialSplitter->width(), 500);
            materialSplitter->setSizes(
                {available * 2 / 5, available * 3 / 5});
        });
    }
    connect(materialSplitter, &QSplitter::splitterMoved, this,
            [this](int, int) {
                if (materialSplitter == nullptr)
                    return;
                QSettings settings("Neutral Software", "Atlas Engine");
                settings.setValue("materialEditor/splitterState",
                                  materialSplitter->saveState());
            });

    auto *surface = new QGroupBox("Surface", properties);
    auto *surfaceForm = new QFormLayout(surface);
    albedoButton = new QPushButton(surface);
    displayColor(albedoButton,
                 jsonColor(material.value("albedo"), QColor(204, 204, 204)));
    metallicField = scalarField(0.0, 1.0, 0.01, surface);
    roughnessField = scalarField(0.02, 1.0, 0.01, surface);
    aoField = scalarField(0.0, 1.0, 0.01, surface);
    reflectivityField = scalarField(0.0, 1.0, 0.01, surface);
    metallicField->setValue(material.value("metallic").toDouble());
    roughnessField->setValue(material.value("roughness").toDouble());
    aoField->setValue(material.value("ao").toDouble());
    reflectivityField->setValue(material.value("reflectivity").toDouble());
    surfaceForm->addRow("Base Color", albedoButton);
    surfaceForm->addRow("Metallic", metallicField);
    surfaceForm->addRow("Roughness", roughnessField);
    surfaceForm->addRow("Ambient Occlusion", aoField);
    surfaceForm->addRow("Reflectivity", reflectivityField);
    propertiesLayout->addWidget(surface);

    auto *emission = new QGroupBox("Emission", properties);
    auto *emissionForm = new QFormLayout(emission);
    emissiveButton = new QPushButton(emission);
    displayColor(emissiveButton,
                 jsonColor(material.value("emissiveColor"), Qt::black));
    emissiveIntensityField = scalarField(0.0, 100.0, 0.1, emission);
    emissiveIntensityField->setValue(
        material.value("emissiveIntensity").toDouble());
    emissionForm->addRow("Color", emissiveButton);
    emissionForm->addRow("Strength", emissiveIntensityField);
    propertiesLayout->addWidget(emission);

    auto *volume = new QGroupBox("Transmission", properties);
    auto *volumeForm = new QFormLayout(volume);
    transmittanceField = scalarField(0.0, 1.0, 0.01, volume);
    iorField = scalarField(1.0, 3.0, 0.01, volume);
    transmittanceField->setValue(material.value("transmittance").toDouble());
    iorField->setValue(material.value("ior").toDouble());
    volumeForm->addRow("Weight", transmittanceField);
    volumeForm->addRow("IOR", iorField);
    propertiesLayout->addWidget(volume);

    auto *normal = new QGroupBox("Normal", properties);
    auto *normalForm = new QFormLayout(normal);
    normalMapField = new QCheckBox(normal);
    normalMapField->setChecked(material.value("useNormalMap").toBool());
    normalStrengthField = scalarField(0.0, 4.0, 0.05, normal);
    normalStrengthField->setValue(
        material.value("normalMapStrength").toDouble());
    normalForm->addRow("Use Normal Map", normalMapField);
    normalForm->addRow("Strength", normalStrengthField);
    propertiesLayout->addWidget(normal);

    auto *tiling = new QGroupBox("Texture Mapping", properties);
    auto *tilingForm = new QFormLayout(tiling);
    textureScaleUField = scalarField(-100.0, 100.0, 0.1, tiling);
    textureScaleVField = scalarField(-100.0, 100.0, 0.1, tiling);
    textureOffsetUField = scalarField(-100.0, 100.0, 0.05, tiling);
    textureOffsetVField = scalarField(-100.0, 100.0, 0.05, tiling);
    textureScaleUField->setValue(
        arrayValue(material.value("textureScale"), 0, 1.0));
    textureScaleVField->setValue(
        arrayValue(material.value("textureScale"), 1, 1.0));
    textureOffsetUField->setValue(
        arrayValue(material.value("textureOffset"), 0, 0.0));
    textureOffsetVField->setValue(
        arrayValue(material.value("textureOffset"), 1, 0.0));
    tilingForm->addRow("Tiling U", textureScaleUField);
    tilingForm->addRow("Tiling V", textureScaleVField);
    tilingForm->addRow("Offset U", textureOffsetUField);
    tilingForm->addRow("Offset V", textureOffsetVField);
    propertiesLayout->addWidget(tiling);

    auto *textures = new QGroupBox("Texture Slots", properties);
    auto *textureLayout = new QVBoxLayout(textures);
    const QList<QPair<QString, QString>> materialSlots{
        {"Base Color", "albedoTexture"},
        {"Normal", "normalTexture"},
        {"Metallic", "metallicTexture"},
        {"Roughness", "roughnessTexture"},
        {"Ambient Occlusion", "aoTexture"},
        {"PBR Pack", "pbrPackTexture"},
        {"Opacity", "opacityTexture"},
        {"Displacement", "displacementTexture"}};
    for (const auto &[label, key] : materialSlots) {
        auto *row = new QWidget(textures);
        row->setObjectName("materialTextureSlot");
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(6, 5, 6, 5);
        rowLayout->setSpacing(6);
        auto *thumbnail = new QLabel(row);
        thumbnail->setObjectName("materialTexturePreview");
        thumbnail->setFixedSize(38, 38);
        auto *field = new QLineEdit(row);
        field->setObjectName("materialTexturePath");
        field->setReadOnly(true);
        field->setPlaceholderText("No image");
        auto *choose = new QToolButton(row);
        choose->setText("Choose…");
        choose->setIcon(styling::icon(styling::Icon::FolderOpen, "#7E929C"));
        choose->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        auto *clear = new QToolButton(row);
        clear->setIcon(styling::icon(styling::Icon::Close, "#A17F7F"));
        clear->setToolTip("Remove texture");
        auto *identity = new QWidget(row);
        auto *identityLayout = new QVBoxLayout(identity);
        identityLayout->setContentsMargins(0, 0, 0, 0);
        identityLayout->setSpacing(2);
        auto *name = new QLabel(label, identity);
        name->setObjectName("materialTextureLabel");
        identityLayout->addWidget(name);
        identityLayout->addWidget(field);
        rowLayout->addWidget(thumbnail);
        rowLayout->addWidget(identity, 1);
        rowLayout->addWidget(choose);
        rowLayout->addWidget(clear);
        textureFields.insert(key, field);
        texturePreviews.insert(key, thumbnail);
        textureLayout->addWidget(row);
        connect(choose, &QToolButton::clicked, this,
                [this, key] { chooseTexture(key); });
        connect(clear, &QToolButton::clicked, this,
                [this, key] { clearTexture(key); });
        updateTextureField(key);
    }
    propertiesLayout->addWidget(textures);
    propertiesLayout->addStretch();

    connect(albedoButton, &QPushButton::clicked, this,
            [this] { setColor("albedo", albedoButton); });
    connect(emissiveButton, &QPushButton::clicked, this,
            [this] { setColor("emissiveColor", emissiveButton); });
    const QList<QDoubleSpinBox *> scalars{metallicField,
                                          roughnessField,
                                          aoField,
                                          reflectivityField,
                                          emissiveIntensityField,
                                          normalStrengthField,
                                          textureScaleUField,
                                          textureScaleVField,
                                          textureOffsetUField,
                                          textureOffsetVField,
                                          transmittanceField,
                                          iorField};
    for (QDoubleSpinBox *field : scalars) {
        connect(field, &QDoubleSpinBox::valueChanged, this,
                [this](double) { materialChanged(); });
    }
    connect(normalMapField, &QCheckBox::toggled, this,
            [this](bool) { materialChanged(); });
    loading = false;
}

void MaterialEditorPanel::setColor(const QString &key, QPushButton *button) {
    const QColor initial = button->property("materialColor").value<QColor>();
    const QColor color = QColorDialog::getColor(
        initial, this, "Choose Material Color", QColorDialog::ShowAlphaChannel);
    if (!color.isValid())
        return;
    const QJsonObject previous = material;
    displayColor(button, color);
    material.insert(key, colorJson(color));
    recordHistory(previous);
    refreshEditedMaterial();
}

void MaterialEditorPanel::chooseTexture(const QString &key) {
    const QString selected = QFileDialog::getOpenFileName(
        this, "Choose Texture", QFileInfo(materialPath).absolutePath(),
        "Images (*.png *.jpg *.jpeg *.tga *.bmp *.hdr *.exr);;All Files (*)");
    if (selected.isEmpty())
        return;
    const QJsonObject previous = material;
    const QDir materialDir(QFileInfo(materialPath).absolutePath());
    material.insert(key, materialDir.relativeFilePath(selected));
    updateTextureField(key);
    recordHistory(previous);
    refreshEditedMaterial();
}

void MaterialEditorPanel::clearTexture(const QString &key) {
    if (!material.contains(key))
        return;
    const QJsonObject previous = material;
    material.remove(key);
    updateTextureField(key);
    recordHistory(previous);
    refreshEditedMaterial();
}

void MaterialEditorPanel::updateTextureField(const QString &key) {
    QLineEdit *field = textureFields.value(key);
    QLabel *thumbnail = texturePreviews.value(key);
    if (field == nullptr || thumbnail == nullptr)
        return;
    const QString path = texturePath(material.value(key));
    field->setText(path);
    const QImage image = loadTextureImage(
        QFileInfo(materialPath).absolutePath(), material.value(key));
    if (image.isNull()) {
        thumbnail->setPixmap(QPixmap());
        thumbnail->setText(path.isEmpty() ? "" : "!");
    } else {
        thumbnail->clear();
        thumbnail->setPixmap(QPixmap::fromImage(image).scaled(
            thumbnail->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MaterialEditorPanel::materialChanged() {
    if (loading || materialPath.isEmpty())
        return;
    const QJsonObject previous = material;
    material.insert("metallic", metallicField->value());
    material.insert("roughness", roughnessField->value());
    material.insert("ao", aoField->value());
    material.insert("reflectivity", reflectivityField->value());
    material.insert("emissiveIntensity", emissiveIntensityField->value());
    material.insert("normalMapStrength", normalStrengthField->value());
    material.insert("useNormalMap", normalMapField->isChecked());
    material.insert("textureScale",
                    QJsonArray{textureScaleUField->value(),
                               textureScaleVField->value()});
    material.insert("textureOffset",
                    QJsonArray{textureOffsetUField->value(),
                               textureOffsetVField->value()});
    material.insert("transmittance", transmittanceField->value());
    material.insert("ior", iorField->value());
    if (material == previous)
        return;
    recordHistory(previous);
    refreshEditedMaterial();
}

void MaterialEditorPanel::refreshEditedMaterial() {
    preview->setMaterial(material, QFileInfo(materialPath).absolutePath());
    statusLabel->setText("Saving…");
    saveTimer->start();
}

void MaterialEditorPanel::recordHistory(const QJsonObject &previous) {
    if (previous == material)
        return;
    undoHistory.append(previous);
    while (undoHistory.size() > 100)
        undoHistory.removeFirst();
    redoHistory.clear();
}

void MaterialEditorPanel::saveMaterial() {
    if (materialPath.isEmpty())
        return;
    saveTimer->stop();
    QSaveFile file(materialPath);
    if (!file.open(QIODevice::WriteOnly)) {
        statusLabel->setText("Save failed");
        return;
    }
    QJsonObject root;
    root.insert("material", material);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        statusLabel->setText("Save failed");
        return;
    }
    statusLabel->setText("Saved");
    if (assignedObjectId >= 0 && viewport != nullptr) {
        viewport->applyRuntimeMaterial(assignedObjectId, materialPath);
    }
    emit materialSaved(materialPath);
}

void MaterialEditorPanel::assignToSelectedObject() {
    if (materialPath.isEmpty() || viewport == nullptr) {
        return;
    }
    const int objectId = viewport->selectedRuntimeObjectId();
    if (objectId < 0) {
        QMessageBox::information(
            this, "Assign Material",
            "Select a renderable object in the Hierarchy or Viewport first.");
        return;
    }
    saveMaterial();
    if (!viewport->applyRuntimeMaterial(objectId, materialPath)) {
        QMessageBox::warning(
            this, "Assign Material",
            "This material can only be assigned to a solid or model object.");
        return;
    }
    assignedObjectId = objectId;
    statusLabel->setText("Assigned · live updates enabled");
}

void MaterialEditorPanel::undo() {
    if (undoHistory.isEmpty() || materialPath.isEmpty())
        return;
    redoHistory.append(material);
    material = undoHistory.takeLast();
    showMaterial();
    saveMaterial();
}

void MaterialEditorPanel::redo() {
    if (redoHistory.isEmpty() || materialPath.isEmpty())
        return;
    undoHistory.append(material);
    material = redoHistory.takeLast();
    showMaterial();
    saveMaterial();
}
