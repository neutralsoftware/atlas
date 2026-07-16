/*
* dockManager.cpp
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Dock management functions
* Copyright (c) 2026 Max Van den Eynde
*/

#include <editor/application/dockManager.h>
#include "DockAreaWidget.h"
#include "DockManager.h"


EditorDockManager::EditorDockManager(ads::CDockManager* dockManager)
    : dockManager(dockManager) {
}

ads::DockWidgetArea EditorDockManager::toAdsArea(EditorDockArea area) const {
    switch (area) {
    case EditorDockArea::Left:
        return ads::LeftDockWidgetArea;
    case EditorDockArea::Right:
        return ads::RightDockWidgetArea;
    case EditorDockArea::Bottom:
        return ads::BottomDockWidgetArea;
    case EditorDockArea::Center:
        return ads::CenterDockWidgetArea;
    }

    return ads::LeftDockWidgetArea;
}

ads::CDockWidget* EditorDockManager::addPanel(const EditorDockPanelDesc& desc) {
    auto* dock = new ads::CDockWidget(desc.title);
    dock->setWidget(desc.widget);

    if (!desc.icon.isNull()) {
        dock->setIcon(desc.icon);
    }

    if (desc.area == EditorDockArea::Center) {
        centerArea = dockManager->setCentralWidget(dock);
        if (centerArea != nullptr) {
            centerArea->setAllowedAreas(ads::OuterDockAreas);
        }
    } else if (centerArea) {
        dockManager->addDockWidget(toAdsArea(desc.area), dock, centerArea);
    } else {
        dockManager->addDockWidget(toAdsArea(desc.area), dock);
    }

    panels.insert(desc.id, dock);
    return dock;
}

ads::CDockWidget* EditorDockManager::panel(const QString& id) const {
    return panels.value(id, nullptr);
}
