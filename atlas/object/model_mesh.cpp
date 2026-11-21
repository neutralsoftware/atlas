#include "atlas/object.h"

void ModelMesh::updateSkin(const std::vector<glm::mat4> &globalPose) {
    jointMatrices.resize(skin.joints.size());
    for (size_t i = 0; i < skin.joints.size(); ++i) {
        const auto &joint = skin.joints[i];
        jointMatrices[i] = globalPose[joint.nodeIndex] * joint.inverseBindMatrix *
                           inverseMeshTransform;
    }
}

void ModelMesh::bindSkin(opal::Pipeline &pipeline) const {
    pipeline.setUniform1i("hasSkin", !jointMatrices.empty());
    if (!jointMatrices.empty())
        pipeline.bindBufferData("skinColumns", jointMatrices.data(),
                                jointMatrices.size() * sizeof(glm::mat4));
}
