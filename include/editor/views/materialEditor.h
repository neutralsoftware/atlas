#ifndef ATLAS_MATERIALEDITOR_H
#define ATLAS_MATERIALEDITOR_H

#include <QHash>
#include <QJsonObject>
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

class MaterialEditorPanel : public QWidget {
    Q_OBJECT

  public:
    explicit MaterialEditorPanel(QWidget *parent = nullptr);
    ~MaterialEditorPanel() override;

  public slots:
    void openMaterial(const QString &path);

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
    void saveMaterial();
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
    QDoubleSpinBox *transmittanceField = nullptr;
    QDoubleSpinBox *iorField = nullptr;
    QCheckBox *normalMapField = nullptr;
    QHash<QString, QLineEdit *> textureFields;
    QHash<QString, QLabel *> texturePreviews;
    QTimer *saveTimer = nullptr;
    QString materialPath;
    QJsonObject material;
    bool loading = false;
};

#endif
