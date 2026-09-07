#include "editor/styling/workbench.h"

/*
 * hierarchy.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Hierarchy panel definition
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/hierarchyPanel.h>
#include <editor/styling/icons.h>

#include <QAction>
#include <QAbstractItemView>
#include <QDialog>
#include <QDropEvent>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPair>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyle>
#include <QToolButton>
#include <QTreeView>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "editor/views/viewport.h"

namespace {
constexpr int ObjectIdRole = Qt::UserRole + 1;
constexpr int ObjectTypeRole = Qt::UserRole + 2;
constexpr int AssetPathRole = Qt::UserRole + 3;
constexpr int CreationTypeRole = Qt::UserRole + 4;
constexpr int CreationNameRole = Qt::UserRole + 5;
constexpr int CreationCategoryRole = Qt::UserRole + 6;
constexpr int NoCreationResultsRole = Qt::UserRole + 7;

struct CreationEntry {
    QString category;
    QString name;
    QString type;
};

const QList<CreationEntry> &creationEntries() {
    static const QList<CreationEntry> entries = {
        {"3D Object", "Cube", "cube"},
        {"3D Object", "Sphere", "sphere"},
        {"3D Object", "Plane", "plane"},
        {"3D Object", "Pyramid", "pyramid"},
        {"3D Object", "Capsule", "capsule"},
        {"3D Object", "Terrain", "terrain"},
        {"Light", "Point Light", "pointLight"},
        {"Light", "Spot Light", "spotLight"},
        {"Light", "Directional Light", "directionalLight"},
        {"Light", "Area Light", "areaLight"},
        {"Light", "Ambient Light", "ambientLight"},
        {"Scene", "Empty Object", "group"},
        {"Scene", "Camera", "camera"},
        {"Scene", "Particle Emitter", "particleEmitter"},
    };
    return entries;
}

int objectCount(const QJsonArray &objects) {
    int count = 0;
    for (const QJsonValue &value : objects) {
        ++count;
        count += objectCount(value.toObject().value("children").toArray());
    }
    return count;
}

QIcon hierarchyIcon(QWidget *, const QString &type) {
    const QString normalized = type.toLower();
    if (normalized == "scene")
        return styling::icon(styling::Icon::CubeFocus, "#7E929C");
    if (normalized == "compound" || normalized == "group")
        return styling::icon(styling::Icon::Folder, "#8490A4");
    if (normalized == "camera")
        return styling::icon(styling::Icon::Camera, "#9E897D");
    if (normalized == "environment")
        return styling::icon(styling::Icon::Globe, "#7E929C");
    if (normalized == "graphite" || normalized == "graphiteasset")
        return styling::icon(styling::Icon::Palette, "#849589");
    if (normalized.contains("light") || normalized == "sun")
        return styling::icon(styling::Icon::Lightbulb, "#A1957D");
    if (normalized == "terrain" || normalized == "landscape")
        return styling::icon(styling::Icon::Mountains, "#849589");
    if (normalized == "particleemitter" || normalized == "particles")
        return styling::icon(styling::Icon::Sparkle, "#8498A8");
    if (normalized == "model")
        return styling::icon(styling::Icon::Cube, "#7E929C");
    if (normalized == "sphere")
        return styling::icon(styling::Icon::Sphere, "#8498A8");
    return styling::icon(styling::Icon::Cube, "#8498A8");
}

QString objectSignature(const QJsonArray &objects) {
    QString signature;
    for (const QJsonValue &value : objects) {
        const QJsonObject object = value.toObject();
        signature += QStringLiteral("%1:%2:%3[")
                         .arg(object.value("id").toInt())
                         .arg(object.value("name").toString(),
                              object.value("type").toString());
        signature += objectSignature(object.value("children").toArray());
        signature += ']';
    }
    return signature;
}
} // namespace

HierarchyPanel::HierarchyPanel(ViewportPanel *viewport, QWidget *parent)
    : QWidget(parent), viewport(viewport) {
    setObjectName("hierarchyPanel");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 10);
    layout->setSpacing(4);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("panelToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);

    addButton = new styling::ToolButton(toolbar);
    addButton->setObjectName("panelAddButton");
    addButton->setIcon(styling::icon(styling::Icon::Plus, "#8498A8"));
    addButton->setText("Add");
    addButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addButton->setPopupMode(QToolButton::InstantPopup);
    addButton->setToolTip("Add Object");

    moreButton = new styling::ToolButton(toolbar);
    moreButton->setObjectName("panelMoreButton");
    moreButton->setIcon(styling::icon(styling::Icon::DotsVertical, "#8490A4"));
    moreButton->setPopupMode(QToolButton::InstantPopup);
    moreButton->setToolTip("Hierarchy actions");


    searchField = new QLineEdit(toolbar);
    searchField->setPlaceholderText("Search hierarchy");
    searchField->setClearButtonEnabled(true);
    searchField->setMinimumWidth(80);
    searchField->setObjectName("hierarchySearch");
    toolbarLayout->addWidget(searchField, 1);
    toolbarLayout->addWidget(addButton);
    toolbarLayout->addWidget(moreButton);
    layout->addWidget(toolbar);

    treeView = new styling::TreeView(this);
    treeView->setObjectName("hierarchyTree");
    model = new QStandardItemModel(this);
    treeView->setModel(model);
    treeView->setHeaderHidden(true);
    treeView->setAnimated(false);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView->setIndentation(20);
    treeView->setIconSize(QSize(18, 18));
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->setUniformRowHeights(true);
    treeView->setAcceptDrops(true);
    treeView->setDragEnabled(true);
    treeView->setDragDropMode(QAbstractItemView::DragDrop);
    treeView->setDefaultDropAction(Qt::MoveAction);
    treeView->setDropIndicatorShown(true);
    treeView->viewport()->setAcceptDrops(true);
    treeView->viewport()->installEventFilter(this);
    layout->addWidget(treeView);

    auto *addMenu = new QMenu(addButton);
    auto *objectsMenu = addMenu->addMenu("3D Object");
    const QList<QPair<QString, QString>> objects = {
        {"Cube", "cube"},       {"Sphere", "sphere"},   {"Plane", "plane"},
        {"Pyramid", "pyramid"}, {"Capsule", "capsule"}, {"Terrain", "terrain"}};
    for (const auto &[label, type] : objects) {
        objectsMenu->addAction(
            hierarchyIcon(this, type), label, this,
            [this, type, label] { createObject(type, label); });
    }

    auto *lightsMenu = addMenu->addMenu("Light");
    const QList<QPair<QString, QString>> lights = {
        {"Point Light", "pointLight"},
        {"Spot Light", "spotLight"},
        {"Directional Light", "directionalLight"},
        {"Area Light", "areaLight"},
        {"Ambient Light", "ambientLight"}};
    for (const auto &[label, type] : lights) {
        lightsMenu->addAction(
            hierarchyIcon(this, type), label, this,
            [this, type, label] { createObject(type, label); });
    }
    addMenu->addSeparator();
    addMenu->addAction(hierarchyIcon(this, "group"), "Empty Object", this,
                       [this] { createObject("group", "Empty Object"); });
    addMenu->addAction(hierarchyIcon(this, "camera"), "Camera", this,
                       [this] { createObject("camera", "Camera"); });
    addMenu->addAction(
        hierarchyIcon(this, "particleEmitter"), "Particle Emitter", this,
        [this] { createObject("particleEmitter", "Particle Emitter"); });
    addButton->setMenu(addMenu);

    auto *moreMenu = new QMenu(moreButton);
    moreMenu->addAction("Focus Selection", this,
                        &HierarchyPanel::focusSelectedObject);
    moreMenu->addAction("Rename", this, &HierarchyPanel::renameSelectedObject);
    moreMenu->addAction("Move to Scene Root", this,
                        &HierarchyPanel::moveSelectedObjectToRoot);
    moreMenu->addSeparator();
    moreMenu->addAction("Save Scene", this, [this] {
        if (this->viewport != nullptr) {
            this->viewport->saveRuntimeScene();
        }
    });
    moreButton->setMenu(moreMenu);

    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &, const QModelIndex &) {
                focusSelectedObject();
            });
    connect(treeView, &QTreeView::doubleClicked, this,
            [this](const QModelIndex &) { renameSelectedObject(); });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            &HierarchyPanel::showContextMenu);
    connect(searchField, &QLineEdit::textChanged, this,
            [this](const QString &query) {
                const QString normalized = query.trimmed();
                for (auto item = itemsById.begin(); item != itemsById.end();
                     ++item) {
                    QStandardItem *entry = item.value();
                    const QModelIndex parentIndex =
                        entry->parent() != nullptr ? entry->parent()->index()
                                                   : QModelIndex();
                    treeView->setRowHidden(
                        entry->row(), parentIndex,
                        !normalized.isEmpty() &&
                            !entry->text().contains(normalized,
                                                    Qt::CaseInsensitive));
                }
            });

    auto *deleteAction = new QAction(this);
    deleteAction->setShortcuts(
        {QKeySequence::Delete, QKeySequence(Qt::META | Qt::Key_Backspace)});
    deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteAction, &QAction::triggered, this,
            &HierarchyPanel::deleteSelectedObject);
    addAction(deleteAction);

    auto *deleteWithXAction = new QAction(treeView);
    deleteWithXAction->setShortcut(QKeySequence(Qt::Key_X));
    deleteWithXAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteWithXAction, &QAction::triggered, this,
            &HierarchyPanel::deleteSelectedObject);
    treeView->addAction(deleteWithXAction);

    auto *renameAction = new QAction(this);
    renameAction->setShortcuts({QKeySequence(Qt::Key_Return),
                                QKeySequence(Qt::Key_Enter),
                                QKeySequence(Qt::Key_F2)});
    renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(renameAction, &QAction::triggered, this,
            &HierarchyPanel::renameSelectedObject);
    addAction(renameAction);

    auto *focusAction = new QAction(this);
    focusAction->setShortcut(Qt::Key_F);
    focusAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(focusAction, &QAction::triggered, this,
            &HierarchyPanel::focusSelectedObject);
    addAction(focusAction);

    auto *createEmptyAction = new QAction(this);
    createEmptyAction->setShortcut(
        QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_N));
    createEmptyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(createEmptyAction, &QAction::triggered, this,
            [this] { createObject("group", "Empty Object"); });
    addAction(createEmptyAction);

    if (viewport != nullptr) {
        addButton->setEnabled(false);
        moreButton->setEnabled(false);
        treeView->setEnabled(false);
        connect(viewport, &ViewportPanel::sceneSnapshotChanged, this,
                &HierarchyPanel::applySceneSnapshot);
        connect(viewport, &ViewportPanel::runtimeAvailabilityChanged, this,
                [this](bool available) {
                    addButton->setEnabled(available);
                    moreButton->setEnabled(available);
                    treeView->setEnabled(available);
                    if (!available) {
                        lastStructureSignature.clear();
                        return;
                    }
                    QTimer::singleShot(0, this, [this] {
                        const QString snapshot =
                            this->viewport->currentSceneSnapshot();
                        if (!snapshot.isEmpty())
                            applySceneSnapshot(snapshot);
                    });
                });
        QTimer::singleShot(0, this, [this] {
            const QString snapshot = this->viewport->currentSceneSnapshot();
            if (!snapshot.isEmpty())
                applySceneSnapshot(snapshot);
        });
    }
}

void HierarchyPanel::applySceneSnapshot(const QString &snapshot) {
    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(snapshot.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const QJsonObject scene = document.object();
    const QString sceneName = scene.value("name").toString("Scene");
    const QJsonArray objects = scene.value("objects").toArray();
    const QJsonArray interfaces = scene.value("ui").toArray();
    const int selectedId = scene.value("selectedId").toInt(-1);
    const QString signature = sceneSignature(sceneName, objects, interfaces);

    const bool incompleteModel = model->rowCount() != 1 ||
                                 itemsById.size() != objectCount(objects) ||
                                 !specialItems.contains("camera") ||
                                 !specialItems.contains("environment") ||
                                 !specialItems.contains("graphite");
    if (signature != lastStructureSignature || incompleteModel) {
        rebuildScene(sceneName, objects, interfaces, selectedId);
        lastStructureSignature = signature;
        return;
    }

    if (treeView->selectionModel()->selectedRows().size() > 1)
        return;

    applyingSnapshot = true;
    const QSignalBlocker blocker(treeView->selectionModel());
    treeView->clearSelection();
    treeView->setCurrentIndex(QModelIndex());
    if (itemsById.contains(selectedId)) {
        const QModelIndex index = itemsById.value(selectedId)->index();
        treeView->setCurrentIndex(index);
        treeView->scrollTo(index, QAbstractItemView::EnsureVisible);
        selectedSpecialType.clear();
    } else if (specialItems.contains(selectedSpecialType)) {
        const QModelIndex index =
            specialItems.value(selectedSpecialType)->index();
        treeView->setCurrentIndex(index);
        treeView->scrollTo(index, QAbstractItemView::EnsureVisible);
    } else {
        treeView->setCurrentIndex(QModelIndex());
    }
    applyingSnapshot = false;
    treeView->viewport()->repaint();
}

void HierarchyPanel::rebuildScene(const QString &sceneName,
                                  const QJsonArray &objects,
                                  const QJsonArray &interfaces,
                                  int selectedId) {
    applyingSnapshot = true;
    model->clear();
    itemsById.clear();
    specialItems.clear();

    auto *root = new QStandardItem(hierarchyIcon(this, "scene"), sceneName);
    root->setData(-1, ObjectIdRole);
    root->setData("scene", ObjectTypeRole);
    root->setEditable(false);
    appendObjects(root, objects);

    auto *mainCamera =
        new QStandardItem(hierarchyIcon(this, "camera"), "Main Camera");
    mainCamera->setData(-1, ObjectIdRole);
    mainCamera->setData("camera", ObjectTypeRole);
    mainCamera->setToolTip("Scene camera");
    mainCamera->setEditable(false);
    specialItems.insert("camera", mainCamera);
    root->appendRow(mainCamera);

    auto *environment =
        new QStandardItem(hierarchyIcon(this, "environment"), "Environment");
    environment->setData(-1, ObjectIdRole);
    environment->setData("environment", ObjectTypeRole);
    environment->setToolTip("Scene atmosphere and environment");
    environment->setEditable(false);
    specialItems.insert("environment", environment);
    root->appendRow(environment);

    auto *graphite =
        new QStandardItem(hierarchyIcon(this, "graphite"), "Graphite Overlay");
    graphite->setData(-1, ObjectIdRole);
    graphite->setData("graphite", ObjectTypeRole);
    graphite->setToolTip("Scene UI overlays");
    graphite->setEditable(false);
    specialItems.insert("graphite", graphite);
    for (const QJsonValue &value : interfaces) {
        QString source;
        bool enabled = true;
        if (value.isString()) {
            source = value.toString();
        } else if (value.isObject()) {
            const QJsonObject entry = value.toObject();
            source = entry.value("source").toString();
            enabled = entry.value("enabled").toBool(true);
        }
        if (source.isEmpty())
            continue;
        QString label = QFileInfo(source).completeBaseName();
        if (label.isEmpty())
            label = source;
        if (!enabled)
            label += " (Disabled)";
        auto *asset =
            new QStandardItem(hierarchyIcon(this, "graphiteAsset"), label);
        asset->setData(-1, ObjectIdRole);
        asset->setData("graphiteAsset", ObjectTypeRole);
        asset->setData(source, AssetPathRole);
        asset->setToolTip(source);
        asset->setEditable(false);
        const QString key = "graphite:" + source;
        specialItems.insert(key, asset);
        graphite->appendRow(asset);
    }
    root->appendRow(graphite);
    model->appendRow(root);
    treeView->expandAll();

    if (itemsById.contains(selectedId)) {
        const QModelIndex index = itemsById.value(selectedId)->index();
        treeView->setCurrentIndex(index);
        treeView->scrollTo(index, QAbstractItemView::EnsureVisible);
        selectedSpecialType.clear();
    } else if (specialItems.contains(selectedSpecialType)) {
        treeView->setCurrentIndex(
            specialItems.value(selectedSpecialType)->index());
    }
    applyingSnapshot = false;
    treeView->doItemsLayout();
    treeView->viewport()->repaint();
}

bool HierarchyPanel::eventFilter(QObject *watched, QEvent *event) {
    if (treeView != nullptr && watched == treeView->viewport() &&
        event->type() == QEvent::MouseButtonPress) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        const QModelIndex index =
            treeView->indexAt(mouse->position().toPoint());
        draggedObjectId =
            index.isValid() ? index.data(ObjectIdRole).toInt() : -1;
    }
    if (treeView != nullptr && watched == treeView->viewport() &&
        (event->type() == QEvent::DragEnter ||
         event->type() == QEvent::DragMove || event->type() == QEvent::Drop)) {
        auto *drop = static_cast<QDropEvent *>(event);
        const QModelIndex index = treeView->indexAt(drop->position().toPoint());
        const int objectId = index.data(ObjectIdRole).toInt();
        if (drop->mimeData()->hasUrls() && objectId >= 0) {
            const QString suffix =
                QFileInfo(drop->mimeData()->urls().constFirst().toLocalFile())
                    .suffix()
                    .toLower();
            const bool supported = suffix == "amat" || suffix == "material" ||
                                   suffix == "ts" || suffix == "js" ||
                                   suffix == "wav" || suffix == "mp3" ||
                                   suffix == "ogg" || suffix == "flac" ||
                                   suffix == "m4a" || suffix == "aac";
            if (supported && event->type() == QEvent::Drop &&
                viewport != nullptr &&
                viewport->attachRuntimeAsset(
                    objectId,
                    drop->mimeData()->urls().constFirst().toLocalFile())) {
                treeView->setCurrentIndex(index);
                viewport->selectRuntimeObject(objectId, false);
                emit objectActivated(objectId);
                drop->acceptProposedAction();
                return true;
            }
            if (supported && event->type() != QEvent::Drop) {
                drop->acceptProposedAction();
                return true;
            }
        }
        if (drop->mimeData()->hasFormat(
                "application/x-qstandarditemmodeldatalist")) {
            const int childId = draggedObjectId;
            const int parentId = index.isValid() ? objectId : -1;
            const bool valid = childId >= 0 && childId != parentId;
            if (valid && event->type() == QEvent::Drop && viewport != nullptr) {
                if (viewport->setRuntimeObjectParent(childId, parentId)) {
                    draggedObjectId = -1;
                    drop->setDropAction(Qt::MoveAction);
                    drop->accept();
                    return true;
                }
            } else if (valid && event->type() != QEvent::Drop) {
                drop->setDropAction(Qt::MoveAction);
                drop->accept();
                return true;
            }
        }
        drop->ignore();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void HierarchyPanel::appendObjects(QStandardItem *parent,
                                   const QJsonArray &objects) {
    for (const QJsonValue &value : objects) {
        const QJsonObject object = value.toObject();
        const int id = object.value("id").toInt(-1);
        const QString name = object.value("name").toString("Object");
        const QString type = object.value("type").toString("gameObject");
        auto *item = new QStandardItem(hierarchyIcon(this, type), name);
        item->setData(id, ObjectIdRole);
        item->setData(type, ObjectTypeRole);
        item->setToolTip(type);
        item->setEditable(false);
        itemsById.insert(id, item);
        parent->appendRow(item);
        appendObjects(item, object.value("children").toArray());
    }
}

void HierarchyPanel::showAddObjectMenu(const QPoint &position) {
    if (addButton->menu() != nullptr) {
        addButton->menu()->popup(position);
    }
}

void HierarchyPanel::showContextMenu(const QPoint &position) {
    const QModelIndex index = treeView->indexAt(position);
    if (index.isValid()) {
        treeView->setCurrentIndex(index);
    }

    QMenu menu(this);
    auto *addMenu = menu.addMenu("Add Object");
    for (QAction *action : addButton->menu()->actions()) {
        addMenu->addAction(action);
    }
    if (index.isValid() && selectedObjectId() >= 0) {
        menu.addSeparator();
        menu.addAction(styling::icon(styling::Icon::Crosshair, "#7E929C"),
                       "Focus", this, &HierarchyPanel::focusSelectedObject);
        menu.addAction(styling::icon(styling::Icon::File, "#8498A8"), "Rename",
                       this, &HierarchyPanel::renameSelectedObject);
        menu.addAction(styling::icon(styling::Icon::TreeStructure, "#849589"),
                       "Move to Scene Root", this,
                       &HierarchyPanel::moveSelectedObjectToRoot);
        menu.addSeparator();
        menu.addAction(styling::icon(styling::Icon::Trash, "#A17F7F"), "Delete",
                       this, &HierarchyPanel::deleteSelectedObject);
    }
    menu.exec(treeView->viewport()->mapToGlobal(position));
}

void HierarchyPanel::createObject(const QString &type,
                                  const QString &displayName) {
    if (viewport == nullptr) {
        return;
    }
    const int parentId = selectedObjectId();
    const int id = viewport->createRuntimeObject(type, displayName);
    if (id >= 0 && parentId >= 0) {
        viewport->setRuntimeObjectParent(id, parentId);
    }
}

void HierarchyPanel::renameSelectedObject() {
    const int id = selectedObjectId();
    if (id < 0 || viewport == nullptr) {
        return;
    }
    QStandardItem *item = itemsById.value(id, nullptr);
    if (item == nullptr) {
        return;
    }
    bool accepted = false;
    const QString name =
        QInputDialog::getText(this, "Rename Object", "Name", QLineEdit::Normal,
                              item->text(), &accepted);
    if (accepted && !name.trimmed().isEmpty()) {
        viewport->renameRuntimeObject(id, name.trimmed());
    }
}

void HierarchyPanel::deleteSelectedObject() {
    if (viewport == nullptr) {
        return;
    }
    QList<int> ids = selectedObjectIds();
    if (ids.isEmpty() && viewport->selectedRuntimeObjectId() >= 0)
        ids.append(viewport->selectedRuntimeObjectId());
    for (int id : ids)
        viewport->deleteRuntimeObject(id);
}

void HierarchyPanel::focusSelectedObject() {
    if (applyingSnapshot || viewport == nullptr) {
        return;
    }
    const int id = selectedObjectId();
    if (id >= 0) {
        selectedSpecialType.clear();
        const QList<int> ids = selectedObjectIds();
        if (ids.size() > 1)
            viewport->focusRuntimeObjects(ids);
        else
            viewport->selectRuntimeObject(id, true);
        emit objectActivated(id);
        return;
    }
    const QString type =
        treeView->currentIndex().data(ObjectTypeRole).toString();
    if (type == "camera") {
        selectedSpecialType = type;
        viewport->selectRuntimeObject(-1, false);
        emit cameraActivated();
    } else if (type == "environment") {
        selectedSpecialType = type;
        viewport->selectRuntimeObject(-1, false);
        emit environmentActivated();
    } else if (type == "graphite" || type == "graphiteAsset") {
        const QString source =
            treeView->currentIndex().data(AssetPathRole).toString();
        selectedSpecialType =
            source.isEmpty() ? "graphite" : "graphite:" + source;
        viewport->selectRuntimeObject(-1, false);
        emit graphiteActivated(source);
    }
}

void HierarchyPanel::moveSelectedObjectToRoot() {
    const int id = selectedObjectId();
    if (id >= 0 && viewport != nullptr) {
        viewport->setRuntimeObjectParent(id, -1);
    }
}

int HierarchyPanel::selectedObjectId() const {
    if (treeView == nullptr || !treeView->currentIndex().isValid()) {
        return -1;
    }
    return treeView->currentIndex().data(ObjectIdRole).toInt();
}

QList<int> HierarchyPanel::selectedObjectIds() const {
    QList<int> ids;
    if (treeView == nullptr || treeView->selectionModel() == nullptr)
        return ids;
    for (const QModelIndex &index :
         treeView->selectionModel()->selectedRows()) {
        bool valid = false;
        const int id = index.data(ObjectIdRole).toInt(&valid);
        if (valid && id >= 0 && !ids.contains(id))
            ids.append(id);
    }
    return ids;
}

void HierarchyPanel::selectAllObjects() {
    if (treeView != nullptr)
        treeView->selectAll();
}

void HierarchyPanel::deselectAllObjects() {
    if (treeView != nullptr)
        treeView->clearSelection();
    if (viewport != nullptr)
        viewport->selectRuntimeObject(-1, false);
}

void HierarchyPanel::focusSearch() {
    searchField->setFocus();
    searchField->selectAll();
}

void HierarchyPanel::showCreationPopup() {
    if (viewport == nullptr || !addButton->isEnabled())
        return;

    QDialog dialog(this);
    dialog.setObjectName("commandPaletteDialog");
    dialog.setWindowTitle("Add Object");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(620, 430);
    auto *layout = new QVBoxLayout(&dialog);
    auto *search = new QLineEdit(&dialog);
    search->setObjectName("commandPaletteSearch");
    search->setPlaceholderText("Type an object to create…");
    auto *objects = new QListWidget(&dialog);
    objects->setObjectName("commandPaletteList");
    layout->addWidget(search);
    layout->addWidget(objects, 1);

    for (const CreationEntry &entry : creationEntries()) {
        auto *item = new QListWidgetItem(objects);
        item->setText(
            QStringLiteral("%1  ·  %2").arg(entry.name, entry.category));
        item->setIcon(hierarchyIcon(this, entry.type));
        item->setData(CreationTypeRole, entry.type);
        item->setData(CreationNameRole, entry.name);
        item->setData(CreationCategoryRole, entry.category);
    }
    if (objects->count() > 0)
        objects->setCurrentRow(0);

    auto *noObjects = new QListWidgetItem("No matching objects", objects);
    noObjects->setData(NoCreationResultsRole, true);
    noObjects->setHidden(true);
    connect(search, &QLineEdit::textChanged, &dialog,
            [objects, noObjects](const QString &text) {
                const QString query = text.trimmed();
                int firstMatch = -1;
                for (int index = 0; index < objects->count(); ++index) {
                    QListWidgetItem *item = objects->item(index);
                    if (item == noObjects)
                        continue;
                    const QString searchable =
                        item->data(CreationNameRole).toString() + ' ' +
                        item->data(CreationCategoryRole).toString() + ' ' +
                        item->data(CreationTypeRole).toString();
                    const bool matches =
                        query.isEmpty() ||
                        searchable.contains(query, Qt::CaseInsensitive);
                    item->setHidden(!matches);
                    if (matches && firstMatch < 0)
                        firstMatch = index;
                }
                noObjects->setHidden(firstMatch >= 0);
                objects->setCurrentItem(
                    firstMatch >= 0 ? objects->item(firstMatch) : noObjects);
            });
    connect(objects, &QListWidget::itemActivated, &dialog,
            [this, &dialog](QListWidgetItem *item) {
                if (item == nullptr ||
                    item->data(NoCreationResultsRole).toBool())
                    return;
                const QString type = item->data(CreationTypeRole).toString();
                const QString name = item->data(CreationNameRole).toString();
                dialog.accept();
                createObject(type, name);
            });
    connect(search, &QLineEdit::returnPressed, &dialog, [objects] {
        if (objects->currentItem() != nullptr)
            emit objects->itemActivated(objects->currentItem());
    });

    auto moveSelection = [objects](int direction) {
        if (objects->count() == 0)
            return;
        int row = objects->currentRow();
        for (int attempt = 0; attempt < objects->count(); ++attempt) {
            row = (row + direction + objects->count()) % objects->count();
            if (!objects->item(row)->isHidden()) {
                objects->setCurrentRow(row);
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

QString HierarchyPanel::sceneSignature(const QString &sceneName,
                                       const QJsonArray &objects,
                                       const QJsonArray &interfaces) const {
    return sceneName + ':' + objectSignature(objects) + ':' +
           QString::fromUtf8(
               QJsonDocument(interfaces).toJson(QJsonDocument::Compact));
}
