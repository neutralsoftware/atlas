#ifndef ATLAS_SPLASHSCREEN_H
#define ATLAS_SPLASHSCREEN_H

#include <QDialog>

class QLabel;

class SplashScreen : public QDialog {
    Q_OBJECT

public:
    explicit SplashScreen(QWidget* parent = nullptr);
    void start(const QString& statusText);
    void setStatus(const QString& statusText);
    void finish();

signals:
    void ready();

private:
    QLabel* statusLabel = nullptr;
};

#endif
