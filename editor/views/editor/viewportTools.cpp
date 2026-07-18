#include <editor/views/viewportTools.h>

#include <editor/views/viewport.h>
#include <editor/styling/icons.h>

#include <QActionGroup>
#include <QComboBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequence>
#include <QList>
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
    playButton->setIcon(styling::icon(styling::Icon::Play, "#52D273"));
    playButton->setToolTip("Play");
    pauseButton = new QToolButton(toolbar);
    pauseButton->setObjectName("viewportPlaybackButton");
    pauseButton->setIcon(styling::icon(styling::Icon::Pause, "#F5B942"));
    pauseButton->setToolTip("Pause");
    stepButton = new QToolButton(toolbar);
    stepButton->setObjectName("viewportPlaybackButton");
    stepButton->setIcon(
        styling::icon(styling::Icon::SkipForward, "#55C2FF"));
    stepButton->setToolTip("Step one frame");
    stopButton = new QToolButton(toolbar);
    stopButton->setObjectName("viewportPlaybackButton");
    stopButton->setIcon(styling::icon(styling::Icon::Stop, "#FF6B7A"));
    stopButton->setToolTip("Stop and restore the scene");
    reloadButton = new QToolButton(toolbar);
    reloadButton->setObjectName("viewportPlaybackButton");
    reloadButton->setIcon(
        styling::icon(styling::Icon::ArrowCounterClockwise, "#A78BFA"));
    reloadButton->setToolTip("Reload runtime");
    playButton->setShortcut(QKeySequence("Ctrl+P"));
    pauseButton->setShortcut(QKeySequence("Ctrl+Shift+P"));
    stepButton->setShortcut(QKeySequence("Ctrl+Alt+P"));

    tools->addStretch();
    tools->addWidget(playButton);
    tools->addWidget(pauseButton);
    tools->addWidget(stepButton);
    tools->addWidget(stopButton);
    tools->addWidget(reloadButton);
    tools->addSpacing(10);

    auto *transformGroup = new QActionGroup(toolbar);
    transformGroup->setExclusive(true);
    const QStringList transformNames{"Select", "Move", "Rotate", "Scale"};
    const QList<styling::Icon> transformIcons{
        styling::Icon::CursorClick, styling::Icon::ArrowsOutCardinal,
        styling::Icon::ArrowClockwise, styling::Icon::BoundingBox};
    const QList<QColor> transformColors{
        QColor("#55C2FF"), QColor("#52D273"), QColor("#F5B942"),
        QColor("#F472B6")};
    for (int index = 0; index < transformNames.size(); ++index) {
        auto *button = new QToolButton(toolbar);
        button->setObjectName("viewportModeButton");
        button->setIcon(
            styling::icon(transformIcons.at(index), transformColors.at(index)));
        button->setToolTip(transformNames.at(index) + " tool");
        button->setCheckable(true);
        auto *action = new QAction(transformNames.at(index), button);
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
    spaceButton->setText("World");
    spaceButton->setIcon(styling::icon(styling::Icon::Globe, "#55C2FF"));
    spaceButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    spaceButton->setToolTip("Transform space (Shift+T)");
    tools->addWidget(spaceButton);

    tools->addSpacing(10);
    auto *shading = new QComboBox(toolbar);
    shading->setObjectName("viewportShadingMode");
    shading->addItems({"Lit", "Wireframe", "Points"});
    shading->setToolTip("Viewport shading");
    auto *shadingIcon = new QLabel(toolbar);
    shadingIcon->setObjectName("viewportShadingIcon");
    shadingIcon->setPixmap(
        styling::icon(styling::Icon::Sphere, "#F472B6").pixmap(17, 17));
    shadingIcon->setToolTip("Viewport shading");
    tools->addWidget(shadingIcon);
    tools->addWidget(shading);

    auto *fpsButton = new QToolButton(toolbar);
    fpsButton->setObjectName("viewportOptionButton");
    fpsButton->setIcon(
        styling::icon(styling::Icon::Monitor, "#52D273"));
    fpsButton->setText("FPS");
    fpsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    fpsButton->setCheckable(true);
    fpsButton->setChecked(true);
    fpsButton->setToolTip("Show frame rate");
    fpsLabel = new QLabel("-- FPS", toolbar);
    fpsLabel->setObjectName("viewportFpsLabel");
    fpsLabel->setMinimumWidth(62);
    tools->addWidget(fpsButton);
    tools->addWidget(fpsLabel);
    tools->addStretch();

    layout->addWidget(toolbar);
    layout->addWidget(viewport, 1);
    shortcutHint = new QLabel(
        "Tab Frame · Right-Drag Pan · Middle-Drag Orbit · G Move · R Rotate · S Scale",
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
    connect(shading, &QComboBox::currentIndexChanged, viewport,
            &ViewportPanel::setRuntimeShadingMode);
    connect(spaceButton, &QToolButton::clicked, viewport,
            &ViewportPanel::toggleTransformSpace);
    connect(viewport, &ViewportPanel::transformSpaceChanged, this,
            [this](bool local) {
                spaceButton->setText(local ? "Local" : "World");
                spaceButton->setIcon(styling::icon(
                    local ? styling::Icon::Cube : styling::Icon::Globe,
                    local ? QColor("#A78BFA") : QColor("#55C2FF")));
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
    connect(sceneTabs, &QTabBar::tabCloseRequested, this, [this](int index) {
        closeSceneTab(index);
    });
    connect(viewport, &ViewportPanel::sceneOpened, this,
            &ViewportTools::openSceneTab);
    refreshSceneTabs();
    updatePlaybackState(0);
}

void ViewportTools::refreshSceneTabs() {
    const QString current = viewport != nullptr ? viewport->currentRuntimeScene()
                                                : QString();
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
