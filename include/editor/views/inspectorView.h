/*
 * inspectorView.h
 * As part of the Atlas project
 * Created by Max Van den Eynde in 2026
 * --------------------------------------
 * Description: Inspector View declaration
 * Copyright (c) 2026 Max Van den Eynde
 */

#ifndef ATLAS_INSPECTORVIEW_H
#define ATLAS_INSPECTORVIEW_H

#include <QJsonObject>
#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QDragEnterEvent;
class QDropEvent;
class QScrollArea;
class QVBoxLayout;
class ViewportPanel;

class InspectorPanel : public QWidget {
    Q_OBJECT

  public:
    explicit InspectorPanel(ViewportPanel *viewport, const QString &projectFile,
                            QWidget *parent = nullptr);

  public slots:
    void applySceneSnapshot(const QString &snapshot);
    void inspectRuntimeObject(int id);
    void inspectCamera();
    void inspectEnvironment();
    void inspectFile(const QString &path);

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

  private:
    void showEmptyState();
    void showObject(const QJsonObject &object);
    void showCamera();
    void showEnvironment();
    void showFile();
    void refreshObjectEditors(const QJsonObject &object);
    void rebuildBody();
    void commitHeaderName();
    QJsonObject findObject(int id) const;
    bool attachAsset(const QString &path, int objectId);

    ViewportPanel *viewport = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *content = nullptr;
    QVBoxLayout *contentLayout = nullptr;
    QLabel *iconLabel = nullptr;
    QLabel *typeLabel = nullptr;
    QLineEdit *nameField = nullptr;
    QJsonObject scene;
    QJsonObject inspectedObject;
    QJsonObject inspectedCamera;
    QString inspectedFile;
    QString projectRoot;
    QString projectFile;
    int inspectedObjectId = -1;
    int lastRuntimeSelection = -1;
    bool fileTarget = false;
    bool cameraTarget = false;
    bool environmentTarget = false;
    bool rebuilding = false;
};

#endif // ATLAS_INSPECTORVIEW_H
