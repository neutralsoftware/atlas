/*
* viewport.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Viewport definitions
* Copyright (c) 2026 Max Van den Eynde
*/

#include <editor/views/viewport.h>

#include <QVBoxLayout>
#include <QLabel>
#include <Qt>

ViewportPanel::ViewportPanel(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel("Atlas Viewport", this);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
}
