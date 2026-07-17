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
#include <QKeySequence>
#include <QMenuBar>
#include <QStyle>
#include <QSettings>
#include <QShowEvent>
#include <QSplitter>
#include <QTimer>
#include <QCloseEvent>
#include <QFileInfo>

#include "DockManager.h"
#include "editor/debug.h"
#include "editor/views/fileExplorer.h"
#include "editor/views/hierarchyPanel.h"
#include "editor/views/inspectorView.h"
#include "editor/views/materialEditor.h"
#include "editor/views/postProcessing.h"
#include "editor/views/viewport.h"
#include "editor/views/viewportTools.h"

namespace {
constexpr int DockStateVersion = 8;
constexpr auto DockStateKey = "docking/state/v8";
}

#ifndef ATLAS_VERSION
#define ATLAS_VERSION "Alpha 9"
#endif

#ifndef ATLAS_BUILD_STRING
#define ATLAS_BUILD_STRING ""
#endif

EditorWindow::EditorWindow(const QString &projectFile, QWidget *parent)
    : QMainWindow(parent), projectFile(projectFile) {
    setupWindow();
    setupMenus();
    setupDocks();

    restoreLayout();
}

void EditorWindow::setupWindow() {
    const auto project = ProjectStore::projectInfo(projectFile);
    projectName = project.has_value() ? project->name : QStringLiteral("Project");
    updateWindowTitle(false);
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
                                     true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton,
                                     false);

    coreManager = new ads::CDockManager(this);
    setCentralWidget(coreManager);
    dockManager = new EditorDockManager(coreManager);
    layoutSaveTimer = new QTimer(this);
    layoutSaveTimer->setSingleShot(true);
    layoutSaveTimer->setInterval(250);
    connect(layoutSaveTimer, &QTimer::timeout, this, [this] {
        if (!restoringLayout && !closing)
            saveLayout();
    });
}

void EditorWindow::setupMenus() {
    auto *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_FileIcon), "New");
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton),
                        "Open");
    auto *saveAction = fileMenu->addAction(
        style()->standardIcon(QStyle::SP_DialogSaveButton), "Save Scene");
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveAction, &QAction::triggered, this, [this] {
        if (materialEditorPanel != nullptr &&
            materialEditorPanel->isVisible()) {
            materialEditorPanel->saveMaterial();
        }
        if (viewportPanel != nullptr) {
            viewportPanel->saveRuntimeScene();
        }
    });
    fileMenu->addSeparator();
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogCloseButton),
                        "Quit", this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu("Edit");
    auto *undoAction = editMenu->addAction(
        style()->standardIcon(QStyle::SP_ArrowBack), "Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(undoAction, &QAction::triggered, this, [this] {
        if (materialEditorPanel != nullptr &&
            materialEditorPanel->isVisible()) {
            materialEditorPanel->undo();
        } else if (viewportPanel != nullptr) {
            viewportPanel->undo();
        }
    });
    auto *redoAction = editMenu->addAction(
        style()->standardIcon(QStyle::SP_ArrowForward), "Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(redoAction, &QAction::triggered, this, [this] {
        if (materialEditorPanel != nullptr &&
            materialEditorPanel->isVisible()) {
            materialEditorPanel->redo();
        } else if (viewportPanel != nullptr) {
            viewportPanel->redo();
        }
    });
    editMenu->addSeparator();
    editMenu->addAction("Preferences");

    viewMenu = menuBar()->addMenu("View");
    auto *resetLayoutAction = viewMenu->addAction(
        style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        "Reset Layout");
    connect(resetLayoutAction, &QAction::triggered, this, [this] {
        if (coreManager != nullptr && !defaultDockState.isEmpty()) {
            restoringLayout = true;
            coreManager->restoreState(defaultDockState, DockStateVersion);
            restoringLayout = false;
            configureDockSplitters();
            if (auto *dock = dockManager->panel("viewport"))
                dock->setAsCurrentTab();
            scheduleLayoutSave();
        }
    });

    windowMenu = menuBar()->addMenu("Window");
    windowMenu->addAction(
        style()->standardIcon(QStyle::SP_TitleBarNormalButton), "Minimize",
        this, &QWidget::showMinimized);
    windowMenu->addAction(
        style()->standardIcon(QStyle::SP_TitleBarMaxButton), "Zoom", this,
        [this] { isMaximized() ? showNormal() : showMaximized(); });

    auto *helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction(style()->standardIcon(QStyle::SP_MessageBoxQuestion),
                        "About Atlas");
}

void EditorWindow::setupDocks() {
    viewportPanel = new ViewportPanel(projectFile);
    connect(viewportPanel, &ViewportPanel::sceneDirtyChanged, this,
            &EditorWindow::updateWindowTitle);
    connect(viewportPanel, &ViewportPanel::runtimeStartupFinished, this,
            [this](bool success, const QString &message) {
                if (startupComplete)
                    return;
                startupComplete = true;
                emit startupStatusChanged(success ? "Project ready"
                                                  : "Runtime unavailable");
                emit startupReady(success, message);
            });
    auto *viewportTools = new ViewportTools(viewportPanel);
    auto *viewportDock = dockManager->addPanel(
        {.id = "viewport",
         .title = "Viewport",
         .widget = viewportTools,
         .area = EditorDockArea::Center,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    auto *hierarchyPanel = new HierarchyPanel(viewportPanel);
    auto *hierarchyDock = dockManager->addPanel(
        {.id = "hierarchy",
         .title = "Hierarchy Panel",
         .widget = hierarchyPanel,
         .area = EditorDockArea::Left,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    inspectorPanel = new InspectorPanel(viewportPanel, projectFile);
    auto *inspectorDock = dockManager->addPanel(
        {.id = "inspector",
         .title = "Inspector",
         .widget = inspectorPanel,
         .area = EditorDockArea::Right,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    auto *contentBrowser = new ContentBrowserPanel(projectFile);
    auto *contentDock = dockManager->addPanel(
        {.id = "fileExplorer",
         .title = "Content Browser",
         .widget = contentBrowser,
         .area = EditorDockArea::Bottom,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    materialEditorPanel = new MaterialEditorPanel(viewportPanel);
    auto *materialDock = dockManager->addPanel(
        {.id = "materialEditor",
         .title = "Material Editor",
         .widget = materialEditorPanel,
         .area = EditorDockArea::Right,
         .icon = style()->standardIcon(QStyle::SP_FileDialogContentsView)});
    coreManager->addDockWidgetTabToArea(materialDock,
                                        viewportDock->dockAreaWidget());

    postProcessingPanel = new PostProcessingPanel(viewportPanel);
    auto *postProcessingDock = dockManager->addPanel(
        {.id = "postProcessing",
         .title = "Post Processing",
         .widget = postProcessingPanel,
         .area = EditorDockArea::Right,
         .icon = style()->standardIcon(QStyle::SP_ComputerIcon)});
    coreManager->addDockWidgetTabToArea(postProcessingDock,
                                        viewportDock->dockAreaWidget());

    viewportDock->setAsCurrentTab();
    defaultDockState = coreManager->saveState(DockStateVersion);

    const QList<ads::CDockWidget *> managedDocks{
        viewportDock, hierarchyDock, inspectorDock, contentDock,
        materialDock, postProcessingDock};
    for (ads::CDockWidget *dock : managedDocks) {
        connect(dock, &ads::CDockWidget::topLevelChanged, this,
                [this](bool) { scheduleLayoutSave(); });
        connect(dock, &ads::CDockWidget::viewToggled, this,
                [this](bool) { scheduleLayoutSave(); });
    }
    connect(coreManager, &ads::CDockManager::dockAreaCreated, this,
            [this](ads::CDockAreaWidget *) {
                QTimer::singleShot(0, this,
                                   &EditorWindow::configureDockSplitters);
            });
    configureDockSplitters();

    if (windowMenu != nullptr) {
        windowMenu->addSeparator();
        for (ads::CDockWidget *dock : managedDocks)
            windowMenu->addAction(dock->toggleViewAction());
        windowMenu->addSeparator();
        const QList<QPair<QString, ads::CDockWidget *>> workspaces{
            {"Viewport", viewportDock},
            {"Material Editor", materialDock},
            {"Post Processing", postProcessingDock},
            {"Hierarchy", hierarchyDock},
            {"Inspector", inspectorDock},
            {"Content Browser", contentDock}};
        for (int index = 0; index < workspaces.size(); ++index) {
            const auto &[name, dock] = workspaces.at(index);
            auto *action = windowMenu->addAction(
                QStringLiteral("Focus %1").arg(name), this, [dock] {
                    dock->toggleView(true);
                    dock->setAsCurrentTab();
                    dock->raise();
                });
            action->setShortcut(QKeySequence(
                QStringLiteral("Meta+%1").arg(index + 1)));
            action->setShortcutContext(Qt::ApplicationShortcut);
        }
    }

    connect(hierarchyPanel, &HierarchyPanel::objectActivated, inspectorPanel,
            &InspectorPanel::inspectRuntimeObject);
    connect(hierarchyPanel, &HierarchyPanel::cameraActivated, inspectorPanel,
            &InspectorPanel::inspectCamera);
    connect(hierarchyPanel, &HierarchyPanel::environmentActivated,
            inspectorPanel, &InspectorPanel::inspectEnvironment);
    connect(hierarchyPanel, &HierarchyPanel::objectActivated, contentBrowser,
            &ContentBrowserPanel::clearSelection);
    connect(viewportPanel, &ViewportPanel::runtimeObjectActivated,
            inspectorPanel, &InspectorPanel::inspectRuntimeObject);
    connect(viewportPanel, &ViewportPanel::runtimeObjectActivated,
            contentBrowser, &ContentBrowserPanel::clearSelection);
    connect(contentBrowser, &ContentBrowserPanel::selectionChanged, this,
            [this](const QString &path) {
                const QString suffix = QFileInfo(path).suffix().toLower();
                if (!path.isEmpty() && suffix != "amat" &&
                    suffix != "material") {
                    viewportPanel->selectRuntimeObject(-1, false);
                }
                this->inspectorPanel->inspectFile(path);
            });
    connect(contentBrowser, &ContentBrowserPanel::assetActivated, this,
            [this, materialDock](const QString &path) {
                materialEditorPanel->openMaterial(path);
                materialDock->toggleView(true);
                materialDock->raise();
            });
}

void EditorWindow::saveLayout() {
    if (coreManager == nullptr)
        return;
    QSettings settings("Neutral Software", "Atlas Engine");

    settings.setValue("window/geometry", saveGeometry());
    settings.setValue(DockStateKey,
                      coreManager->saveState(DockStateVersion));
    settings.sync();
}

void EditorWindow::restoreLayout() {
    QSettings settings("Neutral Software", "Atlas Engine");

    const QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    const QByteArray dockState = settings.value(DockStateKey).toByteArray();

    restoringLayout = true;
    const bool restored = !dockState.isEmpty() &&
                          coreManager->restoreState(dockState,
                                                    DockStateVersion);
    if (!restored && !defaultDockState.isEmpty())
        coreManager->restoreState(defaultDockState, DockStateVersion);
    restoringLayout = false;
    configureDockSplitters();
}

void EditorWindow::configureDockSplitters() {
    if (coreManager == nullptr)
        return;
    for (QSplitter *splitter : coreManager->findChildren<QSplitter *>()) {
        splitter->setHandleWidth(6);
        splitter->setOpaqueResize(true);
        splitter->setChildrenCollapsible(false);
        if (!splitter->property("atlasLayoutTracking").toBool()) {
            splitter->setProperty("atlasLayoutTracking", true);
            connect(splitter, &QSplitter::splitterMoved, this,
                    [this](int, int) { scheduleLayoutSave(); });
        }
    }
}

void EditorWindow::scheduleLayoutSave() {
    if (!restoringLayout && !closing && layoutSaveTimer != nullptr)
        layoutSaveTimer->start();
}

void EditorWindow::updateWindowTitle(bool dirty) {
    const QString name = projectName + (dirty ? "*" : "");
#ifdef ATLAS_DEBUG_BUILD
    const QString build = QStringLiteral(ATLAS_BUILD_STRING);
    setWindowTitle(build.isEmpty()
                       ? QStringLiteral("%1 - Atlas Engine (Development)")
                             .arg(name)
                       : QStringLiteral("%1 - Atlas Engine (Development) + %2")
                             .arg(name, build));
#else
    setWindowTitle(QStringLiteral("%1 - Atlas Engine %2")
                       .arg(name, QStringLiteral(ATLAS_VERSION)));
#endif
}

void EditorWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (startupQueued || startupComplete)
        return;
    startupQueued = true;
    emit startupStatusChanged("Restoring editor workspace...");
    QTimer::singleShot(0, this, [this] {
        configureDockSplitters();
        emit startupStatusChanged("Loading the project runtime...");
        if (viewportPanel != nullptr) {
            viewportPanel->setRuntimeStartupEnabled(true);
        } else {
            startupComplete = true;
            emit startupReady(false, "Viewport is unavailable");
        }
    });
}

void EditorWindow::closeEvent(QCloseEvent *event) {
    if (closing) {
        event->accept();
        return;
    }
    closing = true;
    if (layoutSaveTimer != nullptr)
        layoutSaveTimer->stop();
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
