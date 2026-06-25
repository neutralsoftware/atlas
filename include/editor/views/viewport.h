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

#include <QWidget>

class Context;
class QKeyEvent;
class QMouseEvent;
class QPaintEngine;
class QResizeEvent;
class QShowEvent;
class QTimer;
class QWheelEvent;

class ViewportPanel : public QWidget {
    Q_OBJECT

public:
    explicit ViewportPanel(QWidget* parent = nullptr);
    ~ViewportPanel() override;

protected:
    QPaintEngine* paintEngine() const override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void startRuntime();
    void stopRuntime();
    void stepRuntime();
    void resizeRuntime();
    void sendPointerEvent(int action, float x, float y, int button);

    QTimer* frameTimer = nullptr;
    std::shared_ptr<Context> runtimeContext;
    int runtimeWidth = 0;
    int runtimeHeight = 0;
    float runtimeScale = 0.0f;
};

#endif //ATLAS_VIEWPORT_H
