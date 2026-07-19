#ifndef ATLAS_TOOLCHAININSTALLER_H
#define ATLAS_TOOLCHAININSTALLER_H

#include <QString>
#include <QStringList>

class QWidget;

namespace ToolchainInstaller {
bool ensureInstalled(QWidget* parent = nullptr);
bool install(QWidget* parent = nullptr);
QString executablePath();
bool run(const QStringList& arguments, const QString& workingDirectory,
         QString* errorMessage = nullptr);
}

#endif
