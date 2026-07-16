#ifndef ATLAS_PROJECTBROWSER_H
#define ATLAS_PROJECTBROWSER_H

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QListWidget;
class QPoint;
class QStackedWidget;

class ProjectBrowser : public QMainWindow {
    Q_OBJECT

public:
    explicit ProjectBrowser(QWidget* parent = nullptr);

signals:
    void openProjectRequested(const QString& projectFile);

private:
    void setupUi();
    void reloadProjects();
    void filterProjects(const QString& query);
    void createProject();
    void openExistingProject();
    void openSelectedProject();
    void showProjectMenu(const QPoint& position);

    QLineEdit* searchField = nullptr;
    QListWidget* projectList = nullptr;
    QStackedWidget* projectStack = nullptr;
    QLabel* emptyTitle = nullptr;
};

#endif
