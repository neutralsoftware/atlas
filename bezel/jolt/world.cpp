//
// world.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Physics World creation and some Jolt-required classes
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/jolt/world.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/IssueReporting.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "Jolt/Physics/Collision/CollideShape.h"
#include "Jolt/Physics/Collision/ShapeCast.h"
#include "Jolt/Physics/EPhysicsUpdateError.h"
#include "Jolt/RegisterTypes.h"
#include "bezel/bezel.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include "atlas/tracer/log.h"
#include "bezel/jolt/query.h"
#include <cmath>
#include <cstdarg>

namespace {

class IgnoreBodyFilter final : public JPH::BodyFilter {
  public:
    uint32_t ignoreId;

    explicit IgnoreBodyFilter(uint32_t id) : ignoreId(id) {}

    bool ShouldCollide(const JPH::BodyID &inBodyID) const override {
        if (ignoreId == bezel::INVALID_JOLT_ID) {
            return true;
        }
        return inBodyID.GetIndexAndSequenceNumber() != ignoreId;
    }
};

} // namespace

namespace bezel_jolt {
std::map<JPH::BodyID, bezel::Rigidbody *> bodyIdToRigidbodyMap;
}

BroadPhaseLayerImpl::BroadPhaseLayerImpl() {
    mObjectToBroadPhase[bezel::jolt::layers::NON_MOVING] =
        bezel::jolt::broad_phase_layers::NON_MOVING;
    mObjectToBroadPhase[bezel::jolt::layers::MOVING] =
        bezel::jolt::broad_phase_layers::MOVING;
    mObjectToBroadPhase[bezel::jolt::layers::SENSOR] =
        bezel::jolt::broad_phase_layers::SENSOR;
}

JPH::uint BroadPhaseLayerImpl::GetNumBroadPhaseLayers() const {
    return bezel::jolt::broad_phase_layers::NUM_BROADPHASE_LAYERS;
}

JPH::BroadPhaseLayer
BroadPhaseLayerImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const {
    return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char *BroadPhaseLayerImpl::GetBroadPhaseLayerName(
    JPH::BroadPhaseLayer inLayer) const {
    switch (static_cast<JPH::uint>(inLayer.GetValue())) {
    case 0:
        return "NON_MOVING";
    case 1:
        return "MOVING";
    case 2:
        return "SENSOR";
    default:
        return "UNKNOWN";
    }
}
#endif

bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1,
                                              JPH::ObjectLayer inLayer2) const {
    using namespace bezel::jolt::layers;

    if (inLayer1 == SENSOR)
        return inLayer2 != SENSOR;
    if (inLayer2 == SENSOR)
        return inLayer1 != SENSOR;

    if (inLayer1 == NON_MOVING && inLayer2 == NON_MOVING)
        return false;

    return true;
}

bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(
    JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
    using namespace bezel::jolt::layers;

    switch (inLayer1) {
    case NON_MOVING:
        return inLayer2 == bezel::jolt::broad_phase_layers::MOVING ||
               inLayer2 == bezel::jolt::broad_phase_layers::SENSOR;

    case MOVING:
        return true;

    case SENSOR:
        return inLayer2 == bezel::jolt::broad_phase_layers::MOVING ||
               inLayer2 == bezel::jolt::broad_phase_layers::NON_MOVING;

    default:
        return false;
    }
}

void bezel::PhysicsWorld::addBody(
    const std::shared_ptr<bezel::Rigidbody> &body) {
    bodies.push_back(body->id);
}

void bezel::PhysicsWorld::setGravity(const Position3d &gravity) {
    physicsSystem.SetGravity(JPH::Vec3(static_cast<float>(gravity.x),
                                       static_cast<float>(gravity.y),
                                       static_cast<float>(gravity.z)));
}

void bezel_jolt::AtlasLog(JoltLogLevel level, std::string_view msg) {
    switch (level) {
    case JoltLogLevel::Info:
        atlas_log(std::string("[Jolt] ") + std::string(msg));
        break;
    case JoltLogLevel::Warning:
        atlas_warning(std::string("[Jolt] ") + std::string(msg));
        break;
    case JoltLogLevel::Error:
        atlas_error(std::string("[Jolt] ") + std::string(msg));
        break;
    }
}

JoltLogLevel bezel_jolt::Classify(std::string_view s) {
    if (s.starts_with("Error:") || s.starts_with("ERROR:") ||
        s.starts_with("FATAL") || s.find("failed") != std::string_view::npos ||
        s.find("Out of memory") != std::string_view::npos) {
        return JoltLogLevel::Error;
    }

    if (s.starts_with("Warning:") || s.starts_with("WARN:") ||
        s.find("deprecated") != std::string_view::npos) {
        return JoltLogLevel::Warning;
    }

    return JoltLogLevel::Info;
}

void bezel_jolt::TraceImpl(const char *fmt, ...) {
    char buf[4096];

    va_list args;
    va_start(args, fmt);
    int _ = std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    std::string_view msg(buf);
    bezel_jolt::AtlasLog(bezel_jolt::Classify(msg), msg);
}

bool bezel_jolt::AssertFailedImpl(const char *expr, const char *msg,
                                  const char *file, JPH::uint line) {
    char buf[4096];
    int _ =
        std::snprintf(buf, sizeof(buf), "Assert failed: %s%s%s (%s:%u)", expr,
                      msg ? " : " : "", msg ? msg : "", file, (unsigned)line);

    bezel_jolt::AtlasLog(JoltLogLevel::Error, buf);

    return true;
}

void bezel::PhysicsWorld::init() {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = bezel_jolt::TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = bezel_jolt::AssertFailedImpl);

    if (JPH::Factory::sInstance == nullptr) {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    JPH::RegisterTypes();

    initialized = true;

    tempAllocator = std::make_unique<JPH::TempAllocatorMalloc>();

    uint32_t hw = std::max(1u, std::thread::hardware_concurrency());
    uint32_t num_worker_threads = std::max(1u, hw > 1 ? (hw - 1) : 1);

    constexpr uint32_t MAX_JOBS = 4096;
    constexpr uint32_t MAX_BARRIERS = 64;

    jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        MAX_JOBS, MAX_BARRIERS, num_worker_threads);

    constexpr uint32_t MAX_BODIES = 65536;
    constexpr uint32_t NUM_BODY_MUTEXES = 1024;
    constexpr uint32_t MAX_BODY_PAIRS = 65536;
    constexpr uint32_t MAX_CONTACT_CONSTRAINTS = 65536;

    physicsSystem.Init(MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS,
                       MAX_CONTACT_CONSTRAINTS, broadPhaseLayerInterface,
                       objectVsBroadPhaseLayerFilter, objectLayerPairFilter);

    physicsSystem.SetGravity(JPH::Vec3(0, -9.81f, 0));

    this->collisionDispatcher = std::make_shared<JoltCollisionDispatcher>();

    collisionDispatcher->setup(this);
}

void bezel::PhysicsWorld::update(float dt) {
    constexpr float maxPhysicsDelta = 1.0f / 30.0f;
    constexpr float softBodyStepDelta = 1.0f / 60.0f;
    const float physicsDelta = std::clamp(dt, 0.0f, maxPhysicsDelta);
    const bool hasActiveSoftBodies =
        physicsSystem.GetNumActiveBodies(JPH::EBodyType::SoftBody) > 0;
    const int collisionSteps =
        hasActiveSoftBodies
            ? std::max(1, static_cast<int>(std::ceil(
                              physicsDelta / softBodyStepDelta)))
            : 1;
    JPH::EPhysicsUpdateError error = physicsSystem.Update(
        physicsDelta, collisionSteps, tempAllocator.get(), jobSystem.get());

    if (error != JPH::EPhysicsUpdateError::None) {
        throw std::runtime_error("Jolt PhysicsWorld update error");
    }

    // Breakable constraints handling
    if (dt > 0.0f) {
        for (size_t i = 0; i < breakableConstraints.size();) {
            auto &entry = breakableConstraints[i];
            if (entry.constraint == nullptr) {
                ++i;
                continue;
            }

            const float breakForce = entry.breakForce;
            const float breakTorque = entry.breakTorque;
            if (breakForce <= 0.0f && breakTorque <= 0.0f) {
                ++i;
                continue;
            }

            float appliedForce = 0.0f;
            float appliedTorque = 0.0f;

            JPH::Constraint *c = entry.constraint.GetPtr();
            if (c == nullptr) {
                ++i;
                continue;
            }

            switch (c->GetSubType()) {
            case JPH::EConstraintSubType::Fixed: {
                auto *fixed = static_cast<JPH::FixedConstraint *>(c);

                const JPH::Vec3 lin = fixed->GetTotalLambdaPosition();
                const JPH::Vec3 ang = fixed->GetTotalLambdaRotation();

                appliedForce = lin.Length() / dt;
                appliedTorque = ang.Length() / dt;
                break;
            }

            case JPH::EConstraintSubType::Hinge: {
                auto *hinge = static_cast<JPH::HingeConstraint *>(c);

                const JPH::Vec3 lin = hinge->GetTotalLambdaPosition();
                appliedForce = lin.Length() / dt;

                const auto rot2 = hinge->GetTotalLambdaRotation();
                const float rotImpulseMag =
                    std::sqrt((rot2[0] * rot2[0]) + (rot2[1] * rot2[1]));

                const float limitImpulse =
                    std::abs(hinge->GetTotalLambdaRotationLimits());
                const float motorImpulse =
                    std::abs(hinge->GetTotalLambdaMotor());

                appliedTorque =
                    (rotImpulseMag + limitImpulse + motorImpulse) / dt;
                break;
            }

            case JPH::EConstraintSubType::Distance: {
                auto *distance = static_cast<JPH::DistanceConstraint *>(c);

                const float linImpulse =
                    std::abs(distance->GetTotalLambdaPosition());

                appliedForce = linImpulse / dt;
                appliedTorque = 0.0f;
                break;
            }

            default:
                ++i;
                continue;
            }

            const bool forceBroken =
                (breakForce > 0.0f && appliedForce > breakForce);
            const bool torqueBroken =
                (breakTorque > 0.0f && appliedTorque > breakTorque);

            if (forceBroken || torqueBroken) {
                atlas_log("[Jolt] Breaking constraint (force=" +
                          std::to_string(appliedForce) + "/" +
                          std::to_string(breakForce) +
                          ", torque=" + std::to_string(appliedTorque) + "/" +
                          std::to_string(breakTorque) + ")");

                physicsSystem.RemoveConstraint(c);
                breakableConstraints.erase(
                    breakableConstraints.begin() +
                    static_cast<std::vector<
                        PhysicsWorld::BreakableConstraint>::difference_type>(
                        i));
                continue;
            }

            ++i;
        }
    }

    collisionDispatcher->update(this);
}

bezel::PhysicsWorld::~PhysicsWorld() {
    jobSystem.reset();
    tempAllocator.reset();

    if (initialized) {
        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        initialized = false;
    }
}

bezel::RaycastResult bezel::PhysicsWorld::raycast(const Position3d &origin,
                                                  const Position3d &direction,
                                                  float maxDistance,
                                                  uint32_t ignoreBodyId) const {
    RaycastResult out;

    JPH::RVec3Arg originJolt(origin.x, origin.y, origin.z);
    JPH::RVec3Arg directionJolt(direction.x, direction.y, direction.z);

    const JPH::RRayCast ray(originJolt,
                            directionJolt.Normalized() * maxDistance);

    JPH::RayCastResult hit;

    IgnoreBodyFilter bodyFilter(ignoreBodyId);

    const bool didHit = physicsSystem.GetNarrowPhaseQuery().CastRay(
        ray, hit, {}, {}, bodyFilter);

    if (!didHit) {
        out.hit.didHit = false;
        return out;
    }

    const float dist = hit.mFraction * maxDistance;
    out.closestDistance = dist;

    RaycastHit h;
    h.didHit = true;
    h.distance = dist;
    h.position =
        Position3d(origin.x + (direction.x * hit.mFraction * maxDistance),
                   origin.y + (direction.y * hit.mFraction * maxDistance),
                   origin.z + (direction.z * hit.mFraction * maxDistance));

    JPH::BodyID bodyId = hit.mBodyID;
    auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
    h.rigidbody =
        (it != bezel_jolt::bodyIdToRigidbodyMap.end()) ? it->second : nullptr;
    JPH::Vec3 normal = -ray.mDirection * hit.mFraction;
    h.normal = Normal3d(normal.GetX(), normal.GetY(), normal.GetZ());

    out.hits.push_back(h);
    out.hit = h;

    return out;
}

bezel::RaycastResult
bezel::PhysicsWorld::raycastAll(const Position3d &origin,
                                const Position3d &direction, float maxDistance,
                                uint32_t ignoreBodyId) const {
    RaycastResult out;

    JPH::RVec3Arg originJolt(origin.x, origin.y, origin.z);
    JPH::RVec3Arg directionJolt(direction.x, direction.y, direction.z);

    const JPH::RRayCast ray(originJolt,
                            directionJolt.Normalized() * maxDistance);

    class AllHitsCollector : public JPH::CastRayCollector {
      public:
        std::vector<JPH::RayCastResult> hits;

        void AddHit(const JPH::RayCastResult &result) override {
            hits.push_back(result);
            ResetEarlyOutFraction();
        }
    };

    AllHitsCollector collector;
    IgnoreBodyFilter bodyFilter(ignoreBodyId);
    JPH::RayCastSettings settings;
    physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, {},
                                                {}, bodyFilter);

    if (collector.hits.empty()) {
        out.hit.didHit = false;
        return out;
    }

    float closestDist = std::numeric_limits<float>::max();

    for (const auto &hit : collector.hits) {
        const float dist = hit.mFraction * maxDistance;

        RaycastHit h;
        h.didHit = true;
        h.distance = dist;
        h.position =
            Position3d(origin.x + (direction.x * hit.mFraction * maxDistance),
                       origin.y + (direction.y * hit.mFraction * maxDistance),
                       origin.z + (direction.z * hit.mFraction * maxDistance));

        JPH::BodyID bodyId = hit.mBodyID;
        auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
        h.rigidbody = (it != bezel_jolt::bodyIdToRigidbodyMap.end())
                          ? it->second
                          : nullptr;

        JPH::Vec3 normal = -ray.mDirection * hit.mFraction;
        h.normal = Normal3d(normal.GetX(), normal.GetY(), normal.GetZ());

        out.hits.push_back(h);

        if (dist < closestDist) {
            closestDist = dist;
            out.hit = h;
            out.closestDistance = dist;
        }
    }

    return out;
}

bezel::OverlapResult
bezel::PhysicsWorld::overlap(const std::shared_ptr<bezel::PhysicsWorld> &world,
                             const std::shared_ptr<bezel::Collider> &collider,
                             const Position3d &position,
                             const Rotation3d &rotation,
                             uint32_t ignoreBodyId) const {
    (void)world;

    OverlapResult out;
    if (!collider) {
        return out;
    }

    auto shape = collider->getJoltShape();
    if (shape == nullptr) {
        return out;
    }

    glm::quat glmRotation = glm::normalize(rotation.toGlmQuat());
    JPH::Quat joltRotation(glmRotation.x, glmRotation.y, glmRotation.z,
                           glmRotation.w);

    const JPH::RVec3 joltPosition(position.x, position.y, position.z);
    const JPH::RMat44 transform =
        JPH::RMat44::sRotationTranslation(joltRotation, joltPosition);

    class AllHitsCollector final : public JPH::CollideShapeCollector {
      public:
        std::vector<JPH::CollideShapeResult> hits;

        void AddHit(const JPH::CollideShapeResult &result) override {
            hits.push_back(result);
            ResetEarlyOutFraction();
        }
    };

    AllHitsCollector collector;
    IgnoreBodyFilter bodyFilter(ignoreBodyId);

    JPH::CollideShapeSettings settings;
    physicsSystem.GetNarrowPhaseQuery().CollideShape(
        shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), transform, settings,
        JPH::RVec3::sZero(), collector, {}, {}, bodyFilter);

    out.hits.reserve(collector.hits.size());
    for (const auto &hit : collector.hits) {
        OverlapHit h;
        h.contactPoint =
            Position3d(hit.mContactPointOn2.GetX(), hit.mContactPointOn2.GetY(),
                       hit.mContactPointOn2.GetZ());
        h.penetrationAxis =
            Point3d(hit.mPenetrationAxis.GetX(), hit.mPenetrationAxis.GetY(),
                    hit.mPenetrationAxis.GetZ());
        h.penetrationDepth = hit.mPenetrationDepth;

        const JPH::BodyID bodyId = hit.mBodyID2;
        auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
        h.rigidbody = (it != bezel_jolt::bodyIdToRigidbodyMap.end())
                          ? it->second
                          : nullptr;

        out.hits.push_back(h);
    }
    out.hitAny = !out.hits.empty();
    return out;
}

bezel::SweepResult
bezel::PhysicsWorld::sweep(const std::shared_ptr<bezel::PhysicsWorld> &world,
                           const std::shared_ptr<bezel::Collider> &collider,
                           const Position3d &startPosition,
                           const Rotation3d &startRotation,
                           const Position3d &direction, Position3d &endPosition,
                           uint32_t ignoreBodyId) const {
    (void)world;

    SweepResult out;
    if (!collider) {
        endPosition = startPosition + direction;
        return out;
    }

    auto shape = collider->getJoltShape();
    if (shape == nullptr) {
        endPosition = startPosition + direction;
        return out;
    }

    const float dirLen =
        std::sqrt((direction.x * direction.x) + (direction.y * direction.y) +
                  (direction.z * direction.z));

    if (dirLen <= 0.0f) {
        endPosition = startPosition;
        return out;
    }

    glm::quat glmRotation = glm::normalize(startRotation.toGlmQuat());
    JPH::Quat joltRotation(glmRotation.x, glmRotation.y, glmRotation.z,
                           glmRotation.w);

    const JPH::RVec3 joltStart(startPosition.x, startPosition.y,
                               startPosition.z);
    const JPH::RMat44 startWorldTransform =
        JPH::RMat44::sRotationTranslation(joltRotation, joltStart);

    const JPH::Vec3 castDirection(static_cast<float>(direction.x),
                                  static_cast<float>(direction.y),
                                  static_cast<float>(direction.z));

    const JPH::RShapeCast cast = JPH::RShapeCast::sFromWorldTransform(
        shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), startWorldTransform,
        castDirection);

    class ClosestHitCollector final : public JPH::CastShapeCollector {
      public:
        bool hasHit = false;
        JPH::ShapeCastResult best;

        void AddHit(const JPH::ShapeCastResult &result) override {
            if (!hasHit ||
                result.GetEarlyOutFraction() < best.GetEarlyOutFraction()) {
                hasHit = true;
                best = result;
                UpdateEarlyOutFraction(result.GetEarlyOutFraction());
            }
        }
    };

    ClosestHitCollector collector;
    IgnoreBodyFilter bodyFilter(ignoreBodyId);
    JPH::ShapeCastSettings settings;
    physicsSystem.GetNarrowPhaseQuery().CastShape(
        cast, settings, JPH::RVec3::sZero(), collector, {}, {}, bodyFilter);

    endPosition = startPosition + direction;

    if (!collector.hasHit) {
        out.hitAny = false;
        return out;
    }

    out.hitAny = true;

    const float fraction = collector.best.mFraction;
    const float dist = fraction * dirLen;

    SweepHit hit;
    hit.distance = dist;
    hit.percentage = fraction;
    hit.position = Position3d(collector.best.mContactPointOn2.GetX(),
                              collector.best.mContactPointOn2.GetY(),
                              collector.best.mContactPointOn2.GetZ());

    JPH::Vec3 axis = collector.best.mPenetrationAxis;
    if (axis.LengthSq() > 0.0f) {
        axis = -axis.Normalized();
    }
    hit.normal = Normal3d(axis.GetX(), axis.GetY(), axis.GetZ());

    const JPH::BodyID bodyId = collector.best.mBodyID2;
    auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
    hit.rigidbody =
        (it != bezel_jolt::bodyIdToRigidbodyMap.end()) ? it->second : nullptr;

    out.closest = hit;
    out.hits.push_back(hit);

    endPosition = startPosition + (direction * fraction);
    return out;
}

bezel::SweepResult bezel::PhysicsWorld::sweepAll(
    const std::shared_ptr<bezel::PhysicsWorld> &world,
    const std::shared_ptr<bezel::Collider> &collider,
    const Position3d &startPosition, const Rotation3d &startRotation,
    const Position3d &direction, Position3d &endPosition,
    uint32_t ignoreBodyId) const {
    (void)world;

    SweepResult out;
    if (!collider) {
        endPosition = startPosition + direction;
        return out;
    }

    auto shape = collider->getJoltShape();
    if (shape == nullptr) {
        endPosition = startPosition + direction;
        return out;
    }

    const float dirLen =
        std::sqrt((direction.x * direction.x) + (direction.y * direction.y) +
                  (direction.z * direction.z));
    if (dirLen <= 0.0f) {
        endPosition = startPosition;
        return out;
    }

    glm::quat glmRotation = glm::normalize(startRotation.toGlmQuat());
    JPH::Quat joltRotation(glmRotation.x, glmRotation.y, glmRotation.z,
                           glmRotation.w);

    const JPH::RVec3 joltStart(startPosition.x, startPosition.y,
                               startPosition.z);
    const JPH::RMat44 startWorldTransform =
        JPH::RMat44::sRotationTranslation(joltRotation, joltStart);

    const JPH::Vec3 castDirection(static_cast<float>(direction.x),
                                  static_cast<float>(direction.y),
                                  static_cast<float>(direction.z));

    const JPH::RShapeCast cast = JPH::RShapeCast::sFromWorldTransform(
        shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), startWorldTransform,
        castDirection);

    class AllHitsCollector final : public JPH::CastShapeCollector {
      public:
        std::vector<JPH::ShapeCastResult> hits;

        void AddHit(const JPH::ShapeCastResult &result) override {
            hits.push_back(result);
            ResetEarlyOutFraction();
        }
    };

    AllHitsCollector collector;
    IgnoreBodyFilter bodyFilter(ignoreBodyId);
    JPH::ShapeCastSettings settings;
    physicsSystem.GetNarrowPhaseQuery().CastShape(
        cast, settings, JPH::RVec3::sZero(), collector, {}, {}, bodyFilter);

    endPosition = startPosition + direction;

    if (collector.hits.empty()) {
        out.hitAny = false;
        return out;
    }

    out.hitAny = true;
    out.hits.reserve(collector.hits.size());

    float bestDist = std::numeric_limits<float>::max();
    SweepHit best{};
    bool hasBest = false;

    for (const auto &h : collector.hits) {
        const float fraction = h.mFraction;
        const float dist = fraction * dirLen;

        SweepHit hit;
        hit.distance = dist;
        hit.percentage = fraction;
        hit.position =
            Position3d(h.mContactPointOn2.GetX(), h.mContactPointOn2.GetY(),
                       h.mContactPointOn2.GetZ());

        JPH::Vec3 axis = h.mPenetrationAxis;
        if (axis.LengthSq() > 0.0f) {
            axis = -axis.Normalized();
        }
        hit.normal = Normal3d(axis.GetX(), axis.GetY(), axis.GetZ());

        const JPH::BodyID bodyId = h.mBodyID2;
        auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
        hit.rigidbody = (it != bezel_jolt::bodyIdToRigidbodyMap.end())
                            ? it->second
                            : nullptr;

        out.hits.push_back(hit);

        if (dist < bestDist) {
            bestDist = dist;
            best = hit;
            hasBest = true;
        }
    }

    if (hasBest) {
        out.closest = best;
        endPosition = startPosition + (direction * best.percentage);
    }

    return out;
}
