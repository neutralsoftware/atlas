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

#include <QWidget>

class ViewportPanel : public QWidget {
    Q_OBJECT

public:
    explicit ViewportPanel(QWidget* parent = nullptr);
};

#endif //ATLAS_VIEWPORT_H
