#ifndef ATLAS_MATERIALEDITOR_H
#define ATLAS_MATERIALEDITOR_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;
class QVBoxLayout;
class MaterialPreviewWidget;
class ViewportPanel;

class MaterialEditorPanel : public QWidget {
    Q_OBJECT

  public:
    explicit MaterialEditorPanel(ViewportPanel *viewport,
                                 QWidget *parent = nullptr);
    ~MaterialEditorPanel() override;

  public slots:
    void openMaterial(const QString &path);
    void saveMaterial();
    void undo();
    void redo();

  signals:
    void materialSaved(const QString &path);

  private:
    void showEmptyState();
    void showMaterial();
    void rebuildBody();
    void setColor(const QString &key, QPushButton *button);
    void chooseTexture(const QString &key);
    void clearTexture(const QString &key);
    void updateTextureField(const QString &key);
    void materialChanged();
    void refreshEditedMaterial();
    void recordHistory(const QJsonObject &previous);
    void assignToSelectedObject();
    QJsonObject normalizedMaterial(const QJsonObject &source) const;

    QWidget *body = nullptr;
    QVBoxLayout *bodyLayout = nullptr;
    MaterialPreviewWidget *preview = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *albedoButton = nullptr;
    QPushButton *emissiveButton = nullptr;
    QDoubleSpinBox *metallicField = nullptr;
    QDoubleSpinBox *roughnessField = nullptr;
    QDoubleSpinBox *aoField = nullptr;
    QDoubleSpinBox *reflectivityField = nullptr;
    QDoubleSpinBox *emissiveIntensityField = nullptr;
    QDoubleSpinBox *normalStrengthField = nullptr;
    QDoubleSpinBox *textureScaleUField = nullptr;
    QDoubleSpinBox *textureScaleVField = nullptr;
    QDoubleSpinBox *textureOffsetUField = nullptr;
    QDoubleSpinBox *textureOffsetVField = nullptr;
    QDoubleSpinBox *transmittanceField = nullptr;
    QDoubleSpinBox *iorField = nullptr;
    QCheckBox *normalMapField = nullptr;
    QHash<QString, QLineEdit *> textureFields;
    QHash<QString, QLabel *> texturePreviews;
    QTimer *saveTimer = nullptr;
    ViewportPanel *viewport = nullptr;
    QString materialPath;
    QJsonObject material;
    QList<QJsonObject> undoHistory;
    QList<QJsonObject> redoHistory;
    int assignedObjectId = -1;
    bool loading = false;
};

#endif
