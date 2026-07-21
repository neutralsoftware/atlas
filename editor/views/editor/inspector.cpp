/*
 * inspector.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Inspector definition and functions
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/inspectorView.h>
#include <editor/styling/icons.h>

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
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QKeyEvent>
#include <QPair>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>

#include "editor/views/viewport.h"
#include "editor/widgets/scrubbableSpinBox.h"

namespace {
using PropertyChanged =
    std::function<void(const QString &, const QJsonValue &)>;
struct SyncOption {
    QString label;
    QJsonValue value;
    QJsonObject source;
};
using SyncOptions = QList<SyncOption>;
struct SyncProvider {
    std::function<SyncOptions(const QJsonValue &)> options;
    std::function<QString(const QString &)> matchedName;
    std::function<void(const QString &, const QJsonObject &)> setMatch;
    std::function<void(const QString &)> clearMatch;

    explicit operator bool() const { return static_cast<bool>(options); }

    SyncOptions operator()(const QJsonValue &target) const {
        return options ? options(target) : SyncOptions{};
    }
};

class PickerSearchField : public QLineEdit {
  public:
    explicit PickerSearchField(QMenu *menu) : QLineEdit(menu), menu(menu) {}

  protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up) {
            const QList<QAction *> actions = selectableActions();
            if (!actions.isEmpty()) {
                int index = actions.indexOf(menu->activeAction());
                if (event->key() == Qt::Key_Down)
                    index = (index + 1) % actions.size();
                else
                    index = index <= 0 ? actions.size() - 1 : index - 1;
                menu->setActiveAction(actions.at(index));
            }
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter) {
            const QList<QAction *> actions = selectableActions();
            QAction *action = menu->activeAction();
            if ((action == nullptr || !actions.contains(action)) &&
                !actions.isEmpty()) {
                action = actions.first();
            }
            if (action != nullptr) {
                action->trigger();
                menu->close();
            }
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            menu->close();
            event->accept();
            return;
        }
        QLineEdit::keyPressEvent(event);
    }

  private:
    QList<QAction *> selectableActions() const {
        QList<QAction *> result;
        for (QAction *action : menu->actions()) {
            auto *widgetAction = qobject_cast<QWidgetAction *>(action);
            if (action->isVisible() && action->isEnabled() &&
                !action->isSeparator() &&
                (widgetAction == nullptr ||
                 widgetAction->defaultWidget() == nullptr)) {
                result.append(action);
            }
        }
        return result;
    }

    QMenu *menu = nullptr;
};

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

QIcon inspectorIcon(QWidget *, const QString &type) {
    const QString normalized = type.toLower();
    if (normalized == "folder")
        return styling::icon(styling::Icon::Folder, "#7E929C");
    if (normalized.contains("camera"))
        return styling::icon(styling::Icon::Camera, "#9E897D");
    if (normalized.contains("environment") ||
        normalized.contains("atmosphere"))
        return styling::icon(styling::Icon::Globe, "#7E929C");
    if (normalized.contains("light") || normalized == "sun")
        return styling::icon(styling::Icon::Lightbulb, "#A1957D");
    if (normalized.contains("terrain"))
        return styling::icon(styling::Icon::Mountains, "#849589");
    if (normalized.contains("particle"))
        return styling::icon(styling::Icon::Sparkle, "#8498A8");
    if (normalized.contains("audio") || normalized == "wav" ||
        normalized == "mp3" || normalized == "ogg" ||
        normalized == "flac")
        return styling::icon(styling::Icon::MusicNote, "#849589");
    if (normalized.contains("material"))
        return styling::icon(styling::Icon::Material, "#9E897D");
    if (normalized == "png" || normalized == "jpg" ||
        normalized == "jpeg" || normalized == "bmp" ||
        normalized == "gif" || normalized == "webp" ||
        normalized == "tif" || normalized == "tiff" ||
        normalized == "tga" || normalized == "hdr" || normalized == "exr")
        return styling::icon(styling::Icon::Image, "#A1957D");
    if (normalized.contains("script") || normalized == "ts" ||
        normalized == "js")
        return styling::icon(styling::Icon::FileCode, "#7E929C");
    if (normalized.contains("rigidbody") || normalized.contains("joint"))
        return styling::icon(styling::Icon::Wrench, "#A1957D");
    if (normalized == "sphere")
        return styling::icon(styling::Icon::Sphere, "#8498A8");
    return styling::icon(styling::Icon::Cube, "#8498A8");
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

QJsonObject environmentSchema() {
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
                          {"shadowResolution", 2048}}},
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
        common.insert("shadowResolution", 2048);
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
    if (key == "condition" && path.contains("weather"))
        return {"clear", "rain", "snow", "storm"};
    return {};
}

QDoubleSpinBox *numberField(double value, QWidget *parent) {
    auto *field = new ScrubbableDoubleSpinBox(parent);
    field->setObjectName("inspectorNumberField");
    field->setRange(-1000000000.0, 1000000000.0);
    field->setDecimals(4);
    field->setSingleStep(0.1);
    field->setButtonSymbols(QAbstractSpinBox::NoButtons);
    field->setValue(value);
    field->setKeyboardTracking(true);
    return field;
}

QJsonValue adaptedSyncValue(const QJsonValue &source,
                            const QJsonValue &target,
                            const QString &path) {
    if (target.isDouble()) {
        if (source.isDouble())
            return source;
        const QJsonArray values = source.toArray();
        if (values.isEmpty())
            return target;
        if (path.endsWith("/radius", Qt::CaseInsensitive)) {
            double maximum = 0.0;
            for (const QJsonValue &value : values)
                maximum = std::max(maximum, std::abs(value.toDouble()));
            return maximum * 0.5;
        }
        if (path.endsWith("/height", Qt::CaseInsensitive) &&
            values.size() > 1) {
            return values.at(1);
        }
        return values.first();
    }
    if (!target.isArray())
        return source;
    const int dimensions = target.toArray().size();
    QJsonArray result;
    if (source.isDouble()) {
        while (result.size() < dimensions)
            result.append(source);
        return result;
    }
    const QJsonArray values = source.toArray();
    for (int index = 0; index < dimensions; ++index) {
        result.append(index < values.size() ? values.at(index)
                                            : values.isEmpty()
                                                  ? QJsonValue(0.0)
                                                  : values.last());
    }
    return result;
}

void setNumericEditorValue(QWidget *editor, const QJsonValue &value) {
    QList<QDoubleSpinBox *> fields = editor->findChildren<QDoubleSpinBox *>();
    if (auto *field = qobject_cast<QDoubleSpinBox *>(editor))
        fields.prepend(field);
    const QJsonArray values = value.toArray();
    for (int index = 0; index < fields.size(); ++index) {
        QSignalBlocker blocker(fields.at(index));
        fields.at(index)->setValue(value.isDouble()
                                       ? value.toDouble()
                                       : index < values.size()
                                             ? values.at(index).toDouble()
                                             : 0.0);
    }
}

void addSyncPicker(QHBoxLayout *layout, const QString &path,
                   const QJsonValue &current, const PropertyChanged &changed,
                   const SyncProvider &provider, QWidget *valueEditor,
                   QWidget *parent) {
    auto *button = new QToolButton(parent);
    button->setObjectName("inspectorSyncButton");
    button->setIcon(
        styling::icon(styling::Icon::ArrowCounterClockwise, "#849589"));
    button->setToolTip("Match this value with another property");
    button->setPopupMode(QToolButton::InstantPopup);
    auto showMatch = [button, valueEditor](const QString &name) {
        const bool matched = !name.isEmpty();
        valueEditor->setVisible(!matched);
        button->setText(name);
        button->setToolButtonStyle(matched ? Qt::ToolButtonTextBesideIcon
                                           : Qt::ToolButtonIconOnly);
        button->setProperty("matched", matched);
        button->setSizePolicy(matched ? QSizePolicy::Expanding
                                      : QSizePolicy::Fixed,
                              QSizePolicy::Preferred);
        button->setMinimumWidth(matched ? 180 : 22);
        button->setToolTip(matched
                               ? QStringLiteral("Matched to %1. Click to change")
                                     .arg(name)
                               : "Match this value with another property");
        button->style()->unpolish(button);
        button->style()->polish(button);
    };
    showMatch(provider.matchedName ? provider.matchedName(path) : QString());
    auto *menu = new QMenu(button);
    menu->setMinimumWidth(360);
    auto *searchAction = new QWidgetAction(menu);
    auto *search = new PickerSearchField(menu);
    search->setPlaceholderText("Search properties");
    search->setClearButtonEnabled(true);
    search->setMinimumWidth(340);
    searchAction->setDefaultWidget(search);
    menu->addAction(searchAction);
    menu->addSeparator();
    auto searchable = std::make_shared<QList<QAction *>>();
    auto generated = std::make_shared<QList<QAction *>>();
    QObject::connect(search, &QLineEdit::textChanged, menu,
                     [searchable](const QString &text) {
                         const QString query = text.trimmed().toLower();
                         for (QAction *action : *searchable) {
                             action->setVisible(
                                 query.isEmpty() ||
                                 action->property("searchText")
                                     .toString()
                                     .contains(query));
                         }
                     });
    QObject::connect(
        menu, &QMenu::aboutToShow, search,
        [menu, search, searchable, generated, provider, current, path, changed,
         showMatch, valueEditor] {
            for (QAction *action : *generated) {
                menu->removeAction(action);
                action->deleteLater();
            }
            generated->clear();
            searchable->clear();
            QAction *manual = menu->addAction(
                "Enter value manually", menu,
                [provider, path, showMatch] {
                    if (provider.clearMatch)
                        provider.clearMatch(path);
                    showMatch(QString());
                });
            manual->setProperty("searchText", "enter value manually unlink");
            searchable->append(manual);
            generated->append(manual);
            QAction *section = menu->addSeparator();
            generated->append(section);
            const SyncOptions options =
                provider ? provider(current) : SyncOptions{};
            for (const SyncOption &option : options) {
                QAction *action = menu->addAction(
                    option.label, menu,
                    [current, option, path, changed, provider, showMatch,
                     valueEditor] {
                        const QJsonValue matched =
                            adaptedSyncValue(option.value, current, path);
                        changed(path, matched);
                        setNumericEditorValue(valueEditor, matched);
                        if (provider.setMatch)
                            provider.setMatch(path, option.source);
                        showMatch(option.label);
                    });
                action->setProperty("searchText", option.label.toLower());
                searchable->append(action);
                generated->append(action);
            }
            if (options.isEmpty()) {
                QAction *empty =
                    menu->addAction("No compatible properties");
                empty->setEnabled(false);
                generated->append(empty);
            }
            search->clear();
            search->setFocus();
        });
    button->setMenu(menu);
    layout->addWidget(button);
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
                     const QString &path, const SyncProvider &syncProvider,
                     QWidget *parent) {
    auto *field = new QFrame(parent);
    field->setObjectName("inspectorVectorField");
    auto *layout = new QHBoxLayout(field);
    layout->setContentsMargins(3, 0, 3, 0);
    layout->setSpacing(2);
    auto *valueEditor = new QWidget(field);
    auto *valueLayout = new QHBoxLayout(valueEditor);
    valueLayout->setContentsMargins(0, 0, 0, 0);
    valueLayout->setSpacing(2);
    layout->addWidget(valueEditor, 1);
    auto values = value;
    const int dimensions = std::clamp(static_cast<int>(value.size()), 2, 3);
    while (values.size() < dimensions)
        values.append(0.0);
    const QStringList axes{"X", "Y", "Z"};
    QList<QDoubleSpinBox *> boxes;
    for (int index = 0; index < dimensions; ++index) {
        auto *axis = new QLabel(axes.at(index), valueEditor);
        axis->setObjectName("inspectorAxisLabel");
        auto *box = numberField(values.at(index).toDouble(), valueEditor);
        box->setButtonSymbols(QAbstractSpinBox::NoButtons);
        box->setMinimumWidth(38);
        boxes.append(box);
        valueLayout->addWidget(axis);
        valueLayout->addWidget(box, 1);
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
    addSyncPicker(layout, path, value, changed, syncProvider, valueEditor,
                  field);
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
        swatch->setIcon(styling::colorSwatch(next, QSize(22, 14)));
        swatch->setIconSize(QSize(22, 14));
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
                     const SyncProvider &syncProvider, QWidget *parent);

QWidget *primitiveField(const QString &name, const QString &path,
                        const QJsonValue &value, const PropertyChanged &changed,
                        const SyncProvider &syncProvider, QWidget *parent) {
    if (value.isBool()) {
        auto *field = new QCheckBox(parent);
        field->setChecked(value.toBool());
        QObject::connect(
            field, &QCheckBox::toggled, parent,
            [changed, path](bool checked) { changed(path, checked); });
        return field;
    }
    if (value.isDouble()) {
        auto *container = new QFrame(parent);
        container->setObjectName("inspectorNumericField");
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        auto *field = numberField(value.toDouble(), container);
        layout->addWidget(field, 1);
        addSyncPicker(layout, path, value, changed, syncProvider, field,
                      container);
        QObject::connect(field, &QDoubleSpinBox::valueChanged, container,
                         [field, changed, path](double) {
                             changed(path, field->value());
                         });
        return container;
    }
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (isColorProperty(name, array)) {
            return colorField(array, changed, path, parent);
        }
        if ((array.size() == 2 || array.size() == 3) &&
            isNumericArray(array)) {
            return vectorField(array, changed, path, syncProvider, parent);
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
                     const SyncProvider &syncProvider, QWidget *parent) {
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
                            changed, syncProvider, group);
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
                add->setIcon(
                    styling::icon(styling::Icon::Plus, "#8498A8"));
                add->setToolTip("Add item");
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
                    remove->setIcon(
                        styling::icon(styling::Icon::Trash, "#A17F7F"));
                    remove->setToolTip("Remove item");
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
                                    changed, syncProvider, group);
                }
                layout->addWidget(group);
                continue;
            }
        }
        QWidget *editor =
            primitiveField(key, nextPath, iterator.value(), changed,
                           syncProvider, parent);
        layout->addWidget(propertyRow(humanize(key), editor, parent));
    }
}

QString syncPointerSegment(QString value) {
    return value.replace('~', "~0").replace('/', "~1");
}

void collectSyncOptions(const QString &label, const QJsonValue &value,
                        const QJsonObject &source, const QString &path,
                        SyncOptions &options) {
    if (value.isDouble()) {
        QJsonObject endpoint = source;
        endpoint.insert("path", path);
        endpoint.insert("fallback", value);
        options.append({label, value, endpoint});
        return;
    }
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (!array.isEmpty() && isNumericArray(array)) {
            QJsonObject endpoint = source;
            endpoint.insert("path", path);
            endpoint.insert("fallback", value);
            options.append({label, value, endpoint});
            return;
        }
        for (int index = 0; index < array.size(); ++index) {
            if (array.at(index).isObject()) {
                collectSyncOptions(
                    QStringLiteral("%1 %2").arg(label).arg(index + 1),
                    array.at(index), source,
                    path + '/' + QString::number(index), options);
            }
        }
        return;
    }
    if (!value.isObject())
        return;
    const QJsonObject object = value.toObject();
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        const QString childLabel =
            label.isEmpty() ? humanize(iterator.key())
                            : label + " · " + humanize(iterator.key());
        collectSyncOptions(childLabel, iterator.value(), source,
                           path + '/' + syncPointerSegment(iterator.key()),
                           options);
    }
}

SyncProvider makeSyncProvider(const SyncOptions &options) {
    SyncProvider provider;
    provider.options = [options](const QJsonValue &target) {
        SyncOptions compatible;
        for (const auto &option : options) {
            if ((target.isDouble() &&
                 (option.value.isDouble() || option.value.isArray())) ||
                (target.isArray() &&
                 (option.value.isDouble() || option.value.isArray()))) {
                compatible.append(option);
            }
        }
        return compatible;
    };
    return provider;
}

QJsonObject syncTargetAtPath(QJsonObject target, const QString &path) {
    target.insert("path", target.value("path").toString() + path);
    return target;
}

SyncProvider bindSyncProvider(const SyncProvider &provider,
                              ViewportPanel *viewport, QJsonObject *scene,
                              const QJsonObject &target) {
    SyncProvider bound = provider;
    bound.matchedName = [provider, scene, target](const QString &path) {
        const QJsonObject endpoint = syncTargetAtPath(target, path);
        const QJsonArray bindings = scene->value("propertySyncs").toArray();
        for (const QJsonValue &entry : bindings) {
            const QJsonObject binding = entry.toObject();
            if (binding.value("target").toObject() != endpoint)
                continue;
            const QJsonObject source = binding.value("source").toObject();
            for (const SyncOption &option : provider(QJsonValue(0.0))) {
                if (option.source == source)
                    return option.label;
            }
        }
        return QString();
    };
    bound.setMatch = [viewport, target](const QString &path,
                                       const QJsonObject &source) {
        if (viewport != nullptr)
            viewport->setRuntimePropertySync(syncTargetAtPath(target, path),
                                             source);
    };
    bound.clearMatch = [viewport, target](const QString &path) {
        if (viewport != nullptr)
            viewport->clearRuntimePropertySync(syncTargetAtPath(target, path));
    };
    return bound;
}

QJsonValue objectSyncReference(const QJsonObject &object) {
    const QJsonValue id = object.value("properties").toObject().value("id");
    if (id.isString() && !id.toString().isEmpty())
        return id;
    if (id.isDouble())
        return QString::number(id.toInt());
    return object.value("name");
}

QFrame *componentCard(const QString &title, const QJsonObject &properties,
                      const QString &path, const PropertyChanged &changed,
                      QWidget *parent, const SyncProvider &syncProvider = {},
                      const std::function<void()> &remove = {}) {
    auto *card = new QFrame(parent);
    card->setObjectName("inspectorComponent");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 7);
    layout->setSpacing(2);
    auto *headerRow = new QWidget(card);
    headerRow->setObjectName("inspectorComponentHeaderRow");
    auto *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 3, 0);
    headerLayout->setSpacing(2);
    auto *header = new QToolButton(headerRow);
    header->setObjectName("inspectorComponentHeader");
    header->setText(title);
    header->setCheckable(true);
    header->setChecked(true);
    header->setIcon(styling::icon(styling::Icon::CaretDown, "#8490A4"));
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    headerLayout->addWidget(header, 1);
    if (remove) {
        auto *removeButton = new QToolButton(headerRow);
        removeButton->setObjectName("inspectorComponentRemoveButton");
        removeButton->setIcon(
            styling::icon(styling::Icon::Trash, "#A17F7F"));
        removeButton->setToolTip(QStringLiteral("Remove %1").arg(title));
        headerLayout->addWidget(removeButton);
        QObject::connect(removeButton, &QToolButton::clicked, card, remove);
    }
    layout->addWidget(headerRow);
    auto *body = new QWidget(card);
    body->setObjectName("inspectorComponentBody");
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 2, 0, 0);
    bodyLayout->setSpacing(1);
    addPropertyRows(bodyLayout, properties, path, changed, syncProvider, body);
    layout->addWidget(body);
    QObject::connect(
        header, &QToolButton::toggled, card, [header, body](bool expanded) {
            body->setVisible(expanded);
            header->setIcon(styling::icon(
                expanded ? styling::Icon::CaretDown : styling::Icon::CaretRight,
                "#8490A4"));
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
    setMinimumWidth(360);
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
    if (environmentTarget)
        return;
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
    environmentTarget = false;
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
    environmentTarget = false;
    inspectedFile.clear();
    inspectedObjectId = -1;
    inspectedObject = {};
    inspectedCamera = scene.value("camera").toObject();
    showCamera();
}

void InspectorPanel::inspectEnvironment() {
    fileTarget = false;
    cameraTarget = false;
    environmentTarget = true;
    inspectedFile.clear();
    inspectedObjectId = -1;
    inspectedObject = {};
    inspectedCamera = {};
    showEnvironment();
}

void InspectorPanel::inspectFile(const QString &path) {
    if (path.isEmpty()) {
        fileTarget = false;
        inspectRuntimeObject(lastRuntimeSelection);
        return;
    }
    fileTarget = true;
    cameraTarget = false;
    environmentTarget = false;
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
    const QJsonValue objectReference = objectSyncReference(object);
    const QJsonArray components = object.value("components").toArray();
    SyncOptions syncOptions;
    syncOptions.append(
        {"Object Size", object.value("boundsSize"),
         QJsonObject{{"section", "object"},
                     {"object", objectReference},
                     {"component", "bounds"},
                     {"componentIndex", -1},
                     {"path", QString()}}});
    collectSyncOptions(
        "Transform", transform,
        QJsonObject{{"section", "object"},
                    {"object", objectReference},
                    {"component", "transform"},
                    {"componentIndex", -1}},
        QString(), syncOptions);
    collectSyncOptions(
        "Object", object.value("properties"),
        QJsonObject{{"section", "object"},
                    {"object", objectReference},
                    {"component", "object"},
                    {"componentIndex", -1}},
        QString(), syncOptions);
    for (int index = 0; index < components.size(); ++index) {
        const QJsonObject raw = components.at(index).toObject();
        const QString componentType = raw.value("type").toString("component");
        collectSyncOptions(
            componentTitle(componentType), componentValues(componentType, raw),
            QJsonObject{{"section", "object"},
                        {"object", objectReference},
                        {"component", componentType},
                        {"componentIndex", index}},
            QString(), syncOptions);
    }
    const SyncProvider syncProvider = makeSyncProvider(syncOptions);
    const QJsonObject transformTarget{{"section", "object"},
                                      {"object", objectReference},
                                      {"component", "transform"},
                                      {"componentIndex", -1}};
    contentLayout->addWidget(componentCard(
        "Transform", transform, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update("transform", -1, path, value);
        },
        content, bindSyncProvider(syncProvider, viewport, &scene,
                                  transformTarget)));

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
            content,
            bindSyncProvider(
                syncProvider, viewport, &scene,
                QJsonObject{{"section", "object"},
                            {"object", objectReference},
                            {"component", "object"},
                            {"componentIndex", -1}})));
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
            content,
            bindSyncProvider(
                syncProvider, viewport, &scene,
                QJsonObject{{"section", "object"},
                            {"object", objectReference},
                            {"component", componentType},
                            {"componentIndex", index}}),
            [this, objectId, index, componentType] {
                if (QMessageBox::question(
                        this, "Remove Component",
                        QStringLiteral("Remove %1 from this object?")
                            .arg(componentTitle(componentType))) !=
                    QMessageBox::Yes) {
                    return;
                }
                if (viewport == nullptr ||
                    !viewport->removeRuntimeObjectComponent(objectId,
                                                            index)) {
                    QMessageBox::warning(this, "Remove Component",
                                         "The component could not be removed.");
                }
            }));
        if (componentType.toLower().remove('_').remove('-') == "audioplayer") {
            auto *controls = new QFrame(content);
            controls->setObjectName("inspectorAudioControls");
            auto *controlsLayout = new QHBoxLayout(controls);
            controlsLayout->setContentsMargins(8, 4, 8, 6);
            controlsLayout->setSpacing(5);
            const QStringList audioActions{"Play", "Pause", "Stop"};
            const QList<styling::Icon> audioIcons{
                styling::Icon::Play, styling::Icon::Pause,
                styling::Icon::Stop};
            const QList<QColor> audioColors{
                QColor("#849589"), QColor("#A1957D"), QColor("#A17F7F")};
            for (int actionIndex = 0; actionIndex < audioActions.size();
                 ++actionIndex) {
                const QString &action = audioActions.at(actionIndex);
                auto *button = new QToolButton(controls);
                button->setIcon(styling::icon(
                    audioIcons.at(actionIndex), audioColors.at(actionIndex)));
                button->setToolTip(action);
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
    addComponent->setIcon(
        styling::icon(styling::Icon::Plus, "#8498A8"));
    addComponent->setText("Add Component");
    addComponent->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addComponent->setPopupMode(QToolButton::InstantPopup);
    auto *componentMenu = new QMenu(addComponent);
    auto *searchAction = new QWidgetAction(componentMenu);
    auto *componentSearch = new PickerSearchField(componentMenu);
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
                        {"*.ts",   "*.amat", "*.material", "*.wav",
                         "*.mp3",  "*.ogg",  "*.flac",     "*.m4a",
                         "*.aac"},
                        QDir::Files, QDirIterator::Subdirectories);
    while (assets.hasNext()) {
        const QFileInfo info(assets.next());
        const QString suffix = info.suffix().toLower();
        if (suffix == "ts") {
            QString relativePath = QDir(projectRoot).relativeFilePath(
                info.absoluteFilePath());
            const QStringList pathParts =
                QDir::fromNativeSeparators(relativePath)
                    .split('/', Qt::SkipEmptyParts);
            bool excluded = false;
            for (int index = 0; index + 1 < pathParts.size(); ++index) {
                const QString directory = pathParts.at(index).toLower();
                if (directory == "lib" || directory == "dist" ||
                    directory == "node_modules") {
                    excluded = true;
                    break;
                }
            }
            if (excluded)
                continue;
        }
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

    auto *cameraView = new QToolButton(content);
    cameraView->setObjectName("inspectorAddComponentButton");
    cameraView->setCheckable(true);
    cameraView->setChecked(viewport != nullptr && viewport->isCameraFocused());
    cameraView->setIcon(styling::icon(styling::Icon::Camera, "#9E897D"));
    cameraView->setText(cameraView->isChecked() ? "Camera View Active"
                                                : "Look Through Camera");
    cameraView->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    cameraView->setToolTip("Toggle Main Camera view · Numpad 0");
    contentLayout->addWidget(cameraView);
    connect(cameraView, &QToolButton::clicked, this, [this] {
        if (viewport != nullptr)
            viewport->toggleCameraFocus();
    });
    if (viewport != nullptr) {
        connect(viewport, &ViewportPanel::cameraFocusChanged, cameraView,
                [cameraView](bool focused) {
                    const QSignalBlocker blocker(cameraView);
                    cameraView->setChecked(focused);
                    cameraView->setText(focused ? "Camera View Active"
                                                : "Look Through Camera");
                });
    }

    auto update = [this](const QString &path, const QJsonValue &value) {
        if (viewport != nullptr) {
            viewport->setRuntimeSceneProperty("camera", -1, path, value);
        }
    };
    QJsonObject transform{{"position", inspectedCamera.value("position")},
                          {"target", inspectedCamera.value("target")}};
    QJsonObject projection{
        {"orthographic", inspectedCamera.value("orthographic")},
        {"fov", inspectedCamera.value("fov")},
        {"orthoSize", inspectedCamera.value("orthoSize")},
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
    SyncOptions syncOptions;
    collectSyncOptions("Camera", inspectedCamera,
                       QJsonObject{{"section", "camera"}}, QString(),
                       syncOptions);
    const SyncProvider syncProvider = makeSyncProvider(syncOptions);
    contentLayout->addWidget(componentCard("Transform", transform, QString(),
                                           update, content,
                                           bindSyncProvider(
                                               syncProvider, viewport, &scene,
                                               QJsonObject{{"section",
                                                            "camera"}})));
    contentLayout->addWidget(componentCard(
        "Projection", projection, QString(),
        update,
        content,
        bindSyncProvider(syncProvider, viewport, &scene,
                         QJsonObject{{"section", "camera"}})));
    contentLayout->addWidget(
        componentCard("Depth of Field", focus, QString(), update, content,
                      bindSyncProvider(syncProvider, viewport, &scene,
                                       QJsonObject{{"section", "camera"}})));
    contentLayout->addWidget(componentCard("Camera Controls", controls,
                                           QString(), update, content,
                                           bindSyncProvider(
                                               syncProvider, viewport, &scene,
                                               QJsonObject{{"section",
                                                            "camera"}})));
    contentLayout->addStretch();
}

void InspectorPanel::showEnvironment() {
    rebuildBody();
    auto *header = new QFrame(content);
    header->setObjectName("inspectorHeader");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 10, 10, 10);
    headerLayout->setSpacing(10);
    auto *environmentIcon = new QLabel(header);
    environmentIcon->setObjectName("inspectorObjectIcon");
    environmentIcon->setPixmap(
        inspectorIcon(this, "environment").pixmap(42, 42));
    environmentIcon->setFixedSize(46, 46);
    auto *identity = new QWidget(header);
    auto *identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(2);
    auto *title = new QLabel("Environment", identity);
    title->setObjectName("inspectorCameraTitle");
    auto *kind = new QLabel("Live Scene Atmosphere", identity);
    kind->setObjectName("inspectorTypeLabel");
    identityLayout->addWidget(title);
    identityLayout->addWidget(kind);
    headerLayout->addWidget(environmentIcon);
    headerLayout->addWidget(identity, 1);
    contentLayout->addWidget(header);

    QJsonObject values = mergeObjects(
        environmentSchema(), scene.value("environment").toObject());
    SyncOptions syncOptions;
    collectSyncOptions("Environment", values,
                       QJsonObject{{"section", "environment"}}, QString(),
                       syncOptions);
    const SyncProvider syncProvider = makeSyncProvider(syncOptions);
    QJsonObject atmosphere = values.take("atmosphere").toObject();
    QJsonObject globalLight = atmosphere.take("globalLight").toObject();
    QJsonObject clouds = atmosphere.take("clouds").toObject();
    QJsonObject weather = atmosphere.take("weather").toObject();
    auto update = [this](const QString &prefix, const QString &path,
                         const QJsonValue &value) {
        if (viewport != nullptr)
            viewport->setRuntimeSceneProperty("environment", -1,
                                              prefix + path, value);
    };
    contentLayout->addWidget(componentCard(
        "Environment", values, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update(QString(), path, value);
        },
        content,
        bindSyncProvider(syncProvider, viewport, &scene,
                         QJsonObject{{"section", "environment"}})));
    contentLayout->addWidget(componentCard(
        "Atmosphere", atmosphere, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update("/atmosphere", path, value);
        },
        content,
        bindSyncProvider(syncProvider, viewport, &scene,
                         QJsonObject{{"section", "environment"},
                                     {"path", "/atmosphere"}})));
    contentLayout->addWidget(componentCard(
        "Global Light", globalLight, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update("/atmosphere/globalLight", path, value);
        },
        content,
        bindSyncProvider(syncProvider, viewport, &scene,
                         QJsonObject{{"section", "environment"},
                                     {"path", "/atmosphere/globalLight"}})));
    contentLayout->addWidget(componentCard(
        "Clouds", clouds, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update("/atmosphere/clouds", path, value);
        },
        content,
        bindSyncProvider(syncProvider, viewport, &scene,
                         QJsonObject{{"section", "environment"},
                                     {"path", "/atmosphere/clouds"}})));
    contentLayout->addWidget(componentCard(
        "Weather", weather, QString(),
        [update](const QString &path, const QJsonValue &value) {
            update("/atmosphere/weather", path, value);
        },
        content,
        bindSyncProvider(syncProvider, viewport, &scene,
                         QJsonObject{{"section", "environment"},
                                     {"path", "/atmosphere/weather"}})));
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

    if (info.isFile()) {
        QImageReader reader(info.absoluteFilePath());
        reader.setAutoTransform(true);
        if (reader.canRead()) {
            const QSize sourceSize = reader.size();
            if (sourceSize.isValid() &&
                (sourceSize.width() > 720 || sourceSize.height() > 480)) {
                reader.setScaledSize(
                    sourceSize.scaled(720, 480, Qt::KeepAspectRatio));
            }
            const QImage image = reader.read();
            if (!image.isNull()) {
                auto *previewCard = new QFrame(content);
                previewCard->setObjectName("inspectorComponent");
                auto *previewLayout = new QVBoxLayout(previewCard);
                previewLayout->setContentsMargins(0, 0, 0, 7);
                previewLayout->setSpacing(2);
                auto *previewTitle = new QLabel("Preview", previewCard);
                previewTitle->setObjectName("inspectorComponentHeader");
                auto *previewBody = new QWidget(previewCard);
                previewBody->setObjectName("inspectorComponentBody");
                auto *previewBodyLayout = new QVBoxLayout(previewBody);
                previewBodyLayout->setContentsMargins(5, 5, 5, 5);
                auto *imageLabel = new QLabel(previewBody);
                imageLabel->setAlignment(Qt::AlignCenter);
                imageLabel->setSizePolicy(QSizePolicy::Expanding,
                                          QSizePolicy::Preferred);
                imageLabel->setPixmap(QPixmap::fromImage(image).scaled(
                    360, 240, Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
                previewBodyLayout->addWidget(imageLabel);
                previewLayout->addWidget(previewTitle);
                previewLayout->addWidget(previewBody);
                contentLayout->addWidget(previewCard);
            }
        }
    }

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
