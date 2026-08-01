#include <editor/views/viewportTools.h>

#include <editor/views/viewport.h>
#include <editor/styling/icons.h>

#include <QActionGroup>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequence>
#include <QList>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

ViewportTools::ViewportTools(ViewportPanel *viewport,
                             const QString &projectFile, QWidget *parent)
    : QWidget(parent), viewport(viewport),
      projectRoot(QFileInfo(projectFile).absolutePath()) {
    setObjectName("viewportTools");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    sceneTabs = new QTabBar(this);
    sceneTabs->setObjectName("sceneTabs");
    sceneTabs->setDocumentMode(true);
    sceneTabs->setExpanding(false);
    sceneTabs->setMovable(true);
    sceneTabs->setTabsClosable(true);
    sceneTabs->setVisible(false);
    layout->addWidget(sceneTabs);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("viewportToolbar");
    auto *tools = new QHBoxLayout(toolbar);
    tools->setContentsMargins(5, 3, 5, 3);
    tools->setSpacing(2);

    playButton = new QToolButton(toolbar);
    playButton->setObjectName("viewportPlaybackButton");
    playButton->setIcon(styling::icon(styling::Icon::Play, "#849589"));
    playButton->setToolTip("Play");
    pauseButton = new QToolButton(toolbar);
    pauseButton->setObjectName("viewportPlaybackButton");
    pauseButton->setIcon(styling::icon(styling::Icon::Pause, "#A1957D"));
    pauseButton->setToolTip("Pause");
    stepButton = new QToolButton(toolbar);
    stepButton->setObjectName("viewportPlaybackButton");
    stepButton->setIcon(styling::icon(styling::Icon::SkipForward, "#7E929C"));
    stepButton->setToolTip("Step one frame");
    stopButton = new QToolButton(toolbar);
    stopButton->setObjectName("viewportPlaybackButton");
    stopButton->setIcon(styling::icon(styling::Icon::Stop, "#A17F7F"));
    stopButton->setToolTip("Stop and restore the scene");
    reloadButton = new QToolButton(toolbar);
    reloadButton->setObjectName("viewportPlaybackButton");
    reloadButton->setIcon(
        styling::icon(styling::Icon::ArrowCounterClockwise, "#71889A"));
    reloadButton->setToolTip("Reload runtime");
    playButton->setShortcut(QKeySequence("Ctrl+P"));
    pauseButton->setShortcut(QKeySequence("Ctrl+Shift+P"));
    stepButton->setShortcut(QKeySequence("Ctrl+Alt+P"));

    auto *transformGroup = new QActionGroup(toolbar);
    transformGroup->setExclusive(true);
    const QStringList transformNames{"Select", "Move", "Rotate", "Scale"};
    const QList<styling::Icon> transformIcons{
        styling::Icon::CursorClick, styling::Icon::ArrowsOutCardinal,
        styling::Icon::ArrowClockwise, styling::Icon::BoundingBox};
    const QList<QColor> transformColors{QColor("#7E929C"), QColor("#849589"),
                                        QColor("#A1957D"), QColor("#71889A")};
    for (int index = 0; index < transformNames.size(); ++index) {
        auto *button = new QToolButton(toolbar);
        button->setObjectName("viewportModeButton");
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setCheckable(true);
        auto *action = new QAction(transformNames.at(index), button);
        action->setIcon(
            styling::icon(transformIcons.at(index), transformColors.at(index)));
        action->setToolTip(transformNames.at(index) + " tool");
        action->setCheckable(true);
        action->setData(index);
        button->setDefaultAction(action);
        transformGroup->addAction(action);
        tools->addWidget(button);
        if (index == 0) {
            action->setChecked(true);
        }
    }

    tools->addSpacing(10);
    spaceButton = new QToolButton(toolbar);
    spaceButton->setObjectName("viewportOptionButton");
    spaceButton->setIcon(styling::icon(styling::Icon::Globe, "#7E929C"));
    spaceButton->setToolTip("World transform space · Shift+T");
    tools->addWidget(spaceButton);

    cameraButton = new QToolButton(toolbar);
    cameraButton->setObjectName("viewportOptionButton");
    cameraButton->setIcon(styling::icon(styling::Icon::Camera, "#9E897D"));
    cameraButton->setCheckable(true);
    cameraButton->setToolTip("Look through Main Camera · Numpad 0");
    tools->addWidget(cameraButton);

    tools->addStretch();

    cameraLabel = new QLabel("MAIN CAMERA", toolbar);
    cameraLabel->setObjectName("viewportFpsLabel");
    cameraLabel->setVisible(false);
    tools->addWidget(cameraLabel);
    tools->addWidget(playButton);
    tools->addWidget(pauseButton);
    tools->addWidget(stepButton);
    tools->addWidget(stopButton);
    tools->addWidget(reloadButton);
    tools->addStretch();

    QFile manifest(projectFile);
    bool pathTracingProject = false;
    if (manifest.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString contents = QString::fromUtf8(manifest.readAll());
        pathTracingProject = contents.contains(QRegularExpression(
            QStringLiteral(R"(default\s*=\s*["']path[\s_-]*tracing["'])"),
            QRegularExpression::CaseInsensitiveOption));
    }

    auto *shadingGroup = new QActionGroup(toolbar);
    shadingGroup->setExclusive(true);
    const QStringList shadingNames =
        pathTracingProject ? QStringList{"PBR Preview", "Path Traced"}
                           : QStringList{"Lit", "Wireframe", "Points"};
    const QList<styling::Icon> shadingIcons =
        pathTracingProject
            ? QList<styling::Icon>{styling::Icon::Sphere,
                                   styling::Icon::Aperture}
            : QList<styling::Icon>{styling::Icon::Sphere,
                                   styling::Icon::CubeTransparent,
                                   styling::Icon::DotsNine};
    for (int index = 0; index < shadingNames.size(); ++index) {
        auto *button = new QToolButton(toolbar);
        button->setObjectName("viewportShadingButton");
        button->setToolButtonStyle(pathTracingProject
                                       ? Qt::ToolButtonTextBesideIcon
                                       : Qt::ToolButtonIconOnly);
        button->setCheckable(true);
        auto *action = new QAction(shadingNames.at(index), button);
        action->setIcon(styling::icon(shadingIcons.at(index), "#9AA6B8"));
        action->setToolTip(pathTracingProject
                               ? shadingNames.at(index)
                               : shadingNames.at(index) + " shading");
        action->setCheckable(true);
        action->setData(index);
        button->setDefaultAction(action);
        shadingGroup->addAction(action);
        tools->addWidget(button);
        if (index == 0) {
            action->setChecked(true);
        }
    }

    auto *fpsButton = new QToolButton(toolbar);
    fpsButton->setObjectName("viewportOptionButton");
    fpsButton->setIcon(styling::icon(styling::Icon::Monitor, "#849589"));
    fpsButton->setCheckable(true);
    fpsButton->setChecked(true);
    fpsButton->setToolTip("Toggle frame rate");
    fpsLabel = new QLabel("-- FPS", toolbar);
    fpsLabel->setObjectName("viewportFpsLabel");
    fpsLabel->setMinimumWidth(62);
    tools->addWidget(fpsButton);
    tools->addWidget(fpsLabel);

    layout->addWidget(toolbar);
    layout->addWidget(viewport, 1);
    shortcutHint =
        new QLabel("Tab Frame · Num 0 Camera · Shift+Middle/Right Pan · "
                   "Middle/Right Orbit · G Move · R Rotate · S Scale · X "
                   "Delete",
                   this);
    shortcutHint->setObjectName("viewportShortcutHint");
    shortcutHint->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(shortcutHint);

    connect(playButton, &QToolButton::clicked, viewport,
            &ViewportPanel::playRuntime);
    connect(pauseButton, &QToolButton::clicked, viewport,
            &ViewportPanel::pauseRuntime);
    connect(stepButton, &QToolButton::clicked, viewport,
            &ViewportPanel::stepRuntimeOnce);
    connect(stopButton, &QToolButton::clicked, viewport,
            &ViewportPanel::stopRuntimePlayback);
    connect(reloadButton, &QToolButton::clicked, viewport,
            &ViewportPanel::reloadRuntime);
    connect(transformGroup, &QActionGroup::triggered, this,
            [viewport](QAction *action) {
                viewport->setRuntimeControlMode(action->data().toInt());
            });
    connect(shadingGroup, &QActionGroup::triggered, this,
            [viewport, pathTracingProject](QAction *action) {
                if (pathTracingProject) {
                    viewport->setPathTracingPreview(action->data().toInt() ==
                                                    0);
                } else {
                    viewport->setRuntimeShadingMode(action->data().toInt());
                }
            });
    connect(spaceButton, &QToolButton::clicked, viewport,
            &ViewportPanel::toggleTransformSpace);
    connect(cameraButton, &QToolButton::clicked, viewport,
            &ViewportPanel::toggleCameraFocus);
    connect(viewport, &ViewportPanel::cameraFocusChanged, this,
            [this](bool focused) {
                const QSignalBlocker blocker(cameraButton);
                cameraButton->setChecked(focused);
                cameraButton->setIcon(styling::icon(
                    styling::Icon::Camera,
                    focused ? QColor("#5CC8FF") : QColor("#9E897D")));
                cameraButton->setToolTip(
                    focused
                        ? "Main Camera view active · Esc or Numpad 0 to exit"
                        : "Look through Main Camera · Numpad 0");
                cameraLabel->setVisible(focused);
            });
    connect(viewport, &ViewportPanel::transformSpaceChanged, this,
            [this](bool local) {
                spaceButton->setIcon(styling::icon(
                    local ? styling::Icon::Cube : styling::Icon::Globe,
                    local ? QColor("#71889A") : QColor("#7E929C")));
                spaceButton->setToolTip(
                    local ? "Local transform space · Shift+T"
                          : "World transform space · Shift+T");
            });
    connect(fpsButton, &QToolButton::toggled, fpsLabel, &QWidget::setVisible);
    connect(viewport, &ViewportPanel::frameRateChanged, this,
            [this](float fps) {
                fpsLabel->setText(QStringLiteral("%1 FPS").arg(fps, 0, 'f', 0));
            });
    connect(viewport, &ViewportPanel::playbackStateChanged, this,
            &ViewportTools::updatePlaybackState);
    connect(viewport, &ViewportPanel::transformHintChanged, shortcutHint,
            &QLabel::setText);
    connect(viewport, &ViewportPanel::runtimeAvailabilityChanged, this,
            [this](bool available) {
                runtimeAvailable = available;
                updatePlaybackState(playbackState);
            });
    connect(sceneTabs, &QTabBar::currentChanged, this, [this](int index) {
        if (index >= 0 && index < scenePaths.size() &&
            this->viewport != nullptr)
            this->viewport->openRuntimeScene(scenePaths.at(index));
    });
    connect(sceneTabs, &QTabBar::tabCloseRequested, this,
            [this](int index) { closeSceneTab(index); });
    connect(viewport, &ViewportPanel::sceneOpened, this,
            &ViewportTools::openSceneTab);
    refreshSceneTabs();
    updatePlaybackState(0);
}

void ViewportTools::refreshSceneTabs() {
    const QString current =
        viewport != nullptr ? viewport->currentRuntimeScene() : QString();
    if (!current.trimmed().isEmpty())
        openSceneTab(current);
    updateSceneTabs();
}

void ViewportTools::openSceneTab(const QString &path) {
    const QString absolute = QFileInfo(path).absoluteFilePath();
    if (path.trimmed().isEmpty() || !QFileInfo(absolute).isFile())
        return;
    int index = scenePaths.indexOf(absolute);
    if (index < 0) {
        scenePaths.append(absolute);
        index = sceneTabs->addTab(QFileInfo(absolute).completeBaseName());
        sceneTabs->setTabToolTip(index, absolute);
    }
    const QSignalBlocker blocker(sceneTabs);
    sceneTabs->setCurrentIndex(index);
    updateSceneTabs();
}

void ViewportTools::closeCurrentSceneTab() {
    closeSceneTab(sceneTabs->currentIndex());
}

void ViewportTools::closeSceneTab(int index) {
    if (index < 0 || index >= scenePaths.size())
        return;
    const QSignalBlocker blocker(sceneTabs);
    scenePaths.removeAt(index);
    sceneTabs->removeTab(index);
    if (!scenePaths.isEmpty()) {
        const int next = qMin(index, scenePaths.size() - 1);
        sceneTabs->setCurrentIndex(next);
        if (viewport != nullptr)
            viewport->openRuntimeScene(scenePaths.at(next));
    }
    updateSceneTabs();
}

void ViewportTools::updateSceneTabs() {
    sceneTabs->setVisible(!scenePaths.isEmpty());
    sceneTabs->setTabsClosable(!scenePaths.isEmpty());
}

void ViewportTools::updatePlaybackState(int state) {
    playbackState = state;
    playButton->setEnabled(runtimeAvailable && state != 1);
    pauseButton->setEnabled(runtimeAvailable && state == 1);
    stepButton->setEnabled(runtimeAvailable && state != 0);
    stopButton->setEnabled(runtimeAvailable && state != 0);
    reloadButton->setEnabled(runtimeAvailable);
}
