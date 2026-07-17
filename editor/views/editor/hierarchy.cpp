/*
 * hierarchy.cpp
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Hierarchy panel definition
 * Copyright (c) 2026 Max Van den Eynde
 */

#include <editor/views/hierarchyPanel.h>

#include <QAction>
#include <QAbstractItemView>
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
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPair>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyle>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "editor/views/viewport.h"

namespace {
constexpr int ObjectIdRole = Qt::UserRole + 1;
constexpr int ObjectTypeRole = Qt::UserRole + 2;

QIcon hierarchyIcon(QWidget *widget, const QString &type) {
    const QString normalized = type.toLower();
    QStyle::StandardPixmap fallback = QStyle::SP_FileIcon;
    QString themeName = "application-x-executable";

    if (normalized == "scene") {
        fallback = QStyle::SP_DesktopIcon;
        themeName = "view-grid";
    } else if (normalized == "compound" || normalized == "group") {
        fallback = QStyle::SP_DirClosedIcon;
        themeName = "folder";
    } else if (normalized == "camera") {
        fallback = QStyle::SP_ComputerIcon;
        themeName = "camera-photo";
    } else if (normalized == "environment") {
        fallback = QStyle::SP_DesktopIcon;
        themeName = "weather-clear";
    } else if (normalized.contains("light") || normalized == "sun") {
        fallback = QStyle::SP_MessageBoxInformation;
        themeName = "weather-clear";
    } else if (normalized == "terrain" || normalized == "landscape") {
        fallback = QStyle::SP_DriveHDIcon;
        themeName = "applications-graphics";
    } else if (normalized == "particleemitter" || normalized == "particles") {
        fallback = QStyle::SP_BrowserReload;
        themeName = "weather-showers-scattered";
    } else if (normalized == "model") {
        fallback = QStyle::SP_FileDialogContentsView;
        themeName = "model";
    } else if (normalized == "cube" || normalized == "sphere" ||
               normalized == "plane" || normalized == "pyramid" ||
               normalized == "capsule" || normalized == "solid") {
        fallback = QStyle::SP_DirIcon;
        themeName = "applications-games";
    }

    return QIcon::fromTheme(themeName, widget->style()->standardIcon(fallback));
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
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("panelToolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);

    addButton = new QToolButton(toolbar);
    addButton->setObjectName("panelAddButton");
    addButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    addButton->setText("Add");
    addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addButton->setPopupMode(QToolButton::InstantPopup);
    addButton->setToolTip("Add Object");

    moreButton = new QToolButton(toolbar);
    moreButton->setObjectName("panelMoreButton");
    moreButton->setIcon(
        style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton));
    moreButton->setPopupMode(QToolButton::InstantPopup);
    moreButton->setToolTip("Hierarchy actions");

    toolbarLayout->addWidget(addButton);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(moreButton);
    layout->addWidget(toolbar);

    treeView = new QTreeView(this);
    treeView->setObjectName("hierarchyTree");
    model = new QStandardItemModel(this);
    treeView->setModel(model);
    treeView->setHeaderHidden(true);
    treeView->setAnimated(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
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

    connect(treeView, &QTreeView::clicked, this,
            [this](const QModelIndex &) { focusSelectedObject(); });
    connect(treeView, &QTreeView::doubleClicked, this,
            [this](const QModelIndex &) { renameSelectedObject(); });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            &HierarchyPanel::showContextMenu);

    auto *deleteAction = new QAction(this);
    deleteAction->setShortcuts(
        {QKeySequence::Delete,
         QKeySequence(Qt::META | Qt::Key_Backspace)});
    deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteAction, &QAction::triggered, this,
            &HierarchyPanel::deleteSelectedObject);
    addAction(deleteAction);

    auto *renameAction = new QAction(this);
    renameAction->setShortcut(Qt::Key_F2);
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
        connect(viewport, &ViewportPanel::sceneSnapshotChanged, this,
                &HierarchyPanel::applySceneSnapshot);
        connect(viewport, &ViewportPanel::runtimeAvailabilityChanged, this,
                [this](bool available) {
                    addButton->setEnabled(available);
                    moreButton->setEnabled(available);
                    treeView->setEnabled(available);
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
    const int selectedId = scene.value("selectedId").toInt(-1);
    const QString signature = sceneSignature(sceneName, objects);

    if (signature != lastStructureSignature) {
        rebuildScene(sceneName, objects, selectedId);
        lastStructureSignature = signature;
        return;
    }

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
        const QModelIndex index = specialItems.value(selectedSpecialType)->index();
        treeView->setCurrentIndex(index);
        treeView->scrollTo(index, QAbstractItemView::EnsureVisible);
    } else {
        treeView->setCurrentIndex(QModelIndex());
    }
    applyingSnapshot = false;
}

void HierarchyPanel::rebuildScene(const QString &sceneName,
                                  const QJsonArray &objects, int selectedId) {
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

    auto *environment = new QStandardItem(
        hierarchyIcon(this, "environment"), "Environment");
    environment->setData(-1, ObjectIdRole);
    environment->setData("environment", ObjectTypeRole);
    environment->setToolTip("Scene atmosphere and environment");
    environment->setEditable(false);
    specialItems.insert("environment", environment);
    root->appendRow(environment);
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
}

bool HierarchyPanel::eventFilter(QObject *watched, QEvent *event) {
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
            const bool supported =
                suffix == "amat" || suffix == "material" || suffix == "ts" ||
                suffix == "js" || suffix == "wav" || suffix == "mp3" ||
                suffix == "ogg" || suffix == "flac" || suffix == "m4a" ||
                suffix == "aac";
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
            const int childId = selectedObjectId();
            const int parentId = index.isValid() ? objectId : -1;
            const bool valid = childId >= 0 && childId != parentId;
            if (valid && event->type() == QEvent::Drop && viewport != nullptr) {
                if (viewport->setRuntimeObjectParent(childId, parentId)) {
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
        menu.addAction("Focus", this, &HierarchyPanel::focusSelectedObject);
        menu.addAction("Rename", this, &HierarchyPanel::renameSelectedObject);
        menu.addAction("Move to Scene Root", this,
                       &HierarchyPanel::moveSelectedObjectToRoot);
        menu.addSeparator();
        menu.addAction("Delete", this, &HierarchyPanel::deleteSelectedObject);
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
    const int id = selectedObjectId();
    if (id < 0 || viewport == nullptr) {
        return;
    }
    const QString name = itemsById.value(id)->text();
    if (QMessageBox::question(
            this, "Delete Object",
            QStringLiteral("Delete “%1” and its children?").arg(name)) ==
        QMessageBox::Yes) {
        viewport->deleteRuntimeObject(id);
    }
}

void HierarchyPanel::focusSelectedObject() {
    if (applyingSnapshot || viewport == nullptr) {
        return;
    }
    const int id = selectedObjectId();
    if (id >= 0) {
        selectedSpecialType.clear();
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

QString HierarchyPanel::sceneSignature(const QString &sceneName,
                                       const QJsonArray &objects) const {
    return sceneName + ':' + objectSignature(objects);
}
