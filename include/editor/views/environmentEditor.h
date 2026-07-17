#ifndef ATLAS_ENVIRONMENTEDITOR_H
#define ATLAS_ENVIRONMENTEDITOR_H

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QWidget>

class QFormLayout;
class QLabel;
class QListWidget;
class QStackedWidget;
class QTimer;
class ViewportPanel;

class EnvironmentEditorPanel : public QWidget {
    Q_OBJECT

  public:
    explicit EnvironmentEditorPanel(ViewportPanel *viewport,
                                    QWidget *parent = nullptr);

  public slots:
    void applySceneSnapshot(const QString &snapshot);

  private:
    void rebuildEditor();
    QWidget *createPage(const QString &title, const QString &subtitle);
    QFormLayout *addSection(QWidget *page, const QString &title);
    void addBoolean(QFormLayout *form, const QString &label,
                    const QString &path);
    void addNumber(QFormLayout *form, const QString &label,
                   const QString &path, double minimum, double maximum,
                   double step = 0.05, int decimals = 3);
    void addInteger(QFormLayout *form, const QString &label,
                    const QString &path, int minimum, int maximum);
    void addVector(QFormLayout *form, const QString &label,
                   const QString &path);
    void addColor(QFormLayout *form, const QString &label,
                  const QString &path);
    void addText(QFormLayout *form, const QString &label,
                 const QString &path, const QStringList &choices = {});
    void setEnvironmentValue(const QString &path, const QJsonValue &value);
    QJsonValue environmentValue(const QString &path) const;
    void applyPreview();

    ViewportPanel *viewport = nullptr;
    QListWidget *categories = nullptr;
    QStackedWidget *pages = nullptr;
    QLabel *statusLabel = nullptr;
    QTimer *previewTimer = nullptr;
    QJsonObject environment;
    bool applying = false;
    bool autoApply = false;
};

#endif
