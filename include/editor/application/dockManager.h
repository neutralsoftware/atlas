/*
* dockManager.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Manager for docking in the editor
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_DOCKMANAGER_H
#define ATLAS_DOCKMANAGER_H

#include "DockManager.h"
#include "DockWidget.h"

class QWidget;

enum class EditorDockArea {
    Left,
    Right,
    Bottom,
    Center
};

struct EditorDockPanelDesc {
    QString id;
    QString title;
    QWidget* widget = nullptr;
    EditorDockArea area = EditorDockArea::Left;
    QIcon icon;
};

class EditorDockManager {
public:
    explicit EditorDockManager(ads::CDockManager* dockManager);

    ads::CDockWidget* addPanel(const EditorDockPanelDesc& desc);
    ads::CDockWidget* panel(const QString& id) const;

private:
    ads::DockWidgetArea toAdsArea(EditorDockArea area) const;

    ads::CDockManager* dockManager = nullptr;
    QMap<QString, ads::CDockWidget*> panels;
    ads::CDockAreaWidget* centerArea = nullptr;
};

#endif //ATLAS_DOCKMANAGER_H
