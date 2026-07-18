#include <editor/views/projectBrowser.h>

#include <editor/project/projectStore.h>

#include <QAbstractItemView>
#include <QAction>
#include <QButtonGroup>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QStyle>
#include <QSizePolicy>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#ifndef ATLAS_VERSION
#define ATLAS_VERSION "Alpha 9"
#endif

namespace {
constexpr int ProjectPathRole = Qt::UserRole;
constexpr int ProjectAvailableRole = Qt::UserRole + 1;

class TemplateCard : public QFrame {
public:
    TemplateCard(const QString& title, const QString& description,
                 QWidget* parent = nullptr)
        : QFrame(parent) {
        setProperty("templateCard", true);
        setProperty("selected", false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(112);
        setCursor(Qt::PointingHandCursor);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(8);
        option = new QRadioButton(title, this);
        option->setObjectName("templateOption");
        option->setCursor(Qt::PointingHandCursor);
        layout->addWidget(option);
        auto* descriptionLabel = new QLabel(description, this);
        descriptionLabel->setObjectName("templateDescription");
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(descriptionLabel);
        layout->addStretch();
        connect(option, &QRadioButton::toggled, this, [this](bool selected) {
            setProperty("selected", selected);
            style()->unpolish(this);
            style()->polish(this);
            update();
        });
    }

    QRadioButton* button() const {
        return option;
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            option->setChecked(true);
        }
        QFrame::mousePressEvent(event);
    }

private:
    QRadioButton* option = nullptr;
};

class CreateProjectDialog : public QDialog {
public:
    explicit CreateProjectDialog(QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle("Create an Atlas project");
        setModal(true);
        setMinimumWidth(760);
        setObjectName("createProjectDialog");

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(28, 26, 28, 24);
        root->setSpacing(18);

        auto* title = new QLabel("Create a new project", this);
        title->setObjectName("dialogTitle");
        root->addWidget(title);
        auto* subtitle = new QLabel(
            "Choose a renderer template. You can change these settings later.",
            this);
        subtitle->setObjectName("dialogSubtitle");
        root->addWidget(subtitle);

        auto* templateLayout = new QHBoxLayout();
        templateLayout->setSpacing(12);
        templateGroup = new QButtonGroup(this);
        templateGroup->setExclusive(true);
        auto* pbr = new TemplateCard(
            "PBR", "Deferred physically based rendering for most 3D projects.",
            this);
        auto* ddgi = new TemplateCard(
            "PBR + DDGI",
            "PBR with dynamic diffuse global illumination enabled.", this);
        auto* pathTracing = new TemplateCard(
            "Path Tracing",
            "Progressive ray-traced lighting for high-fidelity scenes.", this);
        templateGroup->addButton(pbr->button(),
                                 static_cast<int>(AtlasProjectTemplate::Pbr));
        templateGroup->addButton(
            ddgi->button(), static_cast<int>(AtlasProjectTemplate::PbrDdgi));
        templateGroup->addButton(
            pathTracing->button(),
            static_cast<int>(AtlasProjectTemplate::PathTracing));
        pbr->button()->setChecked(true);
        templateLayout->addWidget(pbr);
        templateLayout->addWidget(ddgi);
        templateLayout->addWidget(pathTracing);
        root->addLayout(templateLayout);

        auto* fields = new QVBoxLayout();
        fields->setSpacing(8);
        auto* nameLabel = new QLabel("Project name", this);
        nameLabel->setObjectName("fieldLabel");
        fields->addWidget(nameLabel);
        nameField = new QLineEdit(this);
        nameField->setPlaceholderText("My Atlas Project");
        nameField->setClearButtonEnabled(true);
        fields->addWidget(nameField);

        auto* locationLabel = new QLabel("Location", this);
        locationLabel->setObjectName("fieldLabel");
        fields->addWidget(locationLabel);
        auto* locationLayout = new QHBoxLayout();
        locationField = new QLineEdit(this);
        QString defaultLocation = QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation);
        if (defaultLocation.isEmpty()) {
            defaultLocation = QDir::homePath();
        }
        locationField->setText(defaultLocation);
        locationLayout->addWidget(locationField, 1);
        auto* browse = new QPushButton("Browse…", this);
        browse->setProperty("secondary", true);
        locationLayout->addWidget(browse);
        fields->addLayout(locationLayout);
        root->addLayout(fields);

        errorLabel = new QLabel(this);
        errorLabel->setObjectName("dialogError");
        errorLabel->setWordWrap(true);
        errorLabel->hide();
        root->addWidget(errorLabel);

        auto* actions = new QHBoxLayout();
        actions->addStretch();
        auto* cancel = new QPushButton("Cancel", this);
        cancel->setProperty("secondary", true);
        actions->addWidget(cancel);
        createButton = new QPushButton("Create project", this);
        createButton->setObjectName("primaryAction");
        createButton->setDefault(true);
        createButton->setEnabled(false);
        actions->addWidget(createButton);
        root->addLayout(actions);

        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString directory = QFileDialog::getExistingDirectory(
                this, "Choose a project location", locationField->text());
            if (!directory.isEmpty()) {
                locationField->setText(directory);
            }
        });
        const auto updateAvailability = [this] {
            createButton->setEnabled(!nameField->text().trimmed().isEmpty() &&
                                     QDir(locationField->text()).exists());
            errorLabel->hide();
        };
        connect(nameField, &QLineEdit::textChanged, this,
                updateAvailability);
        connect(locationField, &QLineEdit::textChanged, this,
                updateAvailability);
        connect(createButton, &QPushButton::clicked, this, [this] {
            QString error;
            const auto projectTemplate = static_cast<AtlasProjectTemplate>(
                templateGroup->checkedId());
            createdProjectFile = ProjectStore::createProject(
                nameField->text(), locationField->text(), projectTemplate,
                &error);
            if (createdProjectFile.isEmpty()) {
                errorLabel->setText(error);
                errorLabel->show();
                return;
            }
            accept();
        });
        nameField->setFocus();
    }

    QString projectFile() const {
        return createdProjectFile;
    }

private:
    QButtonGroup* templateGroup = nullptr;
    QLineEdit* nameField = nullptr;
    QLineEdit* locationField = nullptr;
    QLabel* errorLabel = nullptr;
    QPushButton* createButton = nullptr;
    QString createdProjectFile;
};

class ProjectRow : public QFrame {
public:
    explicit ProjectRow(const AtlasProjectInfo& project,
                        QWidget* parent = nullptr)
        : QFrame(parent) {
        setObjectName("projectRow");
        setProperty("available", project.available);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(16, 12, 12, 12);
        layout->setSpacing(14);

        auto* copy = new QVBoxLayout();
        copy->setSpacing(3);
        auto* title = new QLabel(project.name, this);
        title->setObjectName("projectName");
        copy->addWidget(title);
        auto* path = new QLabel(project.directory, this);
        path->setObjectName("projectPath");
        path->setTextInteractionFlags(Qt::TextSelectableByMouse);
        copy->addWidget(path);
        layout->addLayout(copy, 1);

        auto* renderer = new QLabel(project.renderer, this);
        renderer->setObjectName("rendererBadge");
        layout->addWidget(renderer);

        auto* date = new QLabel(
            project.lastModified.isValid()
                ? project.lastModified.toString("d MMM yyyy")
                : QStringLiteral("Unavailable"),
            this);
        date->setObjectName("projectDate");
        date->setMinimumWidth(90);
        layout->addWidget(date);

        moreButton = new QToolButton(this);
        moreButton->setObjectName("projectMoreButton");
        moreButton->setIcon(
            style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton));
        moreButton->setToolTip("Project options");
        layout->addWidget(moreButton);
    }

    QToolButton* optionsButton() const {
        return moreButton;
    }

private:
    QToolButton* moreButton = nullptr;
};
}

ProjectBrowser::ProjectBrowser(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("Atlas Engine — Projects");
    setMinimumSize(900, 580);
    resize(1120, 720);
    setupUi();
    reloadProjects();
}

void ProjectBrowser::setupUi() {
    auto* root = new QWidget(this);
    root->setObjectName("projectBrowserRoot");
    setCentralWidget(root);
    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* sidebar = new QFrame(root);
    sidebar->setObjectName("projectSidebar");
    sidebar->setFixedWidth(224);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(22, 26, 22, 22);
    sidebarLayout->setSpacing(18);

    auto* brandLayout = new QHBoxLayout();
    brandLayout->setSpacing(11);
    auto* brandIcon = new QLabel(sidebar);
    brandIcon->setFixedSize(38, 38);
    brandIcon->setPixmap(
        QPixmap(":/editor/assets/Icon-iOS-Default-1024x1024@1x.png")
            .scaled(brandIcon->size(), Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
    brandLayout->addWidget(brandIcon);
    auto* brandCopy = new QVBoxLayout();
    brandCopy->setSpacing(0);
    auto* brand = new QLabel("Atlas Engine", sidebar);
    brand->setObjectName("projectBrand");
    brandCopy->addWidget(brand);
    auto* brandVersion = new QLabel(QStringLiteral(ATLAS_VERSION), sidebar);
    brandVersion->setObjectName("projectSidebarVersion");
    brandCopy->addWidget(brandVersion);
    brandLayout->addLayout(brandCopy);
    brandLayout->addStretch();
    sidebarLayout->addLayout(brandLayout);

    auto* projectsNav = new QPushButton("Projects", sidebar);
    projectsNav->setObjectName("projectNavSelected");
    projectsNav->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    projectsNav->setEnabled(false);
    sidebarLayout->addWidget(projectsNav);
    sidebarLayout->addStretch();

    rootLayout->addWidget(sidebar);

    auto* content = new QWidget(root);
    content->setObjectName("projectBrowserContent");
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(34, 30, 34, 30);
    contentLayout->setSpacing(20);

    auto* headingLayout = new QHBoxLayout();
    auto* headingCopy = new QVBoxLayout();
    headingCopy->setSpacing(4);
    auto* title = new QLabel("Projects", content);
    title->setObjectName("projectBrowserTitle");
    headingCopy->addWidget(title);
    auto* subtitle = new QLabel(
        "Create a project or continue where you left off.", content);
    subtitle->setObjectName("projectBrowserSubtitle");
    headingCopy->addWidget(subtitle);
    headingLayout->addLayout(headingCopy, 1);

    auto* openButton = new QPushButton("Open existing", content);
    openButton->setProperty("secondary", true);
    openButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    headingLayout->addWidget(openButton);
    auto* createButton = new QPushButton("New project", content);
    createButton->setObjectName("primaryAction");
    createButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    headingLayout->addWidget(createButton);
    contentLayout->addLayout(headingLayout);

    searchField = new QLineEdit(content);
    searchField->setObjectName("projectSearch");
    searchField->setPlaceholderText("Search projects");
    searchField->setClearButtonEnabled(true);
    contentLayout->addWidget(searchField);

    projectStack = new QStackedWidget(content);
    projectList = new QListWidget(projectStack);
    projectList->setObjectName("projectList");
    projectList->setSpacing(8);
    projectList->setSelectionMode(QAbstractItemView::SingleSelection);
    projectList->setContextMenuPolicy(Qt::CustomContextMenu);
    projectList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    projectStack->addWidget(projectList);

    auto* empty = new QWidget(projectStack);
    empty->setObjectName("projectEmptyState");
    auto* emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setContentsMargins(40, 40, 40, 40);
    emptyLayout->addStretch();
    emptyTitle = new QLabel("No projects yet", empty);
    emptyTitle->setObjectName("emptyStateTitle");
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);
    auto* emptySubtitle = new QLabel(
        "Create your first Atlas project or open one from disk.", empty);
    emptySubtitle->setObjectName("emptyStateSubtitle");
    emptySubtitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptySubtitle);
    emptyLayout->addStretch();
    projectStack->addWidget(empty);
    contentLayout->addWidget(projectStack, 1);
    rootLayout->addWidget(content, 1);

    connect(createButton, &QPushButton::clicked, this,
            &ProjectBrowser::createProject);
    connect(openButton, &QPushButton::clicked, this,
            &ProjectBrowser::openExistingProject);
    connect(searchField, &QLineEdit::textChanged, this,
            &ProjectBrowser::filterProjects);
    connect(projectList, &QListWidget::itemDoubleClicked, this,
            [this] { openSelectedProject(); });
    connect(projectList, &QListWidget::customContextMenuRequested, this,
            &ProjectBrowser::showProjectMenu);
}

void ProjectBrowser::reloadProjects() {
    projectList->clear();
    const QList<AtlasProjectInfo> projects = ProjectStore::recentProjects();
    for (const AtlasProjectInfo& project : projects) {
        auto* item = new QListWidgetItem(projectList);
        item->setData(ProjectPathRole, project.projectFile);
        item->setData(ProjectAvailableRole, project.available);
        item->setSizeHint(QSize(0, 76));
        auto* row = new ProjectRow(project, projectList);
        projectList->setItemWidget(item, row);
        connect(row->optionsButton(), &QToolButton::clicked, this,
                [this, item, row] {
                    projectList->setCurrentItem(item);
                    const QPoint menuPosition = projectList->viewport()->mapFromGlobal(
                        row->optionsButton()->mapToGlobal(
                            QPoint(0, row->optionsButton()->height())));
                    showProjectMenu(menuPosition);
                });
    }
    filterProjects(searchField->text());
}

void ProjectBrowser::filterProjects(const QString& query) {
    const QString normalized = query.trimmed();
    int visibleCount = 0;
    for (int index = 0; index < projectList->count(); ++index) {
        QListWidgetItem* item = projectList->item(index);
        QWidget* row = projectList->itemWidget(item);
        const QString path = item->data(ProjectPathRole).toString();
        const bool matches =
            normalized.isEmpty() || path.contains(normalized, Qt::CaseInsensitive) ||
            (row != nullptr &&
             row->findChild<QLabel*>("projectName") != nullptr &&
             row->findChild<QLabel*>("projectName")
                 ->text()
                 .contains(normalized, Qt::CaseInsensitive));
        item->setHidden(!matches);
        if (matches) {
            ++visibleCount;
        }
    }
    emptyTitle->setText(normalized.isEmpty() ? "No projects yet"
                                             : "No matching projects");
    projectStack->setCurrentIndex(visibleCount > 0 ? 0 : 1);
}

void ProjectBrowser::createProject() {
    CreateProjectDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    reloadProjects();
    emit openProjectRequested(dialog.projectFile());
}

void ProjectBrowser::openExistingProject() {
    const QString projectFile = QFileDialog::getOpenFileName(
        this, "Open an Atlas project", QDir::homePath(),
        "Atlas projects (*.atlas)");
    if (projectFile.isEmpty()) {
        return;
    }
    if (!ProjectStore::isProjectFile(projectFile)) {
        QMessageBox::warning(this, "Invalid project",
                             "Choose a readable .atlas project file.");
        return;
    }
    ProjectStore::addRecentProject(projectFile);
    reloadProjects();
    emit openProjectRequested(projectFile);
}

void ProjectBrowser::openSelectedProject() {
    QListWidgetItem* item = projectList->currentItem();
    if (item == nullptr) {
        return;
    }
    const QString projectFile = item->data(ProjectPathRole).toString();
    if (!ProjectStore::isProjectFile(projectFile)) {
        QMessageBox::warning(
            this, "Project unavailable",
            "Atlas could not find this project. Remove it from the list or "
            "open it again from its current location.");
        return;
    }
    ProjectStore::addRecentProject(projectFile);
    emit openProjectRequested(projectFile);
}

void ProjectBrowser::showProjectMenu(const QPoint& position) {
    QListWidgetItem* item = projectList->itemAt(position);
    if (item == nullptr) {
        item = projectList->currentItem();
    }
    if (item == nullptr) {
        return;
    }
    projectList->setCurrentItem(item);
    const QString projectFile = item->data(ProjectPathRole).toString();
    const bool available = ProjectStore::isProjectFile(projectFile);

    QMenu menu(this);
    QAction* open = menu.addAction("Open project");
    open->setEnabled(available);
    QAction* reveal = menu.addAction("Show in Finder");
    reveal->setEnabled(QFileInfo::exists(QFileInfo(projectFile).absolutePath()));
    menu.addSeparator();
    QAction* remove = menu.addAction("Remove from list");
    QAction* selected = menu.exec(projectList->viewport()->mapToGlobal(position));
    if (selected == open) {
        openSelectedProject();
    } else if (selected == reveal) {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QFileInfo(projectFile).absolutePath()));
    } else if (selected == remove) {
        ProjectStore::removeRecentProject(projectFile);
        reloadProjects();
    }
}
