#ifndef ATLAS_TOOLCHAININSTALLER_H
#define ATLAS_TOOLCHAININSTALLER_H

class QWidget;

namespace ToolchainInstaller {
bool ensureInstalled(QWidget* parent = nullptr);
bool install(QWidget* parent = nullptr);
}

#endif
