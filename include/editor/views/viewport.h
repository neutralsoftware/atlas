/*
 * viewport.h
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Viewport declaration
 * Copyright (c) 2026 Max Van den Eynde
 */

#ifndef ATLAS_VIEWPORT_H
#define ATLAS_VIEWPORT_H

#include <memory>

#include <QByteArray>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

class Context;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QHideEvent;
class QKeyEvent;
class QJsonValue;
class QMouseEvent;
class QPaintEngine;
class QResizeEvent;
class QSize;
class QShowEvent;
class QTimer;
class QUndoStack;
class QWheelEvent;

class ViewportPanel : public QWidget {
    Q_OBJECT

  public:
    explicit ViewportPanel(const QString &projectFile,
                           QWidget *parent = nullptr);
    ~ViewportPanel() override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void setRuntimeStartupEnabled(bool enabled);
    void shutdownRuntime();
    bool selectRuntimeObject(int id, bool focusCamera = true);
    bool focusRuntimeObjects(const QList<int> &ids);
    bool renameRuntimeObject(int id, const QString &name);
    bool renameRuntimeObjectDirect(int id, const QString &name);
    bool setRuntimeObjectProperty(int id, const QString &component,
                                  int componentIndex,
                                  const QString &propertyPath,
                                  const QJsonValue &value);
    bool setRuntimeSceneProperty(const QString &section, int index,
                                 const QString &propertyPath,
                                 const QJsonValue &value);
    bool setRuntimePropertySync(const QJsonObject &target,
                                const QJsonObject &source);
    bool clearRuntimePropertySync(const QJsonObject &target);
    bool applyRuntimeObjectProperty(int id, const QString &component,
                                    int componentIndex,
                                    const QString &propertyPath,
                                    const QJsonValue &value);
    int addRuntimeObjectComponent(int id, const QString &type,
                                  const QJsonObject &properties);
    bool removeRuntimeObjectComponent(int id, int componentIndex);
    bool controlRuntimeAudio(int id, int componentIndex, const QString &action);
    bool setRuntimeObjectParent(int childId, int parentId);
    bool deleteRuntimeObject(int id);
    int createRuntimeObject(const QString &type, const QString &name = {});
    bool duplicateSelectedRuntimeObject();
    bool copySelectedRuntimeObject();
    bool cutSelectedRuntimeObject();
    bool pasteRuntimeObject();
    bool resetSelectedTransform(int mode);
    bool saveRuntimeScene();
    bool openRuntimeScene(const QString &path);
    bool saveRuntimeSceneAs(const QString &path);
    QString currentRuntimeScene() const;
    QString runtimeProjectFile() const { return projectFile; }
    QString currentSceneSnapshot() const { return lastSceneSnapshot; }
    int selectedRuntimeObjectId() const;
    bool applyRuntimeMaterial(int id, const QString &path);
    bool applyRuntimeMaterialDirect(int id, const QString &path);
    bool attachRuntimeAsset(int id, const QString &path);
    bool importRuntimeModel(const QString &path);
    void undo();
    void redo();
    void playRuntime();
    void toggleRuntimePlayback();
    void pauseRuntime();
    void stepRuntimeOnce();
    void stopRuntimePlayback();
    void reloadRuntime();
    void setRuntimeShadingMode(int mode);
    void setPathTracingPreview(bool enabled);
    bool applyPathTracingSettings(int samplesPerPixel, int bounceLimit,
                                  bool denoising, int accumulationFrames,
                                  bool upscaling, float internalScale);
    void setRuntimeControlMode(int mode);
    void toggleTransformSpace();
    void toggleTransformSnapping();
    void changeTransformSnapIncrement(float factor);
    bool setCameraFocused(bool focused);
    void toggleCameraFocus();
    bool isCameraFocused() const;

  signals:
    void sceneSnapshotChanged(const QString &snapshot);
    void runtimeAvailabilityChanged(bool available);
    void runtimeObjectActivated(int id);
    void playbackStateChanged(int state);
    void frameRateChanged(float framesPerSecond);
    void sceneDirtyChanged(bool dirty);
    void runtimeStartupFinished(bool success, const QString &message);
    void runtimeLoadingStarted();
    void runtimeLoadingStatusChanged(const QString &status);
    void runtimeLoadingFinished();
    void runtimeErrorOccurred(const QString &message);
    void transformHintChanged(const QString &hint);
    void sceneOpened(const QString &path);
    void transformSpaceChanged(bool local);
    void transformSnappingChanged(bool enabled, float increment);
    void cameraFocusChanged(bool focused);

  protected:
    QPaintEngine *paintEngine() const override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

  private:
    void scheduleRuntimeStart();
    void startRuntime();
    void stopRuntime();
    bool stepRuntime();
    void resizeRuntime();
    void sendPointerEvent(int action, float x, float y, int button);
    void refreshSceneSnapshot();
    void setSceneDirty(bool dirty);
    void beginKeyboardTransform(int mode);
    void updateKeyboardTransformAxes(int key, bool exclude);
    void finishKeyboardTransform(bool commit);
    void pushTransformUndo(int objectId, const QJsonObject &before);
    QJsonValue runtimeObjectProperty(int id, const QString &component,
                                     int componentIndex,
                                     const QString &propertyPath) const;

    QTimer *frameTimer = nullptr;
    QTimer *resizeTimer = nullptr;
    QUndoStack *undoStack = nullptr;
    QString projectFile;
    std::shared_ptr<Context> runtimeContext;
    int runtimeWidth = 0;
    int runtimeHeight = 0;
    float runtimeScale = 0.0f;
    QString lastSceneSnapshot;
    QString selectionToRestore;
    QByteArray objectClipboard;
    QJsonObject transformUndoBefore;
    QElapsedTimer snapshotTimer;
    QElapsedTimer frameRateTimer;
    bool runtimeStartQueued = false;
    bool runtimeStartupEnabled = false;
    bool shuttingDown = false;
    bool sceneDirty = false;
    bool playAfterRuntimeStart = false;
    bool leftPointerMoved = false;
    bool keyboardTransformActive = false;
    int keyboardTransformMode = 0;
    int keyboardTransformAxes = 7;
    int playbackState = 0;
    int shadingMode = 0;
    bool pbrPreview = true;
    int rightDragRuntimeButton = 0;
    int middleDragRuntimeButton = 0;
};

#endif // ATLAS_VIEWPORT_H
