#ifndef ATLAS_SPLASHSCREEN_H
#define ATLAS_SPLASHSCREEN_H

#include <QDialog>

class QLabel;
class QTimer;

class SplashScreen : public QDialog {
    Q_OBJECT

public:
    explicit SplashScreen(QWidget* parent = nullptr);
    void start(const QString& statusText, int durationMs = 1200);

signals:
    void ready();

private:
    QLabel* statusLabel = nullptr;
    QTimer* finishTimer = nullptr;
};

#endif
