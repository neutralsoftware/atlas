#include <editor/views/viewportTools.h>

#include <editor/views/viewport.h>

#include <QActionGroup>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequence>
#include <QList>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

ViewportTools::ViewportTools(ViewportPanel *viewport, QWidget *parent)
    : QWidget(parent), viewport(viewport) {
    setObjectName("viewportTools");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("viewportToolbar");
    auto *tools = new QHBoxLayout(toolbar);
    tools->setContentsMargins(5, 3, 5, 3);
    tools->setSpacing(2);

    playButton = new QToolButton(toolbar);
    playButton->setObjectName("viewportPlaybackButton");
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playButton->setToolTip("Play");
    pauseButton = new QToolButton(toolbar);
    pauseButton->setObjectName("viewportPlaybackButton");
    pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause");
    stepButton = new QToolButton(toolbar);
    stepButton->setObjectName("viewportPlaybackButton");
    stepButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    stepButton->setToolTip("Step one frame");
    stopButton = new QToolButton(toolbar);
    stopButton->setObjectName("viewportPlaybackButton");
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton->setToolTip("Stop and restore the scene");
    reloadButton = new QToolButton(toolbar);
    reloadButton->setObjectName("viewportPlaybackButton");
    reloadButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
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
    const QStringList transformNames{"Move", "Rotate", "Scale"};
    const QList<QStyle::StandardPixmap> transformIcons{
        QStyle::SP_ArrowRight, QStyle::SP_BrowserReload,
        QStyle::SP_TitleBarMaxButton};
    for (int index = 0; index < transformNames.size(); ++index) {
        auto *button = new QToolButton(toolbar);
        button->setObjectName("viewportModeButton");
        button->setIcon(style()->standardIcon(transformIcons.at(index)));
        button->setToolTip(transformNames.at(index) + " tool");
        button->setCheckable(true);
        auto *action = new QAction(transformNames.at(index), button);
        action->setCheckable(true);
        action->setData(index + 1);
        button->setDefaultAction(action);
        transformGroup->addAction(action);
        tools->addWidget(button);
        if (index == 0) {
            action->setChecked(true);
        }
    }

    tools->addSpacing(10);
    auto *shading = new QComboBox(toolbar);
    shading->setObjectName("viewportShadingMode");
    shading->addItems({"Lit", "Wireframe", "Points"});
    shading->setToolTip("Viewport shading");
    tools->addWidget(shading);

    auto *fpsButton = new QToolButton(toolbar);
    fpsButton->setObjectName("viewportOptionButton");
    fpsButton->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
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
    shortcutHint = new QLabel("G Move · R Rotate · S Scale", this);
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
    updatePlaybackState(0);
}

void ViewportTools::updatePlaybackState(int state) {
    playbackState = state;
    playButton->setEnabled(runtimeAvailable && state != 1);
    pauseButton->setEnabled(runtimeAvailable && state == 1);
    stepButton->setEnabled(runtimeAvailable && state != 0);
    stopButton->setEnabled(runtimeAvailable && state != 0);
    reloadButton->setEnabled(runtimeAvailable);
}
