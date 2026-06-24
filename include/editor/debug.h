/*
* debug.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Debug views and components
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_DEBUG_H
#define ATLAS_DEBUG_H

#include <QWidget>

class DebugComponentsView final : public QWidget {
    Q_OBJECT

public:
    explicit DebugComponentsView(QWidget* parent = nullptr);

private:
    QWidget* createBasicControlsSection();
    QWidget* createInputSection();
    QWidget* createSelectionSection();
    QWidget* createRangeSection();
    QWidget* createTextSection();
    QWidget* createItemViewsSection();
    QWidget* createTabsSection();
    QWidget* createCollapsibleSection();
    QWidget* createStatusSection();

    QWidget* createSection(const QString& title, QWidget* content);
};

#endif //ATLAS_DEBUG_H
