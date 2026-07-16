#include <editor/views/materialEditor.h>

#include <QCheckBox>
#include <QColorDialog>
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
#include <QSignalBlocker>
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
    return QColor::fromRgbF(std::clamp(array.at(0).toDouble(), 0.0, 1.0),
                            std::clamp(array.at(1).toDouble(), 0.0, 1.0),
                            std::clamp(array.at(2).toDouble(), 0.0, 1.0),
                            array.size() > 3
                                ? std::clamp(array.at(3).toDouble(), 0.0, 1.0)
                                : 1.0);
}

QJsonArray colorJson(const QColor &color) {
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

void displayColor(QPushButton *button, const QColor &color) {
    button->setProperty("materialColor", color);
    button->setText(color.name(QColor::HexRgb).toUpper());
    button->setStyleSheet(
        QStringLiteral("background-color: rgba(%1,%2,%3,%4); color: %5;")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alpha())
            .arg(color.lightnessF() > 0.55 ? "#111111" : "#FFFFFF"));
}

QDoubleSpinBox *scalarField(double minimum, double maximum, double step,
                            QWidget *parent) {
    auto *field = new QDoubleSpinBox(parent);
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
        return object.value("path").toString(
            object.value("source").toString());
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

double channelAt(const QImage &image, double u, double v) {
    if (image.isNull()) {
        return 1.0;
    }
    const int x = std::clamp(static_cast<int>(u * image.width()), 0,
                             image.width() - 1);
    const int y = std::clamp(static_cast<int>(v * image.height()), 0,
                             image.height() - 1);
    return QColor::fromRgba(image.pixel(x, y)).lightnessF();
}

QColor imageAt(const QImage &image, double u, double v,
               const QColor &fallback) {
    if (image.isNull()) {
        return fallback;
    }
    const int x = std::clamp(static_cast<int>(u * image.width()), 0,
                             image.width() - 1);
    const int y = std::clamp(static_cast<int>(v * image.height()), 0,
                             image.height() - 1);
    return QColor::fromRgba(image.pixel(x, y));
}
}

class MaterialPreviewWidget : public QWidget {
  public:
    explicit MaterialPreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent) {
        setObjectName("materialPreview");
        setMinimumSize(250, 250);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setMaterial(const QJsonObject &next, const QString &nextBaseDir) {
        material = next;
        baseDir = nextBaseDir;
        albedoImage = QImage(resolvedTexturePath(
            baseDir, material.value("albedoTexture")));
        normalImage = QImage(resolvedTexturePath(
            baseDir, material.value("normalTexture")));
        metallicImage = QImage(resolvedTexturePath(
            baseDir, material.value("metallicTexture")));
        roughnessImage = QImage(resolvedTexturePath(
            baseDir, material.value("roughnessTexture")));
        aoImage = QImage(
            resolvedTexturePath(baseDir, material.value("aoTexture")));
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
        const double reflectivity = std::clamp(
            material.value("reflectivity").toDouble(0.5), 0.0, 1.0);
        const double emissionStrength = std::max(
            0.0, material.value("emissiveIntensity").toDouble(0.0));
        const double transmission = std::clamp(
            material.value("transmittance").toDouble(0.0), 0.0, 1.0);
        const double normalStrength = std::clamp(
            material.value("normalMapStrength").toDouble(1.0), 0.0, 4.0);
        const bool useNormal =
            material.value("useNormalMap").toBool(true) &&
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
                const int checker = ((x / 20) + (y / 20)) & 1;
                const double background = checker ? 0.105 : 0.135;
                const double px = (x - cx) / radius;
                const double py = (cy - y) / radius;
                const double rr = px * px + py * py;
                if (rr > 1.0) {
                    const int c = static_cast<int>(background * 255.0);
                    line[x] = qRgba(c, c, c, 255);
                    continue;
                }

                double nx = px;
                double ny = py;
                double nz = std::sqrt(std::max(0.0, 1.0 - rr));
                double u = std::atan2(nx, nz) /
                               (2.0 * std::numbers::pi_v<double>) +
                           0.5;
                double v = 0.5 -
                           std::asin(std::clamp(ny, -1.0, 1.0)) /
                               std::numbers::pi_v<double>;
                if (useNormal) {
                    const QColor sampled =
                        imageAt(normalImage, u, v, QColor(128, 128, 255));
                    const double tx = sampled.redF() * 2.0 - 1.0;
                    const double ty = sampled.greenF() * 2.0 - 1.0;
                    nx += tx * normalStrength * 0.28;
                    ny += ty * normalStrength * 0.28;
                    const double length = std::sqrt(nx * nx + ny * ny + nz * nz);
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
                const double diffuse = std::max(0.0, nx * lx + ny * ly + nz * lz);
                const double hx = lx;
                const double hy = ly;
                const double hz = lz + 1.0;
                const double hlen = std::sqrt(hx * hx + hy * hy + hz * hz);
                const double ndh = std::max(
                    0.0, (nx * hx + ny * hy + nz * hz) / hlen);
                const double exponent = 4.0 +
                                        (1.0 - localRoughness) *
                                            (1.0 - localRoughness) * 252.0;
                const double specular =
                    std::pow(ndh, exponent) *
                    (0.12 + reflectivity * 0.88) *
                    (0.35 + localMetallic * 0.65);
                const double fresnel =
                    std::pow(1.0 - std::clamp(nz, 0.0, 1.0), 5.0);
                const double light = localAo * 0.17 +
                                     diffuse * (0.83 - localMetallic * 0.38);
                const double edgeTransmission =
                    transmission * (0.2 + fresnel * 0.55);
                auto output = [&](double base, double texture,
                                  double emitted) {
                    const double surface = base * texture * light + specular +
                                           fresnel * reflectivity * 0.18;
                    return std::clamp(surface * (1.0 - edgeTransmission) +
                                          background * edgeTransmission +
                                          emitted * emissionStrength,
                                      0.0, 1.0);
                };
                line[x] = qRgba(
                    static_cast<int>(output(albedo.redF(),
                                            sampledAlbedo.redF(),
                                            emission.redF()) *
                                     255.0),
                    static_cast<int>(output(albedo.greenF(),
                                            sampledAlbedo.greenF(),
                                            emission.greenF()) *
                                     255.0),
                    static_cast<int>(output(albedo.blueF(),
                                            sampledAlbedo.blueF(),
                                            emission.blueF()) *
                                     255.0),
                    255);
            }
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(rect(), rendered);
    }

  private:
    QJsonObject material;
    QString baseDir;
    QImage albedoImage;
    QImage normalImage;
    QImage metallicImage;
    QImage roughnessImage;
    QImage aoImage;
};

MaterialEditorPanel::MaterialEditorPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("materialEditorPanel");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName("materialEditorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 7, 10, 7);
    titleLabel = new QLabel("Material Editor", header);
    titleLabel->setObjectName("materialEditorTitle");
    statusLabel = new QLabel(header);
    statusLabel->setObjectName("materialEditorStatus");
    auto *saveButton = new QPushButton("Save", header);
    saveButton->setObjectName("materialSaveButton");
    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(statusLabel);
    headerLayout->addWidget(saveButton);
    layout->addWidget(header);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("materialEditorScroll");
    scroll->setWidgetResizable(true);
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
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, "Material Editor",
                             "The material file is not valid JSON.");
        return;
    }
    materialPath = QFileInfo(path).absoluteFilePath();
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

    preview = new MaterialPreviewWidget(body);
    preview->setMaterial(material, QFileInfo(materialPath).absolutePath());
    bodyLayout->addWidget(preview);

    auto *surface = new QGroupBox("Surface", body);
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
    bodyLayout->addWidget(surface);

    auto *emission = new QGroupBox("Emission", body);
    auto *emissionForm = new QFormLayout(emission);
    emissiveButton = new QPushButton(emission);
    displayColor(emissiveButton,
                 jsonColor(material.value("emissiveColor"), Qt::black));
    emissiveIntensityField = scalarField(0.0, 100.0, 0.1, emission);
    emissiveIntensityField->setValue(
        material.value("emissiveIntensity").toDouble());
    emissionForm->addRow("Color", emissiveButton);
    emissionForm->addRow("Strength", emissiveIntensityField);
    bodyLayout->addWidget(emission);

    auto *volume = new QGroupBox("Transmission", body);
    auto *volumeForm = new QFormLayout(volume);
    transmittanceField = scalarField(0.0, 1.0, 0.01, volume);
    iorField = scalarField(1.0, 3.0, 0.01, volume);
    transmittanceField->setValue(
        material.value("transmittance").toDouble());
    iorField->setValue(material.value("ior").toDouble());
    volumeForm->addRow("Weight", transmittanceField);
    volumeForm->addRow("IOR", iorField);
    bodyLayout->addWidget(volume);

    auto *normal = new QGroupBox("Normal", body);
    auto *normalForm = new QFormLayout(normal);
    normalMapField = new QCheckBox(normal);
    normalMapField->setChecked(material.value("useNormalMap").toBool());
    normalStrengthField = scalarField(0.0, 4.0, 0.05, normal);
    normalStrengthField->setValue(
        material.value("normalMapStrength").toDouble());
    normalForm->addRow("Use Normal Map", normalMapField);
    normalForm->addRow("Strength", normalStrengthField);
    bodyLayout->addWidget(normal);

    auto *textures = new QGroupBox("Texture Slots", body);
    auto *textureLayout = new QVBoxLayout(textures);
    const QList<QPair<QString, QString>> materialSlots{
        {"Base Color", "albedoTexture"}, {"Normal", "normalTexture"},
        {"Metallic", "metallicTexture"}, {"Roughness", "roughnessTexture"},
        {"Ambient Occlusion", "aoTexture"}, {"Opacity", "opacityTexture"}};
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
        auto *clear = new QToolButton(row);
        clear->setText("×");
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
    bodyLayout->addWidget(textures);
    bodyLayout->addStretch();

    connect(albedoButton, &QPushButton::clicked, this,
            [this] { setColor("albedo", albedoButton); });
    connect(emissiveButton, &QPushButton::clicked, this,
            [this] { setColor("emissiveColor", emissiveButton); });
    const QList<QDoubleSpinBox *> scalars{
        metallicField,          roughnessField, aoField,
        reflectivityField,      emissiveIntensityField,
        normalStrengthField,    transmittanceField,
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
    displayColor(button, color);
    material.insert(key, colorJson(color));
    materialChanged();
}

void MaterialEditorPanel::chooseTexture(const QString &key) {
    const QString selected = QFileDialog::getOpenFileName(
        this, "Choose Texture", QFileInfo(materialPath).absolutePath(),
        "Images (*.png *.jpg *.jpeg *.tga *.bmp *.hdr *.exr);;All Files (*)");
    if (selected.isEmpty())
        return;
    const QDir materialDir(QFileInfo(materialPath).absolutePath());
    material.insert(key, materialDir.relativeFilePath(selected));
    updateTextureField(key);
    materialChanged();
}

void MaterialEditorPanel::clearTexture(const QString &key) {
    if (!material.contains(key))
        return;
    material.remove(key);
    updateTextureField(key);
    materialChanged();
}

void MaterialEditorPanel::updateTextureField(const QString &key) {
    QLineEdit *field = textureFields.value(key);
    QLabel *thumbnail = texturePreviews.value(key);
    if (field == nullptr || thumbnail == nullptr)
        return;
    const QString path = texturePath(material.value(key));
    field->setText(path);
    const QImage image(resolvedTexturePath(
        QFileInfo(materialPath).absolutePath(), material.value(key)));
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
    material.insert("metallic", metallicField->value());
    material.insert("roughness", roughnessField->value());
    material.insert("ao", aoField->value());
    material.insert("reflectivity", reflectivityField->value());
    material.insert("emissiveIntensity", emissiveIntensityField->value());
    material.insert("normalMapStrength", normalStrengthField->value());
    material.insert("useNormalMap", normalMapField->isChecked());
    material.insert("transmittance", transmittanceField->value());
    material.insert("ior", iorField->value());
    preview->setMaterial(material, QFileInfo(materialPath).absolutePath());
    statusLabel->setText("Saving…");
    saveTimer->start();
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
    emit materialSaved(materialPath);
}
