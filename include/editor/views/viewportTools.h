#ifndef ATLAS_VIEWPORTTOOLS_H
#define ATLAS_VIEWPORTTOOLS_H

#include <QWidget>
#include <QStringList>

class QLabel;
class QTabBar;
class QToolButton;
class ViewportPanel;

class ViewportTools : public QWidget {
    Q_OBJECT

  public:
    explicit ViewportTools(ViewportPanel *viewport, const QString &projectFile,
                           QWidget *parent = nullptr);
    void openSceneTab(const QString &path);
    void closeCurrentSceneTab();
    void refreshSceneTabs();

  private:
    void updatePlaybackState(int state);
    void closeSceneTab(int index);
    void updateSceneTabs();

    ViewportPanel *viewport = nullptr;
    QToolButton *playButton = nullptr;
    QToolButton *pauseButton = nullptr;
    QToolButton *stepButton = nullptr;
    QToolButton *stopButton = nullptr;
    QToolButton *reloadButton = nullptr;
    QToolButton *spaceButton = nullptr;
    QToolButton *cameraButton = nullptr;
    QLabel *fpsLabel = nullptr;
    QLabel *cameraLabel = nullptr;
    QLabel *shortcutHint = nullptr;
    QTabBar *sceneTabs = nullptr;
    QString projectRoot;
    QStringList scenePaths;
    bool runtimeAvailable = false;
    int playbackState = 0;
};

#endif
