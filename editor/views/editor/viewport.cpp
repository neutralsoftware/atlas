/*
 * viewport.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Viewport definitions
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/viewport.h>

#include <atlas/input.h>
#include <atlas/runtime/c_api.h>
#include <atlas/runtime/context.h>
#include <atlas/runtime/scripting.h>

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHideEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QPaintEngine>
#include <QPointer>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QShowEvent>
#include <QString>
#include <QTimer>
#include <QUndoCommand>
#include <QUndoStack>
#include <QWheelEvent>
#include <Qt>

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>
#include <utility>

namespace {
constexpr int RuntimeEditorCameraKeyForward = 0;
constexpr int RuntimeEditorCameraKeyBackward = 1;
constexpr int RuntimeEditorCameraKeyLeft = 2;
constexpr int RuntimeEditorCameraKeyRight = 3;
constexpr int RuntimeEditorCameraKeyUp = 4;
constexpr int RuntimeEditorCameraKeyDown = 5;

int runtimeMouseButton(Qt::MouseButton button) {
    switch (button) {
    case Qt::LeftButton:
        return static_cast<int>(MouseButton::Left);
    case Qt::MiddleButton:
        return static_cast<int>(MouseButton::Middle);
    case Qt::RightButton:
        return static_cast<int>(MouseButton::Right);
    case Qt::BackButton:
        return static_cast<int>(MouseButton::Button4);
    case Qt::ForwardButton:
        return static_cast<int>(MouseButton::Button5);
    default:
        return 0;
    }
}

int activeRuntimeMouseButton(Qt::MouseButtons buttons) {
    if (buttons.testFlag(Qt::RightButton)) {
        return runtimeMouseButton(Qt::RightButton);
    }
    if (buttons.testFlag(Qt::MiddleButton)) {
        return runtimeMouseButton(Qt::MiddleButton);
    }
    if (buttons.testFlag(Qt::LeftButton)) {
        return runtimeMouseButton(Qt::LeftButton);
    }
    return runtimeMouseButton(Qt::LeftButton);
}

int editorCameraKey(int key) {
    switch (key) {
    case Qt::Key_Up:
        return RuntimeEditorCameraKeyForward;
    case Qt::Key_Down:
        return RuntimeEditorCameraKeyBackward;
    case Qt::Key_Left:
        return RuntimeEditorCameraKeyLeft;
    case Qt::Key_Right:
        return RuntimeEditorCameraKeyRight;
    case Qt::Key_PageUp:
        return RuntimeEditorCameraKeyUp;
    case Qt::Key_PageDown:
        return RuntimeEditorCameraKeyDown;
    default:
        return -1;
    }
}

float widgetScale(QWidget *widget) {
    const qreal scale = widget != nullptr ? widget->devicePixelRatioF() : 1.0;
    return scale > 0.0 ? static_cast<float>(scale) : 1.0f;
}

QJsonObject findSnapshotObject(const QJsonArray &objects, int id) {
    for (const QJsonValue &entry : objects) {
        const QJsonObject object = entry.toObject();
        if (object.value("id").toInt(-1) == id) {
            return object;
        }
        const QJsonObject child =
            findSnapshotObject(object.value("children").toArray(), id);
        if (!child.isEmpty()) {
            return child;
        }
    }
    return {};
}

QString decodePointerSegment(QString segment) {
    return segment.replace("~1", "/").replace("~0", "~");
}

QJsonValue snapshotValueAt(QJsonValue value, const QString &path) {
    for (const QString &raw : path.split('/', Qt::SkipEmptyParts)) {
        const QString segment = decodePointerSegment(raw);
        if (value.isObject()) {
            value = value.toObject().value(segment);
        } else if (value.isArray()) {
            bool valid = false;
            const int index = segment.toInt(&valid);
            const QJsonArray array = value.toArray();
            if (!valid || index < 0 || index >= array.size()) {
                return QJsonValue(QJsonValue::Undefined);
            }
            value = array.at(index);
        } else {
            return QJsonValue(QJsonValue::Undefined);
        }
    }
    return value;
}

class RuntimePropertyCommand : public QUndoCommand {
  public:
    RuntimePropertyCommand(ViewportPanel *viewport, int objectId,
                           QString component, int componentIndex, QString path,
                           QJsonValue before, QJsonValue after)
        : viewport(viewport), objectId(objectId),
          component(std::move(component)), componentIndex(componentIndex),
          path(std::move(path)), before(std::move(before)),
          after(std::move(after)) {
        setText(QStringLiteral("Edit %1").arg(this->component));
    }

    void undo() override {
        if (viewport != nullptr) {
            viewport->applyRuntimeObjectProperty(
                objectId, component, componentIndex, path, before);
        }
    }

    void redo() override {
        if (viewport != nullptr) {
            viewport->applyRuntimeObjectProperty(
                objectId, component, componentIndex, path, after);
        }
    }

    int id() const override { return 0x415450; }

    bool mergeWith(const QUndoCommand *other) override {
        const auto *command = dynamic_cast<const RuntimePropertyCommand *>(other);
        if (command == nullptr || command->viewport != viewport ||
            command->objectId != objectId || command->component != component ||
            command->componentIndex != componentIndex || command->path != path) {
            return false;
        }
        after = command->after;
        return true;
    }

  private:
    QPointer<ViewportPanel> viewport;
    int objectId;
    QString component;
    int componentIndex;
    QString path;
    QJsonValue before;
    QJsonValue after;
};

class RuntimeRenameCommand : public QUndoCommand {
  public:
    RuntimeRenameCommand(ViewportPanel *viewport, int objectId, QString before,
                         QString after)
        : viewport(viewport), objectId(objectId), before(std::move(before)),
          after(std::move(after)) {
        setText("Rename Object");
    }

    void undo() override {
        if (viewport != nullptr)
            viewport->renameRuntimeObjectDirect(objectId, before);
    }

    void redo() override {
        if (viewport != nullptr)
            viewport->renameRuntimeObjectDirect(objectId, after);
    }

  private:
    QPointer<ViewportPanel> viewport;
    int objectId;
    QString before;
    QString after;
};
}

ViewportPanel::ViewportPanel(const QString &projectFile, QWidget *parent)
    : QWidget(parent), projectFile(projectFile) {
    setAcceptDrops(true);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    frameTimer = new QTimer(this);
    undoStack = new QUndoStack(this);
    frameTimer->setTimerType(Qt::PreciseTimer);
    connect(frameTimer, &QTimer::timeout, this, [this] { stepRuntime(); });
    if (auto *app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this,
                [this] { shutdownRuntime(); });
    }

}

ViewportPanel::~ViewportPanel() { shutdownRuntime(); }

QSize ViewportPanel::sizeHint() const { return QSize(640, 360); }

QSize ViewportPanel::minimumSizeHint() const { return QSize(1, 1); }

QPaintEngine *ViewportPanel::paintEngine() const { return nullptr; }

void ViewportPanel::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (runtimeContext != nullptr) {
        frameTimer->start(16);
        return;
    }
    scheduleRuntimeStart();
}

void ViewportPanel::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
}

void ViewportPanel::closeEvent(QCloseEvent *event) {
    shutdownRuntime();
    QWidget::closeEvent(event);
}

void ViewportPanel::dragEnterEvent(QDragEnterEvent *event) {
    if (selectedRuntimeObjectId() >= 0 && event->mimeData()->hasUrls()) {
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

void ViewportPanel::dropEvent(QDropEvent *event) {
    const int objectId = selectedRuntimeObjectId();
    if (objectId >= 0 && event->mimeData()->hasUrls() &&
        attachRuntimeAsset(
            objectId,
            event->mimeData()->urls().constFirst().toLocalFile())) {
        event->acceptProposedAction();
        emit runtimeObjectActivated(objectId);
        return;
    }
    event->ignore();
}

void ViewportPanel::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (runtimeContext == nullptr) {
        scheduleRuntimeStart();
        return;
    }
    resizeRuntime();
}

void ViewportPanel::scheduleRuntimeStart() {
    if (shuttingDown || runtimeContext != nullptr || runtimeStartQueued ||
        width() <= 1 || height() <= 1) {
        return;
    }
    runtimeStartQueued = true;
    QTimer::singleShot(0, this, [this] {
        runtimeStartQueued = false;
        if (shuttingDown) {
            return;
        }
        if (runtimeContext == nullptr && width() > 1 && height() > 1) {
            startRuntime();
        }
    });
}

void ViewportPanel::shutdownRuntime() {
    shuttingDown = true;
    runtimeStartQueued = false;
    stopRuntime();
}

void ViewportPanel::mousePressEvent(QMouseEvent *event) {
    setFocus(Qt::MouseFocusReason);
    sendPointerEvent(0, static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()),
                     runtimeMouseButton(event->button()));
    if (event->button() == Qt::LeftButton && runtimeContext != nullptr) {
        emit runtimeObjectActivated(runtimeContext->selectedObjectId());
    }
    event->accept();
}

void ViewportPanel::mouseMoveEvent(QMouseEvent *event) {
    sendPointerEvent(1, static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()),
                     activeRuntimeMouseButton(event->buttons()));
    event->accept();
}

void ViewportPanel::mouseReleaseEvent(QMouseEvent *event) {
    sendPointerEvent(2, static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()),
                     runtimeMouseButton(event->button()));
    event->accept();
}

void ViewportPanel::wheelEvent(QWheelEvent *event) {
    if (runtimeContext == nullptr) {
        QWidget::wheelEvent(event);
        return;
    }
    const float delta = static_cast<float>(event->angleDelta().y()) / 120.0f;
    if (std::abs(delta) > 0.0f) {
        runtimeContext->editorScrollEvent(delta, widgetScale(this));
    }
    event->accept();
}

void ViewportPanel::keyPressEvent(QKeyEvent *event) {
    const int key = editorCameraKey(event->key());
    if (event->isAutoRepeat()) {
        if (key >= 0) {
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
        return;
    }
    if (runtimeContext != nullptr && key >= 0) {
        runtimeContext->editorKeyEvent(key, true);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ViewportPanel::keyReleaseEvent(QKeyEvent *event) {
    const int key = editorCameraKey(event->key());
    if (event->isAutoRepeat()) {
        if (key >= 0) {
            event->accept();
            return;
        }
        QWidget::keyReleaseEvent(event);
        return;
    }
    if (runtimeContext != nullptr && key >= 0) {
        runtimeContext->editorKeyEvent(key, false);
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void ViewportPanel::startRuntime() {
    if (shuttingDown || runtimeContext != nullptr || width() <= 1 ||
        height() <= 1) {
        return;
    }
#ifdef METAL
    const std::string runtimeProjectFile = projectFile.toUtf8().toStdString();
    if (runtimeProjectFile.empty()) {
        qWarning() << "Atlas viewport runtime project file is not configured";
        return;
    }

    void *metalView = reinterpret_cast<void *>(static_cast<quintptr>(winId()));
    if (metalView == nullptr) {
        qWarning() << "Atlas viewport could not resolve a native Metal view";
        return;
    }

    try {
        runtimeContext = runtime::makeContextForMetalViewNonBlocking(
            runtimeProjectFile, metalView);
        runtimeContext->setEditorControlsEnabled(true);
        runtimeContext->setEditorSimulationEnabled(false);
        runtimeContext->setEditorControlMode(1);
        runtimeContext->setEditorShadingMode(shadingMode);
        resizeRuntime();
        refreshSceneSnapshot();
        emit runtimeAvailabilityChanged(true);
        playbackState = 0;
        emit playbackStateChanged(playbackState);
        frameTimer->start(16);
    } catch (const std::exception &error) {
        qWarning().noquote()
            << QStringLiteral("Failed to start Atlas viewport runtime: %1")
                   .arg(QString::fromUtf8(error.what()));
        runtimeContext.reset();
    } catch (...) {
        qWarning() << "Failed to start Atlas viewport runtime";
        runtimeContext.reset();
    }
#else
    qWarning() << "Atlas viewport runtime embedding requires the Metal backend";
#endif
}

void ViewportPanel::stopRuntime() {
    if (frameTimer != nullptr) {
        frameTimer->stop();
    }
    if (runtimeContext == nullptr) {
        return;
    }
    auto context = std::move(runtimeContext);
    lastSceneSnapshot.clear();
    if (undoStack != nullptr)
        undoStack->clear();
    emit runtimeAvailabilityChanged(false);
    playbackState = 0;
    emit playbackStateChanged(playbackState);
    try {
        context->end();
    } catch (const std::exception &error) {
        qWarning().noquote()
            << QStringLiteral("Failed to stop Atlas viewport runtime: %1")
                   .arg(QString::fromUtf8(error.what()));
    } catch (...) {
        qWarning() << "Failed to stop Atlas viewport runtime";
    }
    runtimeWidth = 0;
    runtimeHeight = 0;
    runtimeScale = 0.0f;
}

void ViewportPanel::stepRuntime() {
    if (runtimeContext == nullptr) {
        return;
    }
    try {
        if (!runtimeContext->stepFrame()) {
            stopRuntime();
            return;
        }
        refreshSceneSnapshot();
        emit frameRateChanged(runtimeContext->frameRate());
    } catch (const std::exception &error) {
        qWarning().noquote()
            << QStringLiteral("Atlas viewport runtime frame failed: %1")
                   .arg(QString::fromUtf8(error.what()));
        stopRuntime();
    } catch (...) {
        qWarning() << "Atlas viewport runtime frame failed";
        stopRuntime();
    }
}

void ViewportPanel::resizeRuntime() {
    if (runtimeContext == nullptr) {
        return;
    }
    const int nextWidth = std::max(1, width());
    const int nextHeight = std::max(1, height());
    const float nextScale = widgetScale(this);
    if (nextWidth == runtimeWidth && nextHeight == runtimeHeight &&
        std::abs(nextScale - runtimeScale) <= 0.0001f) {
        return;
    }
    runtimeContext->resize(nextWidth, nextHeight, nextScale);
    runtimeWidth = nextWidth;
    runtimeHeight = nextHeight;
    runtimeScale = nextScale;
}

void ViewportPanel::sendPointerEvent(int action, float x, float y, int button) {
    if (runtimeContext == nullptr || button == 0) {
        return;
    }
    const float flippedY = static_cast<float>(height()) - y;
    runtimeContext->editorPointerEvent(action, x, flippedY, button,
                                       widgetScale(this));
}

bool ViewportPanel::selectRuntimeObject(int id, bool focusCamera) {
    if (runtimeContext == nullptr ||
        !runtimeContext->selectObject(id, focusCamera)) {
        return false;
    }
    refreshSceneSnapshot();
    if (id >= 0) {
        setFocus(Qt::OtherFocusReason);
    }
    return true;
}

bool ViewportPanel::renameRuntimeObject(int id, const QString &name) {
    const QJsonObject object = findSnapshotObject(
        QJsonDocument::fromJson(lastSceneSnapshot.toUtf8())
            .object()
            .value("objects")
            .toArray(),
        id);
    const QString previous = object.value("name").toString();
    if (previous.isEmpty() || previous == name) {
        return previous == name;
    }
    undoStack->push(new RuntimeRenameCommand(this, id, previous, name));
    return true;
}

bool ViewportPanel::renameRuntimeObjectDirect(int id, const QString &name) {
    if (runtimeContext == nullptr ||
        !runtimeContext->renameObject(id, name.toUtf8().toStdString())) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    return true;
}

bool ViewportPanel::setRuntimeObjectProperty(
    int id, const QString &component, int componentIndex,
    const QString &propertyPath, const QJsonValue &value) {
    const QJsonValue previous = runtimeObjectProperty(
        id, component, componentIndex, propertyPath);
    if (previous.isUndefined()) {
        return applyRuntimeObjectProperty(id, component, componentIndex,
                                          propertyPath, value);
    }
    if (previous == value) {
        return true;
    }
    undoStack->push(new RuntimePropertyCommand(
        this, id, component, componentIndex, propertyPath, previous, value));
    return true;
}

bool ViewportPanel::setRuntimeSceneProperty(
    const QString &section, int index, const QString &propertyPath,
    const QJsonValue &value) {
    if (runtimeContext == nullptr || section.isEmpty()) {
        return false;
    }
    QJsonArray wrapper;
    wrapper.append(value);
    const QByteArray payload =
        QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
    try {
        const json parsed = json::parse(payload.constData());
        if (!parsed.is_array() || parsed.empty() ||
            !runtimeContext->setSceneProperty(
                section.toStdString(), index, propertyPath.toStdString(),
                parsed.front())) {
            return false;
        }
    } catch (const json::exception &) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    return true;
}

bool ViewportPanel::applyRuntimeObjectProperty(
    int id, const QString &component, int componentIndex,
    const QString &propertyPath, const QJsonValue &value) {
    if (runtimeContext == nullptr) {
        return false;
    }
    QJsonArray wrapper;
    wrapper.append(value);
    const QByteArray payload =
        QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
    try {
        const json parsed = json::parse(payload.constData());
        if (!parsed.is_array() || parsed.empty() ||
            !runtimeContext->setObjectProperty(
                id, component.toStdString(), componentIndex,
                propertyPath.toStdString(), parsed.front())) {
            return false;
        }
    } catch (const json::exception &) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    return true;
}

int ViewportPanel::addRuntimeObjectComponent(
    int id, const QString &type, const QJsonObject &properties) {
    if (runtimeContext == nullptr || type.isEmpty()) {
        return -1;
    }
    QJsonObject definition = properties;
    definition.insert("type", type);
    const QByteArray payload =
        QJsonDocument(definition).toJson(QJsonDocument::Compact);
    try {
        const json parsed = json::parse(payload.constData());
        const int index = runtimeContext->addObjectComponent(id, parsed);
        if (index >= 0) {
            runtimeContext->saveCurrentScene();
            refreshSceneSnapshot();
        }
        return index;
    } catch (const json::exception &) {
        return -1;
    }
}

bool ViewportPanel::controlRuntimeAudio(int id, int componentIndex,
                                        const QString &action) {
    return runtimeContext != nullptr &&
           runtimeContext->controlObjectAudio(id, componentIndex,
                                              action.toStdString());
}

bool ViewportPanel::setRuntimeObjectParent(int childId, int parentId) {
    if (runtimeContext == nullptr ||
        !runtimeContext->setObjectParent(childId, parentId)) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    return true;
}

bool ViewportPanel::deleteRuntimeObject(int id) {
    if (runtimeContext == nullptr || !runtimeContext->deleteObject(id)) {
        return false;
    }
    if (undoStack != nullptr) {
        undoStack->clear();
    }
    if (!runtimeContext->saveCurrentScene()) {
        qWarning() << "Atlas editor could not persist the deleted object";
    }
    refreshSceneSnapshot();
    emit runtimeObjectActivated(-1);
    return true;
}

int ViewportPanel::createRuntimeObject(const QString &type,
                                       const QString &name) {
    if (runtimeContext == nullptr) {
        return -1;
    }
    const int id = runtimeContext->createObject(type.toUtf8().toStdString(),
                                                name.toUtf8().toStdString());
    if (id >= 0) {
        runtimeContext->saveCurrentScene();
        refreshSceneSnapshot();
    }
    return id;
}

bool ViewportPanel::saveRuntimeScene() {
    return runtimeContext != nullptr && runtimeContext->saveCurrentScene();
}

int ViewportPanel::selectedRuntimeObjectId() const {
    return runtimeContext != nullptr ? runtimeContext->selectedObjectId() : -1;
}

bool ViewportPanel::applyRuntimeMaterial(int id, const QString &path) {
    return applyRuntimeMaterialDirect(id, path);
}

bool ViewportPanel::applyRuntimeMaterialDirect(int id, const QString &path) {
    if (runtimeContext == nullptr || id < 0 || path.isEmpty() ||
        !runtimeContext->setObjectMaterial(id, path.toStdString())) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    return true;
}

bool ViewportPanel::attachRuntimeAsset(int id, const QString &path) {
    const QFileInfo info(path);
    const QString suffix = info.suffix().toLower();
    if (suffix == "amat" || suffix == "material") {
        return applyRuntimeMaterial(id, info.absoluteFilePath());
    }
    if (suffix == "ts" || suffix == "js") {
        return addRuntimeObjectComponent(
                   id, "script",
                   QJsonObject{{"name", info.completeBaseName()},
                               {"source", info.absoluteFilePath()},
                               {"variables", QJsonObject{}}}) >= 0;
    }
    if (suffix == "wav" || suffix == "mp3" || suffix == "ogg" ||
        suffix == "flac" || suffix == "m4a" || suffix == "aac") {
        return addRuntimeObjectComponent(
                   id, "audio_player",
                   QJsonObject{{"source", info.absoluteFilePath()},
                               {"useSpatialization", true},
                               {"volume", 1.0},
                               {"loop", false},
                               {"autoplay", false}}) >= 0;
    }
    return false;
}

void ViewportPanel::undo() {
    if (undoStack != nullptr) {
        undoStack->undo();
        const int selected = selectedRuntimeObjectId();
        if (selected >= 0)
            emit runtimeObjectActivated(selected);
    }
}

void ViewportPanel::redo() {
    if (undoStack != nullptr) {
        undoStack->redo();
        const int selected = selectedRuntimeObjectId();
        if (selected >= 0)
            emit runtimeObjectActivated(selected);
    }
}

QJsonValue ViewportPanel::runtimeObjectProperty(
    int id, const QString &component, int componentIndex,
    const QString &propertyPath) const {
    const QJsonDocument document =
        QJsonDocument::fromJson(lastSceneSnapshot.toUtf8());
    const QJsonObject object = findSnapshotObject(
        document.object().value("objects").toArray(), id);
    if (object.isEmpty())
        return QJsonValue(QJsonValue::Undefined);
    const QString normalized = component.toLower();
    if (normalized == "transform")
        return snapshotValueAt(object, propertyPath);
    if (normalized == "object")
        return snapshotValueAt(object.value("properties"), propertyPath);
    const QJsonArray components = object.value("components").toArray();
    if (componentIndex < 0 || componentIndex >= components.size())
        return QJsonValue(QJsonValue::Undefined);
    return snapshotValueAt(components.at(componentIndex), propertyPath);
}

void ViewportPanel::playRuntime() {
    if (runtimeContext == nullptr) {
        return;
    }
    runtimeContext->setEditorSimulationEnabled(true);
    playbackState = 1;
    emit playbackStateChanged(playbackState);
}

void ViewportPanel::pauseRuntime() {
    if (runtimeContext == nullptr || playbackState == 0) {
        return;
    }
    saveRuntimeScene();
    runtimeContext->setEditorSimulationEnabled(false);
    refreshSceneSnapshot();
    playbackState = 2;
    emit playbackStateChanged(playbackState);
}

void ViewportPanel::stepRuntimeOnce() {
    if (runtimeContext == nullptr || playbackState == 0) {
        return;
    }
    runtimeContext->setEditorSimulationEnabled(true);
    stepRuntime();
    if (runtimeContext != nullptr) {
        saveRuntimeScene();
        runtimeContext->setEditorSimulationEnabled(false);
        playbackState = 2;
        emit playbackStateChanged(playbackState);
    }
}

void ViewportPanel::stopRuntimePlayback() {
    if (runtimeContext == nullptr || playbackState == 0) {
        return;
    }
    reloadRuntime();
}

void ViewportPanel::reloadRuntime() {
    if (shuttingDown) {
        return;
    }
    stopRuntime();
    QTimer::singleShot(0, this, [this] { scheduleRuntimeStart(); });
}

void ViewportPanel::setRuntimeShadingMode(int mode) {
    if (mode < 0 || mode > 2) {
        return;
    }
    shadingMode = mode;
    if (runtimeContext != nullptr) {
        runtimeContext->setEditorShadingMode(mode);
    }
}

void ViewportPanel::setRuntimeControlMode(int mode) {
    if (mode < 0 || mode > 3 || runtimeContext == nullptr) {
        return;
    }
    runtimeContext->setEditorControlMode(mode);
}

void ViewportPanel::refreshSceneSnapshot() {
    if (runtimeContext == nullptr) {
        return;
    }
    const QString snapshot =
        QString::fromStdString(runtimeContext->sceneObjectsJson());
    if (snapshot == lastSceneSnapshot) {
        return;
    }
    lastSceneSnapshot = snapshot;
    emit sceneSnapshotChanged(snapshot);
}
