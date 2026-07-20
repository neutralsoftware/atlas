/*
 * contentBrowser.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Content Browser / File Explorer Declaration
 * Copyright (c) 2026 Max Van den Eynde
 */

#include "editor/views/fileExplorer.h"
#include "editor/application/toolchainInstaller.h"
#include "editor/styling/icons.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileIconProvider>
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
#include <QRegularExpression>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

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

bool isValidEntryName(const QString &name) {
    return !name.isEmpty() && name != "." && name != ".." &&
           !name.contains('/') && !name.contains('\\');
}

QString requestFilePath(QWidget *parent, const QString &directory,
                        const QString &title, const QString &label,
                        const QString &defaultName, const QString &extension) {
    bool accepted = false;
    QString name = QInputDialog::getText(parent, title, label, QLineEdit::Normal,
                                         defaultName, &accepted)
                       .trimmed();
    if (!accepted)
        return {};

    const QString suffix = "." + extension;
    if (name.endsWith(suffix, Qt::CaseInsensitive))
        name.chop(suffix.size());
    name = name.trimmed();
    if (!isValidEntryName(name)) {
        QMessageBox::warning(parent, title, "Enter a valid file name.");
        return {};
    }

    const QString path = QDir(directory).filePath(name + suffix);
    if (QFileInfo::exists(path)) {
        QMessageBox::warning(parent, title,
                             "A file with that name already exists.");
        return {};
    }
    return path;
}

bool copyEntry(const QString &source, const QString &destination) {
    const QFileInfo info(source);
    if (info.isDir()) {
        if (!QDir().mkpath(destination))
            return false;
        QDir directory(source);
        for (const QFileInfo &entry : directory.entryInfoList(
                 QDir::NoDotAndDotDot | QDir::AllEntries)) {
            if (!copyEntry(entry.absoluteFilePath(),
                           QDir(destination).filePath(entry.fileName()))) {
                return false;
            }
        }
        return true;
    }
    return QFile::copy(source, destination);
}

class AtlasFileIconProvider : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo &info) const override {
        if (info.isDir())
            return styling::icon(styling::Icon::Folder, "#7E929C");
        const QString suffix = info.suffix().toLower();
        if (suffix == "ascene")
            return styling::icon(styling::Icon::CubeFocus, "#8498A8");
        if (suffix == "amat" || suffix == "material")
            return styling::icon(styling::Icon::Material, "#9E897D");
        if (suffix == "ts" || suffix == "js" || suffix == "cpp" ||
            suffix == "h" || suffix == "json")
            return styling::icon(styling::Icon::FileCode, "#7E929C");
        if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
            suffix == "bmp" || suffix == "gif" || suffix == "webp" ||
            suffix == "tif" || suffix == "tiff" || suffix == "tga" ||
            suffix == "hdr" || suffix == "exr")
            return styling::icon(styling::Icon::Image, "#A1957D");
        if (suffix == "wav" || suffix == "mp3" || suffix == "ogg" ||
            suffix == "flac")
            return styling::icon(styling::Icon::MusicNote, "#849589");
        return styling::icon(styling::Icon::File, "#8490A4");
    }

    QIcon icon(IconType type) const override {
        if (type == Folder || type == Drive)
            return styling::icon(styling::Icon::Folder, "#7E929C");
        return styling::icon(styling::Icon::File, "#8490A4");
    }
};

AtlasFileIconProvider atlasFileIconProvider;

class ContentBrowserFilterModel : public QSortFilterProxyModel {
  protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override {
        const QModelIndex index =
            sourceModel()->index(sourceRow, 0, sourceParent);
        const QFileInfo info =
            qobject_cast<QFileSystemModel *>(sourceModel())->fileInfo(index);
        const QString name = info.fileName().toLower();
        if (info.isDir()) {
            if (name == "lib" || name == "dist" || name == "node_modules")
                return false;
            return true;
        }
        if (name == "package.json" || name == "package-lock.json" ||
            name == "tsconfig.json" || name == "yarn.lock" ||
            name == "bun.lock" || name == "bun.lockb")
            return false;
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow,
                                                       sourceParent);
    }
};
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
    backButton->setIcon(
        styling::icon(styling::Icon::ArrowLeft, "#AAB4C4"));
    backButton->setToolTip("Back");
    forwardButton = new QToolButton(toolbar);
    forwardButton->setObjectName("browserNavigationButton");
    forwardButton->setIcon(
        styling::icon(styling::Icon::ArrowRight, "#AAB4C4"));
    forwardButton->setToolTip("Forward");
    upButton = new QToolButton(toolbar);
    upButton->setObjectName("browserNavigationButton");
    upButton->setIcon(styling::icon(styling::Icon::ArrowUp, "#AAB4C4"));
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
    createButton->setIcon(styling::icon(styling::Icon::Plus, "#8498A8"));
    createButton->setText("Create");
    createButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    createButton->setPopupMode(QToolButton::InstantPopup);

    revealButton = new QToolButton(toolbar);
    revealButton->setObjectName("browserRevealButton");
    revealButton->setIcon(
        styling::icon(styling::Icon::FolderOpen, "#7E929C"));
    revealButton->setToolTip("Reveal in Finder");

    moreButton = new QToolButton(toolbar);
    moreButton->setObjectName("panelMoreButton");
    moreButton->setIcon(
        styling::icon(styling::Icon::DotsVertical, "#8490A4"));
    moreButton->setPopupMode(QToolButton::InstantPopup);
    moreButton->setToolTip("Content actions");

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
    model->setIconProvider(&atlasFileIconProvider);
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    model->setReadOnly(false);
    model->setRootPath(projectRoot);
    model->sort(0, Qt::AscendingOrder);

    filterModel = new ContentBrowserFilterModel();
    filterModel->setParent(this);
    filterModel->setSourceModel(model);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    filterModel->sort(0, Qt::AscendingOrder);

    gridView = new QListView(this);
    gridView->setObjectName("contentGrid");
    gridView->setModel(filterModel);
    gridView->setViewMode(QListView::IconMode);
    gridView->setFlow(QListView::LeftToRight);
    gridView->setWrapping(true);
    gridView->setResizeMode(QListView::Adjust);
    gridView->setMovement(QListView::Static);
    gridView->setGridSize(QSize(154, 118));
    gridView->setIconSize(QSize(50, 50));
    gridView->setWordWrap(true);
    gridView->setTextElideMode(Qt::ElideNone);
    gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    gridView->setDragEnabled(true);
    gridView->setDragDropMode(QAbstractItemView::DragOnly);
    gridView->setDefaultDropAction(Qt::CopyAction);
    gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    gridView->setUniformItemSizes(true);
    gridView->setSpacing(4);
    layout->addWidget(gridView, 1);

    auto *createMenu = new QMenu(createButton);
    createMenu->addAction(styling::icon(styling::Icon::Folder, "#7E929C"), "Folder",
                          this, &ContentBrowserPanel::createFolder);
    createMenu->addSeparator();
    createMenu->addAction(styling::icon(styling::Icon::CubeFocus, "#8498A8"), "Scene",
                          this, &ContentBrowserPanel::createScene);
    createMenu->addAction(styling::icon(styling::Icon::Material, "#9E897D"),
                          "Material", this,
                          &ContentBrowserPanel::createMaterial);
    createMenu->addAction(styling::icon(styling::Icon::FileCode, "#7E929C"),
                          "TypeScript Script", this,
                          &ContentBrowserPanel::createScript);
    createButton->setMenu(createMenu);

    auto *moreMenu = new QMenu(moreButton);
    moreMenu->addAction(
        styling::icon(styling::Icon::FolderOpen, "#7E929C"), "Open", this, [this] {
        if (gridView->currentIndex().isValid()) {
            openIndex(gridView->currentIndex());
        }
    });
    moreMenu->addAction(styling::icon(styling::Icon::File, "#8498A8"),
                        "Rename", this,
                        &ContentBrowserPanel::renameSelection);
    moreMenu->addAction(styling::icon(styling::Icon::Trash, "#A17F7F"),
                        "Delete", this,
                        &ContentBrowserPanel::deleteSelection);
    moreMenu->addSeparator();
    moreMenu->addAction(styling::icon(styling::Icon::FolderOpen, "#7E929C"),
                        "Reveal in Finder", this,
                        &ContentBrowserPanel::revealSelection);
    moreMenu->addAction(styling::icon(styling::Icon::FileCode, "#8490A4"),
                        "Copy Path", this,
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
                filterModel->setFilterWildcard(search.isEmpty()
                                                   ? QString()
                                                   : "*" + search + "*");
            });
    connect(gridView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
                updateNavigationState();
                emit selectionChanged(selectedPath());
            });

    auto *deleteAction = new QAction(this);
    deleteAction->setShortcuts(
        {QKeySequence::Delete,
         QKeySequence(Qt::META | Qt::Key_Backspace)});
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

    renameAction->setShortcuts({QKeySequence(Qt::Key_Return),
                                QKeySequence(Qt::Key_Enter),
                                QKeySequence(Qt::Key_F2)});

    auto *createFolderAction = new QAction(this);
    createFolderAction->setShortcut(
        QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_N));
    createFolderAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(createFolderAction, &QAction::triggered, this,
            &ContentBrowserPanel::createFolder);
    addAction(createFolderAction);

    auto *revealAction = new QAction(this);
    revealAction->setShortcut(
        QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_R));
    revealAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(revealAction, &QAction::triggered, this,
            &ContentBrowserPanel::revealSelection);
    addAction(revealAction);

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

void ContentBrowserPanel::clearSelection() {
    if (gridView->selectionModel()->selectedIndexes().isEmpty()) {
        return;
    }
    const QSignalBlocker blocker(gridView->selectionModel());
    gridView->clearSelection();
    gridView->setCurrentIndex(QModelIndex());
    updateNavigationState();
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
    gridView->setRootIndex(
        filterModel->mapFromSource(model->index(currentPath)));
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
    emit selectionChanged(QString());
}

void ContentBrowserPanel::openIndex(const QModelIndex &index) {
    const QFileInfo info = model->fileInfo(filterModel->mapToSource(index));
    if (info.isDir()) {
        navigateTo(info.absoluteFilePath());
        return;
    }
    const QString suffix = info.suffix().toLower();
    if (suffix == "amat" || suffix == "material") {
        emit assetActivated(info.absoluteFilePath());
        return;
    }
    if (suffix == "ascene") {
        emit sceneActivated(info.absoluteFilePath());
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
    const QString entryName = name.trimmed();
    if (!accepted) {
        return;
    }
    if (!isValidEntryName(entryName)) {
        QMessageBox::warning(this, "New Folder", "Enter a valid folder name.");
        return;
    }
    if (!QDir(currentPath).mkdir(entryName)) {
        QMessageBox::warning(this, "New Folder",
                             "The folder could not be created.");
    }
}

void ContentBrowserPanel::createScene() {
    const QString path = requestFilePath(this, currentPath, "New Scene",
                                         "Scene name", "New Scene", "ascene");
    if (path.isEmpty())
        return;
    if (writeNewFile(path, EmptyScene)) {
        gridView->setCurrentIndex(
            filterModel->mapFromSource(model->index(path)));
    }
}

void ContentBrowserPanel::createScript() {
    const QString path = requestFilePath(this, currentPath, "New Script",
                                         "Script name", "NewScript", "ts");
    if (path.isEmpty())
        return;
    const QString relativePath = QDir(projectRoot).relativeFilePath(path);
    QString componentName = QFileInfo(path).completeBaseName();
    componentName.remove(QRegularExpression("[^A-Za-z0-9_$]"));
    if (componentName.isEmpty())
        componentName = "NewScript";
    if (componentName.front().isDigit())
        componentName.prepend("Script");
    QString error;
    if (!ToolchainInstaller::run(
            {"script", "new", relativePath, "--component-name", componentName},
            projectRoot, &error)) {
        QMessageBox::warning(
            this, "New Script",
            error.isEmpty() ? "Atlas could not create the script." : error);
        return;
    }
    gridView->setCurrentIndex(filterModel->mapFromSource(model->index(path)));
}

void ContentBrowserPanel::createMaterial() {
    const QString path = requestFilePath(this, currentPath, "New Material",
                                         "Material name", "New Material",
                                         "amat");
    if (path.isEmpty())
        return;
    const QByteArray material =
        "{\n"
        "    \"material\": {\n"
        "        \"albedo\": [0.8, 0.8, 0.8, 1.0],\n"
        "        \"metallic\": 0.0,\n"
        "        \"roughness\": 0.5,\n"
        "        \"ao\": 1.0,\n"
        "        \"reflectivity\": 0.5,\n"
        "        \"emissiveColor\": [0.0, 0.0, 0.0, 1.0],\n"
        "        \"emissiveIntensity\": 0.0,\n"
        "        \"normalMapStrength\": 1.0,\n"
        "        \"useNormalMap\": true,\n"
        "        \"transmittance\": 0.0,\n"
        "        \"ior\": 1.45\n"
        "    }\n"
        "}\n";
    if (writeNewFile(path, material)) {
        const QModelIndex index =
            filterModel->mapFromSource(model->index(path));
        gridView->setCurrentIndex(index);
        emit assetActivated(path);
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
    const QString entryName = name.trimmed();
    if (!accepted || entryName == info.fileName()) {
        return;
    }
    if (!isValidEntryName(entryName)) {
        QMessageBox::warning(this, "Rename", "Enter a valid file name.");
        return;
    }
    if (!QDir(info.absolutePath()).rename(info.fileName(), entryName)) {
        QMessageBox::warning(this, "Rename", "The item could not be renamed.");
        return;
    }
    gridView->setCurrentIndex(
        filterModel->mapFromSource(
            model->index(QDir(info.absolutePath()).filePath(entryName))));
}

void ContentBrowserPanel::deleteSelection() {
    const QModelIndexList selected =
        gridView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }
    for (const QModelIndex &index : selected) {
        const QFileInfo info =
            model->fileInfo(filterModel->mapToSource(index));
        if (info.isSymLink()) {
            QFile::remove(info.absoluteFilePath());
        } else if (info.isDir()) {
            QDir(info.absoluteFilePath()).removeRecursively();
        } else {
            QFile::remove(info.absoluteFilePath());
        }
    }
}

void ContentBrowserPanel::focusSearch() {
    searchField->setFocus();
    searchField->selectAll();
}

void ContentBrowserPanel::copySelection() {
    clipboardPaths.clear();
    for (const QModelIndex &index :
         gridView->selectionModel()->selectedIndexes()) {
        clipboardPaths.append(
            model->filePath(filterModel->mapToSource(index)));
    }
    cutClipboard = false;
}

void ContentBrowserPanel::cutSelection() {
    copySelection();
    cutClipboard = true;
}

void ContentBrowserPanel::pasteSelection() {
    for (const QString &source : std::as_const(clipboardPaths)) {
        const QFileInfo info(source);
        QString destination = QDir(currentPath).filePath(info.fileName());
        if (QFileInfo(destination).exists())
            destination = uniquePath(info.completeBaseName() + " Copy" +
                                     (info.suffix().isEmpty()
                                          ? QString()
                                          : "." + info.suffix()));
        if (cutClipboard) {
            QDir().rename(source, destination);
        } else {
            copyEntry(source, destination);
        }
    }
    if (cutClipboard)
        clipboardPaths.clear();
    refreshAssets();
}

void ContentBrowserPanel::duplicateSelection() {
    copySelection();
    pasteSelection();
}

void ContentBrowserPanel::refreshAssets() {
    model->setRootPath(QString());
    model->setRootPath(projectRoot);
    gridView->setRootIndex(
        filterModel->mapFromSource(model->index(currentPath)));
}

void ContentBrowserPanel::selectAllAssets() { gridView->selectAll(); }

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
    return index.isValid() && gridView->selectionModel()->isSelected(index)
               ? model->filePath(filterModel->mapToSource(index))
               : QString();
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
