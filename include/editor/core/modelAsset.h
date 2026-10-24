#ifndef ATLAS_EDITOR_MODEL_ASSET_H
#define ATLAS_EDITOR_MODEL_ASSET_H

#include <QFileInfo>
#include <QString>

namespace editor {
inline bool isModelAssetPath(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "obj" || suffix == "fbx" || suffix == "gltf" ||
           suffix == "glb" || suffix == "dae" || suffix == "blend" ||
           suffix == "usd" || suffix == "usda" || suffix == "usdc" ||
           suffix == "usdz";
}
}

#endif
