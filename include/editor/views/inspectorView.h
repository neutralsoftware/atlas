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
class QScrollArea;
class QVBoxLayout;
class ViewportPanel;

class InspectorPanel : public QWidget {
    Q_OBJECT

  public:
    explicit InspectorPanel(ViewportPanel *viewport, QWidget *parent = nullptr);

  public slots:
    void applySceneSnapshot(const QString &snapshot);
    void inspectRuntimeObject(int id);
    void inspectFile(const QString &path);

  private:
    void showEmptyState();
    void showObject(const QJsonObject &object);
    void showFile();
    void rebuildBody();
    void commitHeaderName();
    QJsonObject findObject(int id) const;

    ViewportPanel *viewport = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *content = nullptr;
    QVBoxLayout *contentLayout = nullptr;
    QLabel *iconLabel = nullptr;
    QLabel *typeLabel = nullptr;
    QLineEdit *nameField = nullptr;
    QJsonObject scene;
    QJsonObject inspectedObject;
    QString inspectedFile;
    int inspectedObjectId = -1;
    int lastRuntimeSelection = -1;
    bool fileTarget = false;
    bool rebuilding = false;
};

#endif // ATLAS_INSPECTORVIEW_H
