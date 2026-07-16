/*
 * editor.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Editor view and window
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/editorWindow.h>

#include <editor/project/projectStore.h>

#include <QAction>
#include <QList>
#include <QMenuBar>
#include <QStyle>
#include <QSettings>
#include <QTimer>
#include <QVariantList>
#include <QCloseEvent>

#include "DockManager.h"
#include "editor/debug.h"
#include "editor/views/fileExplorer.h"
#include "editor/views/hierarchyPanel.h"
#include "editor/views/inspectorView.h"
#include "editor/views/viewport.h"
#include "editor/views/viewportTools.h"

EditorWindow::EditorWindow(const QString &projectFile, QWidget *parent)
    : QMainWindow(parent), projectFile(projectFile) {
    setupWindow();
    setupMenus();
    setupDocks();

    restoreLayout();
}

void EditorWindow::setupWindow() {
    const auto project = ProjectStore::projectInfo(projectFile);
    setWindowTitle(project.has_value()
                       ? QStringLiteral("%1 — Atlas Engine").arg(project->name)
                       : QStringLiteral("Atlas Engine"));
    resize(1280, 720);
    menuBar()->setNativeMenuBar(false);

    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize,
                                     true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting,
                                     true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DisableStylesheet,
                                     true);
    ads::CDockManager::setConfigFlag(
        ads::CDockManager::DockAreaHasTabsMenuButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton,
                                     false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton,
                                     false);

    coreManager = new ads::CDockManager(this);
    setCentralWidget(coreManager);
    dockManager = new EditorDockManager(coreManager);
}

void EditorWindow::setupMenus() {
    auto *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_FileIcon), "New");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton),
                        "Open");
    auto *saveAction = fileMenu->addAction(
        style()->standardIcon(QStyle::SP_DialogSaveButton), "Save Scene");
    connect(saveAction, &QAction::triggered, this, [this] {
        if (viewportPanel != nullptr) {
            viewportPanel->saveRuntimeScene();
        }
    });
    fileMenu->addSeparator();
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogCloseButton),
                        "Quit", this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction(style()->standardIcon(QStyle::SP_ArrowBack), "Undo");
    editMenu->addAction(style()->standardIcon(QStyle::SP_ArrowForward), "Redo");
    editMenu->addSeparator();
    editMenu->addAction("Preferences");

    auto *viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction(
        style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        "Reset Layout");

    auto *windowMenu = menuBar()->addMenu("Window");
    windowMenu->addAction(
        style()->standardIcon(QStyle::SP_TitleBarNormalButton), "Minimize");
    windowMenu->addAction(style()->standardIcon(QStyle::SP_TitleBarMaxButton),
                          "Zoom");

    auto *helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction(style()->standardIcon(QStyle::SP_MessageBoxQuestion),
                        "About Atlas");
}

void EditorWindow::setupDocks() {
    viewportPanel = new ViewportPanel(projectFile);
    auto *viewportTools = new ViewportTools(viewportPanel);
    dockManager->addPanel(
        {.id = "viewport",
         .title = "Viewport",
         .widget = viewportTools,
         .area = EditorDockArea::Center,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    auto *hierarchyPanel = new HierarchyPanel(viewportPanel);
    dockManager->addPanel(
        {.id = "hierarchy",
         .title = "Hierarchy Panel",
         .widget = hierarchyPanel,
         .area = EditorDockArea::Left,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    inspectorPanel = new InspectorPanel(viewportPanel);
    dockManager->addPanel(
        {.id = "inspector",
         .title = "Inspector",
         .widget = inspectorPanel,
         .area = EditorDockArea::Right,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    auto *contentBrowser = new ContentBrowserPanel(projectFile);
    dockManager->addPanel(
        {.id = "fileExplorer",
         .title = "Content Browser",
         .widget = contentBrowser,
         .area = EditorDockArea::Bottom,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    connect(hierarchyPanel, &HierarchyPanel::objectActivated, inspectorPanel,
            &InspectorPanel::inspectRuntimeObject);
    connect(hierarchyPanel, &HierarchyPanel::objectActivated, contentBrowser,
            &ContentBrowserPanel::clearSelection);
    connect(viewportPanel, &ViewportPanel::runtimeObjectActivated,
            inspectorPanel, &InspectorPanel::inspectRuntimeObject);
    connect(viewportPanel, &ViewportPanel::runtimeObjectActivated,
            contentBrowser, &ContentBrowserPanel::clearSelection);
    connect(contentBrowser, &ContentBrowserPanel::selectionChanged, this,
            [this](const QString &path) {
                if (!path.isEmpty()) {
                    viewportPanel->selectRuntimeObject(-1, false);
                }
                this->inspectorPanel->inspectFile(path);
            });
}

void EditorWindow::saveLayout() {
    QSettings settings("Neutral Software", "Atlas Engine");

    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    settings.setValue("docking/state/v2", coreManager->saveState(2));
    if (inspectorPanel != nullptr) {
        settings.setValue("panels/inspector/width", inspectorPanel->width());
    }
    if (dockManager != nullptr) {
        auto *dock = dockManager->panel("inspector");
        if (dock != nullptr && dock->dockAreaWidget() != nullptr) {
            QVariantList sizes;
            for (int size :
                 coreManager->splitterSizes(dock->dockAreaWidget())) {
                sizes.append(size);
            }
            settings.setValue("panels/inspector/splitterSizes", sizes);
        }
    }
}

void EditorWindow::restoreLayout() {
    QSettings settings("Neutral Software", "Atlas Engine");

    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/state").toByteArray());

    const QByteArray dockState =
        settings.value("docking/state/v2").toByteArray();

    if (!dockState.isEmpty()) {
        coreManager->restoreState(dockState, 2);
    }

    const int inspectorWidth =
        settings.value("panels/inspector/width", 320).toInt();
    const QVariantList storedSizes =
        settings.value("panels/inspector/splitterSizes").toList();
    QTimer::singleShot(0, this,
                       [this, inspectorWidth, storedSizes] {
                           if (inspectorPanel != nullptr &&
                               inspectorWidth > 0) {
                               inspectorPanel->resize(
                                   inspectorWidth, inspectorPanel->height());
                           }
                           if (dockManager == nullptr ||
                               storedSizes.isEmpty()) {
                               return;
                           }
                           auto *dock = dockManager->panel("inspector");
                           if (dock == nullptr ||
                               dock->dockAreaWidget() == nullptr) {
                               return;
                           }
                           QList<int> sizes;
                           for (const QVariant &size : storedSizes) {
                               sizes.append(size.toInt());
                           }
                           coreManager->setSplitterSizes(
                               dock->dockAreaWidget(), sizes);
                       });
}

void EditorWindow::closeEvent(QCloseEvent *event) {
    if (closing) {
        event->accept();
        return;
    }
    closing = true;
    saveLayout();
    for (auto *viewport : findChildren<ViewportPanel *>()) {
        viewport->shutdownRuntime();
    }
    delete dockManager;
    dockManager = nullptr;
    if (coreManager != nullptr) {
        coreManager->deleteLater();
        coreManager = nullptr;
    }
    QMainWindow::closeEvent(event);
    event->accept();
}
