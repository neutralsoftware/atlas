#include "atlas/animation.h"
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace {

template <typename T, typename Interpolate>
T sampleTrack(const std::vector<Keyframe<T>> &keys, float time,
              const T &fallback, Interpolate interpolate) {
    if (keys.empty())
        return fallback;
    const auto next = std::upper_bound(
        keys.begin(), keys.end(), time,
        [](float value, const auto &key) { return value < key.time; });
    if (next == keys.begin())
        return next->value;
    const auto &previous = *std::prev(next);
    if (next == keys.end() || previous.step)
        return previous.value;
    return interpolate(previous.value, next->value,
                       (time - previous.time) / (next->time - previous.time));
}

glm::quat unitRotation(const glm::quat &rotation) {
    const float lengthSquared = glm::dot(rotation, rotation);
    return std::isfinite(lengthSquared) && lengthSquared > 0.0f
               ? rotation / std::sqrt(lengthSquared)
               : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

glm::quat interpolateRotation(const glm::quat &a, const glm::quat &b,
                             float weight) {
    return unitRotation(glm::slerp(unitRotation(a), unitRotation(b), weight));
}

template <typename T>
void prepareTrack(std::vector<Keyframe<T>> &keys, float &duration) {
    std::erase_if(keys, [](const auto &key) { return !std::isfinite(key.time); });
    std::stable_sort(keys.begin(), keys.end(),
                     [](const auto &a, const auto &b) { return a.time < b.time; });
    if (!keys.empty())
        duration = std::max(duration, keys.back().time);
}

}

void Animator::setSkeleton(Skeleton newSkeleton) {
    for (size_t i = 0; i < newSkeleton.nodes.size(); ++i) {
        const auto parent = newSkeleton.nodes[i].parent;
        if (parent != atlas::animation::INVALID_NODE_INDEX && parent >= i)
            throw std::invalid_argument("Skeleton nodes must follow their parents");
    }

    skeleton = std::move(newSkeleton);
    clips.clear();
    bindPose.assign(skeleton.nodes.size(), {});
    for (size_t i = 0; i < skeleton.nodes.size(); ++i) {
        auto &pose = bindPose[i];
        glm::vec3 skew;
        glm::vec4 perspective;
        if (!glm::decompose(skeleton.nodes[i].localTransform, pose.scale,
                            pose.rotation, pose.translation, skew, perspective))
            pose = {};
        pose.rotation = unitRotation(pose.rotation);
    }
    stop();
}

bool Animator::addClip(AnimationClip clip) {
    if (clip.name.empty() || findClip(clip.name))
        return false;
    if (!std::isfinite(clip.duration) || clip.duration < 0.0f)
        clip.duration = 0.0f;

    for (auto &channel : clip.channels) {
        if (channel.targetNode >= skeleton.nodes.size())
            return false;
        prepareTrack(channel.positionKeyframes, clip.duration);
        prepareTrack(channel.rotationKeyframes, clip.duration);
        prepareTrack(channel.scaleKeyframes, clip.duration);
    }
    clips.push_back(std::move(clip));
    return true;
}

const AnimationClip *Animator::findClip(std::string_view name) const {
    const auto found = std::ranges::find_if(
        clips, [&](const auto &clip) { return clip.name == name; });
    return found == clips.end() ? nullptr : &*found;
}

std::optional<Animator::Playback> Animator::makePlayback(
    std::string_view name, bool loop, float speed) const {
    const auto *clip = findClip(name);
    if (!clip || !std::isfinite(speed) || speed <= 0.0f)
        return std::nullopt;
    return Playback{static_cast<size_t>(clip - clips.data()), 0.0f, loop, speed};
}

bool Animator::play(std::string_view name, bool loop, float speed) {
    const auto next = makePlayback(name, loop, speed);
    if (!next)
        return false;
    current = next;
    outgoing.reset();
    fadeSnapshot.clear();
    fadeDuration = 0.0f;
    blend.reset();
    paused = false;
    evaluate();
    return true;
}

bool Animator::crossfade(std::string_view name, float seconds, bool loop,
                         float speed) {
    const auto next = makePlayback(name, loop, speed);
    if (!next || !std::isfinite(seconds) || seconds < 0.0f)
        return false;
    if (!current || seconds == 0.0f)
        return play(name, loop, speed);

    if (fadeDuration > 0.0f || blend) {
        fadeSnapshot = evaluatedPose;
        outgoing.reset();
    } else {
        fadeSnapshot.clear();
        outgoing = current;
    }
    current = next;
    blend.reset();
    fadeDuration = seconds;
    fadeElapsed = 0.0f;
    paused = false;
    evaluate();
    return true;
}

bool Animator::setBlend(std::string_view name, float weight, bool loop,
                        float speed) {
    const auto next = makePlayback(name, loop, speed);
    if (!next || !current || !std::isfinite(weight))
        return false;
    if (!blend || blend->clip != next->clip)
        blend = next;
    blend->loop = loop;
    blend->speed = speed;
    blendWeight = std::clamp(weight, 0.0f, 1.0f);
    evaluate();
    return true;
}

void Animator::clearBlend() {
    blend.reset();
    evaluate();
}

void Animator::stop() {
    current.reset();
    outgoing.reset();
    blend.reset();
    fadeSnapshot.clear();
    fadeDuration = 0.0f;
    paused = false;
    evaluate();
}

bool Animator::isFinished() const {
    return current && !current->loop &&
           current->time >= clips[current->clip].duration;
}

void Animator::advance(Playback &playback, float deltaSeconds) const {
    const float duration = clips[playback.clip].duration;
    const double time = playback.time + static_cast<double>(deltaSeconds) * playback.speed;
    playback.time = playback.loop && duration > 0.0f
                        ? static_cast<float>(std::fmod(time, duration))
                        : static_cast<float>(std::min<double>(time, duration));
}

void Animator::sample(const Playback &playback, std::vector<NodePose> &pose) const {
    pose = bindPose;
    const auto linear = [](const glm::vec3 &a, const glm::vec3 &b, float ratio) {
        return glm::mix(a, b, ratio);
    };
    for (const auto &channel : clips[playback.clip].channels) {
        auto &node = pose[channel.targetNode];
        node.translation = sampleTrack(channel.positionKeyframes, playback.time,
                                       node.translation, linear);
        node.rotation = unitRotation(sampleTrack(channel.rotationKeyframes, playback.time,
                                                 node.rotation, interpolateRotation));
        node.scale = sampleTrack(channel.scaleKeyframes, playback.time, node.scale, linear);
        node.animated = !channel.positionKeyframes.empty() ||
                        !channel.rotationKeyframes.empty() ||
                        !channel.scaleKeyframes.empty();
    }
}

void Animator::evaluate() {
    auto mixPose = [&](const std::vector<NodePose> &other, float weight) {
        if (weight <= 0.0f)
            return;
        if (weight >= 1.0f) {
            evaluatedPose = other;
            return;
        }
        for (size_t i = 0; i < evaluatedPose.size(); ++i) {
            const auto &b = other[i];
            auto &a = evaluatedPose[i];
            a.translation = glm::mix(a.translation, b.translation, weight);
            a.rotation = interpolateRotation(a.rotation, b.rotation, weight);
            a.scale = glm::mix(a.scale, b.scale, weight);
            a.animated = a.animated || b.animated;
        }
    };

    if (current)
        sample(*current, evaluatedPose);
    else
        evaluatedPose = bindPose;

    if (fadeDuration > 0.0f) {
        if (outgoing)
            sample(*outgoing, scratchPose);
        mixPose(outgoing ? scratchPose : fadeSnapshot, 1.0f - fadeElapsed / fadeDuration);
    }
    if (blend && blendWeight > 0.0f) {
        sample(*blend, scratchPose);
        mixPose(scratchPose, blendWeight);
    }

    globalPose.resize(skeleton.nodes.size());
    for (size_t i = 0; i < globalPose.size(); ++i) {
        const auto &node = skeleton.nodes[i];
        const auto &pose = evaluatedPose[i];
        glm::mat4 local = node.localTransform;
        if (pose.animated)
            local = glm::translate(glm::mat4(1.0f), pose.translation) *
                    glm::mat4_cast(pose.rotation) *
                    glm::scale(glm::mat4(1.0f), pose.scale);
        globalPose[i] = node.parent == atlas::animation::INVALID_NODE_INDEX
                            ? local
                            : globalPose[node.parent] * local;
    }
}

void Animator::update(float deltaSeconds) {
    if (!current || paused || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
        return;
    advance(*current, deltaSeconds);
    if (blend)
        advance(*blend, deltaSeconds);
    if (outgoing)
        advance(*outgoing, deltaSeconds);
    if (fadeDuration > 0.0f) {
        fadeElapsed += deltaSeconds;
        if (fadeElapsed >= fadeDuration) {
            fadeDuration = 0.0f;
            outgoing.reset();
            fadeSnapshot.clear();
        }
    }
    evaluate();
}
