#ifndef ATLAS_VIEWPORTTOOLS_H
#define ATLAS_VIEWPORTTOOLS_H

#include <QWidget>

class QLabel;
class QToolButton;
class ViewportPanel;

class ViewportTools : public QWidget {
    Q_OBJECT

  public:
    explicit ViewportTools(ViewportPanel *viewport, QWidget *parent = nullptr);

  private:
    void updatePlaybackState(int state);

    ViewportPanel *viewport = nullptr;
    QToolButton *playButton = nullptr;
    QToolButton *pauseButton = nullptr;
    QToolButton *stepButton = nullptr;
    QToolButton *stopButton = nullptr;
    QToolButton *reloadButton = nullptr;
    QLabel *fpsLabel = nullptr;
    QLabel *shortcutHint = nullptr;
    bool runtimeAvailable = false;
    int playbackState = 0;
};

#endif
