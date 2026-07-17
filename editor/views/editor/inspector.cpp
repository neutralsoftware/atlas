/*
 * inspector.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Inspector definition and functions
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/inspectorView.h>

#include <QAction>
#include <QCheckBox>
#include <QAbstractSpinBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QPair>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <algorithm>
#include <cmath>
#include <functional>

#include "editor/views/viewport.h"

namespace {
using PropertyChanged =
    std::function<void(const QString &, const QJsonValue &)>;

QString humanize(const QString &value) {
    QString result;
    for (int i = 0; i < value.size(); ++i) {
        const QChar character = value.at(i);
        if (i > 0 && character.isUpper() && value.at(i - 1).isLower()) {
            result += ' ';
        }
        result += i == 0 ? character.toUpper() : character;
    }
    return result.replace('_', ' ');
}

QString pointerSegment(QString value) {
    return value.replace('~', "~0").replace('/', "~1");
}

QString childPath(const QString &path, const QString &key) {
    return path + '/' + pointerSegment(key);
}

bool isNumericArray(const QJsonArray &array) {
    return std::all_of(array.begin(), array.end(), [](const QJsonValue &value) {
        return value.isDouble();
    });
}

bool isColorProperty(const QString &name, const QJsonArray &array) {
    return name.contains("color", Qt::CaseInsensitive) &&
           (array.size() == 3 || array.size() == 4) && isNumericArray(array);
}

QIcon inspectorIcon(QWidget *widget, const QString &type) {
    const QString normalized = type.toLower();
    QStyle::StandardPixmap fallback = QStyle::SP_FileIcon;
    QString themeName = "application-x-executable";
    if (normalized == "folder") {
        fallback = QStyle::SP_DirIcon;
        themeName = "folder";
    } else if (normalized.contains("camera")) {
        fallback = QStyle::SP_ComputerIcon;
        themeName = "camera-photo";
    } else if (normalized.contains("light") || normalized == "sun") {
        fallback = QStyle::SP_MessageBoxInformation;
        themeName = "weather-clear";
    } else if (normalized.contains("terrain")) {
        fallback = QStyle::SP_DriveHDIcon;
        themeName = "applications-graphics";
    } else if (normalized.contains("particle")) {
        fallback = QStyle::SP_BrowserReload;
        themeName = "weather-showers-scattered";
    } else if (normalized.contains("audio") || normalized == "wav" ||
               normalized == "mp3" || normalized == "ogg" ||
               normalized == "flac") {
        fallback = QStyle::SP_MediaVolume;
        themeName = "audio-x-generic";
    } else if (normalized == "model") {
        fallback = QStyle::SP_FileDialogContentsView;
        themeName = "model";
    } else if (normalized == "solid" || normalized == "cube" ||
               normalized == "sphere" || normalized == "plane" ||
               normalized == "pyramid" || normalized == "capsule") {
        fallback = QStyle::SP_DirIcon;
        themeName = "applications-games";
    }
    return QIcon::fromTheme(themeName, widget->style()->standardIcon(fallback));
}

QString componentTitle(const QString &type) {
    const QString normalized = type.toLower().remove('_').remove('-');
    if (normalized == "script")
        return "Script";
    if (normalized == "traitscript")
        return "Trait Script";
    if (normalized == "rigidbody")
        return "Rigidbody";
    if (normalized == "audioplayer")
        return "Audio Player";
    if (normalized == "joint")
        return "Joint";
    if (normalized == "fixedjoint")
        return "Fixed Joint";
    if (normalized == "hingejoint")
        return "Hinge Joint";
    if (normalized == "springjoint")
        return "Spring Joint";
    if (normalized == "vehicle")
        return "Vehicle";
    return humanize(type);
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

QJsonObject vehicleWheelSchema() {
    return {{"position", QJsonArray{0.0, 0.0, 0.0}},
            {"enableSuspensionForcePoint", false},
            {"suspensionForcePoint", QJsonArray{0.0, 0.0, 0.0}},
            {"suspensionDirection", QJsonArray{0.0, -1.0, 0.0}},
            {"steeringAxis", QJsonArray{0.0, 1.0, 0.0}},
            {"wheelUp", QJsonArray{0.0, 1.0, 0.0}},
            {"wheelForward", QJsonArray{0.0, 0.0, 1.0}},
            {"suspensionMinLength", 0.0},
            {"suspensionMaxLength", 0.0},
            {"suspensionPreloadLength", 0.0},
            {"suspensionFrequencyHz", 0.0},
            {"suspensionDampingRatio", 0.0},
            {"radius", 0.0},
            {"width", 0.0},
            {"inertia", 0.0},
            {"angularDamping", 0.0},
            {"maxSteerAngleDeg", 0.0},
            {"maxBrakeTorque", 0.0},
            {"maxHandBrakeTorque", 0.0}};
}

QJsonObject vehicleDifferentialSchema() {
    return {{"leftWheel", 0},           {"rightWheel", 0},
            {"differentialRatio", 0.0}, {"leftRightSplit", 0.5},
            {"limitedSlipRatio", 0.0},  {"engineTorqueRatio", 1.0}};
}

QJsonObject componentSchema(const QString &type) {
    const QString normalized = type.toLower().remove('_').remove('-');
    if (normalized == "script") {
        return {{"name", ""}, {"source", ""}, {"variables", QJsonObject{}}};
    }
    if (normalized == "traitscript") {
        return {{"name", ""},
                {"source", ""},
                {"traitedType", ""},
                {"variables", QJsonObject{}}};
    }
    if (normalized == "rigidbody") {
        return {{"mass", 1.0},
                {"sendSignal", ""},
                {"isSensor", false},
                {"collider", QJsonObject{{"type", "box"},
                                         {"size", QJsonArray{1.0, 1.0, 1.0}},
                                         {"radius", 0.5},
                                         {"height", 1.0}}},
                {"friction", 0.5},
                {"tags", QJsonArray{}},
                {"damping", QJsonObject{{"linear", 0.0}, {"angular", 0.0}}},
                {"restitution", 0.0},
                {"motionType", "dynamic"}};
    }
    if (normalized == "audioplayer") {
        return {{"source", ""},
                {"useSpatialization", true},
                {"volume", 1.0},
                {"loop", false},
                {"autoplay", false}};
    }
    QJsonObject joint{
        {"parent", ""},      {"child", ""},
        {"space", "world"},  {"anchor", QJsonArray{0.0, 0.0, 0.0}},
        {"breakForce", 0.0}, {"breakTorque", 0.0}};
    if (normalized == "joint" || normalized == "fixedjoint") {
        return joint;
    }
    if (normalized == "hingejoint") {
        joint.insert("axis1", QJsonArray{1.0, 0.0, 0.0});
        joint.insert("axis2", QJsonArray{0.0, 1.0, 0.0});
        joint.insert("limits", QJsonObject{{"enabled", false},
                                           {"minAngle", 0.0},
                                           {"maxAngle", 0.0}});
        joint.insert("motor", QJsonObject{{"enabled", false},
                                          {"targetVelocity", 0.0},
                                          {"maxForce", 0.0},
                                          {"maxTorque", 0.0}});
        return joint;
    }
    if (normalized == "springjoint") {
        joint.insert("anchorB", QJsonArray{0.0, 0.0, 0.0});
        joint.insert("restLength", 1.0);
        joint.insert("useLimits", false);
        joint.insert("minLength", 0.0);
        joint.insert("maxLength", 1.0);
        joint.insert("spring", QJsonObject{{"enabled", true},
                                           {"mode", "frequencyAndDamping"},
                                           {"frequencyHz", 1.0},
                                           {"dampingRatio", 0.5},
                                           {"stiffness", 1.0},
                                           {"damping", 0.5}});
        return joint;
    }
    if (normalized == "vehicle") {
        return {
            {"settings",
             QJsonObject{
                 {"up", QJsonArray{0.0, 1.0, 0.0}},
                 {"forward", QJsonArray{0.0, 0.0, 1.0}},
                 {"maxPitchRollAngleDeg", 60.0},
                 {"maxSlopeAngleDeg", 45.0},
                 {"wheels", QJsonArray{}},
                 {"controller",
                  QJsonObject{
                      {"engine", QJsonObject{{"maxTorque", 0.0},
                                             {"minRPM", 0.0},
                                             {"maxRPM", 0.0},
                                             {"inertia", 0.0},
                                             {"angularDamping", 0.0}}},
                      {"transmission", QJsonObject{{"type", "automatic"},
                                                   {"gearRatios", QJsonArray{}},
                                                   {"reverseGearRatio", 0.0},
                                                   {"switchTime", 0.0},
                                                   {"clutchReleaseTime", 0.0},
                                                   {"switchLatency", 0.0},
                                                   {"shiftUpRPM", 0.0},
                                                   {"shiftDownRPM", 0.0},
                                                   {"clutchStrength", 0.0}}},
                      {"differentials", QJsonArray{}},
                      {"differentialLimitedSlipRatio", 0.0}}}}}};
    }
    return {};
}

QJsonObject lightSchema(const QString &type) {
    const QString normalized = type.toLower().remove('_').remove('-');
    QJsonObject common{{"color", QJsonArray{1.0, 1.0, 1.0, 1.0}},
                       {"intensity", 1.0}};
    if (normalized == "ambientlight" || normalized == "ambient") {
        common.insert("intensity", 0.5);
        return common;
    }
    common.insert("shineColor", QJsonArray{1.0, 1.0, 1.0, 1.0});
    common.insert("castsShadows", false);
    common.insert("shadowResolution", 2048);
    if (normalized == "directionallight" || normalized == "sun") {
        common.insert("direction", QJsonArray{0.0, -1.0, 0.0});
        common.insert("shadowResolution", 4096);
    } else if (normalized == "pointlight") {
        common.insert("distance", 50.0);
    } else if (normalized == "spotlight") {
        common.insert("direction", QJsonArray{0.0, -1.0, 0.0});
        common.insert("range", 50.0);
        common.insert("cutoff", 35.0);
        common.insert("outerCutoff", 40.0);
    } else if (normalized == "arealight") {
        common.insert("right", QJsonArray{1.0, 0.0, 0.0});
        common.insert("up", QJsonArray{0.0, 1.0, 0.0});
        common.insert("size", QJsonArray{1.0, 1.0});
        common.insert("range", 50.0);
        common.insert("angle", 90.0);
        common.insert("castsBothSides", false);
    }
    return common;
}

QJsonObject componentValues(const QString &type, const QJsonObject &raw) {
    QJsonObject values = raw;
    values.remove("type");
    values = mergeObjects(componentSchema(type), values);
    const QString normalized = type.toLower().remove('_').remove('-');
    if (normalized != "vehicle") {
        return values;
    }
    QJsonObject settings = values.value("settings").toObject();
    QJsonArray wheels = settings.value("wheels").toArray();
    for (int index = 0; index < wheels.size(); ++index) {
        if (wheels.at(index).isObject()) {
            wheels.replace(index, mergeObjects(vehicleWheelSchema(),
                                               wheels.at(index).toObject()));
        }
    }
    settings.insert("wheels", wheels);
    QJsonObject controller = settings.value("controller").toObject();
    QJsonArray differentials = controller.value("differentials").toArray();
    for (int index = 0; index < differentials.size(); ++index) {
        if (differentials.at(index).isObject()) {
            differentials.replace(
                index, mergeObjects(vehicleDifferentialSchema(),
                                    differentials.at(index).toObject()));
        }
    }
    controller.insert("differentials", differentials);
    settings.insert("controller", controller);
    values.insert("settings", settings);
    return values;
}

QString jsonShape(const QJsonValue &value) {
    if (value.isObject()) {
        QString result = "{";
        const QJsonObject object = value.toObject();
        for (auto iterator = object.begin(); iterator != object.end();
             ++iterator) {
            result += iterator.key() + ':' + jsonShape(iterator.value()) + ';';
        }
        return result + '}';
    }
    if (value.isArray()) {
        QString result = "[";
        const QJsonArray array = value.toArray();
        for (const QJsonValue &entry : array) {
            result += jsonShape(entry) + ';';
        }
        return result + ']';
    }
    return "v";
}

QString componentShape(const QJsonArray &components) {
    QString result;
    for (const QJsonValue &entry : components) {
        const QJsonObject component = entry.toObject();
        const QString type = component.value("type").toString();
        result += type + jsonShape(componentValues(type, component));
    }
    return result;
}

QStringList choicesFor(const QString &path) {
    const QString key = path.section('/', -1).toLower();
    if (key == "motiontype")
        return {"static", "dynamic", "kinematic"};
    if (key == "space")
        return {"world", "local"};
    if (key == "mode")
        return {"frequencyAndDamping", "stiffnessAndDamping"};
    if (key == "type" && path.contains("transmission")) {
        return {"automatic", "manual"};
    }
    if (key == "type" && path.contains("collider")) {
        return {"box", "sphere", "capsule", "mesh"};
    }
    return {};
}

QDoubleSpinBox *numberField(double value, QWidget *parent) {
    auto *field = new QDoubleSpinBox(parent);
    field->setObjectName("inspectorNumberField");
    field->setRange(-1000000000.0, 1000000000.0);
    field->setDecimals(4);
    field->setSingleStep(0.1);
    field->setValue(value);
    field->setKeyboardTracking(true);
    return field;
}

void connectLiveText(QLineEdit *field, const std::function<void()> &commit) {
    auto *timer = new QTimer(field);
    timer->setSingleShot(true);
    timer->setInterval(160);
    QObject::connect(field, &QLineEdit::textEdited, timer,
                     [timer] { timer->start(); });
    QObject::connect(timer, &QTimer::timeout, field, commit);
    QObject::connect(field, &QLineEdit::editingFinished, field,
                     [timer, commit] {
                         timer->stop();
                         commit();
                     });
}

QWidget *vectorField(const QJsonArray &value, const PropertyChanged &changed,
                     const QString &path, QWidget *parent) {
    auto *field = new QFrame(parent);
    field->setObjectName("inspectorVectorField");
    auto *layout = new QHBoxLayout(field);
    layout->setContentsMargins(3, 0, 3, 0);
    layout->setSpacing(2);
    auto values = value;
    const int dimensions = std::clamp(static_cast<int>(value.size()), 2, 3);
    while (values.size() < dimensions)
        values.append(0.0);
    const QStringList axes{"X", "Y", "Z"};
    QList<QDoubleSpinBox *> boxes;
    for (int index = 0; index < dimensions; ++index) {
        auto *axis = new QLabel(axes.at(index), field);
        axis->setObjectName("inspectorAxisLabel");
        auto *box = numberField(values.at(index).toDouble(), field);
        box->setButtonSymbols(QAbstractSpinBox::NoButtons);
        box->setMinimumWidth(38);
        boxes.append(box);
        layout->addWidget(axis);
        layout->addWidget(box, 1);
    }
    auto commit = [boxes, changed, path] {
        QJsonArray result;
        for (QDoubleSpinBox *box : boxes)
            result.append(box->value());
        changed(path, result);
    };
    for (QDoubleSpinBox *box : boxes) {
        QObject::connect(box, &QDoubleSpinBox::valueChanged, field,
                         [commit](double) { commit(); });
    }
    return field;
}

QWidget *colorField(const QJsonArray &value, const PropertyChanged &changed,
                    const QString &path, QWidget *parent) {
    auto *field = new QFrame(parent);
    field->setObjectName("inspectorColorField");
    auto *layout = new QHBoxLayout(field);
    layout->setContentsMargins(3, 2, 3, 2);
    layout->setSpacing(5);
    const bool normalized =
        std::all_of(value.begin(), value.end(), [](const QJsonValue &entry) {
            return entry.toDouble() <= 1.0;
        });
    const double factor = normalized ? 255.0 : 1.0;
    QColor color(
        std::clamp(static_cast<int>(value.at(0).toDouble() * factor), 0, 255),
        std::clamp(static_cast<int>(value.at(1).toDouble() * factor), 0, 255),
        std::clamp(static_cast<int>(value.at(2).toDouble() * factor), 0, 255),
        value.size() > 3
            ? std::clamp(static_cast<int>(value.at(3).toDouble() * factor), 0,
                         255)
            : 255);
    auto *swatch = new QPushButton(field);
    swatch->setObjectName("inspectorColorSwatch");
    swatch->setFixedSize(30, 22);
    auto *text = new QLineEdit(field);
    text->setObjectName("inspectorColorText");
    auto updateDisplay = [swatch, text](const QColor &next) {
        swatch->setStyleSheet(
            QStringLiteral("background-color: rgba(%1,%2,%3,%4);")
                .arg(next.red())
                .arg(next.green())
                .arg(next.blue())
                .arg(next.alpha()));
        text->setText(next.name(QColor::HexArgb).toUpper());
    };
    updateDisplay(color);
    layout->addWidget(swatch);
    layout->addWidget(text, 1);
    QObject::connect(
        swatch, &QPushButton::clicked, field,
        [field, color, normalized, value, changed, path,
         updateDisplay]() mutable {
            const QColor next = QColorDialog::getColor(
                color, field, "Choose Color", QColorDialog::ShowAlphaChannel);
            if (!next.isValid())
                return;
            color = next;
            updateDisplay(color);
            const double divisor = normalized ? 255.0 : 1.0;
            QJsonArray result{color.red() / divisor, color.green() / divisor,
                              color.blue() / divisor};
            if (value.size() > 3)
                result.append(color.alpha() / divisor);
            changed(path, result);
        });
    QObject::connect(text, &QLineEdit::editingFinished, field,
                     [text, normalized, value, changed, path, updateDisplay] {
                         const QColor next(text->text());
                         if (!next.isValid())
                             return;
                         updateDisplay(next);
                         const double divisor = normalized ? 255.0 : 1.0;
                         QJsonArray result{next.red() / divisor,
                                           next.green() / divisor,
                                           next.blue() / divisor};
                         if (value.size() > 3)
                             result.append(next.alpha() / divisor);
                         changed(path, result);
                     });
    return field;
}

void addPropertyRows(QVBoxLayout *layout, const QJsonObject &properties,
                     const QString &path, const PropertyChanged &changed,
                     QWidget *parent);

QWidget *primitiveField(const QString &name, const QString &path,
                        const QJsonValue &value, const PropertyChanged &changed,
                        QWidget *parent) {
    if (value.isBool()) {
        auto *field = new QCheckBox(parent);
        field->setChecked(value.toBool());
        QObject::connect(
            field, &QCheckBox::toggled, parent,
            [changed, path](bool checked) { changed(path, checked); });
        return field;
    }
    if (value.isDouble()) {
        auto *field = numberField(value.toDouble(), parent);
        QObject::connect(field, &QDoubleSpinBox::valueChanged, parent,
                         [field, changed, path](double) {
                             changed(path, field->value());
                         });
        return field;
    }
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (isColorProperty(name, array)) {
            return colorField(array, changed, path, parent);
        }
        if ((array.size() == 2 || array.size() == 3) &&
            isNumericArray(array)) {
            return vectorField(array, changed, path, parent);
        }
        auto *field = new QLineEdit(parent);
        QStringList entries;
        for (const QJsonValue &entry : array) {
            entries.append(entry.isString()
                               ? entry.toString()
                               : QString::number(entry.toDouble()));
        }
        field->setText(entries.join(", "));
        field->setPlaceholderText("No items");
        connectLiveText(field, [field, array, changed, path] {
            QJsonArray result;
            for (const QString &entry :
                 field->text().split(',', Qt::SkipEmptyParts)) {
                const QString value = entry.trimmed();
                bool numeric = false;
                const double number = value.toDouble(&numeric);
                result.append(!array.isEmpty() && array.first().isDouble() &&
                                      numeric
                                  ? QJsonValue(number)
                                  : QJsonValue(value));
            }
            changed(path, result);
        });
        return field;
    }
    const QStringList choices = choicesFor(path);
    if (!choices.isEmpty()) {
        auto *field = new QComboBox(parent);
        field->addItems(choices);
        field->setCurrentText(value.toString());
        QObject::connect(
            field, &QComboBox::currentTextChanged, parent,
            [changed, path](const QString &text) { changed(path, text); });
        return field;
    }
    auto *field = new QLineEdit(value.toString(), parent);
    connectLiveText(field,
                    [field, changed, path] { changed(path, field->text()); });
    return field;
}

QFrame *propertyRow(const QString &label, QWidget *editor, QWidget *parent) {
    auto *row = new QFrame(parent);
    row->setObjectName("inspectorPropertyRow");
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 3, 8, 3);
    layout->setSpacing(8);
    auto *name = new QLabel(label, row);
    name->setObjectName("inspectorPropertyLabel");
    name->setMinimumWidth(104);
    name->setMaximumWidth(128);
    layout->addWidget(name);
    layout->addWidget(editor, 1);
    return row;
}

void addPropertyRows(QVBoxLayout *layout, const QJsonObject &properties,
                     const QString &path, const PropertyChanged &changed,
                     QWidget *parent) {
    for (auto iterator = properties.begin(); iterator != properties.end();
         ++iterator) {
        const QString key = iterator.key();
        const QString nextPath = childPath(path, key);
        if (iterator.value().isObject()) {
            auto *group = new QFrame(parent);
            group->setObjectName("inspectorNestedGroup");
            auto *groupLayout = new QVBoxLayout(group);
            groupLayout->setContentsMargins(8, 6, 8, 7);
            groupLayout->setSpacing(2);
            auto *title = new QLabel(humanize(key), group);
            title->setObjectName("inspectorNestedTitle");
            groupLayout->addWidget(title);
            addPropertyRows(groupLayout, iterator.value().toObject(), nextPath,
                            changed, group);
            layout->addWidget(group);
            continue;
        }
        if (iterator.value().isArray()) {
            const QJsonArray array = iterator.value().toArray();
            const bool structuredArray =
                (!array.isEmpty() && array.first().isObject()) ||
                key.compare("wheels", Qt::CaseInsensitive) == 0 ||
                key.compare("differentials", Qt::CaseInsensitive) == 0;
            if (structuredArray) {
                auto *group = new QFrame(parent);
                group->setObjectName("inspectorNestedGroup");
                auto *groupLayout = new QVBoxLayout(group);
                groupLayout->setContentsMargins(8, 6, 8, 7);
                groupLayout->setSpacing(3);
                auto *heading = new QWidget(group);
                auto *headingLayout = new QHBoxLayout(heading);
                headingLayout->setContentsMargins(0, 0, 0, 0);
                headingLayout->setSpacing(4);
                auto *title = new QLabel(humanize(key), heading);
                title->setObjectName("inspectorNestedTitle");
                auto *add = new QToolButton(heading);
                add->setObjectName("inspectorArrayButton");
                add->setText("+");
                add->setToolTip(QStringLiteral("Add %1").arg(humanize(key)));
                headingLayout->addWidget(title, 1);
                headingLayout->addWidget(add);
                groupLayout->addWidget(heading);
                QObject::connect(
                    add, &QToolButton::clicked, group,
                    [array, key, nextPath, changed] {
                        QJsonArray result = array;
                        result.append(
                            key.compare("wheels", Qt::CaseInsensitive) == 0
                                ? vehicleWheelSchema()
                                : vehicleDifferentialSchema());
                        changed(nextPath, result);
                    });
                for (int index = 0; index < array.size(); ++index) {
                    auto *itemHeading = new QWidget(group);
                    auto *itemHeadingLayout = new QHBoxLayout(itemHeading);
                    itemHeadingLayout->setContentsMargins(0, 0, 0, 0);
                    itemHeadingLayout->setSpacing(4);
                    auto *itemTitle = new QLabel(
                        QStringLiteral("%1 %2")
                            .arg(key.compare("wheels", Qt::CaseInsensitive) == 0
                                     ? "Wheel"
                                     : "Differential")
                            .arg(index + 1),
                        itemHeading);
                    itemTitle->setObjectName("inspectorArrayTitle");
                    auto *remove = new QToolButton(itemHeading);
                    remove->setObjectName("inspectorArrayButton");
                    remove->setText("−");
                    remove->setToolTip("Remove");
                    itemHeadingLayout->addWidget(itemTitle, 1);
                    itemHeadingLayout->addWidget(remove);
                    groupLayout->addWidget(itemHeading);
                    QObject::connect(remove, &QToolButton::clicked, group,
                                     [array, index, nextPath, changed] {
                                         QJsonArray result = array;
                                         result.removeAt(index);
                                         changed(nextPath, result);
                                     });
                    addPropertyRows(groupLayout, array.at(index).toObject(),
                                    nextPath + '/' + QString::number(index),
                                    changed, group);
                }
                layout->addWidget(group);
                continue;
            }
        }
        QWidget *editor =
            primitiveField(key, nextPath, iterator.value(), changed, parent);
        layout->addWidget(propertyRow(humanize(key), editor, parent));
    }
}

QFrame *componentCard(const QString &title, const QJsonObject &properties,
                      const QString &path, const PropertyChanged &changed,
                      QWidget *parent) {
    auto *card = new QFrame(parent);
    card->setObjectName("inspectorComponent");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 7);
    layout->setSpacing(2);
    auto *header = new QToolButton(card);
    header->setObjectName("inspectorComponentHeader");
    header->setText(title);
    header->setCheckable(true);
    header->setChecked(true);
    header->setArrowType(Qt::DownArrow);
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    layout->addWidget(header);
    auto *body = new QWidget(card);
    body->setObjectName("inspectorComponentBody");
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 2, 0, 0);
    bodyLayout->setSpacing(1);
    addPropertyRows(bodyLayout, properties, path, changed, body);
    layout->addWidget(body);
    QObject::connect(
        header, &QToolButton::toggled, card, [header, body](bool expanded) {
            body->setVisible(expanded);
            header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        });
    return card;
}

QJsonObject findObjectInArray(const QJsonArray &objects, int id) {
    for (const QJsonValue &value : objects) {
        const QJsonObject object = value.toObject();
        if (object.value("id").toInt(-1) == id)
            return object;
        const QJsonObject child =
            findObjectInArray(object.value("children").toArray(), id);
        if (!child.isEmpty())
            return child;
    }
    return {};
}
} // namespace

InspectorPanel::InspectorPanel(ViewportPanel *viewport,
                               const QString &projectFile, QWidget *parent)
    : QWidget(parent), viewport(viewport) {
    setObjectName("inspectorPanel");
    setAcceptDrops(true);
    const QFileInfo projectInfo(projectFile);
    projectRoot = projectInfo.absoluteDir().absolutePath();
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("inspectorScroll");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content = new QWidget(scrollArea);
    content->setObjectName("inspectorContent");
    contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(8, 8, 8, 10);
    contentLayout->setSpacing(8);
    scrollArea->setWidget(content);
    layout->addWidget(scrollArea);
    if (viewport != nullptr) {
        connect(viewport, &ViewportPanel::sceneSnapshotChanged, this,
                &InspectorPanel::applySceneSnapshot);
    }
    showEmptyState();
}

void InspectorPanel::applySceneSnapshot(const QString &snapshot) {
    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(snapshot.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;
    scene = document.object();
    if (cameraTarget) {
        inspectedCamera = scene.value("camera").toObject();
        return;
    }
    const int selected = scene.value("selectedId").toInt(-1);
    const bool selectionChanged = selected != lastRuntimeSelection;
    lastRuntimeSelection = selected;
    if (selectionChanged) {
        inspectRuntimeObject(selected);
    } else if (!fileTarget && inspectedObjectId >= 0) {
        const QJsonObject updated = findObject(inspectedObjectId);
        const bool contentChanged =
            updated.value("name") != inspectedObject.value("name") ||
            updated.value("properties").toObject().value("material") !=
                inspectedObject.value("properties").toObject().value(
                    "material") ||
            componentShape(updated.value("components").toArray()) !=
                componentShape(inspectedObject.value("components").toArray());
        inspectedObject = updated;
        if (contentChanged) {
            showObject(inspectedObject);
        }
    }
}

void InspectorPanel::inspectRuntimeObject(int id) {
    fileTarget = false;
    cameraTarget = false;
    inspectedFile.clear();
    inspectedCamera = {};
    inspectedObjectId = id;
    inspectedObject = findObject(id);
    if (inspectedObject.isEmpty()) {
        showEmptyState();
    } else {
        showObject(inspectedObject);
    }
}

void InspectorPanel::inspectCamera() {
    fileTarget = false;
    cameraTarget = true;
    inspectedFile.clear();
    inspectedObjectId = -1;
    inspectedObject = {};
    inspectedCamera = scene.value("camera").toObject();
    showCamera();
}

void InspectorPanel::inspectFile(const QString &path) {
    if (path.isEmpty()) {
        fileTarget = false;
        inspectRuntimeObject(lastRuntimeSelection);
        return;
    }
    fileTarget = true;
    cameraTarget = false;
    inspectedObjectId = -1;
    inspectedObject = {};
    inspectedFile = path;
    showFile();
}

void InspectorPanel::showEmptyState() {
    rebuildBody();
    auto *empty = new QWidget(content);
    auto *layout = new QVBoxLayout(empty);
    layout->setContentsMargins(20, 48, 20, 20);
    auto *title = new QLabel("Nothing selected", empty);
    title->setObjectName("inspectorEmptyTitle");
    title->setAlignment(Qt::AlignCenter);
    auto *hint = new QLabel(
        "Select an object in the Hierarchy or an asset in the Content Browser.",
        empty);
    hint->setObjectName("inspectorEmptyHint");
    hint->setWordWrap(true);
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(hint);
    layout->addStretch();
    contentLayout->addWidget(empty, 1);
}

void InspectorPanel::showObject(const QJsonObject &object) {
    rebuildBody();
    const QString type = object.value("type").toString("Object");
    auto *header = new QFrame(content);
    header->setObjectName("inspectorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 10, 10, 10);
    headerLayout->setSpacing(10);
    iconLabel = new QLabel(header);
    iconLabel->setObjectName("inspectorObjectIcon");
    iconLabel->setPixmap(inspectorIcon(this, type).pixmap(42, 42));
    iconLabel->setFixedSize(46, 46);
    auto *identity = new QWidget(header);
    auto *identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(2);
    nameField =
        new QLineEdit(object.value("name").toString("Object"), identity);
    nameField->setObjectName("inspectorNameField");
    typeLabel = new QLabel(humanize(type), identity);
    typeLabel->setObjectName("inspectorTypeLabel");
    identityLayout->addWidget(nameField);
    identityLayout->addWidget(typeLabel);
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(identity, 1);
    contentLayout->addWidget(header);
    connect(nameField, &QLineEdit::editingFinished, this,
            &InspectorPanel::commitHeaderName);

    QPointer<ViewportPanel> runtime(viewport);
    const int objectId = inspectedObjectId;
    auto update = [runtime, objectId](const QString &component, int index,
                                      const QString &path,
                                      const QJsonValue &value) {
        QTimer::singleShot(0,
                           [runtime, objectId, component, index, path, value] {
                               if (runtime != nullptr) {
                                   runtime->setRuntimeObjectProperty(
                                       objectId, component, index, path, value);
                               }
                           });
    };
    QJsonObject transform{{"position", object.value("position")},
                          {"rotation", object.value("rotation")},
                          {"scale", object.value("scale")}};
    contentLayout->addWidget(componentCard(
        "Transform", transform, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update("transform", -1, path, value);
        },
        content));

    QJsonObject objectProperties = object.value("properties").toObject();
    if (type.contains("light", Qt::CaseInsensitive) ||
        type.compare("sun", Qt::CaseInsensitive) == 0) {
        objectProperties = mergeObjects(lightSchema(type), objectProperties);
    }
    const QString materialPath = objectProperties.value("material").toString();
    objectProperties.remove("material");
    const QStringList hidden{"id",       "name",       "type",
                             "position", "rotation",   "scale",
                             "parent",   "components", "objects"};
    for (const QString &key : hidden)
        objectProperties.remove(key);
    if (!objectProperties.isEmpty()) {
        contentLayout->addWidget(componentCard(
            componentTitle(type), objectProperties, QString(),
            [update](const QString &path, const QJsonValue &value) {
                update("object", -1, path, value);
            },
            content));
    }

    if (!materialPath.isEmpty()) {
        contentLayout->addWidget(componentCard(
            "Material", QJsonObject{{"source", materialPath}}, QString(),
            [this, objectId](const QString &path, const QJsonValue &value) {
                if (path == "/source" && value.isString() &&
                    viewport != nullptr) {
                    viewport->applyRuntimeMaterial(objectId, value.toString());
                }
            },
            content));
    }

    const QJsonArray components = object.value("components").toArray();
    for (int index = 0; index < components.size(); ++index) {
        const QJsonObject raw = components.at(index).toObject();
        const QString componentType = raw.value("type").toString("component");
        const QJsonObject values = componentValues(componentType, raw);
        contentLayout->addWidget(componentCard(
            componentTitle(componentType), values, QString(),
            [update, componentType, index](const QString &path,
                                           const QJsonValue &value) {
                update(componentType, index, path, value);
            },
            content));
        if (componentType.toLower().remove('_').remove('-') == "audioplayer") {
            auto *controls = new QFrame(content);
            controls->setObjectName("inspectorAudioControls");
            auto *controlsLayout = new QHBoxLayout(controls);
            controlsLayout->setContentsMargins(8, 4, 8, 6);
            controlsLayout->setSpacing(5);
            const QStringList audioActions{"Play", "Pause", "Stop"};
            for (const QString &action : audioActions) {
                auto *button = new QToolButton(controls);
                button->setText(action);
                controlsLayout->addWidget(button);
                connect(button, &QToolButton::clicked, this,
                        [this, objectId, index, action] {
                            if (viewport != nullptr) {
                                viewport->controlRuntimeAudio(
                                    objectId, index, action.toLower());
                            }
                        });
            }
            controlsLayout->addStretch();
            contentLayout->addWidget(controls);
        }
    }
    auto *addComponent = new QToolButton(content);
    addComponent->setObjectName("inspectorAddComponentButton");
    addComponent->setText("+ Add Component");
    addComponent->setPopupMode(QToolButton::InstantPopup);
    auto *componentMenu = new QMenu(addComponent);
    auto *searchAction = new QWidgetAction(componentMenu);
    auto *componentSearch = new QLineEdit(componentMenu);
    componentSearch->setPlaceholderText("Search components, scripts, materials");
    componentSearch->setClearButtonEnabled(true);
    componentSearch->setMinimumWidth(280);
    searchAction->setDefaultWidget(componentSearch);
    componentMenu->addAction(searchAction);
    componentMenu->addSeparator();
    QList<QAction *> searchableActions;
    const QList<QPair<QString, QString>> componentTypes{
        {"Rigidbody", "rigidbody"},
        {"Audio Player", "audio_player"},
        {"Fixed Joint", "fixed_joint"},
        {"Hinge Joint", "hinge_joint"},
        {"Spring Joint", "spring_joint"},
        {"Vehicle", "vehicle"},
        {"Script", "script"},
        {"Trait Script", "trait_script"},
    };
    for (const auto &[label, componentType] : componentTypes) {
        QAction *action = componentMenu->addAction(
            label, this, [this, componentType, objectId] {
                QPointer<ViewportPanel> runtime(viewport);
                QPointer<InspectorPanel> inspector(this);
                QTimer::singleShot(
                    0, [runtime, inspector, componentType, objectId] {
                        if (runtime == nullptr || inspector == nullptr) {
                            return;
                        }
                        if (runtime->addRuntimeObjectComponent(
                                objectId, componentType,
                                componentSchema(componentType)) < 0) {
                            QMessageBox::information(
                                inspector.data(), "Add Component",
                                "This component is already attached or cannot "
                                "be added to the selected object.");
                        }
                    });
            });
        action->setProperty("searchText", label.toLower());
        searchableActions.append(action);
    }
    QDirIterator assets(projectRoot,
                        {"*.ts",   "*.js",  "*.amat", "*.material",
                         "*.wav",  "*.mp3", "*.ogg",  "*.flac",
                         "*.m4a",  "*.aac"},
                        QDir::Files, QDirIterator::Subdirectories);
    while (assets.hasNext()) {
        const QFileInfo info(assets.next());
        const QString suffix = info.suffix().toLower();
        const bool material = suffix == "amat" || suffix == "material";
        const bool audio = suffix == "wav" || suffix == "mp3" ||
                           suffix == "ogg" || suffix == "flac" ||
                           suffix == "m4a" || suffix == "aac";
        const QString label =
            QStringLiteral("%1 · %2")
                .arg(material ? "Material" : audio ? "Audio" : "Script",
                     info.completeBaseName());
        QAction *action = componentMenu->addAction(
            label, this, [this, objectId, path = info.absoluteFilePath()] {
                attachAsset(path, objectId);
            });
        action->setIcon(inspectorIcon(
            this, material ? "material" : audio ? "audio" : "script"));
        action->setProperty("searchText",
                            (label + ' ' + info.absoluteFilePath()).toLower());
        searchableActions.append(action);
    }
    connect(componentSearch, &QLineEdit::textChanged, componentMenu,
            [searchableActions](const QString &text) {
                const QString query = text.trimmed().toLower();
                for (QAction *action : searchableActions) {
                    action->setVisible(
                        query.isEmpty() ||
                        action->property("searchText").toString().contains(query));
                }
            });
    connect(componentMenu, &QMenu::aboutToShow, componentSearch,
            [componentSearch] {
                componentSearch->clear();
                componentSearch->setFocus();
            });
    addComponent->setMenu(componentMenu);
    contentLayout->addWidget(addComponent);
    contentLayout->addStretch();
}

void InspectorPanel::showCamera() {
    rebuildBody();
    auto *header = new QFrame(content);
    header->setObjectName("inspectorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 10, 10, 10);
    headerLayout->setSpacing(10);
    auto *cameraIcon = new QLabel(header);
    cameraIcon->setObjectName("inspectorObjectIcon");
    cameraIcon->setPixmap(inspectorIcon(this, "camera").pixmap(42, 42));
    cameraIcon->setFixedSize(46, 46);
    auto *identity = new QWidget(header);
    auto *identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(2);
    auto *title = new QLabel("Main Camera", identity);
    title->setObjectName("inspectorCameraTitle");
    auto *kind = new QLabel("Scene Camera", identity);
    kind->setObjectName("inspectorTypeLabel");
    identityLayout->addWidget(title);
    identityLayout->addWidget(kind);
    headerLayout->addWidget(cameraIcon);
    headerLayout->addWidget(identity, 1);
    contentLayout->addWidget(header);

    auto update = [this](const QString &path, const QJsonValue &value) {
        if (viewport != nullptr) {
            viewport->setRuntimeSceneProperty("camera", -1, path, value);
        }
    };
    QJsonObject transform{{"position", inspectedCamera.value("position")},
                          {"target", inspectedCamera.value("target")}};
    QJsonObject projection{
        {"orthographic", inspectedCamera.value("orthographic")},
        {"fieldOfView", inspectedCamera.value("fov")},
        {"orthographicSize", inspectedCamera.value("orthoSize")},
        {"nearClip", inspectedCamera.value("nearClip")},
        {"farClip", inspectedCamera.value("farClip")}};
    QJsonObject focus{{"focusDepth", inspectedCamera.value("focusDepth")},
                      {"focusRange", inspectedCamera.value("focusRange")}};
    QJsonObject controls{
        {"movementSpeed", inspectedCamera.value("movementSpeed")},
        {"mouseSensitivity", inspectedCamera.value("mouseSensitivity")},
        {"controllerLookSensitivity",
         inspectedCamera.value("controllerLookSensitivity")},
        {"lookSmoothness", inspectedCamera.value("lookSmoothness")},
        {"automaticMoving", inspectedCamera.value("automaticMoving")},
        {"actions", inspectedCamera.value("actions").isArray()
                        ? inspectedCamera.value("actions")
                        : QJsonValue(QJsonArray{})}};
    contentLayout->addWidget(componentCard("Transform", transform, QString(),
                                           update, content));
    contentLayout->addWidget(componentCard(
        "Projection", projection, QString(),
        [update](const QString &path, const QJsonValue &value) {
            QString runtimePath = path;
            if (path == "/fieldOfView")
                runtimePath = "/fov";
            else if (path == "/orthographicSize")
                runtimePath = "/orthoSize";
            update(runtimePath, value);
        },
        content));
    contentLayout->addWidget(
        componentCard("Depth of Field", focus, QString(), update, content));
    contentLayout->addWidget(
        componentCard("Camera Controls", controls, QString(), update, content));
    contentLayout->addStretch();
}

bool InspectorPanel::attachAsset(const QString &path, int objectId) {
    return viewport != nullptr && objectId >= 0 &&
           viewport->attachRuntimeAsset(objectId, path);
}

void InspectorPanel::dragEnterEvent(QDragEnterEvent *event) {
    if (inspectedObjectId >= 0 && event->mimeData()->hasUrls()) {
        const QString suffix =
            QFileInfo(event->mimeData()->urls().constFirst().toLocalFile())
                .suffix()
                .toLower();
        if (suffix == "amat" || suffix == "material" || suffix == "ts" ||
            suffix == "js" || suffix == "wav" || suffix == "mp3" ||
            suffix == "ogg" || suffix == "flac" || suffix == "m4a" ||
            suffix == "aac") {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void InspectorPanel::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls() && inspectedObjectId >= 0 &&
        attachAsset(event->mimeData()->urls().constFirst().toLocalFile(),
                    inspectedObjectId)) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void InspectorPanel::showFile() {
    rebuildBody();
    const QFileInfo info(inspectedFile);
    if (!info.exists()) {
        showEmptyState();
        return;
    }
    auto *header = new QFrame(content);
    header->setObjectName("inspectorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 10, 10, 10);
    headerLayout->setSpacing(10);
    iconLabel = new QLabel(header);
    iconLabel->setObjectName("inspectorObjectIcon");
    const QString iconType = info.isDir() ? "folder" : info.suffix();
    iconLabel->setPixmap(inspectorIcon(this, iconType).pixmap(42, 42));
    iconLabel->setFixedSize(46, 46);
    auto *identity = new QWidget(header);
    auto *identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(2);
    nameField = new QLineEdit(info.fileName(), identity);
    nameField->setObjectName("inspectorNameField");
    typeLabel = new QLabel(
        info.isDir() ? "Folder" : info.suffix().toUpper() + " Asset", identity);
    typeLabel->setObjectName("inspectorTypeLabel");
    identityLayout->addWidget(nameField);
    identityLayout->addWidget(typeLabel);
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(identity, 1);
    contentLayout->addWidget(header);
    connect(nameField, &QLineEdit::editingFinished, this,
            &InspectorPanel::commitHeaderName);

    QJsonObject metadata{
        {"path", info.absoluteFilePath()},
        {"size", static_cast<double>(info.size())},
        {"modified", info.lastModified().toString(Qt::ISODate)}};
    auto *metadataCard = componentCard(
        "Asset", metadata, QString(),
        [](const QString &, const QJsonValue &) {}, content);
    metadataCard->setEnabled(false);
    contentLayout->addWidget(metadataCard);

    if (!info.isDir() &&
        (info.suffix().compare("ascene", Qt::CaseInsensitive) == 0 ||
         info.suffix().compare("amat", Qt::CaseInsensitive) == 0 ||
         info.suffix().compare("json", Qt::CaseInsensitive) == 0)) {
        QFile file(info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonParseError error;
            const QJsonDocument document =
                QJsonDocument::fromJson(file.readAll(), &error);
            if (error.error == QJsonParseError::NoError &&
                document.isObject()) {
                auto *propertiesCard = componentCard(
                    info.suffix().compare("ascene", Qt::CaseInsensitive) == 0
                        ? "Scene"
                        : "Properties",
                    document.object(), QString(),
                    [](const QString &, const QJsonValue &) {}, content);
                propertiesCard->setEnabled(false);
                contentLayout->addWidget(propertiesCard);
            }
        }
    }
    auto *open = new QPushButton("Open in Default App", content);
    open->setObjectName("inspectorOpenAssetButton");
    connect(open, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(inspectedFile));
    });
    contentLayout->addWidget(open);
    contentLayout->addStretch();
}

void InspectorPanel::rebuildBody() {
    rebuilding = true;
    while (QLayoutItem *item = contentLayout->takeAt(0)) {
        if (item->widget() != nullptr)
            item->widget()->deleteLater();
        delete item;
    }
    iconLabel = nullptr;
    typeLabel = nullptr;
    nameField = nullptr;
    rebuilding = false;
}

void InspectorPanel::commitHeaderName() {
    if (rebuilding || nameField == nullptr)
        return;
    const QString name = nameField->text().trimmed();
    if (name.isEmpty())
        return;
    if (!fileTarget) {
        const int id = inspectedObjectId;
        QPointer<ViewportPanel> runtime(viewport);
        QTimer::singleShot(0, [runtime, id, name] {
            if (runtime != nullptr)
                runtime->renameRuntimeObject(id, name);
        });
        return;
    }
    const QFileInfo info(inspectedFile);
    if (name == info.fileName() || name.contains('/') || name.contains('\\')) {
        return;
    }
    const QString next = info.dir().filePath(name);
    if (QFileInfo::exists(next) || !QFile::rename(inspectedFile, next)) {
        QMessageBox::warning(this, "Rename Asset",
                             "The asset could not be renamed.");
        nameField->setText(info.fileName());
        return;
    }
    inspectedFile = next;
    showFile();
}

QJsonObject InspectorPanel::findObject(int id) const {
    if (id < 0)
        return {};
    return findObjectInArray(scene.value("objects").toArray(), id);
}
