#include <editor/project/projectStore.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStringList>
#include <QTextStream>

#include <algorithm>

namespace {
QString normalizedProjectPath(const QString& projectFile) {
    QFileInfo info(projectFile);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QString tomlString(QString value) {
    value.replace('\\', "\\\\");
    value.replace('"', "\\\"");
    value.replace('\n', "\\n");
    return value;
}

bool writeFile(const QString& path, const QByteArray& contents,
               QString* errorMessage) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    if (file.write(contents) != contents.size() || !file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

QString projectConfig(const QString& name,
                      AtlasProjectTemplate projectTemplate) {
    QString renderer = "deferred";
    const bool globalIllumination =
        projectTemplate == AtlasProjectTemplate::PbrDdgi;
    if (projectTemplate == AtlasProjectTemplate::PathTracing) {
        renderer = "pathtracing";
    }

    QString config;
    QTextStream stream(&config);
    stream << "app_name = \"" << tomlString(name) << "\"\n";
    stream << "atlas_version = \"alpha9\"\n";
    stream << "backend = \"AUTO\"\n";
    stream << "name = \"" << tomlString(name) << "\"\n";
    stream << "platform = \"DESKTOP\"\n\n";
    stream << "[game]\n";
    stream << "assets = [\"assets/\"]\n";
    stream << "main_scene = \"main.ascene\"\n\n";
    stream << "[pack]\n";
    stream << "icon = \"none\"\n";
    stream << "supported_platforms = \"all\"\n\n";
    stream << "[renderer]\n";
    stream << "default = \"" << renderer << "\"\n";
    stream << "global_illumination = "
           << (globalIllumination ? "true" : "false") << "\n\n";
    stream << "[window]\n";
    stream << "dimensions = [1280, 720]\n";
    stream << "mouse_capture = false\n";
    stream << "multisampling = "
           << (projectTemplate == AtlasProjectTemplate::PathTracing ? "false"
                                                                    : "true")
           << "\n";
    stream << "ssaoScale = 0.5\n";
    return config;
}

QByteArray starterScene(AtlasProjectTemplate projectTemplate) {
    QByteArray scene = QByteArrayLiteral(R"({
    "name": "Main Scene",
    "id": "main_scene",
    "objects": [
        {
            "name": "Cube",
            "type": "solid",
            "solid_type": "cube",
            "position": [0.0, 0.0, 0.0],
            "rotation": [0.0, 0.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "material": "",
            "components": []
        }
    ],
    "lights": [
        {
            "type": "ambient",
            "intensity": 0.25
        }
    ],
    "camera": {
        "position": [0.0, 1.5, -5.0],
        "target": [0.0, 0.0, 0.0],
        "fov": 60.0
    },
    "targets": [
        {
            "name": "Main Target",
            "type": "%RENDER_TARGET_TYPE%",
            "render": true,
            "display": true
        }
    ],
    "environment": {
        "automaticAmbient": true,
        "atmosphereSky": true
    }
}
)");
    scene.replace("%RENDER_TARGET_TYPE%",
                  projectTemplate == AtlasProjectTemplate::PathTracing
                      ? "scene"
                      : "multisampled");
    return scene;
}

QString capture(const QString& contents, const QString& pattern) {
    const QRegularExpression expression(
        pattern, QRegularExpression::MultilineOption);
    const QRegularExpressionMatch match = expression.match(contents);
    return match.hasMatch() ? match.captured(1) : QString();
}
}

QList<AtlasProjectInfo> ProjectStore::recentProjects() {
    QSettings settings("Neutral Software", "Atlas Engine");
    const QStringList recent =
        settings.value("projects/recentFiles").toStringList();
    QList<AtlasProjectInfo> projects;
    for (const QString& path : recent) {
        const auto info = projectInfo(path);
        if (info.has_value()) {
            projects.append(*info);
        }
    }
    return projects;
}

std::optional<AtlasProjectInfo> ProjectStore::projectInfo(
    const QString& projectFile) {
    if (projectFile.trimmed().isEmpty()) {
        return std::nullopt;
    }

    const QFileInfo fileInfo(projectFile);
    AtlasProjectInfo result;
    result.projectFile = normalizedProjectPath(projectFile);
    result.directory = QFileInfo(result.projectFile).absolutePath();
    result.name = QFileInfo(result.directory).fileName();
    result.renderer = "Unknown";
    result.available = isProjectFile(result.projectFile);
    result.lastModified = fileInfo.lastModified();

    if (!result.available) {
        return result;
    }

    QFile file(result.projectFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }
    const QString contents = QString::fromUtf8(file.readAll());
    const QString configuredName =
        capture(contents, QStringLiteral("^name\\s*=\\s*\"([^\"]+)\""));
    if (!configuredName.isEmpty()) {
        result.name = configuredName;
    }

    const QString renderer = capture(
        contents, QStringLiteral("^default\\s*=\\s*\"([^\"]+)\""));
    const bool ddgi = QRegularExpression(
                          QStringLiteral(
                              "^global_illumination\\s*=\\s*true\\s*$"),
                          QRegularExpression::MultilineOption |
                              QRegularExpression::CaseInsensitiveOption)
                          .match(contents)
                          .hasMatch();
    if (renderer.compare("pathtracing", Qt::CaseInsensitive) == 0) {
        result.renderer = "Path Tracing";
    } else if (ddgi) {
        result.renderer = "PBR + DDGI";
    } else {
        result.renderer = "PBR";
    }
    return result;
}

QString ProjectStore::createProject(const QString& name,
                                    const QString& parentDirectory,
                                    AtlasProjectTemplate projectTemplate,
                                    QString* errorMessage) {
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "Enter a project name.";
        }
        return QString();
    }
    if (trimmedName.contains(QRegularExpression(QStringLiteral(
            R"([/\\:*?"<>|]))")))) {
        if (errorMessage != nullptr) {
            *errorMessage = "The project name contains unsupported characters.";
        }
        return QString();
    }

    QDir parent(parentDirectory);
    if (!parent.exists()) {
        if (errorMessage != nullptr) {
            *errorMessage = "Choose an existing project location.";
        }
        return QString();
    }

    const QString projectDirectory = parent.filePath(trimmedName);
    if (QFileInfo::exists(projectDirectory)) {
        if (errorMessage != nullptr) {
            *errorMessage = "A folder with this project name already exists.";
        }
        return QString();
    }

    QDir root;
    if (!root.mkpath(projectDirectory + "/assets/scripts")) {
        if (errorMessage != nullptr) {
            *errorMessage = "Atlas could not create the project folder.";
        }
        return QString();
    }

    const QString projectFile = projectDirectory + "/project.atlas";
    QString writeError;
    const bool wroteProject =
        writeFile(projectFile, projectConfig(trimmedName, projectTemplate).toUtf8(),
                  &writeError);
    const bool wroteScene =
        wroteProject && writeFile(projectDirectory + "/main.ascene",
                                  starterScene(projectTemplate), &writeError);
    if (!wroteProject || !wroteScene) {
        QDir(projectDirectory).removeRecursively();
        if (errorMessage != nullptr) {
            *errorMessage = writeError.isEmpty()
                                ? "Atlas could not write the project files."
                                : writeError;
        }
        return QString();
    }

    addRecentProject(projectFile);
    return normalizedProjectPath(projectFile);
}

bool ProjectStore::isProjectFile(const QString& projectFile) {
    const QFileInfo info(projectFile);
    return info.exists() && info.isFile() && info.isReadable() &&
           info.suffix().compare("atlas", Qt::CaseInsensitive) == 0;
}

void ProjectStore::addRecentProject(const QString& projectFile) {
    const QString normalized = normalizedProjectPath(projectFile);
    if (normalized.isEmpty()) {
        return;
    }
    QSettings settings("Neutral Software", "Atlas Engine");
    QStringList recent = settings.value("projects/recentFiles").toStringList();
    recent.removeAll(normalized);
    recent.prepend(normalized);
    while (recent.size() > 20) {
        recent.removeLast();
    }
    settings.setValue("projects/recentFiles", recent);
}

void ProjectStore::removeRecentProject(const QString& projectFile) {
    const QString normalized = normalizedProjectPath(projectFile);
    QSettings settings("Neutral Software", "Atlas Engine");
    QStringList recent = settings.value("projects/recentFiles").toStringList();
    recent.removeAll(normalized);
    settings.setValue("projects/recentFiles", recent);
}

QString ProjectStore::templateName(AtlasProjectTemplate projectTemplate) {
    switch (projectTemplate) {
    case AtlasProjectTemplate::Pbr:
        return "PBR";
    case AtlasProjectTemplate::PbrDdgi:
        return "PBR + DDGI";
    case AtlasProjectTemplate::PathTracing:
        return "Path Tracing";
    }
    return "PBR";
}
