#ifndef ATLAS_INPUTACTIONSDIALOG_H
#define ATLAS_INPUTACTIONSDIALOG_H

#include <QDialog>
#include <QJsonValue>
#include <QList>
#include <QString>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

class InputActionsDialog : public QDialog {
  public:
    enum class ActionKind { Button, Axis1D, Axis2D };

    explicit InputActionsDialog(const QString &projectFile,
                                QWidget *parent = nullptr);

  private:
    struct ButtonBinding {
        QString source = "Keyboard";
        QString value = "Space";
        int controllerId = -1;
        int controllerButton = 0;
    };

    struct ActionDefinition {
        QString name;
        ActionKind kind = ActionKind::Button;
        QList<ButtonBinding> buttonBindings;
        QString positiveX = "D";
        QString negativeX = "A";
        QString positiveY = "W";
        QString negativeY = "S";
        bool mouseAxis = false;
        bool controllerAxis = false;
        int controllerId = -1;
        int controllerAxisX = 0;
        int controllerAxisY = 1;
        double deadzone = 0.2;
        double scaleX = 1.0;
        double scaleY = 1.0;
        bool normalize = false;
        bool invertY = false;
        bool clamp = true;
    };

    void setupUi();
    void load();
    bool save();
    bool updateProjectManifest(QString *errorMessage);
    void addAction(ActionKind kind);
    void duplicateAction();
    void removeAction();
    void selectAction(int index);
    void storeCurrentAction();
    void refreshList();
    void refreshEditor();
    void refreshBindingTable();
    void refreshScriptExample();
    void addButtonBinding();
    QJsonValue serializeButtonBinding(const ButtonBinding &binding) const;
    ButtonBinding parseButtonBinding(const QJsonValue &value) const;
    QString uniqueName(const QString &base) const;

    QString projectFile;
    QString actionsFile;
    QList<ActionDefinition> actions;
    int currentIndex = -1;
    bool updating = false;

    QLineEdit *searchField = nullptr;
    QListWidget *actionList = nullptr;
    QPushButton *duplicateButton = nullptr;
    QPushButton *removeButton = nullptr;
    QLineEdit *nameField = nullptr;
    QComboBox *kindField = nullptr;
    QStackedWidget *bindingPages = nullptr;
    QTableWidget *buttonBindings = nullptr;
    QComboBox *positiveXField = nullptr;
    QComboBox *negativeXField = nullptr;
    QComboBox *positiveYField = nullptr;
    QComboBox *negativeYField = nullptr;
    QLabel *positiveYLabel = nullptr;
    QLabel *negativeYLabel = nullptr;
    QCheckBox *mouseAxisField = nullptr;
    QCheckBox *controllerAxisField = nullptr;
    QSpinBox *controllerIdField = nullptr;
    QSpinBox *controllerAxisXField = nullptr;
    QSpinBox *controllerAxisYField = nullptr;
    QLabel *controllerAxisYLabel = nullptr;
    QDoubleSpinBox *deadzoneField = nullptr;
    QDoubleSpinBox *scaleXField = nullptr;
    QDoubleSpinBox *scaleYField = nullptr;
    QLabel *scaleYLabel = nullptr;
    QCheckBox *normalizeField = nullptr;
    QCheckBox *invertYField = nullptr;
    QCheckBox *clampField = nullptr;
    QPlainTextEdit *scriptExample = nullptr;
};

#endif
