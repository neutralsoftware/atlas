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
#include <QApplication>
#include <QCoreApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QList>
#include <QKeySequence>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSpinBox>
#include <QStyle>
#include <QSettings>
#include <QStandardPaths>
#include <QShowEvent>
#include <QShortcut>
#include <QSplitter>
#include <QTabWidget>
#include <QTimer>
#include <QCloseEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QUrl>

#include <functional>
#include <limits>

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

bool copyExportPath(const QString &sourcePath, const QString &destinationPath,
                    QString *error) {
    const QFileInfo source(sourcePath);
    if (source.isDir()) {
        if (!QDir().mkpath(destinationPath)) {
            if (error != nullptr)
                *error = QStringLiteral("Could not create %1")
                             .arg(destinationPath);
            return false;
        }
        QDir sourceDirectory(sourcePath);
        const QFileInfoList entries = sourceDirectory.entryInfoList(
            QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden |
            QDir::System);
        for (const QFileInfo &entry : entries) {
            if (!copyExportPath(
                    entry.absoluteFilePath(),
                    QDir(destinationPath).filePath(entry.fileName()), error))
                return false;
        }
        return true;
    }
    QFile::remove(destinationPath);
    if (!QFile::copy(sourcePath, destinationPath)) {
        if (error != nullptr)
            *error = QStringLiteral("Could not copy %1").arg(sourcePath);
        return false;
    }
    QFile::setPermissions(destinationPath, source.permissions());
    return true;
}

QString atlasCliPath() {
    const QDir applicationDirectory(QCoreApplication::applicationDirPath());
    const QStringList candidates{
        applicationDirectory.filePath("../../target/debug/atlas"),
        applicationDirectory.filePath("../../target/release/atlas"),
        applicationDirectory.filePath("atlas")};
    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.isFile() && info.isExecutable())
            return info.absoluteFilePath();
    }
    return QStandardPaths::findExecutable("atlas");
}

QString tomlQuoted(QString value) {
    value.replace('\\', "\\\\");
    value.replace('"', "\\\"");
    value.replace('\n', "\\n");
    return QStringLiteral("\"%1\"").arg(value);
}

void setTomlValue(QStringList *lines, const QString &section,
                  const QString &key, const QString &value) {
    int start = 0;
    int end = lines->size();
    if (!section.isEmpty()) {
        const QString heading = QStringLiteral("[%1]").arg(section);
        start = lines->indexOf(heading);
        if (start < 0) {
            if (!lines->isEmpty() && !lines->last().isEmpty())
                lines->append(QString());
            lines->append(heading);
            lines->append(QStringLiteral("%1 = %2").arg(key, value));
            return;
        }
        ++start;
    }
    for (int index = start; index < lines->size(); ++index) {
        if (lines->at(index).trimmed().startsWith('[')) {
            end = index;
            break;
        }
    }
    const QRegularExpression expression(
        QStringLiteral("^\\s*%1\\s*=").arg(QRegularExpression::escape(key)));
    for (int index = start; index < end; ++index) {
        if (expression.match(lines->at(index)).hasMatch()) {
            (*lines)[index] = QStringLiteral("%1 = %2").arg(key, value);
            return;
        }
    }
    lines->insert(end, QStringLiteral("%1 = %2").arg(key, value));
}
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
    qApp->installEventFilter(this);
    refreshScriptWatcher();
}

void EditorWindow::setupWindow() {
    const auto project = ProjectStore::projectInfo(projectFile);
    projectName = project.has_value() ? project->name : QStringLiteral("Project");
    updateWindowTitle(false);
    resize(1280, 720);
    menuBar()->setNativeMenuBar(true);

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
    scriptWatcher = new QFileSystemWatcher(this);
    layoutSaveTimer->setSingleShot(true);
    layoutSaveTimer->setInterval(250);
    connect(layoutSaveTimer, &QTimer::timeout, this, [this] {
        if (!restoringLayout && !closing)
            saveLayout();
    });
    connect(scriptWatcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString &) {
                QTimer::singleShot(120, this, [this] {
                    refreshScriptWatcher();
                    if (viewportPanel != nullptr)
                        viewportPanel->reloadRuntime();
                });
            });
}

void EditorWindow::setupMenus() {
    auto *fileMenu = menuBar()->addMenu("File");
    auto addCommand = [this](QMenu *menu, const QString &name,
                             const QString &shortcut,
                             const std::function<void()> &handler) {
        QAction *action = menu->addAction(name, this, handler);
        if (!shortcut.isEmpty()) {
            const bool plainShift = shortcut.startsWith("Shift+") &&
                                    !shortcut.contains("Meta+") &&
                                    !shortcut.contains("Alt+") &&
                                    !shortcut.contains("Ctrl+");
            if (plainShift) {
                action->setProperty("atlasShortcut", shortcut);
            } else {
                QString nativeShortcut = shortcut;
                nativeShortcut.replace("Meta+", "Ctrl+");
                action->setShortcut(QKeySequence(nativeShortcut));
                action->setShortcutContext(Qt::ApplicationShortcut);
            }
        }
        return action;
    };
    addCommand(fileMenu, "New Scene", "Meta+N", [this] { createScene(); });
    addCommand(fileMenu, "Open Scene…", "Meta+O", [this] { openScene(); });
    auto *saveAction = fileMenu->addAction("Save Scene");
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
    addCommand(fileMenu, "Save Scene As…", "Meta+Shift+S",
               [this] { saveSceneAs(); });
    addCommand(fileMenu, "Close Scene", "Meta+W", [this] {
        if (viewportTools != nullptr)
            viewportTools->closeCurrentSceneTab();
    });
    fileMenu->addSeparator();
    addCommand(fileMenu, "Export Project…", QString(),
               [this] { showExportDialog(); });
    addCommand(fileMenu, "Build Project", "Meta+B",
               [this] { runProjectCommand(true); });
    addCommand(fileMenu, "Run Project", "Meta+Shift+B",
               [this] { runProjectCommand(false); });
    fileMenu->addSeparator();
    auto *quitAction = addCommand(fileMenu, "Quit Atlas Engine", "Meta+Q",
                                  [] { QApplication::quit(); });
    quitAction->setMenuRole(QAction::QuitRole);

    auto *editMenu = menuBar()->addMenu("Edit");
    auto *undoAction = editMenu->addAction("Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(undoAction, &QAction::triggered, this, [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget())) {
            field->undo();
        } else if (materialEditorPanel != nullptr &&
                   materialEditorPanel->isAncestorOf(
                       QApplication::focusWidget())) {
            materialEditorPanel->undo();
        } else if (viewportPanel != nullptr) {
            viewportPanel->undo();
        }
    });
    auto *redoAction = editMenu->addAction("Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(redoAction, &QAction::triggered, this, [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget())) {
            field->redo();
        } else if (materialEditorPanel != nullptr &&
                   materialEditorPanel->isAncestorOf(
                       QApplication::focusWidget())) {
            materialEditorPanel->redo();
        } else if (viewportPanel != nullptr) {
            viewportPanel->redo();
        }
    });
    editMenu->addSeparator();
    addCommand(editMenu, "Find…", "Meta+F",
               [this] { showGlobalSearch(); });
    editMenu->addSeparator();
    addCommand(editMenu, "Cut", "Meta+X", [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->cut();
        else if (contentBrowserHasFocus())
            contentBrowser->cutSelection();
        else if (viewportPanel != nullptr)
            viewportPanel->cutSelectedRuntimeObject();
    });
    addCommand(editMenu, "Copy", "Meta+C", [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->copy();
        else if (contentBrowserHasFocus())
            contentBrowser->copySelection();
        else if (viewportPanel != nullptr)
            viewportPanel->copySelectedRuntimeObject();
    });
    addCommand(editMenu, "Paste", "Meta+V", [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->paste();
        else if (contentBrowserHasFocus())
            contentBrowser->pasteSelection();
        else if (viewportPanel != nullptr)
            viewportPanel->pasteRuntimeObject();
    });
    addCommand(editMenu, "Duplicate", "Meta+D", [this] {
        if (contentBrowserHasFocus())
            contentBrowser->duplicateSelection();
        else if (viewportPanel != nullptr)
            viewportPanel->duplicateSelectedRuntimeObject();
    });
    addCommand(editMenu, "Delete", "Backspace", [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->backspace();
        else if (contentBrowserHasFocus())
            contentBrowser->deleteSelection();
        else if (hierarchyPanel != nullptr)
            hierarchyPanel->deleteSelectedObject();
    });
    addCommand(editMenu, "Select All Objects", "Meta+A", [this] {
        if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->selectAll();
        else if (contentBrowserHasFocus() && contentBrowser != nullptr)
            contentBrowser->selectAllAssets();
        else if (hierarchyPanel != nullptr)
            hierarchyPanel->selectAllObjects();
    });
    addCommand(editMenu, "Deselect All Objects", QString(), [this] {
        if (hierarchyPanel != nullptr)
            hierarchyPanel->deselectAllObjects();
    });
    editMenu->addSeparator();
    auto *settingsAction =
        addCommand(editMenu, "Project Settings…", "Meta+,",
                   [this] { showProjectSettings(); });
    settingsAction->setMenuRole(QAction::PreferencesRole);

    auto *objectMenu = menuBar()->addMenu("Object");
    addCommand(objectMenu, "Create Empty", "Shift+N", [this] {
        if (hierarchyPanel != nullptr)
            hierarchyPanel->createObject("group", "Empty Object");
    });
    addCommand(objectMenu, "Create Camera", "Shift+C", [this] {
        if (hierarchyPanel != nullptr)
            hierarchyPanel->createObject("camera", "Camera");
    });
    addCommand(objectMenu, "Create Light", "Shift+L", [this] {
        if (hierarchyPanel != nullptr)
            hierarchyPanel->createObject("pointLight", "Point Light");
    });
    addCommand(objectMenu, "Add Object…", "Shift+A", [this] {
        if (hierarchyPanel != nullptr)
            hierarchyPanel->showCreationPopup();
    });
    addCommand(objectMenu, "Rename", QString(), [this] {
        if (contentBrowserHasFocus())
            contentBrowser->renameSelection();
        else if (hierarchyPanel != nullptr)
            hierarchyPanel->renameSelectedObject();
    });
    addCommand(objectMenu, "Reparent…", "Shift+R", [this] {
        if (hierarchyPanel == nullptr || viewportPanel == nullptr)
            return;
        const int child = viewportPanel->selectedRuntimeObjectId();
        bool accepted = false;
        const int parent = QInputDialog::getInt(
            this, "Reparent Object", "Parent object runtime ID (-1 for root)",
            -1, -1, std::numeric_limits<int>::max(), 1, &accepted);
        if (accepted && child >= 0)
            viewportPanel->setRuntimeObjectParent(child, parent);
    });
    objectMenu->addSeparator();
    addCommand(objectMenu, "Reset Position", "Meta+Alt+G", [this] {
        if (viewportPanel != nullptr)
            viewportPanel->resetSelectedTransform(1);
    });
    addCommand(objectMenu, "Reset Rotation", "Meta+Alt+R", [this] {
        if (viewportPanel != nullptr)
            viewportPanel->resetSelectedTransform(2);
    });
    addCommand(objectMenu, "Reset Scale", "Meta+Alt+S", [this] {
        if (viewportPanel != nullptr)
            viewportPanel->resetSelectedTransform(3);
    });

    viewMenu = menuBar()->addMenu("View");
    auto *resetLayoutAction = viewMenu->addAction("Reset Layout");
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
    addCommand(viewMenu, "Search Hierarchy", "Meta+Shift+F", [this] {
        if (hierarchyPanel != nullptr)
            hierarchyPanel->focusSearch();
    });
    addCommand(viewMenu, "Search Assets", "Meta+Alt+F", [this] {
        if (contentBrowser != nullptr)
            contentBrowser->focusSearch();
    });
    addCommand(viewMenu, "Toggle Local / World Transform Space", "Shift+T",
               [this] {
                   if (viewportPanel != nullptr)
                       viewportPanel->toggleTransformSpace();
               });
    addCommand(viewMenu, "Toggle Transform Snapping", QString(), [this] {
        if (viewportPanel != nullptr)
            viewportPanel->toggleTransformSnapping();
    });
    addCommand(viewMenu, "Increase Snapping Increment", QString(), [this] {
        if (viewportPanel != nullptr)
            viewportPanel->changeTransformSnapIncrement(2.0f);
    });
    addCommand(viewMenu, "Decrease Snapping Increment", QString(), [this] {
        if (viewportPanel != nullptr)
            viewportPanel->changeTransformSnapIncrement(0.5f);
    });

    auto *runMenu = menuBar()->addMenu("Run");
    addCommand(runMenu, "Play / Pause", "Meta+P", [this] {
        if (viewportPanel == nullptr)
            return;
        viewportPanel->toggleRuntimePlayback();
    });
    addCommand(runMenu, "Stop", "Meta+Shift+L", [this] {
        if (viewportPanel != nullptr)
            viewportPanel->stopRuntimePlayback();
    });
    addCommand(runMenu, "Step One Frame", "Meta+Shift+K", [this] {
        if (viewportPanel != nullptr)
            viewportPanel->stepRuntimeOnce();
    });
    addCommand(runMenu, "Reload Scripts", QString(), [this] {
        if (viewportPanel != nullptr)
            viewportPanel->reloadRuntime();
    });
    addCommand(runMenu, "Refresh Asset Database", "Meta+Shift+R", [this] {
        if (contentBrowser != nullptr)
            contentBrowser->refreshAssets();
    });
    addCommand(runMenu, "Take Viewport Screenshot", QString(),
               [this] { takeViewportScreenshot(); });

    auto *toolsMenu = menuBar()->addMenu("Tools");
    auto *toolsSettings =
        addCommand(toolsMenu, "Project Settings…", QString(),
                   [this] { showProjectSettings(); });
    toolsSettings->setMenuRole(QAction::NoRole);
    addCommand(toolsMenu, "Command Palette…", "Meta+Shift+P",
               [this] { showCommandPalette(); });

    windowMenu = menuBar()->addMenu("Window");
    windowMenu->addAction("Minimize", this, &QWidget::showMinimized);
    windowMenu->addAction("Zoom", this,
        [this] { isMaximized() ? showNormal() : showMaximized(); });

    auto *helpMenu = menuBar()->addMenu("Help");
    auto *aboutAction = helpMenu->addAction("About Atlas Engine", this, [this] {
        QMessageBox::about(
            this, "About Atlas Engine",
            QStringLiteral("Atlas Engine %1\nby Neutral Software")
                .arg(QStringLiteral(ATLAS_VERSION)));
    });
    aboutAction->setMenuRole(QAction::AboutRole);
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
    viewportTools = new ViewportTools(viewportPanel, projectFile);
    auto *viewportDock = dockManager->addPanel(
        {.id = "viewport",
         .title = "Viewport",
         .widget = viewportTools,
         .area = EditorDockArea::Center,
         .icon = style()->standardIcon(QStyle::SP_DirOpenIcon)});

    hierarchyPanel = new HierarchyPanel(viewportPanel);
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

    contentBrowser = new ContentBrowserPanel(projectFile);
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
        for (ads::CDockWidget *dock : managedDocks) {
            dock->toggleViewAction()->setIcon({});
            windowMenu->addAction(dock->toggleViewAction());
        }
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
                QStringLiteral("Ctrl+%1").arg(index + 1)));
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
    connect(contentBrowser, &ContentBrowserPanel::sceneActivated, this,
            [this](const QString &path) {
                if (viewportPanel != nullptr &&
                    viewportPanel->openRuntimeScene(path) &&
                    viewportTools != nullptr) {
                    viewportTools->openSceneTab(path);
                }
            });
}

void EditorWindow::createScene() {
    const QString root = QFileInfo(projectFile).absolutePath();
    QString path = QFileDialog::getSaveFileName(
        this, "Create Atlas Scene", QDir(root).filePath("New Scene.ascene"),
        "Atlas Scene (*.ascene)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".ascene", Qt::CaseInsensitive))
        path += ".ascene";
    const QString name = QFileInfo(path).completeBaseName();
    const QByteArray contents =
        QJsonDocument(QJsonObject{
                          {"name", name},
                          {"id", name.toLower().replace(' ', '_')},
                          {"objects", QJsonArray{}},
                          {"lights", QJsonArray{}},
                          {"camera", QJsonObject{{"position", QJsonArray{0.0, 1.5, -5.0}},
                                                 {"target", QJsonArray{0.0, 0.0, 0.0}},
                                                 {"fov", 60.0}}},
                          {"targets", QJsonArray{QJsonObject{{"name", "Main Target"},
                                                             {"type", "scene"},
                                                             {"render", true},
                                                             {"display", true}}}}})
            .toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() ||
        !file.commit()) {
        QMessageBox::warning(this, "Create Scene",
                             "The scene could not be created.");
        return;
    }
    if (viewportPanel != nullptr && viewportPanel->openRuntimeScene(path) &&
        viewportTools != nullptr) {
        viewportTools->refreshSceneTabs();
        viewportTools->openSceneTab(path);
    }
}

void EditorWindow::openScene() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Atlas Scene", QFileInfo(projectFile).absolutePath(),
        "Atlas Scene (*.ascene)");
    if (!path.isEmpty() && viewportPanel != nullptr &&
        viewportPanel->openRuntimeScene(path) && viewportTools != nullptr) {
        viewportTools->openSceneTab(path);
    }
}

void EditorWindow::saveSceneAs() {
    if (viewportPanel == nullptr)
        return;
    QString path = QFileDialog::getSaveFileName(
        this, "Save Atlas Scene As", viewportPanel->currentRuntimeScene(),
        "Atlas Scene (*.ascene)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".ascene", Qt::CaseInsensitive))
        path += ".ascene";
    if (viewportPanel->saveRuntimeSceneAs(path) && viewportTools != nullptr) {
        viewportTools->refreshSceneTabs();
        viewportTools->openSceneTab(path);
    }
}

void EditorWindow::showProjectSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle("Project Settings");
    dialog.resize(720, 520);
    auto *layout = new QVBoxLayout(&dialog);
    auto *tabs = new QTabWidget(&dialog);
    const QString settingsDirectory =
        QDir(QFileInfo(projectFile).absolutePath()).filePath(".atlas");
    QDir().mkpath(settingsDirectory);
    QSettings settings(QDir(settingsDirectory).filePath("project-settings.ini"),
                       QSettings::IniFormat);
    auto addPage = [tabs](const QString &name) {
        auto *page = new QWidget(tabs);
        auto *form = new QFormLayout(page);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        tabs->addTab(page, name);
        return form;
    };
    auto *general = addPage("General");
    auto *defaultScene = new QComboBox(&dialog);
    QDirIterator sceneIterator(QFileInfo(projectFile).absolutePath(),
                               {"*.ascene"}, QDir::Files,
                               QDirIterator::Subdirectories);
    while (sceneIterator.hasNext()) {
        const QString scene = sceneIterator.next();
        defaultScene->addItem(
            QDir(QFileInfo(projectFile).absolutePath()).relativeFilePath(scene),
            scene);
    }
    const QString configuredScene =
        settings.value("project/defaultScene", "main.ascene").toString();
    int defaultSceneIndex = defaultScene->findText(configuredScene);
    if (defaultSceneIndex < 0)
        defaultSceneIndex = 0;
    defaultScene->setCurrentIndex(defaultSceneIndex);
    auto *companyName = new QLineEdit(
        settings.value("project/company", "Neutral Software").toString(),
        &dialog);
    auto *gameVersion = new QLineEdit(
        settings.value("project/version", "1.0.0").toString(), &dialog);
    auto *windowWidth = new QSpinBox(&dialog);
    windowWidth->setRange(320, 16384);
    windowWidth->setValue(settings.value("project/windowWidth", 1280).toInt());
    auto *windowHeight = new QSpinBox(&dialog);
    windowHeight->setRange(240, 16384);
    windowHeight->setValue(settings.value("project/windowHeight", 720).toInt());
    auto *fullscreen = new QCheckBox("Start in fullscreen", &dialog);
    fullscreen->setChecked(
        settings.value("project/fullscreen", false).toBool());
    general->addRow("Default scene", defaultScene);
    general->addRow("Company", companyName);
    general->addRow("Version", gameVersion);
    general->addRow("Window width", windowWidth);
    general->addRow("Window height", windowHeight);
    general->addRow(QString(), fullscreen);
    auto *rendering = addPage("Rendering");
    auto *renderer = new QComboBox(&dialog);
    renderer->addItems({"PBR", "PBR + DDGI", "Path Tracing"});
    renderer->setCurrentText(settings.value("project/renderer", "PBR").toString());
    auto *frameLimit = new QSpinBox(&dialog);
    frameLimit->setRange(0, 1000);
    frameLimit->setValue(settings.value("project/frameLimit", 0).toInt());
    rendering->addRow("Renderer", renderer);
    rendering->addRow("Frame limit (0 = unlimited)", frameLimit);
    auto *physics = addPage("Physics");
    auto *gravity = new QLineEdit(settings.value("project/gravity", "0, -9.81, 0").toString(), &dialog);
    auto *fixedStep = new QLineEdit(settings.value("project/fixedStep", "0.0166667").toString(), &dialog);
    physics->addRow("Gravity", gravity);
    physics->addRow("Fixed timestep", fixedStep);
    auto *input = addPage("Input");
    auto *inputMap = new QLineEdit(settings.value("project/inputMap", "input.json").toString(), &dialog);
    auto *controller = new QComboBox(&dialog);
    controller->addItems({"Automatic", "Keyboard + Mouse", "Gamepad"});
    controller->setCurrentText(settings.value("project/controller", "Automatic").toString());
    input->addRow("Input map", inputMap);
    input->addRow("Primary controller", controller);
    auto *build = addPage("Build & Run");
    auto *buildCommand = new QLineEdit(settings.value("project/buildCommand", "atlas pack --backend METAL").toString(), &dialog);
    auto *runCommand = new QLineEdit(settings.value("project/runCommand", "atlas run project.atlas").toString(), &dialog);
    build->addRow("Build command", buildCommand);
    build->addRow("Run command", runCommand);
    auto *editor = addPage("Editor");
    auto *autosave = new QSpinBox(&dialog);
    autosave->setRange(0, 120);
    autosave->setValue(settings.value("project/autosaveMinutes", 5).toInt());
    auto *snap = new QLineEdit(settings.value("project/snapIncrement", "0.5").toString(), &dialog);
    editor->addRow("Autosave interval (minutes)", autosave);
    editor->addRow("Transform snapping", snap);
    auto *packaging = addPage("Packaging");
    auto *identifier = new QLineEdit(
        settings.value("project/bundleIdentifier",
                       "org.atlasengine." + projectName.toLower().replace(' ', '-'))
            .toString(),
        &dialog);
    auto *iconPath = new QLineEdit(
        settings.value("project/icon", "none").toString(), &dialog);
    auto *backend = new QComboBox(&dialog);
    backend->addItems({"METAL", "VULKAN", "OPENGL"});
    backend->setCurrentText(
        settings.value("project/exportBackend", "METAL").toString());
    packaging->addRow("Bundle identifier", identifier);
    packaging->addRow("Application icon", iconPath);
    packaging->addRow("Renderer backend", backend);
    layout->addWidget(tabs);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Cancel | QDialogButtonBox::Save, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    if (dialog.exec() != QDialog::Accepted)
        return;
    settings.setValue("project/defaultScene", defaultScene->currentText());
    settings.setValue("project/company", companyName->text());
    settings.setValue("project/version", gameVersion->text());
    settings.setValue("project/windowWidth", windowWidth->value());
    settings.setValue("project/windowHeight", windowHeight->value());
    settings.setValue("project/fullscreen", fullscreen->isChecked());
    settings.setValue("project/renderer", renderer->currentText());
    settings.setValue("project/frameLimit", frameLimit->value());
    settings.setValue("project/gravity", gravity->text());
    settings.setValue("project/fixedStep", fixedStep->text());
    settings.setValue("project/inputMap", inputMap->text());
    settings.setValue("project/controller", controller->currentText());
    settings.setValue("project/buildCommand", buildCommand->text());
    settings.setValue("project/runCommand", runCommand->text());
    settings.setValue("project/autosaveMinutes", autosave->value());
    settings.setValue("project/snapIncrement", snap->text());
    settings.setValue("project/bundleIdentifier", identifier->text());
    settings.setValue("project/icon", iconPath->text());
    settings.setValue("project/exportBackend", backend->currentText());
    settings.sync();
    QFile manifest(projectFile);
    if (manifest.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines =
            QString::fromUtf8(manifest.readAll()).split('\n');
        manifest.close();
        setTomlValue(&lines, QString(), "backend",
                     tomlQuoted(backend->currentText()));
        setTomlValue(&lines, "game", "main_scene",
                     tomlQuoted(defaultScene->currentText()));
        setTomlValue(&lines, "pack", "identifier",
                     tomlQuoted(identifier->text()));
        setTomlValue(&lines, "pack", "version",
                     tomlQuoted(gameVersion->text()));
        setTomlValue(&lines, "pack", "icon",
                     tomlQuoted(iconPath->text()));
        setTomlValue(
            &lines, "window", "dimensions",
            QStringLiteral("[%1, %2]")
                .arg(windowWidth->value())
                .arg(windowHeight->value()));
        setTomlValue(&lines, "window", "fullscreen",
                     fullscreen->isChecked() ? "true" : "false");
        const QString rendererName =
            renderer->currentText() == "Path Tracing" ? "pathtracing"
                                                       : "deferred";
        setTomlValue(&lines, "renderer", "default",
                     tomlQuoted(rendererName));
        setTomlValue(&lines, "renderer", "global_illumination",
                     renderer->currentText() == "PBR + DDGI" ? "true"
                                                               : "false");
        QSaveFile outputFile(projectFile);
        const QByteArray contents = lines.join('\n').toUtf8();
        if (!outputFile.open(QIODevice::WriteOnly) ||
            outputFile.write(contents) != contents.size() ||
            !outputFile.commit()) {
            QMessageBox::warning(this, "Project Settings",
                                 "The project manifest could not be updated.");
        }
    }
}

void EditorWindow::showExportDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Export Atlas Project");
    dialog.resize(660, 460);
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *platform = new QComboBox(&dialog);
#ifdef Q_OS_MACOS
    platform->addItem("macOS");
#elif defined(Q_OS_WIN)
    platform->addItem("Windows");
#else
    platform->addItem("Linux");
#endif
    platform->setEnabled(false);
    auto *configuration = new QComboBox(&dialog);
    configuration->addItems({"Release", "Debug"});
    const QString settingsDirectory =
        QDir(QFileInfo(projectFile).absolutePath()).filePath(".atlas");
    QDir().mkpath(settingsDirectory);
    QSettings settings(
        QDir(settingsDirectory).filePath("project-settings.ini"),
        QSettings::IniFormat);
    auto *backend = new QComboBox(&dialog);
    backend->addItems({"METAL", "VULKAN", "OPENGL"});
    backend->setCurrentText(
        settings.value("project/exportBackend", "METAL").toString());
    auto *output = new QLineEdit(
        settings
            .value("project/exportDirectory",
                   QDir(QFileInfo(projectFile).absolutePath())
                       .filePath("Exports"))
            .toString(),
        &dialog);
    auto *browse = new QPushButton("Choose…", &dialog);
    auto *outputRow = new QWidget(&dialog);
    auto *outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->addWidget(output, 1);
    outputLayout->addWidget(browse);
    form->addRow("Platform", platform);
    form->addRow("Configuration", configuration);
    form->addRow("Backend", backend);
    form->addRow("Destination", outputRow);
    layout->addLayout(form);
    auto *summary = new QLabel(
        "Atlas will save the current scene and package the configured runtime with the project resources.",
        &dialog);
    summary->setWordWrap(true);
    layout->addWidget(summary);
    auto *progress = new QProgressBar(&dialog);
    progress->setRange(0, 0);
    progress->setVisible(false);
    layout->addWidget(progress);
    auto *log = new QPlainTextEdit(&dialog);
    log->setReadOnly(true);
    log->setPlaceholderText("Packaging output will appear here.");
    layout->addWidget(log, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto *exportButton = buttons->addButton("Export", QDialogButtonBox::AcceptRole);
    auto *revealButton = buttons->addButton("Reveal Export", QDialogButtonBox::ActionRole);
    revealButton->setEnabled(false);
    layout->addWidget(buttons);
    connect(browse, &QPushButton::clicked, &dialog, [&dialog, output] {
        const QString directory = QFileDialog::getExistingDirectory(
            &dialog, "Export Destination", output->text());
        if (!directory.isEmpty())
            output->setText(directory);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(revealButton, &QPushButton::clicked, &dialog, [output] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(output->text()));
    });
    auto *process = new QProcess(&dialog);
    connect(process, &QProcess::readyReadStandardOutput, &dialog,
            [process, log] {
                log->appendPlainText(
                    QString::fromUtf8(process->readAllStandardOutput()).trimmed());
            });
    connect(process, &QProcess::readyReadStandardError, &dialog,
            [process, log] {
                log->appendPlainText(
                    QString::fromUtf8(process->readAllStandardError()).trimmed());
            });
    connect(process, &QProcess::errorOccurred, &dialog,
            [process, progress, exportButton, log](QProcess::ProcessError) {
                progress->setVisible(false);
                exportButton->setEnabled(true);
                log->appendPlainText(process->errorString());
            });
    connect(process, &QProcess::finished, &dialog,
            [this, process, progress, exportButton, revealButton, output,
             log](int exitCode, QProcess::ExitStatus status) {
                progress->setVisible(false);
                exportButton->setEnabled(true);
                if (status != QProcess::NormalExit || exitCode != 0) {
                    log->appendPlainText("Export failed.");
                    return;
                }
                const QString dist =
                    QDir(QFileInfo(projectFile).absolutePath()).filePath("dist");
                QDir destination(output->text());
                if (!destination.exists() && !QDir().mkpath(destination.path())) {
                    log->appendPlainText("Could not create the export destination.");
                    return;
                }
                const QFileInfoList packages = QDir(dist).entryInfoList(
                    QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden);
                if (packages.isEmpty()) {
                    log->appendPlainText("Atlas Pack produced no distributable files.");
                    return;
                }
                QString error;
                for (const QFileInfo &package : packages) {
                    const QString target = destination.filePath(package.fileName());
                    if (QFileInfo(target).isDir())
                        QDir(target).removeRecursively();
                    else
                        QFile::remove(target);
                    if (!copyExportPath(package.absoluteFilePath(), target,
                                        &error)) {
                        log->appendPlainText(error);
                        return;
                    }
                }
                log->appendPlainText(
                    QStringLiteral("Export complete: %1").arg(destination.path()));
                revealButton->setEnabled(true);
            });
    connect(exportButton, &QPushButton::clicked, &dialog,
            [this, process, output, platform, configuration, backend, progress,
             exportButton, revealButton, log] {
        const QString program = atlasCliPath();
        QStringList arguments;
        if (program.isEmpty()) {
            QMessageBox::warning(
                this, "Export Project",
                "Atlas CLI was not found. Install it or place it beside Atlas Editor.");
            return;
        }
        QSettings settings(
            QDir(QFileInfo(projectFile).absolutePath())
                .filePath(".atlas/project-settings.ini"),
            QSettings::IniFormat);
        settings.setValue("project/exportDirectory", output->text());
        settings.setValue("project/exportPlatform", platform->currentText());
        settings.setValue("project/exportConfiguration",
                          configuration->currentText());
        settings.setValue("project/exportBackend", backend->currentText());
        settings.sync();
        if (viewportPanel != nullptr)
            viewportPanel->saveRuntimeScene();
        log->clear();
        log->appendPlainText("Starting Atlas Pack…");
        progress->setVisible(true);
        exportButton->setEnabled(false);
        revealButton->setEnabled(false);
        arguments << "pack" << "--backend" << backend->currentText();
        if (configuration->currentText() == "Release")
            arguments << "--release" << "1";
        process->setWorkingDirectory(QFileInfo(projectFile).absolutePath());
        process->start(program, arguments);
    });
    connect(&dialog, &QDialog::finished, process, [process] {
        if (process->state() == QProcess::NotRunning)
            return;
        process->terminate();
        if (!process->waitForFinished(1000)) {
            process->kill();
            process->waitForFinished(1000);
        }
    });
    dialog.exec();
}

void EditorWindow::showCommandPalette() {
    QDialog dialog(this);
    dialog.setWindowTitle("Command Palette");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(620, 430);
    auto *layout = new QVBoxLayout(&dialog);
    auto *search = new QLineEdit(&dialog);
    search->setPlaceholderText("Type a command…");
    auto *commands = new QListWidget(&dialog);
    layout->addWidget(search);
    layout->addWidget(commands, 1);
    const QList<QAction *> actions = findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->text().isEmpty() || action->isSeparator() ||
            !action->isEnabled()) {
            continue;
        }
        auto *item = new QListWidgetItem(commands);
        item->setText(action->text().remove('&'));
        item->setData(Qt::UserRole,
                      QVariant::fromValue<quintptr>(
                          reinterpret_cast<quintptr>(action)));
        const QString shortcut =
            !action->shortcut().isEmpty()
                ? action->shortcut().toString(QKeySequence::NativeText)
                : action->property("atlasShortcut").toString();
        if (!shortcut.isEmpty())
            item->setText(item->text() + "\t" +
                          shortcut);
    }
    connect(search, &QLineEdit::textChanged, &dialog,
            [commands](const QString &text) {
                for (int index = 0; index < commands->count(); ++index) {
                    QListWidgetItem *item = commands->item(index);
                    item->setHidden(!item->text().contains(
                        text, Qt::CaseInsensitive));
                }
                for (int index = 0; index < commands->count(); ++index) {
                    if (!commands->item(index)->isHidden()) {
                        commands->setCurrentRow(index);
                        break;
                    }
                }
            });
    connect(commands, &QListWidget::itemActivated, &dialog,
            [&dialog](QListWidgetItem *item) {
                auto *action = reinterpret_cast<QAction *>(
                    item->data(Qt::UserRole).value<quintptr>());
                dialog.accept();
                if (action != nullptr)
                    action->trigger();
            });
    connect(search, &QLineEdit::returnPressed, &dialog, [commands] {
        if (commands->currentItem() != nullptr)
            emit commands->itemActivated(commands->currentItem());
    });
    auto moveSelection = [commands](int direction) {
        if (commands->count() == 0)
            return;
        int row = commands->currentRow();
        for (int attempt = 0; attempt < commands->count(); ++attempt) {
            row = (row + direction + commands->count()) % commands->count();
            if (!commands->item(row)->isHidden()) {
                commands->setCurrentRow(row);
                return;
            }
        }
    };
    auto *down = new QShortcut(QKeySequence(Qt::Key_Down), &dialog);
    auto *up = new QShortcut(QKeySequence(Qt::Key_Up), &dialog);
    connect(down, &QShortcut::activated, &dialog,
            [moveSelection] { moveSelection(1); });
    connect(up, &QShortcut::activated, &dialog,
            [moveSelection] { moveSelection(-1); });
    search->setFocus();
    dialog.exec();
}

void EditorWindow::showGlobalSearch() {
    QDialog dialog(this);
    dialog.setWindowTitle("Search Atlas Project");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(680, 460);
    auto *layout = new QVBoxLayout(&dialog);
    auto *search = new QLineEdit(&dialog);
    search->setPlaceholderText("Search scenes, assets, and commands…");
    auto *results = new QListWidget(&dialog);
    layout->addWidget(search);
    layout->addWidget(results, 1);
    constexpr int SearchKindRole = Qt::UserRole + 1;
    constexpr int SearchValueRole = Qt::UserRole + 2;
    QDirIterator iterator(QFileInfo(projectFile).absolutePath(), QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        const QString relative =
            QDir(QFileInfo(projectFile).absolutePath()).relativeFilePath(path);
        if (relative.startsWith(".git/") || relative.startsWith("build/") ||
            relative.startsWith("dist/"))
            continue;
        auto *item = new QListWidgetItem(
            QStringLiteral("Asset  %1").arg(relative), results);
        item->setToolTip(path);
        item->setData(SearchKindRole, 0);
        item->setData(SearchValueRole, path);
    }
    const QJsonDocument snapshot = viewportPanel != nullptr
                                       ? QJsonDocument::fromJson(
                                             viewportPanel->currentSceneSnapshot()
                                                 .toUtf8())
                                       : QJsonDocument();
    std::function<void(const QJsonArray &)> addObjects;
    addObjects = [&addObjects, results](const QJsonArray &objects) {
        for (const QJsonValue &value : objects) {
            const QJsonObject object = value.toObject();
            const QString name = object.value("name").toString();
            const int id = object.value("id").toInt(-1);
            if (!name.isEmpty() && id >= 0) {
                auto *item = new QListWidgetItem(
                    QStringLiteral("Object  %1").arg(name), results);
                item->setToolTip(object.value("type").toString());
                item->setData(SearchKindRole, 1);
                item->setData(SearchValueRole, id);
            }
            addObjects(object.value("children").toArray());
        }
    };
    addObjects(snapshot.object().value("objects").toArray());
    const QList<QAction *> actions = findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->text().isEmpty() || action->isSeparator() ||
            !action->isEnabled())
            continue;
        auto *item = new QListWidgetItem(
            QStringLiteral("Command  %1").arg(action->text().remove('&')),
            results);
        item->setData(SearchKindRole, 2);
        item->setData(
            SearchValueRole,
            QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(action)));
    }
    connect(search, &QLineEdit::textChanged, &dialog,
            [results](const QString &text) {
                for (int index = 0; index < results->count(); ++index) {
                    QListWidgetItem *item = results->item(index);
                    item->setHidden(!item->text().contains(
                                        text, Qt::CaseInsensitive) &&
                                    !item->toolTip().contains(
                                        text, Qt::CaseInsensitive));
                }
                for (int index = 0; index < results->count(); ++index) {
                    if (!results->item(index)->isHidden()) {
                        results->setCurrentRow(index);
                        break;
                    }
                }
            });
    connect(results, &QListWidget::itemActivated, &dialog,
            [this, &dialog](QListWidgetItem *item) {
                const int kind = item->data(Qt::UserRole + 1).toInt();
                dialog.accept();
                if (kind == 1 && viewportPanel != nullptr) {
                    const int id = item->data(Qt::UserRole + 2).toInt();
                    viewportPanel->selectRuntimeObject(id, false);
                    viewportPanel->focusRuntimeObjects({id});
                    return;
                }
                if (kind == 2) {
                    auto *action = reinterpret_cast<QAction *>(
                        item->data(Qt::UserRole + 2).value<quintptr>());
                    if (action != nullptr)
                        action->trigger();
                    return;
                }
                const QString path =
                    item->data(Qt::UserRole + 2).toString();
                if (path.endsWith(".ascene", Qt::CaseInsensitive) &&
                    viewportPanel != nullptr) {
                    if (viewportPanel->openRuntimeScene(path) &&
                        viewportTools != nullptr)
                        viewportTools->openSceneTab(path);
                } else {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                }
            });
    connect(search, &QLineEdit::returnPressed, &dialog, [results] {
        if (results->currentItem() != nullptr)
            emit results->itemActivated(results->currentItem());
    });
    auto moveSelection = [results](int direction) {
        if (results->count() == 0)
            return;
        int row = results->currentRow();
        for (int attempt = 0; attempt < results->count(); ++attempt) {
            row = (row + direction + results->count()) % results->count();
            if (!results->item(row)->isHidden()) {
                results->setCurrentRow(row);
                return;
            }
        }
    };
    auto *down = new QShortcut(QKeySequence(Qt::Key_Down), &dialog);
    auto *up = new QShortcut(QKeySequence(Qt::Key_Up), &dialog);
    connect(down, &QShortcut::activated, &dialog,
            [moveSelection] { moveSelection(1); });
    connect(up, &QShortcut::activated, &dialog,
            [moveSelection] { moveSelection(-1); });
    if (results->count() > 0)
        results->setCurrentRow(0);
    search->setFocus();
    dialog.exec();
}

void EditorWindow::runProjectCommand(bool buildOnly) {
    const QString settingsDirectory =
        QDir(QFileInfo(projectFile).absolutePath()).filePath(".atlas");
    QDir().mkpath(settingsDirectory);
    QSettings settings(QDir(settingsDirectory).filePath("project-settings.ini"),
                       QSettings::IniFormat);
    const QString command =
        settings.value(buildOnly ? "project/buildCommand"
                                 : "project/runCommand",
                       buildOnly ? "atlas pack --backend METAL"
                                 : "atlas run project.atlas")
            .toString()
            .trimmed();
    if (command.isEmpty())
        return;
    if (viewportPanel != nullptr)
        viewportPanel->saveRuntimeScene();
    QProcess::startDetached("/bin/zsh", {"-lc", command},
                            QFileInfo(projectFile).absolutePath());
}

void EditorWindow::takeViewportScreenshot() {
    if (viewportPanel == nullptr)
        return;
    const QString path = QFileDialog::getSaveFileName(
        this, "Save Viewport Screenshot",
        QDir(QFileInfo(projectFile).absolutePath())
            .filePath("Atlas Viewport " +
                      QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") +
                      ".png"),
        "PNG Image (*.png)");
    if (!path.isEmpty())
        viewportPanel->grab().save(path, "PNG");
}

void EditorWindow::refreshScriptWatcher() {
    if (scriptWatcher == nullptr)
        return;
    const QStringList existing = scriptWatcher->files();
    if (!existing.isEmpty())
        scriptWatcher->removePaths(existing);
    QStringList scripts;
    QDirIterator iterator(QFileInfo(projectFile).absolutePath(),
                          {"*.js", "*.ts"}, QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
        scripts.append(iterator.next());
    if (!scripts.isEmpty())
        scriptWatcher->addPaths(scripts);
}

bool EditorWindow::contentBrowserHasFocus() const {
    QWidget *focused = QApplication::focusWidget();
    return contentBrowser != nullptr && focused != nullptr &&
           (focused == contentBrowser || contentBrowser->isAncestorOf(focused));
}

bool EditorWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        QWidget *focused = QApplication::focusWidget();
        const bool typing = qobject_cast<QLineEdit *>(focused) != nullptr;
        if (!typing && key->key() == Qt::Key_Tab &&
            key->modifiers() == Qt::NoModifier && !key->isAutoRepeat() &&
            hierarchyPanel != nullptr) {
            hierarchyPanel->focusSelectedObject();
            return true;
        }
        if (!typing && key->modifiers() == Qt::ShiftModifier &&
            !key->isAutoRepeat()) {
            if (key->key() == Qt::Key_N && hierarchyPanel != nullptr)
                hierarchyPanel->createObject("group", "Empty Object");
            else if (key->key() == Qt::Key_C && hierarchyPanel != nullptr)
                hierarchyPanel->createObject("camera", "Camera");
            else if (key->key() == Qt::Key_L && hierarchyPanel != nullptr)
                hierarchyPanel->createObject("pointLight", "Point Light");
            else if (key->key() == Qt::Key_A && hierarchyPanel != nullptr)
                hierarchyPanel->showCreationPopup();
            else if (key->key() == Qt::Key_T && viewportPanel != nullptr)
                viewportPanel->toggleTransformSpace();
            else if (key->key() == Qt::Key_R && viewportPanel != nullptr) {
                const int child = viewportPanel->selectedRuntimeObjectId();
                bool accepted = false;
                const int parent = QInputDialog::getInt(
                    this, "Reparent Object",
                    "Parent object runtime ID (-1 for root)", -1, -1,
                    std::numeric_limits<int>::max(), 1, &accepted);
                if (accepted && child >= 0)
                    viewportPanel->setRuntimeObjectParent(child, parent);
            } else {
                return QMainWindow::eventFilter(watched, event);
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
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
