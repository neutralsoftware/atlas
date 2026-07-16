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
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEngine>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QShowEvent>
#include <QString>
#include <QTimer>
#include <QWheelEvent>
#include <Qt>

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>

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
    case Qt::Key_W:
    case Qt::Key_Up:
        return RuntimeEditorCameraKeyForward;
    case Qt::Key_S:
    case Qt::Key_Down:
        return RuntimeEditorCameraKeyBackward;
    case Qt::Key_A:
    case Qt::Key_Left:
        return RuntimeEditorCameraKeyLeft;
    case Qt::Key_D:
    case Qt::Key_Right:
        return RuntimeEditorCameraKeyRight;
    case Qt::Key_E:
    case Qt::Key_PageUp:
    case Qt::Key_Space:
        return RuntimeEditorCameraKeyUp;
    case Qt::Key_Q:
    case Qt::Key_PageDown:
    case Qt::Key_C:
        return RuntimeEditorCameraKeyDown;
    default:
        return -1;
    }
}

float widgetScale(QWidget *widget) {
    const qreal scale = widget != nullptr ? widget->devicePixelRatioF() : 1.0;
    return scale > 0.0 ? static_cast<float>(scale) : 1.0f;
}
} // namespace

ViewportPanel::ViewportPanel(const QString &projectFile, QWidget *parent)
    : QWidget(parent), projectFile(projectFile) {
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
    frameTimer->setTimerType(Qt::PreciseTimer);
    connect(frameTimer, &QTimer::timeout, this, [this] { stepRuntime(); });
    if (auto *app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this,
                [this] { shutdownRuntime(); });
    }

    winId();
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
    if (frameTimer != nullptr) {
        frameTimer->stop();
    }
    QWidget::hideEvent(event);
}

void ViewportPanel::closeEvent(QCloseEvent *event) {
    shutdownRuntime();
    QWidget::closeEvent(event);
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
        !isVisible() || width() <= 1 || height() <= 1) {
        return;
    }
    runtimeStartQueued = true;
    QTimer::singleShot(0, this, [this] {
        runtimeStartQueued = false;
        if (shuttingDown) {
            return;
        }
        if (runtimeContext == nullptr && isVisible() && width() > 1 &&
            height() > 1) {
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
        resizeRuntime();
        refreshSceneSnapshot();
        emit runtimeAvailabilityChanged(true);
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
    emit runtimeAvailabilityChanged(false);
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
    setFocus(Qt::OtherFocusReason);
    return true;
}

bool ViewportPanel::renameRuntimeObject(int id, const QString &name) {
    if (runtimeContext == nullptr ||
        !runtimeContext->renameObject(id, name.toUtf8().toStdString())) {
        return false;
    }
    refreshSceneSnapshot();
    return true;
}

bool ViewportPanel::setRuntimeObjectParent(int childId, int parentId) {
    if (runtimeContext == nullptr ||
        !runtimeContext->setObjectParent(childId, parentId)) {
        return false;
    }
    refreshSceneSnapshot();
    return true;
}

bool ViewportPanel::deleteRuntimeObject(int id) {
    if (runtimeContext == nullptr || !runtimeContext->deleteObject(id)) {
        return false;
    }
    refreshSceneSnapshot();
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
        refreshSceneSnapshot();
    }
    return id;
}

bool ViewportPanel::saveRuntimeScene() {
    return runtimeContext != nullptr && runtimeContext->saveCurrentScene();
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
