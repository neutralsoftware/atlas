#include "atlas/object.h"

void ModelMesh::updateSkin(const std::vector<glm::mat4> &globalPose) {
    if (std::ranges::any_of(skin.joints, [&](const auto &joint) {
            return joint.nodeIndex >= globalPose.size();
        })) {
        jointMatrices.clear();
        return;
    }
    jointMatrices.resize(skin.joints.size());
    for (size_t i = 0; i < skin.joints.size(); ++i) {
        const auto &joint = skin.joints[i];
        jointMatrices[i] = globalPose[joint.nodeIndex] * joint.inverseBindMatrix *
                           inverseMeshTransform;
    }
}

glm::vec3 ModelMesh::posedPosition(const CoreVertex &vertex) const {
    const glm::vec4 rest(vertex.position.toGlm(), 1.0f);
    if (jointMatrices.empty() || vertex.weights.x <= 0.0f)
        return glm::vec3(rest);
    glm::vec4 position(0.0f);
    for (int i = 0; i < 4; ++i) {
        if (vertex.weights[i] > 0.0f)
            position += (jointMatrices[vertex.joints[i]] * rest) * vertex.weights[i];
    }
    return glm::vec3(position);
}

void ModelMesh::bindSkin(opal::Pipeline &pipeline) const {
    pipeline.setUniform1i("hasSkin", !jointMatrices.empty());
    if (!jointMatrices.empty())
        pipeline.bindBufferData("skinColumns", jointMatrices.data(),
                                jointMatrices.size() * sizeof(glm::mat4));
}
