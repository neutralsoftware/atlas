//
// animation.h
// As part of the Atlas project
// Created by Neutral Software in 2026
// --------------------------------------------------
// Description: Animation engine and systems definition
// Copyright (c) 2025 Neutral Software
//

#ifndef ATLAS_ANIMATION_H
#define ATLAS_ANIMATION_H

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/quaternion_float.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace atlas::animation {
using NodeIndex = uint32_t;
constexpr NodeIndex INVALID_NODE_INDEX = UINT32_MAX;
} // namespace atlas::animation

struct SkeletonNode {
    std::string name;
    atlas::animation::NodeIndex parent;

    glm::mat4 localTransform;
};

struct Skeleton {
    std::vector<SkeletonNode> nodes;
};

struct SkinJoint {
    atlas::animation::NodeIndex nodeIndex =
        atlas::animation::INVALID_NODE_INDEX;
    glm::mat4 inverseBindMatrix{1.0f};
};

struct Skin {
    std::vector<SkinJoint> joints;
};

template <typename T> struct Keyframe {
    float time = 0.0f;
    T value;
};

struct AnimationChannel {
    atlas::animation::NodeIndex targetNode =
        atlas::animation::INVALID_NODE_INDEX;

    std::vector<Keyframe<glm::vec3>> positionKeyframes;
    std::vector<Keyframe<glm::quat>> rotationKeyframes;
    std::vector<Keyframe<glm::vec3>> scaleKeyframes;
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;

    std::vector<AnimationChannel> channels;
};

#endif // ATLAS_ANIMATION_H