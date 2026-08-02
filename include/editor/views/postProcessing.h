#ifndef ATLAS_POSTPROCESSING_H
#define ATLAS_POSTPROCESSING_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QWidget>

class QComboBox;
class QLabel;
class QToolButton;
class QVBoxLayout;
class ViewportPanel;

class PostProcessingPanel : public QWidget {
    Q_OBJECT

  public:
    explicit PostProcessingPanel(ViewportPanel *viewport,
                                 QWidget *parent = nullptr);

  public slots:
    void applySceneSnapshot(const QString &snapshot);

  private:
    void rebuildTargetList();
    void rebuildEditor();
    void addTarget();
    void removeTarget();
    void addEffect(const QString &type);
    void removeEffect(int effectIndex);
    void moveEffect(int effectIndex, int offset);
    void setTargetValue(const QString &path, const QJsonValue &value);
    void setEffectValue(int effectIndex, const QString &key,
                        const QJsonValue &value);
    void setBloomThreshold(double value);
    void replaceTargets();

    ViewportPanel *viewport = nullptr;
    QComboBox *targetSelector = nullptr;
    QToolButton *removeTargetButton = nullptr;
    QLabel *statusLabel = nullptr;
    QWidget *body = nullptr;
    QVBoxLayout *bodyLayout = nullptr;
    QJsonArray targets;
    QJsonObject environment;
    int targetIndex = -1;
    bool applying = false;
};

#endif
