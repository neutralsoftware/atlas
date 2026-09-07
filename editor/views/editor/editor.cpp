#include "editor/styling/workbench.h"

/*
 * editor.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Editor view and window
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/editorWindow.h>

#include <editor/application/toolchainInstaller.h>
#include <editor/project/projectStore.h>

#include <QAction>
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
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
#include <QFrame>
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
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QShowEvent>
#include <QShortcut>
#include <QSplitter>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QCloseEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QInputDialog>
#include <QImageReader>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QUrl>

#include <functional>
#include <limits>

#include "DockManager.h"
#include "DockAreaWidget.h"
#include "editor/debug.h"
#include "editor/styling/icons.h"
#include "editor/views/fileExplorer.h"
#include "editor/views/hierarchyPanel.h"
#include "editor/views/inspectorView.h"
#include "editor/views/inputActionsDialog.h"
#include "editor/views/materialEditor.h"
#include "editor/views/graphiteEditor.h"
#include "editor/views/postProcessing.h"
#include "editor/views/viewport.h"
#include "editor/views/viewportTools.h"
#include "editor/views/splashScreen.h"

namespace {
constexpr int DockStateVersion = 11;
constexpr auto DockStateKey = "docking/state/v11";

QIcon commandIcon(const QString &name) {
    const QString command = name.toLower();
    if (command.contains("save"))
        return styling::icon(styling::Icon::FloppyDisk, "#A1957D");
    if (command.contains("open"))
        return styling::icon(styling::Icon::FolderOpen, "#7E929C");
    if (command.contains("new") || command.contains("create") ||
        command.contains("add"))
        return styling::icon(styling::Icon::Plus, "#8498A8");
    if (command.contains("export"))
        return styling::icon(styling::Icon::Export, "#7E929C");
    if (command.contains("build"))
        return styling::icon(styling::Icon::Package, "#A1957D");
    if (command.contains("run") || command.contains("play"))
        return styling::icon(styling::Icon::RocketLaunch, "#849589");
    if (command.contains("stop"))
        return styling::icon(styling::Icon::Stop, "#A17F7F");
    if (command.contains("reload") || command.contains("refresh") ||
        command.contains("undo"))
        return styling::icon(styling::Icon::ArrowCounterClockwise, "#7E929C");
    if (command.contains("redo"))
        return styling::icon(styling::Icon::ArrowClockwise, "#7E929C");
    if (command.contains("settings"))
        return styling::icon(styling::Icon::Gear, "#8498A8");
    if (command.contains("find") || command.contains("search") ||
        command.contains("palette"))
        return styling::icon(styling::Icon::MagnifyingGlass, "#7E929C");
    if (command.contains("screenshot"))
        return styling::icon(styling::Icon::Camera, "#9E897D");
    if (command.contains("delete") || command.contains("remove"))
        return styling::icon(styling::Icon::Trash, "#A17F7F");
    if (command.contains("layout") || command.contains("window"))
        return styling::icon(styling::Icon::Layout, "#8498A8");
    if (command.contains("close") || command.contains("quit"))
        return styling::icon(styling::Icon::Close, "#A17F7F");
    if (command.contains("camera"))
        return styling::icon(styling::Icon::Camera, "#9E897D");
    if (command.contains("light"))
        return styling::icon(styling::Icon::Lightbulb, "#A1957D");
    if (command.contains("object"))
        return styling::icon(styling::Icon::Cube, "#8498A8");
    return {};
}

bool copyExportPath(const QString &sourcePath, const QString &destinationPath,
                    QString *error) {
    const QFileInfo source(sourcePath);
    if (source.isDir()) {
        if (!QDir().mkpath(destinationPath)) {
            if (error != nullptr)
                *error =
                    QStringLiteral("Could not create %1").arg(destinationPath);
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

QString tomlValue(const QStringList &lines, const QString &section,
                  const QString &key) {
    int start = 0;
    int end = lines.size();
    if (!section.isEmpty()) {
        start = lines.indexOf(QStringLiteral("[%1]").arg(section));
        if (start < 0)
            return {};
        ++start;
    }
    for (int index = start; index < lines.size(); ++index) {
        if (lines.at(index).trimmed().startsWith('[')) {
            end = index;
            break;
        }
    }
    const QRegularExpression expression(
        QStringLiteral("^\\s*%1\\s*=\\s*(.+?)\\s*$")
            .arg(QRegularExpression::escape(key)));
    for (int index = start; index < end; ++index) {
        const auto match = expression.match(lines.at(index));
        if (!match.hasMatch())
            continue;
        QString value = match.captured(1).trimmed();
        if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"'))
            value = value.mid(1, value.size() - 2);
        return value;
    }
    return {};
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
} // namespace

#ifndef ATLAS_VERSION
#define ATLAS_VERSION "No version"
#endif

#ifndef ATLAS_BUILD_STRING
#define ATLAS_BUILD_STRING ""
#endif

EditorWindow::EditorWindow(const QString &projectFile, QWidget *parent)
    : QMainWindow(parent), projectFile(projectFile) {
    setupWindow();
    setupMenus();
    setupDocks();
    setupWorkspaceBar();

    restoreLayout();
    qApp->installEventFilter(this);
    refreshScriptWatcher();
}

void EditorWindow::setupWindow() {
    const auto project = ProjectStore::projectInfo(projectFile);
    projectName =
        project.has_value() ? project->name : QStringLiteral("Project");
    updateWindowTitle(false);
    setMinimumSize(1100, 700);
    resize(1512, 940);
    menuBar()->setNativeMenuBar(true);
    menuBar()->setObjectName("atlasMenuBar");

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
    auto *workspaceFrame = new QWidget(this);
    workspaceFrame->setObjectName("editorFrame");
    auto *workspaceLayout = new QVBoxLayout(workspaceFrame);
    workspaceLayout->setContentsMargins(10, 0, 10, 0);
    workspaceLayout->setSpacing(0);
    workspaceLayout->addWidget(coreManager);
    setCentralWidget(workspaceFrame);
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
        action->setIcon(commandIcon(name));
        if (!shortcut.isEmpty()) {
            const bool plainShift =
                shortcut.startsWith("Shift+") && !shortcut.contains("Meta+") &&
                !shortcut.contains("Alt+") && !shortcut.contains("Ctrl+");
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
    addCommand(fileMenu, "New Graphite UI", QString(), [this] {
        if (contentBrowser != nullptr)
            contentBrowser->createUI();
    });
    addCommand(fileMenu, "Open Scene…", "Meta+O", [this] { openScene(); });
    auto *saveAction = fileMenu->addAction("Save Scene");
    saveAction->setIcon(styling::icon(styling::Icon::FloppyDisk, "#A1957D"));
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveAction, &QAction::triggered, this, [this] {
        if (materialEditorPanel != nullptr &&
            materialEditorPanel->isVisible()) {
            materialEditorPanel->saveMaterial();
        }
        if (graphiteEditorPanel != nullptr &&
            graphiteEditorPanel->isVisible()) {
            graphiteEditorPanel->saveUI();
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
    undoAction->setIcon(
        styling::icon(styling::Icon::ArrowCounterClockwise, "#7E929C"));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(undoAction, &QAction::triggered, this,
            &EditorWindow::undoActiveEditor);
    auto *redoAction = editMenu->addAction("Redo");
    redoAction->setIcon(
        styling::icon(styling::Icon::ArrowClockwise, "#7E929C"));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(redoAction, &QAction::triggered, this,
            &EditorWindow::redoActiveEditor);
    editMenu->addSeparator();
    addCommand(editMenu, "Find…", "Meta+F", [this] { showGlobalSearch(); });
    editMenu->addSeparator();
    addCommand(editMenu, "Cut", "Meta+X", [this] {
        if (auto *field =
                qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->cut();
        else if (contentBrowserHasFocus())
            contentBrowser->cutSelection();
        else if (viewportPanel != nullptr)
            viewportPanel->cutSelectedRuntimeObject();
    });
    addCommand(editMenu, "Copy", "Meta+C", [this] {
        if (auto *field =
                qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->copy();
        else if (contentBrowserHasFocus())
            contentBrowser->copySelection();
        else if (viewportPanel != nullptr)
            viewportPanel->copySelectedRuntimeObject();
    });
    addCommand(editMenu, "Paste", "Meta+V", [this] {
        if (auto *field =
                qobject_cast<QLineEdit *>(QApplication::focusWidget()))
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
        if (auto *field =
                qobject_cast<QLineEdit *>(QApplication::focusWidget()))
            field->backspace();
        else if (contentBrowserHasFocus())
            contentBrowser->deleteSelection();
        else if (hierarchyPanel != nullptr)
            hierarchyPanel->deleteSelectedObject();
    });
    addCommand(editMenu, "Select All Objects", "Meta+A", [this] {
        if (auto *field =
                qobject_cast<QLineEdit *>(QApplication::focusWidget()))
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
    auto *settingsAction = addCommand(editMenu, "Project Settings…", "Meta+,",
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
    resetLayoutAction->setIcon(styling::icon(styling::Icon::Layout, "#8498A8"));
    connect(resetLayoutAction, &QAction::triggered, this, [this] {
        if (coreManager != nullptr && !defaultDockState.isEmpty()) {
            restoringLayout = true;
            coreManager->restoreState(defaultDockState, DockStateVersion);
            restoringLayout = false;
            configureDockSplitters();
            if (auto *dock = dockManager->panel("workspace"))
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
    auto *toolsSettings = addCommand(toolsMenu, "Project Settings…", QString(),
                                     [this] { showProjectSettings(); });
    toolsSettings->setMenuRole(QAction::NoRole);
    addCommand(toolsMenu, "Install Atlas Toolchain…", QString(),
               [this] { ToolchainInstaller::install(this); });
    addCommand(toolsMenu, "Command Palette…", "Meta+Shift+P",
               [this] { showCommandPalette(); });

    windowMenu = menuBar()->addMenu("Window");
    windowMenu->addAction("Minimize", this, &QWidget::showMinimized);
    windowMenu->addAction("Zoom", this, [this] {
        isMaximized() ? showNormal() : showMaximized();
    });
    windowMenu->addSeparator();
    windowMenu->addAction("Controller Actions", this,
                          [this] { showInputActions(); });

    auto *helpMenu = menuBar()->addMenu("Help");
    auto *aboutAction = helpMenu->addAction("About Atlas Engine", this, [this] {
        QMessageBox::about(
            this, "About Atlas Engine",
            QStringLiteral("Atlas Engine %1\nby Neutral Software")
                .arg(QStringLiteral(ATLAS_VERSION)));
    });
    aboutAction->setIcon(styling::icon(styling::Icon::Info, "#7E929C"));
    aboutAction->setMenuRole(QAction::AboutRole);
}

void EditorWindow::showInputActions() {
    if (inputActionsDialog != nullptr) {
        inputActionsDialog->showNormal();
        inputActionsDialog->raise();
        inputActionsDialog->activateWindow();
        return;
    }
    inputActionsDialog = new InputActionsDialog(projectFile, this);
    inputActionsDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(inputActionsDialog, &InputActionsDialog::actionsSaved, this,
            [this] {
                if (viewportPanel != nullptr)
                    viewportPanel->reloadRuntime();
            });
    connect(inputActionsDialog, &QObject::destroyed, this,
            [this] { inputActionsDialog = nullptr; });
    inputActionsDialog->show();
    inputActionsDialog->raise();
    inputActionsDialog->activateWindow();
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
    connect(viewportPanel, &ViewportPanel::runtimeLoadingStarted, this, [this] {
        if (!startupComplete || assetLoadingSplash != nullptr) {
            return;
        }
        assetLoadingSplash = new SplashScreen(this);
        assetLoadingSplash->start("Loading assets...");
    });
    connect(viewportPanel, &ViewportPanel::runtimeLoadingStatusChanged, this,
            [this](const QString &status) {
                emit startupStatusChanged(status);
                if (assetLoadingSplash != nullptr) {
                    assetLoadingSplash->setStatus(status);
                }
            });
    connect(viewportPanel, &ViewportPanel::runtimeLoadingFinished, this,
            [this] {
                if (assetLoadingSplash == nullptr) {
                    return;
                }
                assetLoadingSplash->finish();
                assetLoadingSplash->deleteLater();
                assetLoadingSplash = nullptr;
            });
    viewportTools = new ViewportTools(viewportPanel, projectFile);
    materialEditorPanel = new MaterialEditorPanel(viewportPanel);
    postProcessingPanel = new PostProcessingPanel(viewportPanel);
    graphiteEditorPanel = new GraphiteEditorPanel(viewportPanel, projectFile);
    connect(materialEditorPanel, &MaterialEditorPanel::materialSaved, this,
            [this](const QString &) { workspaceChangesPending = true; });
    connect(postProcessingPanel, &PostProcessingPanel::settingsChanged, this,
            [this] { workspaceChangesPending = true; });
    connect(graphiteEditorPanel, &GraphiteEditorPanel::documentSaved, this,
            [this] { workspaceChangesPending = true; });
    workspaceStack = new styling::WorkspaceStack(this);
    workspaceStack->setObjectName("editorWorkspaceStack");
    workspaceStack->addWidget(viewportTools);
    workspaceStack->addWidget(materialEditorPanel);
    workspaceStack->addWidget(postProcessingPanel);
    workspaceStack->addWidget(graphiteEditorPanel);
    workspaceStack->setCurrentIndex(0);
    auto *workspaceSurface = new QWidget(this);
    workspaceSurface->setObjectName("workspaceSurface");
    auto *surfaceLayout = new QVBoxLayout(workspaceSurface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->setSpacing(0);
    auto *workspaceHeading = new QWidget(workspaceSurface);
    workspaceHeading->setObjectName("workspaceHeading");
    auto *headingLayout = new QHBoxLayout(workspaceHeading);
    headingLayout->setContentsMargins(8, 5, 8, 5);
    headingLayout->setSpacing(4);
    auto *switcher = new QWidget(workspaceHeading);
    switcher->setObjectName("workspaceSwitcher");
    auto *switcherLayout = new QHBoxLayout(switcher);
    switcherLayout->setContentsMargins(0, 0, 0, 0);
    switcherLayout->setSpacing(3);
    workspaceModeGroup = new QButtonGroup(switcher);
    workspaceModeGroup->setExclusive(true);
    const QStringList modes{"Scene", "Shading", "Effects", "Interface"};
    for (int index = 0; index < modes.size(); ++index) {
        auto *button = new styling::ToolButton(switcher);
        button->setObjectName("workspaceModeButton");
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setText(modes.at(index));
        button->setCheckable(true);
        button->setChecked(index == 0);
        workspaceModeGroup->addButton(button, index);
        switcherLayout->addWidget(button);
        connect(button, &QToolButton::clicked, this,
                [this, index] { activateWorkspace(index); });
    }
    headingLayout->addWidget(switcher);
    headingLayout->addStretch();
    auto *workspaceSearch = new styling::ToolButton(workspaceHeading);
    workspaceSearch->setIcon(styling::icon(styling::Icon::MagnifyingGlass));
    workspaceSearch->setToolTip("Search scenes, objects and assets");
    connect(workspaceSearch, &QToolButton::clicked, this, &EditorWindow::showGlobalSearch);
    headingLayout->addWidget(workspaceSearch);
    surfaceLayout->addWidget(workspaceHeading);
    surfaceLayout->addWidget(workspaceStack, 1);
    auto *workspaceDock = dockManager->addPanel(
        {.id = "workspace",
         .title = "Workspace",
         .widget = workspaceSurface,
         .area = EditorDockArea::Center,
         .icon = styling::icon(styling::Icon::CubeFocus, "#7E929C")});
    workspaceDock->setFeature(ads::CDockWidget::NoTab, true);
    if (auto *area = workspaceDock->dockAreaWidget())
        area->setDockAreaFlag(ads::CDockAreaWidget::HideSingleWidgetTitleBar, true);

    hierarchyPanel = new HierarchyPanel(viewportPanel);
    auto *hierarchyDock = dockManager->addPanel(
        {.id = "hierarchy",
         .title = "Scene Collection",
         .widget = hierarchyPanel,
         .area = EditorDockArea::Left,
         .icon = styling::icon(styling::Icon::TreeStructure, "#8498A8")});

    inspectorPanel = new InspectorPanel(viewportPanel, projectFile);
    auto *inspectorDock = dockManager->addPanel(
        {.id = "inspector",
         .title = "Inspector",
         .widget = inspectorPanel,
         .area = EditorDockArea::Right,
         .icon = styling::icon(styling::Icon::SlidersHorizontal, "#A1957D")});

    contentBrowser = new ContentBrowserPanel(projectFile);
    auto *contentDock = dockManager->addPanel(
        {.id = "fileExplorer",
         .title = "Content Browser",
         .widget = contentBrowser,
         .area = EditorDockArea::Bottom,
         .icon = styling::icon(styling::Icon::FolderOpen, "#7E929C")});

    runtimeErrors = new QPlainTextEdit(this);
    runtimeErrors->setObjectName("runtimeErrors");
    runtimeErrors->setReadOnly(true);
    runtimeErrors->setLineWrapMode(QPlainTextEdit::NoWrap);
    runtimeErrors->setPlaceholderText(
        "Runtime and script errors will appear here.");
    auto *errorDock = dockManager->addPanel(
        {.id = "errors",
         .title = "Errors",
         .widget = runtimeErrors,
         .area = EditorDockArea::Bottom,
         .icon = styling::icon(styling::Icon::Warning, "#A17F7F")});
    if (contentDock->dockAreaWidget() != nullptr) {
        coreManager->addDockWidget(ads::CenterDockWidgetArea, errorDock,
                                   contentDock->dockAreaWidget());
    }
    errorDock->toggleView(false);
    connect(viewportPanel, &ViewportPanel::runtimeErrorOccurred, this,
            [this, errorDock](const QString &message) {
                const QString normalized = message.trimmed();
                if (normalized.isEmpty())
                    return;
                runtimeErrors->appendPlainText(
                    QStringLiteral("[%1] %2")
                        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"),
                             normalized));
                errorDock->toggleView(true);
                errorDock->setAsCurrentTab();
                errorDock->raise();
                statusBar()->showMessage("Runtime error reported", 5000);
            });

    workspaceDock->setAsCurrentTab();
    coreManager->setSplitterSizes(workspaceDock->dockAreaWidget(), {250, 870, 360});
    coreManager->setSplitterSizes(contentDock->dockAreaWidget(), {650, 240});
    defaultDockState = coreManager->saveState(DockStateVersion);

    const QList<ads::CDockWidget *> managedDocks{workspaceDock, hierarchyDock,
                                                 inspectorDock, contentDock,
                                                 errorDock};
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
            dock->toggleViewAction()->setIcon(dock->icon());
            windowMenu->addAction(dock->toggleViewAction());
        }
        windowMenu->addSeparator();
        const QList<QPair<QString, ads::CDockWidget *>> panels{
            {"Workspace", workspaceDock},
            {"Hierarchy", hierarchyDock},
            {"Inspector", inspectorDock},
            {"Content Browser", contentDock},
            {"Errors", errorDock}};
        for (int index = 0; index < panels.size(); ++index) {
            const auto &[name, dock] = panels.at(index);
            auto *action = windowMenu->addAction(
                QStringLiteral("Focus %1").arg(name), this, [dock] {
                    dock->toggleView(true);
                    dock->setAsCurrentTab();
                    dock->raise();
                });
            action->setIcon(dock->icon());
            action->setShortcut(
                QKeySequence(QStringLiteral("Ctrl+%1").arg(index + 1)));
            action->setShortcutContext(Qt::ApplicationShortcut);
        }
        windowMenu->addSeparator();
        const QList<QPair<QString, int>> workspaceModes{{"Scene", 0},
                                                        {"Shading", 1},
                                                        {"Post-Processing", 2},
                                                        {"Graphite", 3}};
        for (const auto &[name, index] : workspaceModes) {
            auto *action = windowMenu->addAction(
                QStringLiteral("Open %1 Workspace").arg(name), this,
                [this, workspaceDock, index] {
                    activateWorkspace(index);
                    workspaceDock->toggleView(true);
                    workspaceDock->setAsCurrentTab();
                    workspaceDock->raise();
                });
            action->setIcon(
                index == 0 ? styling::icon(styling::Icon::CubeFocus, "#7E929C")
                : index == 1 ? styling::icon(styling::Icon::Material, "#A1957D")
                : index == 2
                    ? styling::icon(styling::Icon::FilmStrip, "#849589")
                    : styling::icon(styling::Icon::Palette, "#849589"));
        }
    }

    connect(hierarchyPanel, &HierarchyPanel::objectActivated, inspectorPanel,
            &InspectorPanel::inspectRuntimeObject);
    connect(hierarchyPanel, &HierarchyPanel::cameraActivated, inspectorPanel,
            &InspectorPanel::inspectCamera);
    connect(hierarchyPanel, &HierarchyPanel::environmentActivated,
            inspectorPanel, &InspectorPanel::inspectEnvironment);
    connect(hierarchyPanel, &HierarchyPanel::graphiteActivated, this,
            [this](const QString &path) {
                if (!path.isEmpty() && graphiteEditorPanel != nullptr &&
                    viewportPanel != nullptr) {
                    QString resolved = path;
                    if (QFileInfo(resolved).isRelative()) {
                        resolved =
                            QDir(QFileInfo(viewportPanel->currentRuntimeScene())
                                     .absolutePath())
                                .filePath(resolved);
                    }
                    graphiteEditorPanel->openUI(resolved);
                }
                activateWorkspace(3);
            });
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
            [this](const QString &path) {
                materialEditorPanel->openMaterial(path);
                activateWorkspace(1);
            });
    connect(contentBrowser, &ContentBrowserPanel::sceneActivated, this,
            [this](const QString &path) {
                if (viewportPanel != nullptr &&
                    viewportPanel->openRuntimeScene(path) &&
                    viewportTools != nullptr) {
                    viewportTools->openSceneTab(path);
                }
            });
    connect(contentBrowser, &ContentBrowserPanel::uiActivated, this,
            [this](const QString &path) {
                graphiteEditorPanel->openUI(path);
                activateWorkspace(3);
            });
    connect(graphiteEditorPanel, &GraphiteEditorPanel::previewRequested, this,
            [this] { activateWorkspace(0); });
}

void EditorWindow::setupWorkspaceBar() {
    auto *bar = new QToolBar("Workspace", this);
    bar->setObjectName("workspaceBar");
    bar->setMovable(false);
    bar->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, bar);
    auto *chrome = new QWidget(bar);
    chrome->setObjectName("workspaceChrome");
    chrome->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *layout = new QGridLayout(chrome);
    layout->setContentsMargins(12, 7, 12, 7);
    layout->setHorizontalSpacing(16);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(2, 1);
    auto *activity = new QWidget(chrome);
    activity->setObjectName("workspaceActivity");
    auto *activityLayout = new QHBoxLayout(activity);
    activityLayout->setContentsMargins(0, 0, 0, 0);
    activityLayout->setSpacing(4);
    auto *mark = new QLabel(activity);
    mark->setObjectName("workspaceMark");
    mark->setPixmap(styling::brandMark(QSize(26, 26)));
    activityLayout->addWidget(mark);
    activityLayout->addSpacing(10);
    auto addPanelToggle = [this, activity, activityLayout](const QString &id,
                                                           const QString &text,
                                                           styling::Icon icon) {
        auto *dock = dockManager->panel(id);
        if (dock == nullptr)
            return;
        auto *button = new styling::ToolButton(activity);
        button->setObjectName("workspacePanelButton");
        button->setIcon(styling::icon(icon));
        button->setText(text);
        button->setToolTip(text);
        button->setCheckable(true);
        button->setChecked(!dock->isClosed());
        activityLayout->addWidget(button);
        connect(button, &QToolButton::toggled, dock,
                [dock](bool visible) { dock->toggleView(visible); });
        connect(dock, &ads::CDockWidget::viewToggled, button,
                [button](bool visible) {
                    const QSignalBlocker blocker(button);
                    button->setChecked(visible);
                });
    };
    addPanelToggle("hierarchy", "Scene collection", styling::Icon::Sidebar);
    addPanelToggle("fileExplorer", "Assets", styling::Icon::Folder);
    addPanelToggle("inspector", "Inspector", styling::Icon::SlidersHorizontal);
    auto *commands = new styling::ToolButton(activity);
    commands->setIcon(styling::icon(styling::Icon::MagnifyingGlass));
    commands->setToolTip("Search commands · " +
                        QKeySequence("Ctrl+Shift+P").toString(QKeySequence::NativeText));
    connect(commands, &QToolButton::clicked, this, &EditorWindow::showCommandPalette);
    activityLayout->addWidget(commands);
    activityLayout->addStretch();
    layout->addWidget(activity, 0, 0);

    auto *project = new styling::ToolButton(chrome);
    project->setObjectName("workspaceProject");
    project->setIcon(styling::icon(styling::Icon::FolderOpen));
    project->setText(projectName);
    project->setMaximumWidth(280);
    project->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    project->setPopupMode(QToolButton::InstantPopup);
    project->setToolTip(projectName);
    auto *projectMenu = new QMenu(project);
    projectMenu->addAction("Project Settings…", this, &EditorWindow::showProjectSettings);
    projectMenu->addAction("Input Actions…", this, &EditorWindow::showInputActions);
    projectMenu->addSeparator();
    projectMenu->addAction("Export Project…", this, &EditorWindow::showExportDialog);
    project->setMenu(projectMenu);
    layout->addWidget(project, 0, 1, Qt::AlignCenter);

    auto *utilities = new QWidget(chrome);
    utilities->setObjectName("workspaceUtilities");
    auto *utilityLayout = new QHBoxLayout(utilities);
    utilityLayout->setContentsMargins(0, 0, 0, 0);
    utilityLayout->setSpacing(5);
    utilityLayout->addStretch();
    auto *save = new styling::ToolButton(utilities);
    save->setIcon(styling::icon(styling::Icon::FloppyDisk));
    save->setToolTip("Save current work");
    connect(save, &QToolButton::clicked, this, [this] {
        if (materialEditorPanel != nullptr && materialEditorPanel->isVisible())
            materialEditorPanel->saveMaterial();
        if (graphiteEditorPanel != nullptr && graphiteEditorPanel->isVisible())
            graphiteEditorPanel->saveUI();
        if (viewportPanel != nullptr)
            viewportPanel->saveRuntimeScene();
    });
    utilityLayout->addWidget(save);
    auto *build = new styling::ToolButton(utilities);
    build->setIcon(styling::icon(styling::Icon::Package));
    build->setToolTip("Build project");
    connect(build, &QToolButton::clicked, this, [this] { runProjectCommand(true); });
    utilityLayout->addWidget(build);
    auto *launch = new styling::ToolButton(utilities);
    launch->setObjectName("workspaceLaunchButton");
    launch->setIcon(styling::icon(styling::Icon::Play));
    launch->setText("Run");
    launch->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    launch->setToolTip("Run project");
    connect(launch, &QToolButton::clicked, this, [this] { runProjectCommand(false); });
    utilityLayout->addWidget(launch);
    layout->addWidget(utilities, 0, 2);
    bar->addWidget(chrome);

    statusBar()->setObjectName("atlasStatusBar");
    statusBar()->showMessage("Scene workspace");
    auto *runtimeStatus = new QWidget(statusBar());
    runtimeStatus->setObjectName("statusRuntime");
    auto *runtimeLayout = new QHBoxLayout(runtimeStatus);
    runtimeLayout->setContentsMargins(5, 0, 7, 0);
    runtimeLayout->setSpacing(4);
    auto *runtimeIcon = new QLabel(runtimeStatus);
    runtimeIcon->setObjectName("statusRuntimeIcon");
    runtimeIcon->setPixmap(
        styling::icon(styling::Icon::Check, "#849589").pixmap(14, 14));
    auto *runtimeText = new QLabel("Runtime ready", runtimeStatus);
    runtimeText->setObjectName("statusRuntimeText");
    runtimeLayout->addWidget(runtimeIcon);
    runtimeLayout->addWidget(runtimeText);
    auto *renderer = new QLabel(statusBar());
    renderer->setObjectName("statusRenderer");
    const auto projectInfo = ProjectStore::projectInfo(projectFile);
    renderer->setText(projectInfo.has_value() ? projectInfo->renderer
                                              : QStringLiteral("ATLAS"));
    auto *version = new QLabel(QStringLiteral(ATLAS_VERSION), statusBar());
    version->setObjectName("statusVersion");
    statusBar()->addPermanentWidget(runtimeStatus);
    statusBar()->addPermanentWidget(renderer);
    statusBar()->addPermanentWidget(version);
    connect(
        viewportPanel, &ViewportPanel::runtimeAvailabilityChanged, this,
        [this, runtimeIcon, runtimeText](bool available) {
            runtimeIcon->setPixmap(
                styling::icon(available ? styling::Icon::Check
                                        : styling::Icon::Warning,
                              available ? QColor("#849589") : QColor("#A1957D"))
                    .pixmap(14, 14));
            runtimeText->setText(available ? "Runtime ready"
                                           : "Runtime unavailable");
            statusBar()->showMessage(
                available ? "Runtime ready" : "Runtime unavailable", 3000);
        });
}

void EditorWindow::activateWorkspace(int index) {
    if (workspaceStack == nullptr || index < 0 ||
        index >= workspaceStack->count())
        return;
    const int previousIndex = workspaceStack->currentIndex();
    if (previousIndex != index && coreManager != nullptr) {
        QSettings settings("Neutral Software", "Atlas Engine");
        const auto layoutKey = [](int mode) {
            return QStringLiteral("workspace/layout/v11/%1").arg(mode);
        };
        if (!restoringLayout)
            settings.setValue(layoutKey(previousIndex),
                              coreManager->saveState(DockStateVersion));
        const QByteArray state = settings.value(layoutKey(index)).toByteArray();
        if (state.isEmpty() || !coreManager->restoreState(state, DockStateVersion)) {
            if (index == 0) {
                coreManager->restoreState(defaultDockState, DockStateVersion);
            } else {
                for (const QString &id : {QStringLiteral("hierarchy"),
                                          QStringLiteral("inspector")}) {
                    if (auto *dock = dockManager->panel(id))
                        dock->toggleView(false);
                }
            }
        }
        configureDockSplitters();
    }
    if (index == 0 && previousIndex == 1 && materialEditorPanel != nullptr)
        materialEditorPanel->flushPendingSave();
    if (index == 0 && previousIndex == 3 && graphiteEditorPanel != nullptr)
        graphiteEditorPanel->flushPendingSave();
    workspaceStack->setCurrentIndex(index);
    if (index == 0 && previousIndex != 0 && workspaceChangesPending &&
        viewportPanel != nullptr) {
        workspaceChangesPending = false;
        viewportPanel->reloadRuntime();
    }
    if (workspaceModeGroup != nullptr) {
        if (auto *button = workspaceModeGroup->button(index)) {
            button->setChecked(true);
            if (auto *icon = findChild<QLabel *>("workspaceContextIcon"))
                icon->setPixmap(button->icon().pixmap(15, 15));
            if (auto *title = findChild<QLabel *>("workspaceContextTitle"))
                title->setText(button->text());
            if (auto *path = findChild<QLabel *>("workspaceContextPath"))
                path->setText(button->property("workspacePath").toString());
        }
    }
    scheduleLayoutSave();
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
        QJsonDocument(
            QJsonObject{
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
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(contents) != contents.size() || !file.commit()) {
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
    dialog.setObjectName("projectSettingsDialog");
    dialog.setWindowTitle("Project Settings");
    dialog.resize(720, 520);
    auto *layout = new QVBoxLayout(&dialog);
    auto *header = new QFrame(&dialog);
    header->setObjectName("dialogHero");
    auto *headerLayout = new QHBoxLayout(header);
    auto *headerIcon = new QLabel(header);
    headerIcon->setObjectName("dialogHeroIcon");
    headerIcon->setPixmap(
        styling::icon(styling::Icon::Gear, "#8498A8").pixmap(28, 28));
    auto *headerCopy = new QVBoxLayout();
    auto *headerTitle = new QLabel("Project Settings", header);
    headerTitle->setObjectName("dialogHeroTitle");
    auto *headerSubtitle = new QLabel("Configure runtime, rendering, controls, "
                                      "and packaging for this project.",
                                      header);
    headerSubtitle->setObjectName("dialogHeroSubtitle");
    headerCopy->addWidget(headerTitle);
    headerCopy->addWidget(headerSubtitle);
    headerLayout->addWidget(headerIcon);
    headerLayout->addLayout(headerCopy, 1);
    layout->addWidget(header);
    auto *tabs = new QTabWidget(&dialog);
    const QString settingsDirectory =
        QDir(QFileInfo(projectFile).absolutePath()).filePath(".atlas");
    QDir().mkpath(settingsDirectory);
    QSettings settings(QDir(settingsDirectory).filePath("project-settings.ini"),
                       QSettings::IniFormat);
    QStringList projectLines;
    QFile projectManifest(projectFile);
    if (projectManifest.open(QIODevice::ReadOnly | QIODevice::Text)) {
        projectLines =
            QString::fromUtf8(projectManifest.readAll()).split('\n');
    }
    const QString configuredRenderer =
        tomlValue(projectLines, "renderer", "default");
    const bool configuredGlobalIllumination =
        tomlValue(projectLines, "renderer", "global_illumination") == "true";
    const QString rendererDisplay =
        configuredRenderer == "pathtracing"
            ? QStringLiteral("Path Tracing")
            : configuredGlobalIllumination ? QStringLiteral("PBR + DDGI")
                                           : QStringLiteral("PBR");
    const QString dimensionsValue =
        tomlValue(projectLines, "window", "dimensions");
    const auto dimensionsMatch =
        QRegularExpression(QStringLiteral(
                               "^\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\]$"))
            .match(dimensionsValue);
    const int configuredWidth =
        dimensionsMatch.hasMatch()
            ? dimensionsMatch.captured(1).toInt()
            : settings.value("project/windowWidth", 1280).toInt();
    const int configuredHeight =
        dimensionsMatch.hasMatch()
            ? dimensionsMatch.captured(2).toInt()
            : settings.value("project/windowHeight", 720).toInt();
    const QString upscalingValue =
        tomlValue(projectLines, "renderer", "use_upscaling");
    const bool configuredUpscaling =
        upscalingValue.isEmpty()
            ? settings.value("project/useUpscaling", true).toBool()
            : upscalingValue == "true";
    bool scaleValid = false;
    const double configuredScaleValue =
        tomlValue(projectLines, "renderer", "upscaling_ratio")
            .toDouble(&scaleValid);
    const int configuredScale =
        scaleValid
            ? std::clamp(qRound(configuredScaleValue * 100.0), 25, 100)
            : settings.value("project/internalScale", 67).toInt();
    bool samplesValid = false;
    const int configuredSamples =
        tomlValue(projectLines, "renderer", "samples_per_pixel")
            .toInt(&samplesValid);
    bool bouncesValid = false;
    const int configuredBounces =
        tomlValue(projectLines, "renderer", "max_bounces")
            .toInt(&bouncesValid);
    bool accumulationValid = false;
    const int configuredAccumulation =
        tomlValue(projectLines, "renderer", "accumulation_frames")
            .toInt(&accumulationValid);
    const QString denoisingValue =
        tomlValue(projectLines, "renderer", "denoising");
    auto addPage = [tabs](const QString &name, styling::Icon icon,
                          const QColor &color) {
        auto *page = new QWidget(tabs);
        auto *form = new QFormLayout(page);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        tabs->addTab(page, styling::icon(icon, color), name);
        return form;
    };
    auto *general = addPage("General", styling::Icon::Gear, "#8498A8");
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
    windowWidth->setValue(configuredWidth);
    auto *windowHeight = new QSpinBox(&dialog);
    windowHeight->setRange(240, 16384);
    windowHeight->setValue(configuredHeight);
    auto *resolutionPreset = new QComboBox(&dialog);
    resolutionPreset->addItem("HD · 1280 × 720", QSize(1280, 720));
    resolutionPreset->addItem("Full HD · 1920 × 1080", QSize(1920, 1080));
    resolutionPreset->addItem("QHD · 2560 × 1440", QSize(2560, 1440));
    resolutionPreset->addItem("4K UHD · 3840 × 2160", QSize(3840, 2160));
    resolutionPreset->addItem("Custom", QSize());
    int matchingResolution = resolutionPreset->count() - 1;
    for (int index = 0; index < resolutionPreset->count() - 1; ++index) {
        if (resolutionPreset->itemData(index).toSize() ==
            QSize(configuredWidth, configuredHeight)) {
            matchingResolution = index;
            break;
        }
    }
    resolutionPreset->setCurrentIndex(matchingResolution);
    connect(resolutionPreset, &QComboBox::currentIndexChanged, &dialog,
            [resolutionPreset, windowWidth, windowHeight](int index) {
                const QSize resolution = resolutionPreset->itemData(index).toSize();
                if (!resolution.isValid())
                    return;
                windowWidth->setValue(resolution.width());
                windowHeight->setValue(resolution.height());
            });
    auto syncResolutionPreset = [=] {
        int index = resolutionPreset->count() - 1;
        const QSize resolution(windowWidth->value(), windowHeight->value());
        for (int candidate = 0; candidate < resolutionPreset->count() - 1;
             ++candidate) {
            if (resolutionPreset->itemData(candidate).toSize() == resolution) {
                index = candidate;
                break;
            }
        }
        QSignalBlocker blocker(resolutionPreset);
        resolutionPreset->setCurrentIndex(index);
    };
    connect(windowWidth, &QSpinBox::valueChanged, &dialog,
            syncResolutionPreset);
    connect(windowHeight, &QSpinBox::valueChanged, &dialog,
            syncResolutionPreset);
    auto *fullscreen = new QCheckBox("Start in fullscreen", &dialog);
    fullscreen->setChecked(
        settings.value("project/fullscreen", false).toBool());
    general->addRow("Default scene", defaultScene);
    general->addRow("Company", companyName);
    general->addRow("Version", gameVersion);
    general->addRow("Output resolution", resolutionPreset);
    general->addRow("Window width", windowWidth);
    general->addRow("Window height", windowHeight);
    general->addRow(QString(), fullscreen);
    auto *rendering = addPage("Rendering", styling::Icon::Aperture, "#9E897D");
    auto *renderer = new QComboBox(&dialog);
    renderer->addItems({"PBR", "PBR + DDGI", "Path Tracing"});
    renderer->setCurrentText(rendererDisplay);
    auto *frameLimit = new QSpinBox(&dialog);
    frameLimit->setRange(0, 1000);
    frameLimit->setValue(settings.value("project/frameLimit", 0).toInt());
    auto *ssr = new QCheckBox("Enable screen-space reflections", &dialog);
    ssr->setChecked(settings.value("project/ssr", false).toBool());
    auto *ssrQuality = new QComboBox(&dialog);
    ssrQuality->addItems({"Low", "Medium", "High"});
    ssrQuality->setCurrentIndex(
        settings.value("project/ssrQuality", 1).toInt());
    auto *ssrDebug = new QCheckBox("Show SSR hit confidence", &dialog);
    ssrDebug->setChecked(settings.value("project/ssrDebug", false).toBool());
    auto *upscaling = new QCheckBox("Enable Metal upscaling", &dialog);
    upscaling->setChecked(configuredUpscaling);
    auto *upscalingQuality = new QComboBox(&dialog);
    upscalingQuality->addItem("Quality", 75);
    upscalingQuality->addItem("Balanced", 67);
    upscalingQuality->addItem("Performance", 50);
    upscalingQuality->addItem("Ultra Performance", 33);
    upscalingQuality->addItem("Custom", -1);
    auto *internalScale = new QSpinBox(&dialog);
    internalScale->setRange(25, 100);
    internalScale->setSuffix("%");
    internalScale->setValue(configuredScale);
    int matchingScale = upscalingQuality->count() - 1;
    for (int index = 0; index < upscalingQuality->count() - 1; ++index) {
        if (upscalingQuality->itemData(index).toInt() == configuredScale) {
            matchingScale = index;
            break;
        }
    }
    upscalingQuality->setCurrentIndex(matchingScale);
    auto *samplesPerPixel = new QSpinBox(&dialog);
    samplesPerPixel->setRange(1, 64);
    samplesPerPixel->setValue(samplesValid ? configuredSamples : 4);
    auto *maxBounces = new QSpinBox(&dialog);
    maxBounces->setRange(1, 16);
    maxBounces->setValue(bouncesValid ? configuredBounces : 8);
    auto *denoising = new QCheckBox("Variance-guided denoising", &dialog);
    denoising->setChecked(denoisingValue.isEmpty() || denoisingValue == "true");
    auto *accumulationFrames = new QSpinBox(&dialog);
    accumulationFrames->setRange(1, 2048);
    accumulationFrames->setValue(accumulationValid ? configuredAccumulation
                                                   : 512);
    auto *internalResolution = new QLabel(&dialog);
    auto updateInternalResolution = [=] {
        const int scale = upscaling->isChecked() ? internalScale->value() : 100;
        internalResolution->setText(
            QStringLiteral("%1 × %2 internal → %3 × %4 output")
                .arg(std::max(1, windowWidth->value() * scale / 100))
                .arg(std::max(1, windowHeight->value() * scale / 100))
                .arg(windowWidth->value())
                .arg(windowHeight->value()));
    };
    connect(upscalingQuality, &QComboBox::currentIndexChanged, &dialog,
            [upscalingQuality, internalScale](int index) {
                const int scale = upscalingQuality->itemData(index).toInt();
                if (scale > 0)
                    internalScale->setValue(scale);
            });
    connect(internalScale, &QSpinBox::valueChanged, &dialog,
            [upscalingQuality](int value) {
                const int index = upscalingQuality->findData(value);
                QSignalBlocker blocker(upscalingQuality);
                upscalingQuality->setCurrentIndex(
                    index >= 0 ? index : upscalingQuality->count() - 1);
            });
    connect(upscaling, &QCheckBox::toggled, &dialog,
            [internalScale, upscalingQuality](bool enabled) {
                internalScale->setEnabled(enabled);
                upscalingQuality->setEnabled(enabled);
            });
    for (auto *spinBox : {windowWidth, windowHeight, internalScale}) {
        connect(spinBox, &QSpinBox::valueChanged, &dialog,
                updateInternalResolution);
    }
    connect(upscaling, &QCheckBox::toggled, &dialog,
            updateInternalResolution);
    internalScale->setEnabled(upscaling->isChecked());
    upscalingQuality->setEnabled(upscaling->isChecked());
    updateInternalResolution();
    rendering->addRow("Renderer", renderer);
    rendering->addRow(QString(), ssr);
    rendering->addRow("SSR quality", ssrQuality);
    rendering->addRow(QString(), ssrDebug);
    rendering->addRow(QString(), upscaling);
    rendering->addRow("Upscaling quality", upscalingQuality);
    rendering->addRow("Internal render scale", internalScale);
    rendering->addRow("Effective resolution", internalResolution);
    rendering->addRow("Samples per pixel", samplesPerPixel);
    rendering->addRow("Maximum light bounces", maxBounces);
    rendering->addRow(QString(), denoising);
    rendering->addRow("Temporal accumulation", accumulationFrames);
    rendering->addRow("Frame limit (0 = unlimited)", frameLimit);
    auto updatePathTracingControls = [=] {
        const bool enabled = renderer->currentText() == "Path Tracing";
        samplesPerPixel->setEnabled(enabled);
        maxBounces->setEnabled(enabled);
        denoising->setEnabled(enabled);
        accumulationFrames->setEnabled(enabled);
    };
    connect(renderer, &QComboBox::currentTextChanged, &dialog,
            updatePathTracingControls);
    updatePathTracingControls();
    auto *physics = addPage("Physics", styling::Icon::Wrench, "#A1957D");
    auto *gravity = new QLineEdit(
        settings.value("project/gravity", "0, -9.81, 0").toString(), &dialog);
    auto *fixedStep = new QLineEdit(
        settings.value("project/fixedStep", "0.0166667").toString(), &dialog);
    physics->addRow("Gravity", gravity);
    physics->addRow("Fixed timestep", fixedStep);
    auto *input = addPage("Input", styling::Icon::GameController, "#849589");
    auto *inputMap = new QLineEdit(
        settings.value("project/inputMap", "input.json").toString(), &dialog);
    auto *controller = new QComboBox(&dialog);
    controller->addItems({"Automatic", "Keyboard + Mouse", "Gamepad"});
    controller->setCurrentText(
        settings.value("project/controller", "Automatic").toString());
    input->addRow("Input map", inputMap);
    input->addRow("Primary controller", controller);
    auto *build = addPage("Build & Run", styling::Icon::Package, "#A1957D");
    auto *buildCommand = new QLineEdit(
        settings.value("project/buildCommand", "atlas pack --backend METAL")
            .toString(),
        &dialog);
    auto *runCommand = new QLineEdit(
        settings.value("project/runCommand", "atlas run project.atlas")
            .toString(),
        &dialog);
    build->addRow("Build command", buildCommand);
    build->addRow("Run command", runCommand);
    auto *editor = addPage("Editor", styling::Icon::Layout, "#7E929C");
    auto *autosave = new QSpinBox(&dialog);
    autosave->setRange(0, 120);
    autosave->setValue(settings.value("project/autosaveMinutes", 5).toInt());
    auto *snap = new QLineEdit(
        settings.value("project/snapIncrement", "0.5").toString(), &dialog);
    editor->addRow("Autosave interval (minutes)", autosave);
    editor->addRow("Transform snapping", snap);
    auto *packaging = addPage("Packaging", styling::Icon::Export, "#8498A8");
    auto *identifier = new QLineEdit(
        settings
            .value("project/bundleIdentifier",
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
    buttons->button(QDialogButtonBox::Save)
        ->setIcon(styling::icon(styling::Icon::FloppyDisk, "#A1957D"));
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
    settings.setValue("project/ssr", ssr->isChecked());
    settings.setValue("project/ssrQuality", ssrQuality->currentIndex());
    settings.setValue("project/ssrDebug", ssrDebug->isChecked());
    settings.setValue("project/useUpscaling", upscaling->isChecked());
    settings.setValue("project/internalScale", internalScale->value());
    settings.setValue("project/pathTracingSamples", samplesPerPixel->value());
    settings.setValue("project/pathTracingBounces", maxBounces->value());
    settings.setValue("project/pathTracingDenoising", denoising->isChecked());
    settings.setValue("project/pathTracingAccumulation",
                      accumulationFrames->value());
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
    bool manifestUpdated = false;
    QFile manifest(projectFile);
    if (manifest.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines = QString::fromUtf8(manifest.readAll()).split('\n');
        manifest.close();
        setTomlValue(&lines, QString(), "backend",
                     tomlQuoted(backend->currentText()));
        setTomlValue(&lines, "game", "main_scene",
                     tomlQuoted(defaultScene->currentText()));
        setTomlValue(&lines, "pack", "identifier",
                     tomlQuoted(identifier->text()));
        setTomlValue(&lines, "pack", "version",
                     tomlQuoted(gameVersion->text()));
        setTomlValue(&lines, "pack", "icon", tomlQuoted(iconPath->text()));
        setTomlValue(&lines, "window", "dimensions",
                     QStringLiteral("[%1, %2]")
                         .arg(windowWidth->value())
                         .arg(windowHeight->value()));
        setTomlValue(&lines, "window", "fullscreen",
                     fullscreen->isChecked() ? "true" : "false");
        const QString rendererName = renderer->currentText() == "Path Tracing"
                                         ? "pathtracing"
                                         : "deferred";
        setTomlValue(&lines, "renderer", "default", tomlQuoted(rendererName));
        setTomlValue(&lines, "renderer", "global_illumination",
                     renderer->currentText() == "PBR + DDGI" ? "true"
                                                             : "false");
        setTomlValue(&lines, "renderer", "ssr",
                     ssr->isChecked() ? "true" : "false");
        setTomlValue(&lines, "renderer", "ssr_quality",
                     QString::number(ssrQuality->currentIndex()));
        setTomlValue(&lines, "renderer", "ssr_debug",
                     ssrDebug->isChecked() ? "true" : "false");
        setTomlValue(&lines, "renderer", "use_upscaling",
                     upscaling->isChecked() ? "true" : "false");
        setTomlValue(&lines, "renderer", "upscaling_ratio",
                     QString::number(internalScale->value() / 100.0, 'f', 2));
        setTomlValue(&lines, "renderer", "samples_per_pixel",
                     QString::number(samplesPerPixel->value()));
        setTomlValue(&lines, "renderer", "max_bounces",
                     QString::number(maxBounces->value()));
        setTomlValue(&lines, "renderer", "denoising",
                     denoising->isChecked() ? "true" : "false");
        setTomlValue(&lines, "renderer", "accumulation_frames",
                     QString::number(accumulationFrames->value()));
        QSaveFile outputFile(projectFile);
        const QByteArray contents = lines.join('\n').toUtf8();
        if (!outputFile.open(QIODevice::WriteOnly) ||
            outputFile.write(contents) != contents.size() ||
            !outputFile.commit()) {
            QMessageBox::warning(this, "Project Settings",
                                 "The project manifest could not be updated.");
        } else {
            manifestUpdated = true;
        }
    }
    if (manifestUpdated && viewportPanel != nullptr) {
        viewportPanel->applyPathTracingSettings(
            samplesPerPixel->value(), maxBounces->value(),
            denoising->isChecked(), accumulationFrames->value(),
            upscaling->isChecked(), internalScale->value() / 100.0f);
    }
}

void EditorWindow::showExportDialog() {
    QDialog dialog(this);
    dialog.setObjectName("exportDialog");
    dialog.setWindowTitle("Export Atlas Project");
    dialog.resize(660, 460);
    auto *layout = new QVBoxLayout(&dialog);
    auto *header = new QFrame(&dialog);
    header->setObjectName("dialogHero");
    auto *headerLayout = new QHBoxLayout(header);
    auto *headerIcon = new QLabel(header);
    headerIcon->setObjectName("dialogHeroIcon");
    headerIcon->setPixmap(
        styling::icon(styling::Icon::Export, "#849589").pixmap(28, 28));
    auto *headerCopy = new QVBoxLayout();
    auto *headerTitle = new QLabel("Export Project", header);
    headerTitle->setObjectName("dialogHeroTitle");
    auto *headerSubtitle = new QLabel(
        "Package the current project into a distributable application.",
        header);
    headerSubtitle->setObjectName("dialogHeroSubtitle");
    headerCopy->addWidget(headerTitle);
    headerCopy->addWidget(headerSubtitle);
    headerLayout->addWidget(headerIcon);
    headerLayout->addLayout(headerCopy, 1);
    layout->addWidget(header);
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
    QSettings settings(QDir(settingsDirectory).filePath("project-settings.ini"),
                       QSettings::IniFormat);
    auto *backend = new QComboBox(&dialog);
    backend->addItems({"METAL", "VULKAN", "OPENGL"});
    backend->setCurrentText(
        settings.value("project/exportBackend", "METAL").toString());
    auto *output = new QLineEdit(
        settings
            .value(
                "project/exportDirectory",
                QDir(QFileInfo(projectFile).absolutePath()).filePath("Exports"))
            .toString(),
        &dialog);
    auto *browse = new styling::Button("Choose…", &dialog);
    browse->setIcon(styling::icon(styling::Icon::FolderOpen, "#7E929C"));
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
    auto *summary =
        new QLabel("Atlas will save the current scene and package the "
                   "configured runtime with the project resources.",
                   &dialog);
    summary->setObjectName("exportSummary");
    summary->setWordWrap(true);
    layout->addWidget(summary);
    auto *progress = new QProgressBar(&dialog);
    progress->setRange(0, 0);
    progress->setVisible(false);
    layout->addWidget(progress);
    auto *log = new QPlainTextEdit(&dialog);
    log->setObjectName("exportLog");
    log->setReadOnly(true);
    log->setPlaceholderText("Packaging output will appear here.");
    layout->addWidget(log, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto *exportButton =
        buttons->addButton("Export", QDialogButtonBox::AcceptRole);
    exportButton->setIcon(
        styling::icon(styling::Icon::RocketLaunch, "#849589"));
    auto *revealButton =
        buttons->addButton("Reveal Export", QDialogButtonBox::ActionRole);
    revealButton->setIcon(styling::icon(styling::Icon::FolderOpen, "#7E929C"));
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
    connect(
        process, &QProcess::readyReadStandardOutput, &dialog, [process, log] {
            log->appendPlainText(
                QString::fromUtf8(process->readAllStandardOutput()).trimmed());
        });
    connect(
        process, &QProcess::readyReadStandardError, &dialog, [process, log] {
            log->appendPlainText(
                QString::fromUtf8(process->readAllStandardError()).trimmed());
        });
    connect(process, &QProcess::errorOccurred, &dialog,
            [process, progress, exportButton, log](QProcess::ProcessError) {
                progress->setVisible(false);
                exportButton->setEnabled(true);
                log->appendPlainText(process->errorString());
            });
    connect(
        process, &QProcess::finished, &dialog,
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
                log->appendPlainText(
                    "Could not create the export destination.");
                return;
            }
            const QFileInfoList packages = QDir(dist).entryInfoList(
                QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden);
            if (packages.isEmpty()) {
                log->appendPlainText(
                    "Atlas Pack produced no distributable files.");
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
    connect(
        exportButton, &QPushButton::clicked, &dialog,
        [this, process, output, platform, configuration, backend, progress,
         exportButton, revealButton, log] {
            const QString program = atlasCliPath();
            QStringList arguments;
            if (program.isEmpty()) {
                QMessageBox::warning(this, "Export Project",
                                     "Atlas CLI was not found. Install it or "
                                     "place it beside Atlas Editor.");
                return;
            }
            QSettings settings(QDir(QFileInfo(projectFile).absolutePath())
                                   .filePath(".atlas/project-settings.ini"),
                               QSettings::IniFormat);
            settings.setValue("project/exportDirectory", output->text());
            settings.setValue("project/exportPlatform",
                              platform->currentText());
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
    dialog.setObjectName("commandPaletteDialog");
    dialog.setWindowTitle("Command Palette");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(720, 470);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);
    auto *search = new QLineEdit(&dialog);
    search->setObjectName("commandPaletteSearch");
    search->setClearButtonEnabled(true);
    search->addAction(styling::icon(styling::Icon::MagnifyingGlass), QLineEdit::LeadingPosition);
    search->setPlaceholderText("Type a command…");
    auto *commands = new QListWidget(&dialog);
    commands->setObjectName("commandPaletteList");
    layout->addWidget(search);
    layout->addWidget(commands, 1);
    auto *hint = new QLabel("↑ ↓  Navigate     Return  Open     Esc  Dismiss", &dialog);
    hint->setObjectName("paletteHint");
    layout->addWidget(hint);
    const QList<QAction *> actions = findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->text().isEmpty() || action->isSeparator() ||
            !action->isEnabled()) {
            continue;
        }
        auto *item = new QListWidgetItem(commands);
        item->setText(action->text().remove('&'));
        item->setIcon(action->icon());
        item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(
                                        reinterpret_cast<quintptr>(action)));
        const QString shortcut =
            !action->shortcut().isEmpty()
                ? action->shortcut().toString(QKeySequence::NativeText)
                : action->property("atlasShortcut").toString();
        if (!shortcut.isEmpty())
            item->setText(item->text() + "\t" + shortcut);
    }
    if (commands->count() > 0)
        commands->setCurrentRow(0);
    auto *noCommands = new QListWidgetItem("No matching commands", commands);
    noCommands->setData(Qt::UserRole, QVariant::fromValue<quintptr>(0));
    noCommands->setData(Qt::UserRole + 1, true);
    noCommands->setHidden(true);
    connect(search, &QLineEdit::textChanged, &dialog,
            [commands, noCommands](const QString &text) {
                int firstMatch = -1;
                for (int index = 0; index < commands->count(); ++index) {
                    QListWidgetItem *item = commands->item(index);
                    if (item == noCommands)
                        continue;
                    if (firstMatch < 0 &&
                        item->text().contains(text, Qt::CaseInsensitive))
                        firstMatch = index;
                }
                if (firstMatch >= 0) {
                    commands->setCurrentRow(firstMatch);
                    noCommands->setHidden(true);
                } else {
                    noCommands->setHidden(false);
                    commands->setCurrentItem(noCommands);
                }
                for (int index = 0; index < commands->count(); ++index) {
                    QListWidgetItem *item = commands->item(index);
                    if (item != noCommands)
                        item->setHidden(
                            !item->text().contains(text, Qt::CaseInsensitive));
                }
            });
    connect(commands, &QListWidget::itemActivated, &dialog,
            [&dialog](QListWidgetItem *item) {
                if (item->data(Qt::UserRole + 1).toBool())
                    return;
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
    dialog.setObjectName("globalSearchDialog");
    dialog.setWindowTitle("Search Atlas Project");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(960, 560);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);
    auto *search = new QLineEdit(&dialog);
    search->setObjectName("globalSearchField");
    search->setClearButtonEnabled(true);
    search->addAction(styling::icon(styling::Icon::MagnifyingGlass), QLineEdit::LeadingPosition);
    search->setPlaceholderText("Search scenes, assets, and commands…");
    auto *results = new QListWidget(&dialog);
    results->setObjectName("globalSearchResults");
    layout->addWidget(search);
    auto *splitter = new QSplitter(Qt::Horizontal, &dialog);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);
    splitter->addWidget(results);
    auto *previewPane = new QWidget(splitter);
    previewPane->setObjectName("searchPreviewPane");
    auto *previewLayout = new QVBoxLayout(previewPane);
    previewLayout->setContentsMargins(16, 14, 12, 10);
    previewLayout->setSpacing(10);
    auto *previewTitle = new styling::ElidedLabel("Preview", previewPane);
    previewTitle->setObjectName("workspaceTitle");
    previewLayout->addWidget(previewTitle);
    auto *previewStack = new QStackedWidget(previewPane);
    auto *previewText = new QPlainTextEdit(previewStack);
    previewText->setObjectName("searchPreviewText");
    previewText->setReadOnly(true);
    previewText->setLineWrapMode(QPlainTextEdit::NoWrap);
    previewText->setPlaceholderText("Select a result to preview it.");
    auto *previewImage = new QLabel(previewStack);
    previewImage->setAlignment(Qt::AlignCenter);
    previewStack->addWidget(previewText);
    previewStack->addWidget(previewImage);
    previewLayout->addWidget(previewStack, 1);
    splitter->addWidget(previewPane);
    splitter->setSizes({480, 440});
    layout->addWidget(splitter, 1);
    auto *hint = new QLabel("↑ ↓  Navigate     Return  Open     Esc  Dismiss", &dialog);
    hint->setObjectName("paletteHint");
    layout->addWidget(hint);
    constexpr int SearchKindRole = Qt::UserRole + 1;
    constexpr int SearchValueRole = Qt::UserRole + 2;
    connect(results, &QListWidget::currentItemChanged, &dialog,
            [previewTitle, previewText, previewStack, previewImage](QListWidgetItem *item) {
                previewStack->setCurrentIndex(0);
                previewText->clear();
                previewImage->clear();
                if (item == nullptr || item->data(SearchKindRole).toInt() == 3) {
                    previewTitle->setText("Preview");
                    return;
                }
                previewTitle->setText(item->text());
                previewTitle->setToolTip(item->toolTip());
                if (item->data(SearchKindRole).toInt() != 0) {
                    previewText->setPlainText(item->data(styling::SecondaryTextRole).toString());
                    return;
                }
                const QString path = item->data(SearchValueRole).toString();
                const QFileInfo info(path);
                const QStringList textTypes{"ts", "js", "json", "atlas", "ascene", "aui",
                                            "amat", "toml", "txt", "md", "glsl", "vert", "frag"};
                if (textTypes.contains(info.suffix().toLower())) {
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly)) {
                        QByteArray data = file.read(65536);
                        QString text = QString::fromUtf8(data);
                        if (!file.atEnd())
                            text += "\n\n… Preview limited to 64 KB";
                        previewText->setPlainText(text);
                        return;
                    }
                }
                QImageReader reader(path);
                const QSize dimensions = reader.size();
                if (dimensions.isValid() && dimensions.width() <= 16384 &&
                    dimensions.height() <= 16384) {
                    reader.setAutoTransform(true);
                    reader.setScaledSize(dimensions.scaled(QSize(360, 300), Qt::KeepAspectRatio));
                    const QImage image = reader.read();
                    if (!image.isNull()) {
                        previewImage->setPixmap(QPixmap::fromImage(image));
                        previewStack->setCurrentIndex(1);
                        return;
                    }
                }
                previewText->setPlainText(QStringLiteral("%1\n\n%2\n%3 KB")
                    .arg(info.fileName(), item->data(styling::SecondaryTextRole).toString())
                    .arg(info.size() / 1024.0, 0, 'f', 1));
            });
    QDirIterator iterator(QFileInfo(projectFile).absolutePath(), QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        const QString relative =
            QDir(QFileInfo(projectFile).absolutePath()).relativeFilePath(path);
        if (relative.startsWith(".git/") || relative.startsWith("build/") ||
            relative.startsWith("dist/") || relative.startsWith(".jj/") ||
            relative.startsWith("node_modules/") || relative.startsWith("target/"))
            continue;
        auto *item = new QListWidgetItem(
            QFileInfo(path).fileName(), results);
        item->setIcon(styling::icon(styling::Icon::File, "#7E929C"));
        item->setToolTip(path);
        item->setData(styling::SecondaryTextRole, "Asset · " + QFileInfo(relative).path());
        item->setData(SearchKindRole, 0);
        item->setData(SearchValueRole, path);
    }
    const QJsonDocument snapshot =
        viewportPanel != nullptr
            ? QJsonDocument::fromJson(
                  viewportPanel->currentSceneSnapshot().toUtf8())
            : QJsonDocument();
    std::function<void(const QJsonArray &)> addObjects;
    addObjects = [&addObjects, results](const QJsonArray &objects) {
        for (const QJsonValue &value : objects) {
            const QJsonObject object = value.toObject();
            const QString name = object.value("name").toString();
            const int id = object.value("id").toInt(-1);
            if (!name.isEmpty() && id >= 0) {
                auto *item = new QListWidgetItem(
                    name, results);
                item->setIcon(styling::icon(styling::Icon::Cube, "#8498A8"));
                item->setToolTip(object.value("type").toString());
                item->setData(styling::SecondaryTextRole, "Scene object · " + item->toolTip());
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
            action->text().remove('&'),
            results);
        item->setIcon(action->icon());
        item->setData(SearchKindRole, 2);
        item->setData(styling::SecondaryTextRole, "Command · " + action->shortcut().toString(QKeySequence::NativeText));
        item->setData(SearchValueRole, QVariant::fromValue<quintptr>(
                                           reinterpret_cast<quintptr>(action)));
    }
    if (results->count() > 0)
        results->setCurrentRow(0);
    auto *noResults = new QListWidgetItem("No matching results", results);
    noResults->setData(SearchKindRole, 3);
    noResults->setHidden(true);
    connect(
        search, &QLineEdit::textChanged, &dialog,
        [results, noResults](const QString &text) {
            int firstMatch = -1;
            for (int index = 0; index < results->count(); ++index) {
                QListWidgetItem *item = results->item(index);
                if (item == noResults)
                    continue;
                const bool matches =
                    item->text().contains(text, Qt::CaseInsensitive) ||
                    item->toolTip().contains(text, Qt::CaseInsensitive);
                if (firstMatch < 0 && matches)
                    firstMatch = index;
            }
            if (firstMatch >= 0) {
                results->setCurrentRow(firstMatch);
                noResults->setHidden(true);
            } else {
                noResults->setHidden(false);
                results->setCurrentItem(noResults);
            }
            for (int index = 0; index < results->count(); ++index) {
                QListWidgetItem *item = results->item(index);
                if (item != noResults)
                    item->setHidden(
                        !item->text().contains(text, Qt::CaseInsensitive) &&
                        !item->toolTip().contains(text, Qt::CaseInsensitive));
            }
        });
    connect(results, &QListWidget::itemActivated, &dialog,
            [this, &dialog](QListWidgetItem *item) {
                const int kind = item->data(Qt::UserRole + 1).toInt();
                if (kind == 3)
                    return;
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
                const QString path = item->data(Qt::UserRole + 2).toString();
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
    search->setFocus();
    dialog.exec();
}

void EditorWindow::runProjectCommand(bool buildOnly) {
    const QString settingsDirectory =
        QDir(QFileInfo(projectFile).absolutePath()).filePath(".atlas");
    QDir().mkpath(settingsDirectory);
    QSettings settings(QDir(settingsDirectory).filePath("project-settings.ini"),
                       QSettings::IniFormat);
    const QString settingsKey =
        buildOnly ? "project/buildCommand" : "project/runCommand";
    const QString defaultCommand =
        buildOnly ? "atlas pack --backend METAL" : "atlas run project.atlas";
    const QString command =
        settings.value(settingsKey, defaultCommand).toString().trimmed();
    if (command.isEmpty())
        return;
    if (viewportPanel != nullptr)
        viewportPanel->saveRuntimeScene();
    if (!buildOnly) {
        QString error;
        if (!ToolchainInstaller::run({"script", "compile"},
                                     QFileInfo(projectFile).absolutePath(),
                                     &error)) {
            QMessageBox::warning(
                this, "Script Compilation Failed",
                error.isEmpty() ? "Atlas could not compile the project scripts."
                                : error);
            return;
        }
    }
    const QString workingDirectory = QFileInfo(projectFile).absolutePath();
    if (!settings.contains(settingsKey) || command == defaultCommand) {
        const QString executable = ToolchainInstaller::executablePath();
        if (executable.isEmpty()) {
            QMessageBox::warning(this,
                                 buildOnly ? "Build Project" : "Run Project",
                                 "Atlas CLI was not found. Install the Atlas "
                                 "toolchain from the Tools menu.");
            return;
        }
        const QStringList arguments =
            buildOnly ? QStringList{"pack", "--backend", "METAL"}
                      : QStringList{"run", "project.atlas"};
        QProcess::startDetached(executable, arguments, workingDirectory);
        return;
    }
    QProcess::startDetached("/bin/zsh", {"-lc", command}, workingDirectory);
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

void EditorWindow::undoActiveEditor() {
    if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget())) {
        field->undo();
    } else if (materialEditorPanel != nullptr &&
               materialEditorPanel->isAncestorOf(QApplication::focusWidget())) {
        materialEditorPanel->undo();
    } else if (graphiteEditorPanel != nullptr &&
               graphiteEditorPanel->isAncestorOf(QApplication::focusWidget())) {
        graphiteEditorPanel->undo();
    } else if (viewportPanel != nullptr) {
        viewportPanel->undo();
    }
}

void EditorWindow::redoActiveEditor() {
    if (auto *field = qobject_cast<QLineEdit *>(QApplication::focusWidget())) {
        field->redo();
    } else if (materialEditorPanel != nullptr &&
               materialEditorPanel->isAncestorOf(QApplication::focusWidget())) {
        materialEditorPanel->redo();
    } else if (graphiteEditorPanel != nullptr &&
               graphiteEditorPanel->isAncestorOf(QApplication::focusWidget())) {
        graphiteEditorPanel->redo();
    } else if (viewportPanel != nullptr) {
        viewportPanel->redo();
    }
}

bool EditorWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Polish) {
        if (auto *menu = qobject_cast<QMenu *>(watched)) {
            menu->setAttribute(Qt::WA_TranslucentBackground);
        } else if (auto *dialog = qobject_cast<QDialog *>(watched);
                   dialog != nullptr &&
                   dialog->windowFlags().testFlag(Qt::FramelessWindowHint)) {
            dialog->setAttribute(Qt::WA_TranslucentBackground);
        }
    }
    if (viewportPanel != nullptr &&
        viewportPanel->routeRuntimeInputEvent(event))
        return true;
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (!key->isAutoRepeat() && key->matches(QKeySequence::Undo)) {
            undoActiveEditor();
            return true;
        }
        if (!key->isAutoRepeat() && key->matches(QKeySequence::Redo)) {
            redoActiveEditor();
            return true;
        }
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
    settings.setValue(DockStateKey, coreManager->saveState(DockStateVersion));
    if (workspaceStack != nullptr)
        settings.setValue(
            QStringLiteral("workspace/layout/v11/%1").arg(workspaceStack->currentIndex()),
            coreManager->saveState(DockStateVersion));
    if (workspaceStack != nullptr)
        settings.setValue("workspace/mode", workspaceStack->currentIndex());
    settings.sync();
}

void EditorWindow::restoreLayout() {
    QSettings settings("Neutral Software", "Atlas Engine");

    const QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    const QByteArray dockState = settings.value(DockStateKey).toByteArray();

    restoringLayout = true;
    const bool restored =
        !dockState.isEmpty() &&
        coreManager->restoreState(dockState, DockStateVersion);
    if (!restored && !defaultDockState.isEmpty())
        coreManager->restoreState(defaultDockState, DockStateVersion);
    if (workspaceStack != nullptr)
        activateWorkspace(
            std::clamp(settings.value("workspace/mode", 0).toInt(), 0,
                       workspaceStack->count() - 1));
    restoringLayout = false;
    configureDockSplitters();
}

void EditorWindow::configureDockSplitters() {
    if (coreManager == nullptr)
        return;
    if (dockManager != nullptr) {
        if (auto *workspace = dockManager->panel("workspace")) {
            if (auto *area = workspace->dockAreaWidget())
                area->setDockAreaFlag(ads::CDockAreaWidget::HideSingleWidgetTitleBar, true);
        }
    }
    for (QSplitter *splitter : coreManager->findChildren<QSplitter *>()) {
        splitter->setHandleWidth(10);
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
#ifdef ATLAS_RELEASE_BUILD
    setWindowTitle(QStringLiteral("%1 - Atlas Engine %2")
                       .arg(name, QStringLiteral(ATLAS_VERSION)));
#else
    const QString build = QStringLiteral(ATLAS_BUILD_STRING);
    setWindowTitle(
        build.isEmpty()
            ? QStringLiteral("%1 - Atlas Engine (Development)").arg(name)
            : QStringLiteral("%1 - Atlas Engine (Development) + %2")
                  .arg(name, build));
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
