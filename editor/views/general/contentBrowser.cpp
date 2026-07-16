/*
 * contentBrowser.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Content Browser / File Explorer Declaration
 * Copyright (c) 2026 Max Van den Eynde
 */

#include "editor/views/fileExplorer.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QSaveFile>
#include <QSize>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {
const QByteArray EmptyScene = R"({
    "name": "New Scene",
    "id": "new_scene",
    "objects": [],
    "lights": [
        {
            "type": "ambient",
            "intensity": 0.25
        }
    ],
    "camera": {
        "position": [0.0, 1.5, -5.0],
        "target": [0.0, 0.0, 0.0],
        "fov": 60.0
    },
    "targets": [
        {
            "name": "Main Target",
            "type": "scene",
            "render": true,
            "display": true
        }
    ]
}
)";

bool writeNewFile(const QString &path, const QByteArray &contents) {
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) &&
           file.write(contents) == contents.size() && file.commit();
}
} // namespace

ContentBrowserPanel::ContentBrowserPanel(const QString &projectFile,
                                         QWidget *parent)
    : QWidget(parent) {
    setObjectName("contentBrowserPanel");
    const QFileInfo projectInfo(projectFile);
    projectRoot = projectInfo.absoluteDir().canonicalPath();
    if (projectRoot.isEmpty()) {
        projectRoot = projectInfo.absoluteDir().absolutePath();
    }

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("panelToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);

    backButton = new QToolButton(toolbar);
    backButton->setObjectName("browserNavigationButton");
    backButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    backButton->setToolTip("Back");
    forwardButton = new QToolButton(toolbar);
    forwardButton->setObjectName("browserNavigationButton");
    forwardButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    forwardButton->setToolTip("Forward");
    upButton = new QToolButton(toolbar);
    upButton->setObjectName("browserNavigationButton");
    upButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    upButton->setToolTip("Parent Folder");

    pathField = new QLineEdit(toolbar);
    pathField->setObjectName("contentPathField");
    pathField->setReadOnly(true);

    searchField = new QLineEdit(toolbar);
    searchField->setObjectName("contentSearchField");
    searchField->setPlaceholderText("Search");
    searchField->setClearButtonEnabled(true);
    searchField->setMaximumWidth(180);

    createButton = new QToolButton(toolbar);
    createButton->setObjectName("panelAddButton");
    createButton->setText("+ Create");
    createButton->setPopupMode(QToolButton::InstantPopup);

    revealButton = new QToolButton(toolbar);
    revealButton->setObjectName("browserRevealButton");
    revealButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    revealButton->setText("Reveal");
    revealButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    revealButton->setToolTip("Reveal in Finder");

    moreButton = new QToolButton(toolbar);
    moreButton->setObjectName("panelMoreButton");
    moreButton->setText("•••");
    moreButton->setPopupMode(QToolButton::InstantPopup);

    toolbarLayout->addWidget(backButton);
    toolbarLayout->addWidget(forwardButton);
    toolbarLayout->addWidget(upButton);
    toolbarLayout->addWidget(pathField, 1);
    toolbarLayout->addWidget(searchField);
    toolbarLayout->addWidget(createButton);
    toolbarLayout->addWidget(revealButton);
    toolbarLayout->addWidget(moreButton);
    layout->addWidget(toolbar);

    model = new QFileSystemModel(this);
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    model->setReadOnly(false);
    model->setRootPath(projectRoot);

    gridView = new QListView(this);
    gridView->setObjectName("contentGrid");
    gridView->setModel(model);
    gridView->setViewMode(QListView::IconMode);
    gridView->setFlow(QListView::LeftToRight);
    gridView->setWrapping(true);
    gridView->setResizeMode(QListView::Adjust);
    gridView->setMovement(QListView::Static);
    gridView->setGridSize(QSize(112, 104));
    gridView->setIconSize(QSize(56, 56));
    gridView->setWordWrap(true);
    gridView->setTextElideMode(Qt::ElideMiddle);
    gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    gridView->setUniformItemSizes(true);
    layout->addWidget(gridView, 1);

    auto *createMenu = new QMenu(createButton);
    createMenu->addAction(style()->standardIcon(QStyle::SP_DirIcon), "Folder",
                          this, &ContentBrowserPanel::createFolder);
    createMenu->addSeparator();
    createMenu->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Scene",
                          this, &ContentBrowserPanel::createScene);
    createMenu->addAction(style()->standardIcon(QStyle::SP_FileIcon),
                          "TypeScript Script", this,
                          &ContentBrowserPanel::createScript);
    createButton->setMenu(createMenu);

    auto *moreMenu = new QMenu(moreButton);
    moreMenu->addAction("Open", this, [this] {
        if (gridView->currentIndex().isValid()) {
            openIndex(gridView->currentIndex());
        }
    });
    moreMenu->addAction("Rename", this, &ContentBrowserPanel::renameSelection);
    moreMenu->addAction("Delete", this, &ContentBrowserPanel::deleteSelection);
    moreMenu->addSeparator();
    moreMenu->addAction("Reveal in Finder", this,
                        &ContentBrowserPanel::revealSelection);
    moreMenu->addAction("Copy Path", this,
                        &ContentBrowserPanel::copySelectionPath);
    moreButton->setMenu(moreMenu);

    connect(gridView, &QListView::doubleClicked, this,
            &ContentBrowserPanel::openIndex);
    connect(gridView, &QListView::customContextMenuRequested, this,
            &ContentBrowserPanel::showContextMenu);
    connect(revealButton, &QToolButton::clicked, this,
            &ContentBrowserPanel::revealSelection);
    connect(backButton, &QToolButton::clicked, this, [this] {
        if (historyIndex > 0) {
            --historyIndex;
            navigateTo(history.at(historyIndex), false);
        }
    });
    connect(forwardButton, &QToolButton::clicked, this, [this] {
        if (historyIndex + 1 < history.size()) {
            ++historyIndex;
            navigateTo(history.at(historyIndex), false);
        }
    });
    connect(upButton, &QToolButton::clicked, this, [this] {
        navigateTo(QFileInfo(currentPath).absoluteDir().absolutePath());
    });
    connect(searchField, &QLineEdit::textChanged, this,
            [this](const QString &search) {
                model->setNameFilters(search.isEmpty()
                                          ? QStringList()
                                          : QStringList{"*" + search + "*"});
                model->setNameFilterDisables(false);
            });
    connect(gridView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] { updateNavigationState(); });

    auto *deleteAction = new QAction(this);
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteAction, &QAction::triggered, this,
            &ContentBrowserPanel::deleteSelection);
    addAction(deleteAction);

    auto *renameAction = new QAction(this);
    renameAction->setShortcut(Qt::Key_F2);
    renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(renameAction, &QAction::triggered, this,
            &ContentBrowserPanel::renameSelection);
    addAction(renameAction);

    navigateTo(projectRoot);
}

void ContentBrowserPanel::setRootPath(const QString &path) {
    const QFileInfo info(path);
    projectRoot = info.isDir() ? info.canonicalFilePath()
                               : info.absoluteDir().canonicalPath();
    if (projectRoot.isEmpty()) {
        projectRoot = info.isDir() ? info.absoluteFilePath()
                                   : info.absoluteDir().absolutePath();
    }
    model->setRootPath(projectRoot);
    history.clear();
    historyIndex = -1;
    navigateTo(projectRoot);
}

void ContentBrowserPanel::navigateTo(const QString &path, bool recordHistory) {
    const QFileInfo info(path);
    const QString target = info.canonicalFilePath().isEmpty()
                               ? info.absoluteFilePath()
                               : info.canonicalFilePath();
    if (!info.isDir() || !isInsideProject(target)) {
        return;
    }

    currentPath = target;
    gridView->setRootIndex(model->index(currentPath));
    gridView->clearSelection();
    searchField->clear();

    if (recordHistory) {
        while (history.size() > historyIndex + 1) {
            history.removeLast();
        }
        if (history.isEmpty() || history.constLast() != currentPath) {
            history.append(currentPath);
        }
        historyIndex = history.size() - 1;
    }
    updateNavigationState();
}

void ContentBrowserPanel::openIndex(const QModelIndex &index) {
    const QFileInfo info = model->fileInfo(index);
    if (info.isDir()) {
        navigateTo(info.absoluteFilePath());
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
}

void ContentBrowserPanel::showContextMenu(const QPoint &position) {
    const QModelIndex index = gridView->indexAt(position);
    if (index.isValid()) {
        gridView->setCurrentIndex(index);
        if (!gridView->selectionModel()->isSelected(index)) {
            gridView->selectionModel()->select(
                index, QItemSelectionModel::ClearAndSelect);
        }
    }

    QMenu menu(this);
    auto *createMenu = menu.addMenu("Create");
    for (QAction *action : createButton->menu()->actions()) {
        createMenu->addAction(action);
    }
    if (index.isValid()) {
        menu.addSeparator();
        menu.addAction("Open", this, [this, index] { openIndex(index); });
        menu.addAction("Rename", this, &ContentBrowserPanel::renameSelection);
        menu.addAction("Delete", this, &ContentBrowserPanel::deleteSelection);
        menu.addSeparator();
        menu.addAction("Reveal in Finder", this,
                       &ContentBrowserPanel::revealSelection);
        menu.addAction("Copy Path", this,
                       &ContentBrowserPanel::copySelectionPath);
    } else {
        menu.addSeparator();
        menu.addAction("Reveal This Folder in Finder", this, [this] {
#ifdef Q_OS_MACOS
            QProcess::startDetached("/usr/bin/open", {currentPath});
#else
            QDesktopServices::openUrl(QUrl::fromLocalFile(currentPath));
#endif
        });
    }
    menu.exec(gridView->viewport()->mapToGlobal(position));
}

void ContentBrowserPanel::showCreateMenu(const QPoint &position) {
    if (createButton->menu() != nullptr) {
        createButton->menu()->popup(position);
    }
}

void ContentBrowserPanel::createFolder() {
    bool accepted = false;
    const QString name =
        QInputDialog::getText(this, "New Folder", "Folder name",
                              QLineEdit::Normal, "New Folder", &accepted);
    if (accepted && !name.trimmed().isEmpty()) {
        QDir(currentPath).mkdir(name.trimmed());
    }
}

void ContentBrowserPanel::createScene() {
    const QString path = uniquePath("New Scene.ascene");
    if (writeNewFile(path, EmptyScene)) {
        gridView->setCurrentIndex(model->index(path));
    }
}

void ContentBrowserPanel::createScript() {
    const QString path = uniquePath("NewScript.ts");
    const QByteArray script = "import { Component } from \"atlas\";\n\n"
                              "export class NewScript extends Component {\n"
                              "    init() {}\n\n"
                              "    update(deltaTime: number) {}\n"
                              "}\n";
    if (writeNewFile(path, script)) {
        gridView->setCurrentIndex(model->index(path));
    }
}

void ContentBrowserPanel::renameSelection() {
    const QString path = selectedPath();
    if (path.isEmpty()) {
        return;
    }
    const QFileInfo info(path);
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this, "Rename", "Name", QLineEdit::Normal, info.fileName(), &accepted);
    if (!accepted || name.trimmed().isEmpty() || name == info.fileName()) {
        return;
    }
    QDir(info.absolutePath()).rename(info.fileName(), name.trimmed());
}

void ContentBrowserPanel::deleteSelection() {
    const QModelIndexList selected =
        gridView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }
    const QString prompt =
        selected.size() == 1
            ? QStringLiteral("Delete “%1”? This cannot be undone.")
                  .arg(model->fileName(selected.first()))
            : QStringLiteral("Delete %1 items? This cannot be undone.")
                  .arg(selected.size());
    if (QMessageBox::warning(this, "Delete Assets", prompt,
                             QMessageBox::Cancel | QMessageBox::Yes,
                             QMessageBox::Cancel) != QMessageBox::Yes) {
        return;
    }
    for (const QModelIndex &index : selected) {
        const QFileInfo info = model->fileInfo(index);
        if (info.isDir()) {
            QDir(info.absoluteFilePath()).removeRecursively();
        } else {
            QFile::remove(info.absoluteFilePath());
        }
    }
}

void ContentBrowserPanel::revealSelection() const {
    const QString path =
        selectedPath().isEmpty() ? currentPath : selectedPath();
#ifdef Q_OS_MACOS
    QProcess::startDetached("/usr/bin/open", {"-R", path});
#elif defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe",
                            {"/select,", QDir::toNativeSeparators(path)});
#else
    const QFileInfo info(path);
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(info.isDir() ? path : info.absolutePath()));
#endif
}

void ContentBrowserPanel::copySelectionPath() const {
    const QString path =
        selectedPath().isEmpty() ? currentPath : selectedPath();
    QApplication::clipboard()->setText(path);
}

QString ContentBrowserPanel::selectedPath() const {
    const QModelIndex index = gridView->currentIndex();
    return index.isValid() ? model->filePath(index) : QString();
}

QString ContentBrowserPanel::uniquePath(const QString &baseName) const {
    const QFileInfo base(baseName);
    QString path = QDir(currentPath).filePath(baseName);
    int suffix = 2;
    while (QFileInfo::exists(path)) {
        const QString name =
            base.completeBaseName() + ' ' + QString::number(suffix++) +
            (base.suffix().isEmpty() ? QString() : '.' + base.suffix());
        path = QDir(currentPath).filePath(name);
    }
    return path;
}

bool ContentBrowserPanel::isInsideProject(const QString &path) const {
    const QString root = QDir::cleanPath(projectRoot);
    const QString candidate = QDir::cleanPath(path);
    return candidate == root || candidate.startsWith(root + QDir::separator());
}

void ContentBrowserPanel::updateNavigationState() {
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex + 1 < history.size());
    upButton->setEnabled(currentPath != projectRoot);
    pathField->setText(
        QDir(projectRoot).relativeFilePath(currentPath) == "."
            ? QFileInfo(projectRoot).fileName()
            : QFileInfo(projectRoot).fileName() + '/' +
                  QDir(projectRoot).relativeFilePath(currentPath));
    const bool hasSelection = !selectedPath().isEmpty();
    moreButton->setEnabled(hasSelection);
}
