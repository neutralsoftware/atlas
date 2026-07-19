/*
 * viewport.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Viewport definitions
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/viewport.h>
#include <editor/application/toolchainInstaller.h>

#include <atlas/input.h>
#include <atlas/runtime/c_api.h>
#include <atlas/runtime/context.h>
#include <atlas/runtime/scripting.h>

#include <QCloseEvent>
#include <QCoreApplication>
#include <QCursor>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QHideEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
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

int activeRuntimeMouseButton(Qt::MouseButtons buttons,
                             int rightDragRuntimeButton) {
    if (buttons.testFlag(Qt::RightButton)) {
        return rightDragRuntimeButton;
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

QJsonObject findSnapshotObjectByName(const QJsonArray &objects,
                                     const QString &name) {
    for (const QJsonValue &entry : objects) {
        const QJsonObject object = entry.toObject();
        if (object.value("name").toString() == name)
            return object;
        const QJsonObject child = findSnapshotObjectByName(
            object.value("children").toArray(), name);
        if (!child.isEmpty())
            return child;
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
                           QJsonValue before, QJsonValue after,
                           QUndoCommand *parent = nullptr)
        : QUndoCommand(parent), viewport(viewport), objectId(objectId),
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
    resizeTimer = new QTimer(this);
    environmentReloadTimer = new QTimer(this);
    undoStack = new QUndoStack(this);
    frameTimer->setTimerType(Qt::PreciseTimer);
    resizeTimer->setSingleShot(true);
    resizeTimer->setInterval(0);
    environmentReloadTimer->setSingleShot(true);
    environmentReloadTimer->setInterval(140);
    connect(frameTimer, &QTimer::timeout, this, [this] { stepRuntime(); });
    connect(resizeTimer, &QTimer::timeout, this,
            [this] { resizeRuntime(); });
    connect(environmentReloadTimer, &QTimer::timeout, this,
            &ViewportPanel::reloadRuntime);
    if (auto *app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this,
                [this] { shutdownRuntime(); });
    }

}

ViewportPanel::~ViewportPanel() { shutdownRuntime(); }

QSize ViewportPanel::sizeHint() const { return QSize(640, 360); }

QSize ViewportPanel::minimumSizeHint() const { return QSize(1, 1); }

QPaintEngine *ViewportPanel::paintEngine() const { return nullptr; }

void ViewportPanel::setRuntimeStartupEnabled(bool enabled) {
    runtimeStartupEnabled = enabled;
    if (runtimeStartupEnabled)
        scheduleRuntimeStart();
}

void ViewportPanel::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (runtimeContext != nullptr) {
        frameTimer->start(16);
        return;
    }
    if (runtimeStartupEnabled)
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
    if (event->mimeData()->hasUrls()) {
        const QString suffix =
            QFileInfo(event->mimeData()->urls().constFirst().toLocalFile())
                .suffix()
                .toLower();
        const bool model = suffix == "obj" || suffix == "fbx" ||
                           suffix == "gltf" || suffix == "glb" ||
                           suffix == "dae";
        if (model ||
            (selectedRuntimeObjectId() >= 0 &&
             (suffix == "amat" || suffix == "material" || suffix == "ts" ||
            suffix == "js" || suffix == "wav" || suffix == "mp3" ||
            suffix == "ogg" || suffix == "flac" || suffix == "m4a" ||
              suffix == "aac"))) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void ViewportPanel::dropEvent(QDropEvent *event) {
    const QString path =
        event->mimeData()->hasUrls()
            ? event->mimeData()->urls().constFirst().toLocalFile()
            : QString();
    const QString suffix = QFileInfo(path).suffix().toLower();
    if ((suffix == "obj" || suffix == "fbx" || suffix == "gltf" ||
         suffix == "glb" || suffix == "dae") &&
        importRuntimeModel(path)) {
        event->acceptProposedAction();
        return;
    }
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
    if (runtimeContext == nullptr && runtimeStartupEnabled) {
        scheduleRuntimeStart();
        return;
    }
    if (!resizeTimer->isActive())
        resizeTimer->start();
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
    if (resizeTimer != nullptr)
        resizeTimer->stop();
    if (environmentReloadTimer != nullptr)
        environmentReloadTimer->stop();
    stopRuntime();
}

void ViewportPanel::mousePressEvent(QMouseEvent *event) {
    setFocus(Qt::MouseFocusReason);
    if (keyboardTransformActive &&
        (event->button() == Qt::LeftButton ||
         event->button() == Qt::RightButton)) {
        finishKeyboardTransform(event->button() == Qt::LeftButton);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        leftPointerMoved = false;
        const int selected = selectedRuntimeObjectId();
        transformUndoBefore =
            findSnapshotObject(
                QJsonDocument::fromJson(lastSceneSnapshot.toUtf8())
                    .object()
                    .value("objects")
                    .toArray(),
                selected);
    }
    int pointerButton = runtimeMouseButton(event->button());
    if (event->button() == Qt::RightButton) {
        rightDragRuntimeButton =
            event->modifiers().testFlag(Qt::ShiftModifier)
                ? runtimeMouseButton(Qt::RightButton)
                : runtimeMouseButton(Qt::MiddleButton);
        pointerButton = rightDragRuntimeButton;
    }
    sendPointerEvent(0, static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()), pointerButton);
    if (event->button() == Qt::LeftButton && runtimeContext != nullptr) {
        emit runtimeObjectActivated(runtimeContext->selectedObjectId());
    }
    event->accept();
}

void ViewportPanel::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton))
        leftPointerMoved = true;
    sendPointerEvent(1, static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()),
                     activeRuntimeMouseButton(event->buttons(),
                                              rightDragRuntimeButton));
    if (keyboardTransformActive) {
        const QRect bounds(mapToGlobal(QPoint(0, 0)), size());
        QPoint cursor = event->globalPosition().toPoint();
        bool wrapped = false;
        if (cursor.x() <= bounds.left() + 2) {
            cursor.setX(bounds.right() - 3);
            wrapped = true;
        } else if (cursor.x() >= bounds.right() - 2) {
            cursor.setX(bounds.left() + 3);
            wrapped = true;
        }
        if (cursor.y() <= bounds.top() + 2) {
            cursor.setY(bounds.bottom() - 3);
            wrapped = true;
        } else if (cursor.y() >= bounds.bottom() - 2) {
            cursor.setY(bounds.top() + 3);
            wrapped = true;
        }
        if (wrapped)
            QCursor::setPos(cursor);
    }
    event->accept();
}

void ViewportPanel::mouseReleaseEvent(QMouseEvent *event) {
    const int pointerButton = event->button() == Qt::RightButton
                                  ? rightDragRuntimeButton
                                  : runtimeMouseButton(event->button());
    sendPointerEvent(2, static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()), pointerButton);
    if (event->button() == Qt::LeftButton && leftPointerMoved &&
        runtimeContext != nullptr && selectedRuntimeObjectId() >= 0) {
        const int selected = selectedRuntimeObjectId();
        runtimeContext->saveCurrentScene();
        refreshSceneSnapshot();
        pushTransformUndo(selected, transformUndoBefore);
        transformUndoBefore = {};
        setSceneDirty(true);
        emit runtimeObjectActivated(selected);
    }
    leftPointerMoved = false;
    if (event->button() == Qt::RightButton)
        rightDragRuntimeButton = 0;
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
    if (!event->isAutoRepeat() && runtimeContext != nullptr &&
        playbackState == 0) {
        if (keyboardTransformActive) {
            if (event->key() == Qt::Key_Escape) {
                finishKeyboardTransform(false);
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Return ||
                event->key() == Qt::Key_Enter) {
                finishKeyboardTransform(true);
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_X || event->key() == Qt::Key_Y ||
                event->key() == Qt::Key_Z) {
                updateKeyboardTransformAxes(
                    event->key(),
                    event->modifiers().testFlag(Qt::ShiftModifier));
                event->accept();
                return;
            }
        } else if (event->key() == Qt::Key_X &&
                   selectedRuntimeObjectId() >= 0) {
            deleteRuntimeObject(selectedRuntimeObjectId());
            event->accept();
            return;
        } else if (event->key() == Qt::Key_G ||
                   event->key() == Qt::Key_R ||
                   event->key() == Qt::Key_S) {
            beginKeyboardTransform(event->key() == Qt::Key_G   ? 1
                                   : event->key() == Qt::Key_R ? 2
                                                               : 3);
            event->accept();
            return;
        }
    }
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
        emit runtimeStartupFinished(false,
                                    "Runtime project file is not configured");
        return;
    }

    void *metalView = reinterpret_cast<void *>(static_cast<quintptr>(winId()));
    if (metalView == nullptr) {
        qWarning() << "Atlas viewport could not resolve a native Metal view";
        emit runtimeStartupFinished(false,
                                    "Viewport native surface is unavailable");
        return;
    }

    try {
        runtimeContext = runtime::makeContextForMetalViewNonBlocking(
            runtimeProjectFile, metalView);
        runtimeContext->setEditorControlsEnabled(true);
        runtimeContext->setEditorSimulationEnabled(false);
        runtimeContext->setEditorControlMode(0);
        runtimeContext->setEditorShadingMode(shadingMode);
        resizeRuntime();
        refreshSceneSnapshot();
        if (!selectionToRestore.isEmpty()) {
            const QJsonDocument document =
                QJsonDocument::fromJson(lastSceneSnapshot.toUtf8());
            const QJsonObject restored = findSnapshotObjectByName(
                document.object().value("objects").toArray(),
                selectionToRestore);
            const int restoredId = restored.value("id").toInt(-1);
            selectionToRestore.clear();
            if (restoredId >= 0 &&
                runtimeContext->selectObject(restoredId, false)) {
                refreshSceneSnapshot();
                emit runtimeObjectActivated(restoredId);
            }
        }
        emit runtimeAvailabilityChanged(true);
        playbackState = 0;
        emit playbackStateChanged(playbackState);
        frameTimer->start(16);
        emit sceneOpened(currentRuntimeScene());
        emit runtimeStartupFinished(true, {});
        if (playAfterRuntimeStart) {
            playAfterRuntimeStart = false;
            runtimeContext->setEditorSimulationEnabled(true);
            playbackState = 1;
            emit playbackStateChanged(playbackState);
        }
    } catch (const std::exception &error) {
        qWarning().noquote()
            << QStringLiteral("Failed to start Atlas viewport runtime: %1")
                   .arg(QString::fromUtf8(error.what()));
        runtimeContext.reset();
        playAfterRuntimeStart = false;
        emit runtimeStartupFinished(false,
                                    QString::fromUtf8(error.what()));
    } catch (...) {
        qWarning() << "Failed to start Atlas viewport runtime";
        runtimeContext.reset();
        playAfterRuntimeStart = false;
        emit runtimeStartupFinished(false, "Runtime initialization failed");
    }
#else
    qWarning() << "Atlas viewport runtime embedding requires the Metal backend";
    emit runtimeStartupFinished(false,
                                "Runtime embedding requires the Metal backend");
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
    try {
        runtimeContext->resize(nextWidth, nextHeight, nextScale);
        runtimeWidth = nextWidth;
        runtimeHeight = nextHeight;
        runtimeScale = nextScale;
    } catch (const std::exception &error) {
        qWarning().noquote()
            << QStringLiteral("Atlas viewport resize failed: %1")
                   .arg(QString::fromUtf8(error.what()));
    } catch (...) {
        qWarning() << "Atlas viewport resize failed";
    }
}

void ViewportPanel::sendPointerEvent(int action, float x, float y, int button) {
    if (keyboardTransformActive && action == 1 && button == 0)
        button = 1;
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

bool ViewportPanel::focusRuntimeObjects(const QList<int> &ids) {
    if (runtimeContext == nullptr || ids.isEmpty())
        return false;
    std::vector<int> runtimeIds;
    runtimeIds.reserve(static_cast<std::size_t>(ids.size()));
    for (int id : ids)
        runtimeIds.push_back(id);
    return runtimeContext->focusObjects(runtimeIds);
}

bool ViewportPanel::renameRuntimeObject(int id, const QString &name) {
    if (playbackState != 0)
        return false;
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
    if (runtimeContext == nullptr || playbackState != 0 ||
        !runtimeContext->renameObject(id, name.toUtf8().toStdString())) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    setSceneDirty(true);
    return true;
}

bool ViewportPanel::setRuntimeObjectProperty(
    int id, const QString &component, int componentIndex,
    const QString &propertyPath, const QJsonValue &value) {
    if (playbackState != 0)
        return false;
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
    if (runtimeContext == nullptr || playbackState != 0 || section.isEmpty()) {
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
    setSceneDirty(true);
    if (section.compare("environment", Qt::CaseInsensitive) == 0)
        environmentReloadTimer->start();
    return true;
}

bool ViewportPanel::setRuntimePropertySync(const QJsonObject &target,
                                           const QJsonObject &source) {
    if (runtimeContext == nullptr || playbackState != 0 || target.isEmpty() ||
        source.isEmpty()) {
        return false;
    }
    try {
        const json parsedTarget = json::parse(
            QJsonDocument(target).toJson(QJsonDocument::Compact).constData());
        const json parsedSource = json::parse(
            QJsonDocument(source).toJson(QJsonDocument::Compact).constData());
        if (!runtimeContext->setPropertySync(parsedTarget, parsedSource) ||
            !runtimeContext->saveCurrentScene()) {
            return false;
        }
    } catch (const json::exception &) {
        return false;
    }
    refreshSceneSnapshot();
    setSceneDirty(true);
    return true;
}

bool ViewportPanel::clearRuntimePropertySync(const QJsonObject &target) {
    if (runtimeContext == nullptr || playbackState != 0 || target.isEmpty())
        return false;
    try {
        const json parsedTarget = json::parse(
            QJsonDocument(target).toJson(QJsonDocument::Compact).constData());
        if (!runtimeContext->clearPropertySync(parsedTarget) ||
            !runtimeContext->saveCurrentScene()) {
            return false;
        }
    } catch (const json::exception &) {
        return false;
    }
    refreshSceneSnapshot();
    setSceneDirty(true);
    return true;
}

bool ViewportPanel::applyRuntimeObjectProperty(
    int id, const QString &component, int componentIndex,
    const QString &propertyPath, const QJsonValue &value) {
    if (runtimeContext == nullptr || playbackState != 0) {
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
    setSceneDirty(true);
    return true;
}

int ViewportPanel::addRuntimeObjectComponent(
    int id, const QString &type, const QJsonObject &properties) {
    if (runtimeContext == nullptr || playbackState != 0 || type.isEmpty()) {
        return -1;
    }
    QJsonObject definition = properties;
    definition.insert("type", type);
    if (type.toLower().remove('_').remove('-') == "rigidbody") {
        QJsonObject collider = definition.value("collider").toObject();
        collider.insert("inheritObjectSize", true);
        definition.insert("collider", collider);
    }
    const QByteArray payload =
        QJsonDocument(definition).toJson(QJsonDocument::Compact);
    try {
        const json parsed = json::parse(payload.constData());
        const int index = runtimeContext->addObjectComponent(id, parsed);
        if (index >= 0) {
            runtimeContext->saveCurrentScene();
            refreshSceneSnapshot();
            setSceneDirty(true);
        }
        return index;
    } catch (const json::exception &) {
        return -1;
    }
}

bool ViewportPanel::removeRuntimeObjectComponent(int id, int componentIndex) {
    if (runtimeContext == nullptr || playbackState != 0 ||
        !runtimeContext->removeObjectComponent(id, componentIndex) ||
        !runtimeContext->saveCurrentScene()) {
        return false;
    }
    refreshSceneSnapshot();
    setSceneDirty(true);
    const QJsonDocument document =
        QJsonDocument::fromJson(lastSceneSnapshot.toUtf8());
    selectionToRestore =
        findSnapshotObject(document.object().value("objects").toArray(), id)
            .value("name")
            .toString();
    QTimer::singleShot(0, this, &ViewportPanel::reloadRuntime);
    return true;
}

bool ViewportPanel::controlRuntimeAudio(int id, int componentIndex,
                                        const QString &action) {
    return runtimeContext != nullptr &&
           runtimeContext->controlObjectAudio(id, componentIndex,
                                              action.toStdString());
}

bool ViewportPanel::setRuntimeObjectParent(int childId, int parentId) {
    if (runtimeContext == nullptr || playbackState != 0 ||
        !runtimeContext->setObjectParent(childId, parentId)) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    setSceneDirty(true);
    return true;
}

bool ViewportPanel::deleteRuntimeObject(int id) {
    if (runtimeContext == nullptr || playbackState != 0 ||
        !runtimeContext->deleteObject(id)) {
        return false;
    }
    if (undoStack != nullptr) {
        undoStack->clear();
    }
    if (!runtimeContext->saveCurrentScene()) {
        qWarning() << "Atlas editor could not persist the deleted object";
    }
    refreshSceneSnapshot();
    setSceneDirty(true);
    emit runtimeObjectActivated(-1);
    return true;
}

int ViewportPanel::createRuntimeObject(const QString &type,
                                       const QString &name) {
    if (runtimeContext == nullptr || playbackState != 0) {
        return -1;
    }
    const int id = runtimeContext->createObject(type.toUtf8().toStdString(),
                                                name.toUtf8().toStdString());
    if (id >= 0) {
        runtimeContext->saveCurrentScene();
        refreshSceneSnapshot();
        setSceneDirty(true);
    }
    return id;
}

bool ViewportPanel::copySelectedRuntimeObject() {
    if (runtimeContext == nullptr || selectedRuntimeObjectId() < 0)
        return false;
    const std::string definition =
        runtimeContext->objectDefinitionJson(selectedRuntimeObjectId());
    objectClipboard = QByteArray::fromStdString(definition);
    return !objectClipboard.isEmpty();
}

bool ViewportPanel::cutSelectedRuntimeObject() {
    const int id = selectedRuntimeObjectId();
    return id >= 0 && copySelectedRuntimeObject() && deleteRuntimeObject(id);
}

bool ViewportPanel::pasteRuntimeObject() {
    if (runtimeContext == nullptr || playbackState != 0 ||
        objectClipboard.isEmpty()) {
        return false;
    }
    const int id = runtimeContext->pasteObjectDefinition(
        objectClipboard.toStdString());
    if (id < 0)
        return false;
    if (undoStack != nullptr)
        undoStack->clear();
    refreshSceneSnapshot();
    setSceneDirty(true);
    emit runtimeObjectActivated(id);
    return true;
}

bool ViewportPanel::duplicateSelectedRuntimeObject() {
    return copySelectedRuntimeObject() && pasteRuntimeObject();
}

bool ViewportPanel::resetSelectedTransform(int mode) {
    const int id = selectedRuntimeObjectId();
    if (id < 0)
        return false;
    if (mode == 1)
        return setRuntimeObjectProperty(id, "transform", -1, "/position",
                                        QJsonArray{0.0, 0.0, 0.0});
    if (mode == 2)
        return setRuntimeObjectProperty(id, "transform", -1, "/rotation",
                                        QJsonArray{0.0, 0.0, 0.0});
    if (mode == 3)
        return setRuntimeObjectProperty(id, "transform", -1, "/scale",
                                        QJsonArray{1.0, 1.0, 1.0});
    return false;
}

bool ViewportPanel::saveRuntimeScene() {
    if (playbackState != 0)
        return false;
    const bool saved =
        runtimeContext != nullptr && runtimeContext->saveCurrentScene();
    if (saved)
        setSceneDirty(false);
    return saved;
}

bool ViewportPanel::openRuntimeScene(const QString &path) {
    if (runtimeContext == nullptr || playbackState != 0 || path.isEmpty() ||
        !runtimeContext->openSceneFile(path.toStdString())) {
        return false;
    }
    if (undoStack != nullptr)
        undoStack->clear();
    selectionToRestore.clear();
    refreshSceneSnapshot();
    setSceneDirty(false);
    emit sceneOpened(path);
    return true;
}

bool ViewportPanel::saveRuntimeSceneAs(const QString &path) {
    if (runtimeContext == nullptr || playbackState != 0 || path.isEmpty() ||
        !saveRuntimeScene()) {
        return false;
    }
    const QString source = currentRuntimeScene();
    if (source.isEmpty())
        return false;
    if (QFileInfo(source).absoluteFilePath() != QFileInfo(path).absoluteFilePath()) {
        if (QFile::exists(path) && !QFile::remove(path))
            return false;
        if (!QFile::copy(source, path))
            return false;
    }
    return openRuntimeScene(path);
}

QString ViewportPanel::currentRuntimeScene() const {
    return runtimeContext != nullptr
               ? QString::fromStdString(runtimeContext->currentScenePath())
               : QString();
}

int ViewportPanel::selectedRuntimeObjectId() const {
    return runtimeContext != nullptr ? runtimeContext->selectedObjectId() : -1;
}

bool ViewportPanel::applyRuntimeMaterial(int id, const QString &path) {
    return applyRuntimeMaterialDirect(id, path);
}

bool ViewportPanel::applyRuntimeMaterialDirect(int id, const QString &path) {
    if (runtimeContext == nullptr || playbackState != 0 || id < 0 ||
        path.isEmpty() ||
        !runtimeContext->setObjectMaterial(id, path.toStdString())) {
        return false;
    }
    runtimeContext->saveCurrentScene();
    refreshSceneSnapshot();
    setSceneDirty(true);
    reloadRuntime();
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

bool ViewportPanel::importRuntimeModel(const QString &path) {
    if (runtimeContext == nullptr || playbackState != 0 || path.isEmpty())
        return false;
    const QJsonObject definition{
        {"type", "model"},
        {"name", QFileInfo(path).completeBaseName()},
        {"source", QFileInfo(path).absoluteFilePath()},
        {"position", QJsonArray{0.0, 0.0, 0.0}},
        {"rotation", QJsonArray{0.0, 0.0, 0.0}},
        {"scale", QJsonArray{1.0, 1.0, 1.0}},
        {"components", QJsonArray{}}};
    const int id = runtimeContext->pasteObjectDefinition(
        QJsonDocument(definition).toJson(QJsonDocument::Compact).toStdString());
    if (id < 0)
        return false;
    refreshSceneSnapshot();
    setSceneDirty(true);
    emit runtimeObjectActivated(id);
    return true;
}

void ViewportPanel::undo() {
    if (undoStack != nullptr && playbackState == 0) {
        undoStack->undo();
        const int selected = selectedRuntimeObjectId();
        if (selected >= 0)
            emit runtimeObjectActivated(selected);
    }
}

void ViewportPanel::redo() {
    if (undoStack != nullptr && playbackState == 0) {
        undoStack->redo();
        const int selected = selectedRuntimeObjectId();
        if (selected >= 0)
            emit runtimeObjectActivated(selected);
    }
}

void ViewportPanel::setSceneDirty(bool dirty) {
    if (sceneDirty == dirty)
        return;
    sceneDirty = dirty;
    emit sceneDirtyChanged(sceneDirty);
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
    finishKeyboardTransform(false);
    if (playbackState == 0 && !saveRuntimeScene()) {
        qWarning() << "Atlas editor could not checkpoint the scene for play";
        return;
    }
    QString error;
    if (!ToolchainInstaller::run(
            {"script", "compile"}, QFileInfo(projectFile).absolutePath(),
            &error)) {
        QMessageBox::warning(
            this, "Script Compilation Failed",
            error.isEmpty() ? "Atlas could not compile the project scripts."
                            : error);
        return;
    }
    playAfterRuntimeStart = true;
    reloadRuntime();
}

void ViewportPanel::toggleRuntimePlayback() {
    if (playbackState == 1)
        pauseRuntime();
    else
        playRuntime();
}

void ViewportPanel::pauseRuntime() {
    if (runtimeContext == nullptr || playbackState == 0) {
        return;
    }
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
    if (selectionToRestore.isEmpty() && runtimeContext != nullptr) {
        const int selected = selectedRuntimeObjectId();
        if (selected >= 0) {
            const QJsonDocument document =
                QJsonDocument::fromJson(lastSceneSnapshot.toUtf8());
            selectionToRestore = findSnapshotObject(
                                     document.object()
                                         .value("objects")
                                         .toArray(),
                                     selected)
                                     .value("name")
                                     .toString();
        }
    }
    finishKeyboardTransform(false);
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
    finishKeyboardTransform(false);
    runtimeContext->setEditorControlMode(mode);
}

void ViewportPanel::toggleTransformSpace() {
    if (runtimeContext != nullptr)
        emit transformSpaceChanged(runtimeContext->toggleEditorTransformSpace());
}

void ViewportPanel::toggleTransformSnapping() {
    if (runtimeContext == nullptr)
        return;
    const bool enabled = runtimeContext->toggleEditorTransformSnapping();
    const float increment =
        runtimeContext->changeEditorTransformSnapIncrement(1.0f);
    emit transformSnappingChanged(enabled, increment);
}

void ViewportPanel::changeTransformSnapIncrement(float factor) {
    if (runtimeContext == nullptr)
        return;
    const float increment =
        runtimeContext->changeEditorTransformSnapIncrement(factor);
    emit transformSnappingChanged(true, increment);
}

void ViewportPanel::beginKeyboardTransform(int mode) {
    if (runtimeContext == nullptr || selectedRuntimeObjectId() < 0)
        return;
    transformUndoBefore =
        findSnapshotObject(
            QJsonDocument::fromJson(lastSceneSnapshot.toUtf8())
                .object()
                .value("objects")
                .toArray(),
            selectedRuntimeObjectId());
    const QPoint pointer = mapFromGlobal(QCursor::pos());
    if (!runtimeContext->beginEditorKeyboardTransform(
            mode, static_cast<float>(pointer.x()),
            static_cast<float>(height() - pointer.y()), widgetScale(this))) {
        return;
    }
    keyboardTransformActive = true;
    keyboardTransformMode = mode;
    keyboardTransformAxes = 7;
    grabMouse();
    const QString operation =
        mode == 1 ? "Move" : mode == 2 ? "Rotate" : "Scale";
    emit transformHintChanged(
        QStringLiteral("%1 · All axes · X/Y/Z constrain · Shift+Axis exclude "
                       "· Enter/LMB confirm · Esc/RMB cancel")
            .arg(operation));
}

void ViewportPanel::updateKeyboardTransformAxes(int key, bool exclude) {
    if (!keyboardTransformActive || runtimeContext == nullptr)
        return;
    const int bit = key == Qt::Key_X ? 1 : key == Qt::Key_Y ? 2 : 4;
    if (exclude) {
        keyboardTransformAxes = 7 & ~bit;
    } else if (keyboardTransformAxes == 7) {
        keyboardTransformAxes = bit;
    } else {
        keyboardTransformAxes |= bit;
    }
    runtimeContext->setEditorKeyboardTransformAxes(keyboardTransformAxes);
    QString axes;
    if ((keyboardTransformAxes & 1) != 0)
        axes += 'X';
    if ((keyboardTransformAxes & 2) != 0)
        axes += 'Y';
    if ((keyboardTransformAxes & 4) != 0)
        axes += 'Z';
    const QString operation = keyboardTransformMode == 1   ? "Move"
                              : keyboardTransformMode == 2 ? "Rotate"
                                                           : "Scale";
    emit transformHintChanged(
        QStringLiteral("%1 · %2 locked · X/Y/Z add axes · Shift+Axis exclude "
                       "· Enter/LMB confirm · Esc/RMB cancel")
            .arg(operation, axes));
}

void ViewportPanel::finishKeyboardTransform(bool commit) {
    if (!keyboardTransformActive)
        return;
    if (runtimeContext != nullptr) {
        const int selected = selectedRuntimeObjectId();
        runtimeContext->finishEditorKeyboardTransform(commit);
        if (commit) {
            runtimeContext->saveCurrentScene();
            setSceneDirty(true);
        }
        refreshSceneSnapshot();
        if (commit)
            pushTransformUndo(selected, transformUndoBefore);
        if (selected >= 0)
            emit runtimeObjectActivated(selected);
    }
    keyboardTransformActive = false;
    releaseMouse();
    keyboardTransformMode = 0;
    keyboardTransformAxes = 7;
    transformUndoBefore = {};
    emit transformHintChanged(
        "Tab Frame · Right-Drag Pan · Middle-Drag Orbit · G Move · R Rotate · S Scale · X Delete");
}

void ViewportPanel::pushTransformUndo(int objectId,
                                      const QJsonObject &before) {
    if (undoStack == nullptr || objectId < 0 || before.isEmpty())
        return;
    const QJsonObject after = findSnapshotObject(
        QJsonDocument::fromJson(lastSceneSnapshot.toUtf8())
            .object()
            .value("objects")
            .toArray(),
        objectId);
    if (after.isEmpty())
        return;
    auto *command = new QUndoCommand("Transform Object");
    const QList<QPair<QString, QString>> properties{
        {"position", "/position"},
        {"rotation", "/rotation"},
        {"scale", "/scale"}};
    for (const auto &[key, path] : properties) {
        if (before.value(key) != after.value(key)) {
            new RuntimePropertyCommand(this, objectId, "transform", -1, path,
                                       before.value(key), after.value(key),
                                       command);
        }
    }
    if (command->childCount() == 0) {
        delete command;
        return;
    }
    undoStack->push(command);
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
