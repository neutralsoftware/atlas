#ifndef ATLAS_PROJECTSTORE_H
#define ATLAS_PROJECTSTORE_H

#include <QDateTime>
#include <QList>
#include <QString>

#include <optional>

enum class AtlasProjectTemplate {
    Pbr,
    PbrDdgi,
    PathTracing
};

struct AtlasProjectInfo {
    QString name;
    QString projectFile;
    QString directory;
    QString renderer;
    QDateTime lastModified;
    bool available = false;
};

class ProjectStore {
public:
    static QList<AtlasProjectInfo> recentProjects();
    static std::optional<AtlasProjectInfo> projectInfo(
        const QString& projectFile);
    static QString createProject(const QString& name,
                                 const QString& parentDirectory,
                                 AtlasProjectTemplate projectTemplate,
                                 QString* errorMessage);
    static bool isProjectFile(const QString& projectFile);
    static void addRecentProject(const QString& projectFile);
    static void removeRecentProject(const QString& projectFile);
    static QString templateName(AtlasProjectTemplate projectTemplate);
};

#endif
