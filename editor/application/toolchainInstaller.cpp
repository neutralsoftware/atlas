#include "editor/application/toolchainInstaller.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
struct ToolchainPaths {
    QString bundledCli;
    QString bundledRuntime;
    QString installedCli;
    QString installedRuntime;
    QString config;
};

QByteArray digest(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return {};
    return hash.result();
}

bool filesMatch(const QString& left, const QString& right) {
    const QFileInfo leftInfo(left);
    const QFileInfo rightInfo(right);
    return leftInfo.isFile() && rightInfo.isFile() &&
           leftInfo.size() == rightInfo.size() && digest(left) == digest(right);
}

ToolchainPaths paths() {
    const QDir contents(QCoreApplication::applicationDirPath() + "/..");
    const QString installRoot =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        "/Atlas Engine/toolchains/" + ATLAS_TOOLCHAIN_VERSION;
    const QString home = QDir::homePath();
    return {
        contents.filePath("Helpers/atlas"),
        contents.filePath("Frameworks/runtime.dylib"),
        installRoot + "/bin/atlas",
        installRoot + "/lib/runtime.dylib",
        home + "/.atlas/config.json",
    };
}

QJsonObject readConfig(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject();
}

bool configMatches(const ToolchainPaths& toolchain) {
    const QJsonObject root = readConfig(toolchain.config);
    const QJsonObject entry = root.value(ATLAS_TOOLCHAIN_VERSION).toObject();
    const QJsonObject onboarding = entry.value("onboardingData").toObject();
    return onboarding.value("atlasExecutablePath").toString() ==
               toolchain.installedCli &&
           onboarding.value("runtimeLib").toString() ==
               toolchain.installedRuntime;
}

bool copyFile(const QString& source, const QString& destination,
              QString* error) {
    QFile input(source);
    if (!input.open(QIODevice::ReadOnly)) {
        *error = input.errorString();
        return false;
    }
    QSaveFile output(destination);
    if (!output.open(QIODevice::WriteOnly)) {
        *error = output.errorString();
        return false;
    }
    while (!input.atEnd()) {
        const QByteArray data = input.read(1024 * 1024);
        if (data.isEmpty() && input.error() != QFileDevice::NoError) {
            *error = input.errorString();
            return false;
        }
        if (output.write(data) != data.size()) {
            *error = output.errorString();
            return false;
        }
    }
    if (!output.commit()) {
        *error = output.errorString();
        return false;
    }
    if (!QFile::setPermissions(destination, QFile::permissions(source))) {
        *error = QStringLiteral("Could not apply permissions to %1")
                     .arg(destination);
        return false;
    }
    return true;
}

bool writeConfig(const ToolchainPaths& toolchain, QString* error) {
    QJsonObject root = readConfig(toolchain.config);
    QJsonObject entry = root.value(ATLAS_TOOLCHAIN_VERSION).toObject();
    QJsonObject onboarding = entry.value("onboardingData").toObject();
    onboarding.insert("atlasExecutablePath", toolchain.installedCli);
    onboarding.insert("runtimeLib", toolchain.installedRuntime);
    entry.insert("onboardingData", onboarding);
    root.insert(ATLAS_TOOLCHAIN_VERSION, entry);

    const QFileInfo configInfo(toolchain.config);
    if (!QDir().mkpath(configInfo.absolutePath())) {
        *error = QStringLiteral("Could not create %1")
                     .arg(configInfo.absolutePath());
        return false;
    }
    QSaveFile output(toolchain.config);
    if (!output.open(QIODevice::WriteOnly)) {
        *error = output.errorString();
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (output.write(data) != data.size() || !output.commit()) {
        *error = output.errorString();
        return false;
    }
    return true;
}
}

bool ToolchainInstaller::ensureInstalled(QWidget* parent) {
    const ToolchainPaths toolchain = paths();
    if (!QFileInfo::exists(toolchain.bundledCli) ||
        !QFileInfo::exists(toolchain.bundledRuntime))
        return true;

    const bool installed = filesMatch(toolchain.bundledCli,
                                      toolchain.installedCli) &&
                           filesMatch(toolchain.bundledRuntime,
                                      toolchain.installedRuntime) &&
                           configMatches(toolchain);
    if (installed)
        return true;

    QMessageBox prompt(parent);
    prompt.setWindowTitle("Welcome to Atlas Engine");
    prompt.setIcon(QMessageBox::Information);
    prompt.setText("Install the Atlas toolchain for this user?");
    prompt.setInformativeText(
        "Atlas Engine includes the command-line tools and runtime required to create, run, and export projects. They will be installed in your user Library and do not require administrator access.");
    auto* installButton = prompt.addButton("Install Toolchain",
                                           QMessageBox::AcceptRole);
    prompt.addButton("Not Now", QMessageBox::RejectRole);
    prompt.setDefaultButton(installButton);
    prompt.exec();
    if (prompt.clickedButton() != installButton)
        return false;

    const QFileInfo cliInfo(toolchain.installedCli);
    const QFileInfo runtimeInfo(toolchain.installedRuntime);
    QString error;
    if (!QDir().mkpath(cliInfo.absolutePath()) ||
        !QDir().mkpath(runtimeInfo.absolutePath())) {
        error = "Could not create the Atlas toolchain directory.";
    } else if (!copyFile(toolchain.bundledCli, toolchain.installedCli,
                         &error) ||
               !copyFile(toolchain.bundledRuntime,
                         toolchain.installedRuntime, &error) ||
               !writeConfig(toolchain, &error)) {
    }
    if (!error.isEmpty()) {
        QMessageBox::critical(parent, "Toolchain Installation Failed", error);
        return false;
    }

    QMessageBox::information(
        parent, "Atlas Toolchain Installed",
        "The Atlas toolchain is ready. Projects can now be created, run, and exported from Atlas Engine.");
    return true;
}
