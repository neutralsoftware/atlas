#ifndef ATLAS_GRAPHITE_EDITOR_H
#define ATLAS_GRAPHITE_EDITOR_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

class GraphiteCanvas;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QUndoStack;
class ViewportPanel;

class GraphiteEditorPanel : public QWidget {
    Q_OBJECT

  public:
    explicit GraphiteEditorPanel(ViewportPanel *viewport,
                                 const QString &projectFile,
                                 QWidget *parent = nullptr);
    ~GraphiteEditorPanel() override;

    void openUI(const QString &path);
    void saveUI();
    void undo();
    void redo();

  signals:
    void previewRequested();

  private:
    void showEmptyState();
    void rebuildTree();
    void rebuildInspector();
    void refreshDocument(bool recordUndo = true);
    void addElement(const QString &type);
    void deleteSelectedElement();
    void duplicateSelectedElement();
    void addScriptComponent();
    void removeScriptComponent();
    void attachToScene(bool preview);
    void setDocument(const QJsonObject &next, bool recordUndo);
    QJsonObject elementAtPath(const QList<int> &path) const;
    void replaceElement(const QList<int> &path, const QJsonObject &element);
    void removeElement(const QList<int> &path);
    QList<int> selectedPath() const;
    QTreeWidgetItem *appendTreeElement(QTreeWidgetItem *parent,
                                       const QJsonObject &element,
                                       const QList<int> &path);
    QJsonObject defaultElement(const QString &type);

    ViewportPanel *viewport = nullptr;
    QString projectFile;
    QString uiPath;
    QJsonObject document;
    QLabel *titleLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QTreeWidget *tree = nullptr;
    GraphiteCanvas *canvas = nullptr;
    QWidget *inspectorBody = nullptr;
    QVBoxLayout *inspectorLayout = nullptr;
    QUndoStack *undoStack = nullptr;
    bool loading = false;
    int nextElementNumber = 1;
    QString styleVariant = "normal";
};

#endif
