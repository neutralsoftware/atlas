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
#include "glm/ext/vector_uint4.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace atlas::animation {
using NodeIndex = uint32_t;
constexpr NodeIndex INVALID_NODE_INDEX = UINT32_MAX;
} // namespace atlas::animation

struct SkeletonNode {
    std::string name;
    atlas::animation::NodeIndex parent = atlas::animation::INVALID_NODE_INDEX;

    glm::mat4 localTransform{1.0f};
};

struct Skeleton {
    std::vector<SkeletonNode> nodes;
};

struct SkinJoint {
    atlas::animation::NodeIndex nodeIndex =
        atlas::animation::INVALID_NODE_INDEX;
    glm::mat4 inverseBindMatrix{1.0f};
};

struct VertexSkinData {
    glm::uvec4 joints{0};
    glm::vec4 weights{0.0f};
};

struct Skin {
    std::vector<SkinJoint> joints;
};

template <typename T> struct Keyframe {
    float time = 0.0f;
    T value{};
    bool step = false;
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

class Animator {
  public:
    void setSkeleton(Skeleton skeleton);
    bool addClip(AnimationClip clip);
    const Skeleton &getSkeleton() const { return skeleton; }
    const std::vector<AnimationClip> &getClips() const { return clips; }
    const AnimationClip *findClip(std::string_view name) const;

    bool play(std::string_view clip, bool loop = true, float speed = 1.0f);
    bool crossfade(std::string_view clip, float seconds, bool loop = true,
                   float speed = 1.0f);
    bool setBlend(std::string_view clip, float weight, bool loop = true,
                  float speed = 1.0f);
    void clearBlend();
    void stop();
    void setPaused(bool value) { paused = value; }
    bool isFinished() const;
    void update(float deltaSeconds);
    const std::vector<glm::mat4> &getGlobalPose() const { return globalPose; }

  private:
    struct NodePose {
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        bool animated = false;
    };

    struct Playback {
        std::size_t clip;
        float time;
        bool loop;
        float speed;
    };

    std::optional<Playback> makePlayback(std::string_view name, bool loop,
                                        float speed) const;
    void advance(Playback &playback, float deltaSeconds) const;
    void sample(const Playback &playback, std::vector<NodePose> &pose) const;
    void evaluate();

    Skeleton skeleton;
    std::vector<AnimationClip> clips;
    std::vector<NodePose> bindPose;
    std::vector<NodePose> evaluatedPose;
    std::vector<NodePose> scratchPose;
    std::vector<NodePose> fadeSnapshot;
    std::vector<glm::mat4> globalPose;
    std::optional<Playback> current;
    std::optional<Playback> outgoing;
    std::optional<Playback> blend;
    float blendWeight = 0.0f;
    float fadeDuration = 0.0f;
    float fadeElapsed = 0.0f;
    bool paused = false;
};

#endif // ATLAS_ANIMATION_H