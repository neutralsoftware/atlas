#include <editor/views/materialEditor.h>
#include <editor/widgets/scrubbableSpinBox.h>
#include <editor/styling/icons.h>

#include <editor/views/viewport.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QPair>
#include <QSaveFile>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
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

double channelAt(const QImage &image, double u, double v) {
    if (image.isNull()) {
        return 1.0;
    }
    const int x =
        std::clamp(static_cast<int>(u * image.width()), 0, image.width() - 1);
    const int y =
        std::clamp(static_cast<int>(v * image.height()), 0, image.height() - 1);
    return QColor::fromRgba(image.pixel(x, y)).lightnessF();
}

QColor imageAt(const QImage &image, double u, double v,
               const QColor &fallback) {
    if (image.isNull()) {
        return fallback;
    }
    const int x =
        std::clamp(static_cast<int>(u * image.width()), 0, image.width() - 1);
    const int y =
        std::clamp(static_cast<int>(v * image.height()), 0, image.height() - 1);
    return QColor::fromRgba(image.pixel(x, y));
}
} // namespace

class MaterialPreviewWidget : public QWidget {
  public:
    explicit MaterialPreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent) {
        setObjectName("materialPreview");
        setMinimumSize(80, 80);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setMaterial(const QJsonObject &next, const QString &nextBaseDir) {
        material = next;
        baseDir = nextBaseDir;
        albedoImage =
            loadTextureImage(baseDir, material.value("albedoTexture"));
        normalImage =
            loadTextureImage(baseDir, material.value("normalTexture"));
        metallicImage =
            loadTextureImage(baseDir, material.value("metallicTexture"));
        roughnessImage =
            loadTextureImage(baseDir, material.value("roughnessTexture"));
        aoImage = loadTextureImage(baseDir, material.value("aoTexture"));
        displacementImage =
            loadTextureImage(baseDir, material.value("displacementTexture"));
        textureScaleU = arrayValue(material.value("textureScale"), 0, 1.0);
        textureScaleV = arrayValue(material.value("textureScale"), 1, 1.0);
        textureOffsetU = arrayValue(material.value("textureOffset"), 0, 0.0);
        textureOffsetV = arrayValue(material.value("textureOffset"), 1, 0.0);
        update();
    }

    void setEnvironmentMode(int mode) {
        environmentMode = mode;
        update();
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        const qreal scale = devicePixelRatioF();
        const int widthPixels = std::max(1, static_cast<int>(width() * scale));
        const int heightPixels =
            std::max(1, static_cast<int>(height() * scale));
        QImage rendered(widthPixels, heightPixels, QImage::Format_ARGB32);
        rendered.setDevicePixelRatio(scale);

        const QColor albedo =
            jsonColor(material.value("albedo"), QColor::fromRgbF(.8, .8, .8));
        const QColor emission = jsonColor(material.value("emissiveColor"),
                                          QColor::fromRgbF(0, 0, 0));
        const double metallic =
            std::clamp(material.value("metallic").toDouble(0.0), 0.0, 1.0);
        const double roughness =
            std::clamp(material.value("roughness").toDouble(0.5), 0.02, 1.0);
        const double ao =
            std::clamp(material.value("ao").toDouble(1.0), 0.0, 1.0);
        const double reflectivity =
            std::clamp(material.value("reflectivity").toDouble(0.5), 0.0, 1.0);
        const double emissionStrength =
            std::max(0.0, material.value("emissiveIntensity").toDouble(0.0));
        const double transmission =
            std::clamp(material.value("transmittance").toDouble(0.0), 0.0, 1.0);
        const double normalStrength = std::clamp(
            material.value("normalMapStrength").toDouble(1.0), 0.0, 4.0);
        const bool useNormal = material.value("useNormalMap").toBool(true) &&
                               !normalImage.isNull();
        const double cx = widthPixels * 0.5;
        const double cy = heightPixels * 0.5;
        const double radius = std::min(widthPixels, heightPixels) * 0.39;
        const double lx = -0.42;
        const double ly = -0.55;
        const double lz = 0.72;

        for (int y = 0; y < heightPixels; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(rendered.scanLine(y));
            for (int x = 0; x < widthPixels; ++x) {
                const QColor background = environmentAt(
                    (static_cast<double>(x) / widthPixels) * 2.0 - 1.0,
                    1.0 - (static_cast<double>(y) / heightPixels) * 2.0);
                const double px = (x - cx) / radius;
                const double py = (cy - y) / radius;
                const double rr = px * px + py * py;
                if (rr > 1.0) {
                    line[x] = background.rgba();
                    continue;
                }

                double nx = px;
                double ny = py;
                double nz = std::sqrt(std::max(0.0, 1.0 - rr));
                double u =
                    std::atan2(nx, nz) / (2.0 * std::numbers::pi_v<double>)+0.5;
                double v = 0.5 - std::asin(std::clamp(ny, -1.0, 1.0)) /
                                     std::numbers::pi_v<double>;
                u = u * textureScaleU + textureOffsetU;
                v = v * textureScaleV + textureOffsetV;
                u -= std::floor(u);
                v -= std::floor(v);
                if (useNormal) {
                    const QColor sampled =
                        imageAt(normalImage, u, v, QColor(128, 128, 255));
                    const double tx = sampled.redF() * 2.0 - 1.0;
                    const double ty = sampled.greenF() * 2.0 - 1.0;
                    nx += tx * normalStrength * 0.28;
                    ny += ty * normalStrength * 0.28;
                    const double length =
                        std::sqrt(nx * nx + ny * ny + nz * nz);
                    nx /= length;
                    ny /= length;
                    nz /= length;
                }

                const QColor sampledAlbedo =
                    imageAt(albedoImage, u, v, QColor(255, 255, 255));
                const double localMetallic = std::clamp(
                    metallic * channelAt(metallicImage, u, v), 0.0, 1.0);
                const double localRoughness = std::clamp(
                    roughness * channelAt(roughnessImage, u, v), 0.02, 1.0);
                const double localAo =
                    std::clamp(ao * channelAt(aoImage, u, v), 0.0, 1.0);
                const double diffuse =
                    std::max(0.0, nx * lx + ny * ly + nz * lz);
                const double hx = lx;
                const double hy = ly;
                const double hz = lz + 1.0;
                const double hlen = std::sqrt(hx * hx + hy * hy + hz * hz);
                const double ndh =
                    std::max(0.0, (nx * hx + ny * hy + nz * hz) / hlen);
                const double exponent = 4.0 + (1.0 - localRoughness) *
                                                  (1.0 - localRoughness) *
                                                  252.0;
                const double specular = std::pow(ndh, exponent) *
                                        (0.12 + reflectivity * 0.88) *
                                        (0.35 + localMetallic * 0.65);
                const double fresnel =
                    std::pow(1.0 - std::clamp(nz, 0.0, 1.0), 5.0);
                const double light =
                    localAo * 0.17 + diffuse * (0.83 - localMetallic * 0.38);
                const double edgeTransmission =
                    transmission * (0.2 + fresnel * 0.55);
                const double rx = 2.0 * nx * nz;
                const double ry = 2.0 * ny * nz;
                const QColor reflected = environmentAt(rx, ry);
                const double reflectionWeight =
                    std::clamp(reflectivity * (0.12 + localMetallic * 0.88) *
                                       (1.0 - localRoughness * 0.72) +
                                   fresnel * 0.24,
                               0.0, 0.92);
                auto output = [&](double base, double texture, double emitted,
                                  double environment, double behind) {
                    double surface = base * texture * light + specular +
                                     fresnel * reflectivity * 0.18;
                    surface = surface * (1.0 - reflectionWeight) +
                              environment * reflectionWeight;
                    return std::clamp(surface * (1.0 - edgeTransmission) +
                                          behind * edgeTransmission +
                                          emitted * emissionStrength,
                                      0.0, 1.0);
                };
                line[x] = qRgba(
                    static_cast<int>(output(albedo.redF(), sampledAlbedo.redF(),
                                            emission.redF(), reflected.redF(),
                                            background.redF()) *
                                     255.0),
                    static_cast<int>(
                        output(albedo.greenF(), sampledAlbedo.greenF(),
                               emission.greenF(), reflected.greenF(),
                               background.greenF()) *
                        255.0),
                    static_cast<int>(output(albedo.blueF(),
                                            sampledAlbedo.blueF(),
                                            emission.blueF(), reflected.blueF(),
                                            background.blueF()) *
                                     255.0),
                    255);
            }
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(rect(), rendered);
    }

  private:
    QColor environmentAt(double x, double y) const {
        const double horizon = std::clamp((y + 1.0) * 0.5, 0.0, 1.0);
        if (environmentMode == 1) {
            const double sun = std::pow(
                std::max(0.0, 1.0 - std::hypot(x + 0.38, y - 0.08)), 12.0);
            return QColor::fromRgbF(
                std::clamp(0.16 + horizon * 0.58 + sun, 0.0, 1.0),
                std::clamp(0.07 + horizon * 0.27 + sun * 0.55, 0.0, 1.0),
                std::clamp(0.12 + horizon * 0.24 + sun * 0.18, 0.0, 1.0));
        }
        if (environmentMode == 2) {
            const double cloud =
                std::pow(std::max(0.0, std::sin(x * 8.0 + y * 3.0)), 6.0) *
                0.22;
            return QColor::fromRgbF(
                std::clamp(0.12 + horizon * 0.3 + cloud, 0.0, 1.0),
                std::clamp(0.24 + horizon * 0.42 + cloud, 0.0, 1.0),
                std::clamp(0.39 + horizon * 0.48 + cloud, 0.0, 1.0));
        }
        const double strip = std::pow(std::max(0.0, 1.0 - std::abs(y)), 24.0);
        const double panel =
            std::pow(std::max(0.0, std::cos(x * 5.5)), 18.0) * 0.58;
        const double value = 0.055 + horizon * 0.12 + strip * (0.34 + panel);
        return QColor::fromRgbF(std::clamp(value * 0.92, 0.0, 1.0),
                                std::clamp(value * 0.98, 0.0, 1.0),
                                std::clamp(value, 0.0, 1.0));
    }

    QJsonObject material;
    QString baseDir;
    QImage albedoImage;
    QImage normalImage;
    QImage metallicImage;
    QImage roughnessImage;
    QImage aoImage;
    QImage displacementImage;
    double textureScaleU = 1.0;
    double textureScaleV = 1.0;
    double textureOffsetU = 0.0;
    double textureOffsetV = 0.0;
    int environmentMode = 0;
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
        result.insert("reflectivity", 0.5);
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

    auto *splitter = new QSplitter(Qt::Horizontal, body);
    splitter->setChildrenCollapsible(false);
    auto *previewPane = new QWidget(splitter);
    previewPane->setMinimumWidth(1);
    previewPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *previewLayout = new QVBoxLayout(previewPane);
    previewLayout->setContentsMargins(0, 0, 5, 0);
    previewLayout->setSpacing(8);
    preview = new MaterialPreviewWidget(previewPane);
    preview->setMaterial(material, QFileInfo(materialPath).absolutePath());
    auto *previewOptions = new QWidget(previewPane);
    auto *previewOptionsLayout = new QHBoxLayout(previewOptions);
    previewOptionsLayout->setContentsMargins(0, 0, 0, 0);
    auto *previewLabel = new QLabel("Preview Environment", previewOptions);
    auto *environment = new QComboBox(previewOptions);
    environment->addItems({"Studio", "Sunset", "Open Sky"});
    previewOptionsLayout->addWidget(previewLabel);
    previewOptionsLayout->addStretch();
    previewOptionsLayout->addWidget(environment);
    previewLayout->addWidget(preview, 1);
    previewLayout->addWidget(previewOptions);
    connect(environment, &QComboBox::currentIndexChanged, preview,
            &MaterialPreviewWidget::setEnvironmentMode);

    auto *propertiesScroll = new QScrollArea(splitter);
    propertiesScroll->setObjectName("materialPropertiesScroll");
    propertiesScroll->setWidgetResizable(true);
    propertiesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *properties = new QWidget(propertiesScroll);
    properties->setMinimumWidth(1);
    properties->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *propertiesLayout = new QVBoxLayout(properties);
    propertiesLayout->setContentsMargins(5, 0, 0, 0);
    propertiesLayout->setSpacing(9);
    propertiesScroll->setWidget(properties);
    splitter->addWidget(previewPane);
    splitter->addWidget(propertiesScroll);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    bodyLayout->addWidget(splitter, 1);

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
