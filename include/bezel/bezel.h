//
// bezel.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Common Bezel API definition
// Copyright (c) 2025 maxvdec
//

#ifndef BEZEL_H
#define BEZEL_H

#include "atlas/units.h"
#include "glm/ext/vector_float3.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <limits>
#ifndef BEZEL_NATIVE
#include <bezel/jolt/world.h>
#endif

#ifndef BEZEL_NATIVE
class CoreObject;

namespace JPH {
class VehicleConstraint;
class WheeledVehicleController;
class VehicleCollisionTester;
} // namespace JPH
#endif

#ifndef BEZEL_NATIVE
using namespace bezel_jolt;
#endif

class Window;

namespace bezel {

/**
 * @file bezel/bezel.h
 * @brief Public Bezel physics abstraction used by Atlas.
 *
 * Bezel provides a backend-agnostic interface for rigid bodies, colliders,
 * joint constraints, and vehicle simulation. When `BEZEL_NATIVE` is not
 * defined, this implementation is backed by Jolt Physics.
 *
 * \note This is an alpha API and may change.
 */

constexpr uint32_t INVALID_JOLT_ID = UINT32_MAX;

/**
 * @brief Maps between an underlying physics engine ID and an Atlas object ID.
 */
struct BodyIdentifier {
    /** @brief Backend body identifier (Jolt body id when using Jolt). */
    uint32_t joltId;
    /** @brief Corresponding Atlas object identifier. */
    uint32_t atlasId;
};

/**
 * @brief Motion type for a rigid body.
 */
enum class MotionType { Static, Dynamic, Kinematic };

class PhysicsWorld;

/**
 * @brief Base collider interface.
 */
class Collider {
  public:
    virtual ~Collider() = default;
    /** @brief Returns the smallest extent used for broad-phase heuristics. */
    virtual float getMinExtent() const = 0;

#ifndef BEZEL_NATIVE
    virtual JPH::RefConst<JPH::Shape> getJoltShape() const = 0;
#endif
};

/**
 * @brief Axis-aligned box collider defined by half-extents.
 */
class BoxCollider : public Collider {
  public:
    Position3d halfExtents;

    BoxCollider(const Position3d &halfExtents) : halfExtents(halfExtents) {}

    float getMinExtent() const override {
        return std::min({halfExtents.x, halfExtents.y, halfExtents.z});
    }

#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

/**
 * @brief Capsule collider defined by radius and height.
 */
class CapsuleCollider : public Collider {
  public:
    float radius;
    float height;

    float getMinExtent() const override {
        return std::min(radius * 2.0f, height);
    }

    CapsuleCollider(float radius, float height)
        : radius(radius), height(height) {}
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

/**
 * @brief Sphere collider defined by radius.
 */
class SphereCollider : public Collider {
  public:
    float radius;

    float getMinExtent() const override { return radius * 2.0f; }

    explicit SphereCollider(float radius) : radius(radius) {}
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

/**
 * @brief Triangle mesh collider defined by indexed geometry.
 *
 * \warning Mesh colliders are typically more expensive than primitives.
 */
class MeshCollider : public Collider {
  public:
    std::vector<Position3d> vertices;
    std::vector<uint32_t> indices;

    float getMinExtent() const override {
        float minExtent = std::numeric_limits<float>::max();
        for (const auto &vert : vertices) {
            minExtent = std::min({minExtent, vert.x, vert.y, vert.z});
        }
        return minExtent;
    }

    MeshCollider(const std::vector<Position3d> &vertices,
                 const std::vector<uint32_t> &indices);
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

/**
 * @brief Dispatch interface used to surface collision events to the engine.
 */
class CollisionDispatcher {
  public:
    virtual ~CollisionDispatcher() = default;

    virtual void update(bezel::PhysicsWorld *world) = 0;
    virtual void setup(bezel::PhysicsWorld *world) = 0;
};

/**
 * @brief Single hit returned by raycast queries.
 */
struct RaycastHit {
    Position3d position = {0.0f, 0.0f, 0.0f};
    Normal3d normal = {0.0f, 0.0f, 0.0f};
    float distance = 0.0;
    bezel::Rigidbody *rigidbody = nullptr;
    bool didHit = false;
};

/**
 * @brief Aggregated results for raycast queries.
 */
struct RaycastResult {
    float closestDistance = -1;
    std::vector<RaycastHit> hits;
    RaycastHit hit;
};

/**
 * @brief Single hit returned by overlap queries.
 */
struct OverlapHit {
    Position3d contactPoint = {0.0f, 0.0f, 0.0f};
    Point3d penetrationAxis = {0.0f, 0.0f, 0.0f};
    float penetrationDepth = 0.0f;
    bezel::Rigidbody *rigidbody = nullptr;
};

/**
 * @brief Aggregated results for overlap queries.
 */
struct OverlapResult {
    std::vector<OverlapHit> hits;
    bool hitAny = false;
};

/**
 * @brief Single hit returned by sweep queries.
 */
struct SweepHit {
    bezel::Rigidbody *rigidbody = nullptr;
    float distance = 0.0f;
    float percentage = 0.0f;
    Position3d position = {0.0f, 0.0f, 0.0f};
    Normal3d normal = {0.0f, 0.0f, 0.0f};
};

/**
 * @brief Aggregated results for sweep queries.
 */
struct SweepResult {
    std::vector<SweepHit> hits;
    SweepHit closest;
    bool hitAny = false;
};

/**
 * @brief Marker type representing the static world as a joint endpoint.
 */
struct WorldBody {};

/**
 * @brief Joint endpoint referencing either a `Rigidbody*` or the world.
 */
using JointChild = std::variant<Rigidbody *, WorldBody>;

enum class SpringMode { FrequencyAndDamping, StiffnessAndDamping };

/**
 * @brief Coordinate space used by joint anchors and constraints.
 */
enum class Space { Local, Global };

/**
 * @brief Spring parameters used by spring joints.
 */
struct Spring {
    bool enabled = false;

    SpringMode mode = SpringMode::FrequencyAndDamping;

    float frequencyHz = 0.0f;
    float dampingRatio = 0.0f;

    float stiffness = 0.0f;
    float damping = 0.0f;
};

/**
 * @brief Optional angular limit range (degrees or radians depending on
 * backend).
 */
struct AngleLimits {
    bool enabled = false;
    float minAngle = 0.0f;
    float maxAngle = 0.0f;
};

/**
 * @brief Motor parameters for driving joints.
 */
struct Motor {
    bool enabled = false;
    float maxForce = 0.0f;
    float maxTorque = 0.0f;
};

/**
 * @brief Base joint interface.
 */
class Joint {
  public:
    virtual ~Joint() = default;

    JointChild parent;
    JointChild child;

    JPH::Constraint *joint = nullptr;

    Space space = Space::Global;

    Position3d anchor = Position3d::invalid();

    float breakForce = 0.0f;
    float breakTorque = 0.0f;

    virtual void create(std::shared_ptr<PhysicsWorld> world) = 0;
    /** @brief Breaks the joint by disabling its underlying constraint. */
    void breakJoint() const { joint->SetEnabled(false); };
};

class FixedJoint final : public Joint {
  public:
    /** @brief Creates the fixed joint in the provided world. */
    void create(std::shared_ptr<PhysicsWorld> world) override;
};

class HingeJoint final : public Joint {
  public:
    Normal3d axis1 = Normal3d::up();
    Normal3d axis2 = Normal3d::up();

    AngleLimits limits;
    Motor motor;

    /** @brief Creates the hinge joint in the provided world. */
    void create(std::shared_ptr<PhysicsWorld> world) override;
};

class SpringJoint final : public Joint {
  public:
    Position3d anchorB = Position3d::invalid();

    float restLength = 1.0f;

    bool useLimits = false;
    float minLength = 0.0f;
    float maxLength = 0.0f;

    Spring spring;

    /** @brief Creates the spring joint in the provided world. */
    void create(std::shared_ptr<PhysicsWorld> world) override;
};

// ----------------------------
// Vehicle (Jolt VehicleConstraint)
// ----------------------------

struct VehicleWheelSettings {
    /** @brief Wheel center position in chassis local space. */
    Position3d position = {0.0f, 0.0f, 0.0f};

    bool enableSuspensionForcePoint = false;
    Position3d suspensionForcePoint = {0.0f, 0.0f, 0.0f};

    Normal3d suspensionDirection = {0.0f, -1.0f, 0.0f};
    Normal3d steeringAxis = {0.0f, 1.0f, 0.0f};
    Normal3d wheelUp = {0.0f, 1.0f, 0.0f};
    Normal3d wheelForward = {0.0f, 0.0f, 1.0f};

    float suspensionMinLength = 0.3f;
    float suspensionMaxLength = 0.5f;
    float suspensionPreloadLength = 0.0f;
    float suspensionFrequencyHz = 1.5f;
    float suspensionDampingRatio = 0.5f;

    float radius = 0.3f;
    float width = 0.1f;

    float inertia = 0.9f;
    float angularDamping = 0.2f;
    float maxSteerAngleDeg = 70.0f;
    float maxBrakeTorque = 1500.0f;
    float maxHandBrakeTorque = 4000.0f;
};

struct VehicleDifferential {
    /** @brief Index of the left wheel participating in the differential. */
    int leftWheel = -1;
    /** @brief Index of the right wheel participating in the differential. */
    int rightWheel = -1;
    /** @brief Ratio applied between engine and wheel angular speeds. */
    float differentialRatio = 3.42f;
    /** @brief Torque split between left and right wheels [0, 1]. */
    float leftRightSplit = 0.5f;
    /** @brief Limited-slip ratio controlling wheel speed difference. */
    float limitedSlipRatio = 1.4f;
    /** @brief Fraction of engine torque fed into this differential. */
    float engineTorqueRatio = 1.0f;
};

struct VehicleEngine {
    /** @brief Maximum engine torque output. */
    float maxTorque = 500.0f;
    /** @brief Minimum engine RPM when running. */
    float minRPM = 1000.0f;
    /** @brief Redline RPM. */
    float maxRPM = 6000.0f;
    /** @brief Engine rotational inertia. */
    float inertia = 0.5f;
    /** @brief Passive engine angular damping. */
    float angularDamping = 0.2f;
};

enum class VehicleTransmissionMode { Auto, Manual };

struct VehicleTransmission {
    /** @brief Whether shifting is automatic or manual. */
    VehicleTransmissionMode mode = VehicleTransmissionMode::Auto;
    /** @brief Forward gear ratios in ascending gear order. */
    std::vector<float> gearRatios = {2.66f, 1.78f, 1.3f, 1.0f, 0.74f};
    /** @brief Reverse gear ratios. */
    std::vector<float> reverseGearRatios = {-2.90f};
    /** @brief Gear switch duration in seconds. */
    float switchTime = 0.5f;
    /** @brief Clutch release duration in seconds. */
    float clutchReleaseTime = 0.3f;
    /** @brief Delay before a shift starts. */
    float switchLatency = 0.5f;
    /** @brief RPM threshold for automatic upshift. */
    float shiftUpRPM = 4000.0f;
    /** @brief RPM threshold for automatic downshift. */
    float shiftDownRPM = 2000.0f;
    /** @brief Clutch torque capacity scalar. */
    float clutchStrength = 10.0f;
};

struct VehicleControllerSettings {
    VehicleEngine engine;
    VehicleTransmission transmission;
    std::vector<VehicleDifferential> differentials;
    float differentialLimitedSlipRatio = 1.4f;
};

struct VehicleSettings {
    Normal3d up = {0.0f, 1.0f, 0.0f};
    Normal3d forward = {0.0f, 0.0f, 1.0f};

    float maxPitchRollAngleDeg = 180.0f;

    std::vector<VehicleWheelSettings> wheels;
    VehicleControllerSettings controller;

    float maxSlopeAngleDeg = 80.0f;
};

/**
 * @brief High-level vehicle wrapper around Jolt's `VehicleConstraint`.
 *
 * \subsection bezel-vehicle-example Example
 * ```cpp
 * auto world = std::make_shared<bezel::PhysicsWorld>();
 * world->init();
 *
 * auto chassis = std::make_shared<bezel::Rigidbody>();
 * chassis->mass = 1200.0f;
 * chassis->setCollider(std::make_shared<bezel::BoxCollider>(Position3d{1.0f,
 * 0.5f, 2.0f})); chassis->create(world);
 *
 * bezel::Vehicle vehicle;
 * vehicle.chassis = chassis.get();
 * vehicle.settings.wheels.resize(4);
 * vehicle.create(world);
 *
 * // Each step
 * vehicle.setDriverInput(1.0f, 0.0f, 0.0f, 0.0f);
 * world->update(dt);
 * ```
 */
class Vehicle final {
  public:
    Rigidbody *chassis = nullptr;
    VehicleSettings settings;

    Vehicle() = default;
    Vehicle(const Vehicle &other);
    Vehicle &operator=(const Vehicle &other);
    Vehicle(Vehicle &&other) noexcept;
    Vehicle &operator=(Vehicle &&other) noexcept;

    void create(const std::shared_ptr<PhysicsWorld> &world);
    void destroy(const std::shared_ptr<PhysicsWorld> &world);
    /** @brief Returns whether the backend vehicle constraint exists. */
    bool isCreated() const;

    /** @brief Updates steering, throttle, brake, and handbrake inputs. */
    void setDriverInput(float forward, float right, float brake,
                        float handBrake);

  private:
#ifndef BEZEL_NATIVE
    JPH::VehicleConstraint *constraint = nullptr;
    JPH::WheeledVehicleController *controller = nullptr;
    JPH::RefConst<JPH::VehicleCollisionTester> collisionTester = nullptr;
#endif
};

/**
 * @brief Backend rigid body representation used by Atlas components.
 */
struct Rigidbody {
    /** @brief World-space body position. */
    Position3d position;
    /** @brief Euler rotation representation in world space. */
    Rotation3d rotation;
    /** @brief Quaternion rotation representation in world space. */
    glm::quat rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool isSensor = false;
    std::string sensorSignal;

    float mass = 0.0f;
    float friction = 0.5f;
    float restitution = 0.0f;

    std::vector<std::string> tags;

    Position3d linearVelocity = {-1.0f, -1.0f, -1.0f};
    Position3d angularVelocity = {-1.0f, -1.0f, -1.0f};
    float maxLinearVelocity = -1.0f;
    float maxAngularVelocity = -1.0f;
    Position3d impulse = {0.0f, 0.0f, 0.0f};
    Position3d force = {0.0f, 0.0f, 0.0f};
    Position3d forcePoint = {0.0f, 0.0f, 0.0f};

    float linearDamping = 0.05f;
    float angularDamping = 0.1f;

    bool addLinearVelocity = false;
    bool addAngularVelocity = false;

    /** @brief Sets the body's world position and updates the backend body. */
    void setPosition(const Position3d &position,
                     const std::shared_ptr<PhysicsWorld> &world);
    /** @brief Sets the body's world rotation and updates the backend body. */
    void setRotation(const Rotation3d &rotation,
                     const std::shared_ptr<PhysicsWorld> &world);

    RaycastResult raycast(const Position3d &direction, float maxDistance,
                          const std::shared_ptr<PhysicsWorld> &world,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID) const;
    RaycastResult raycastAll(const Position3d &direction, float maxDistance,
                             const std::shared_ptr<PhysicsWorld> &world,
                             uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    OverlapResult overlap(const std::shared_ptr<PhysicsWorld> &world,
                          std::shared_ptr<Collider> collider,
                          const Position3d &position,
                          const Rotation3d &rotation,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    SweepResult sweep(const std::shared_ptr<PhysicsWorld> &world,
                      std::shared_ptr<Collider> collider,
                      const Position3d &direction, Position3d &endPosition,
                      uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    SweepResult sweepAll(const std::shared_ptr<PhysicsWorld> &world,
                         std::shared_ptr<Collider> collider,
                         const Position3d &direction, Position3d &endPosition,
                         uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    std::shared_ptr<Collider> collider;

    BodyIdentifier id = {.joltId = INVALID_JOLT_ID, .atlasId = 0};
    MotionType motionType = MotionType::Dynamic;

    Velocity3d
    getAngularVelocity(const std::shared_ptr<PhysicsWorld> &world) const;
    Velocity3d
    getLinearVelocity(const std::shared_ptr<PhysicsWorld> &world) const;
    Velocity3d getVelocity(const std::shared_ptr<PhysicsWorld> &world) const;

    void setMaximumAngularVelocity(float maxAngularVelocity);
    void setMaximumLinearVelocity(float maxLinearVelocity);

    /** @brief Creates and registers the backend body in the world. */
    void create(const std::shared_ptr<PhysicsWorld> &world);
    /** @brief Replaces the collider used by this rigidbody. */
    void setCollider(std::shared_ptr<Collider> collider);

    void applyProperties(const std::shared_ptr<PhysicsWorld> &world);

    /** @brief Recreates or updates backend body state after property changes.
     */
    void refresh(const std::shared_ptr<PhysicsWorld> &world);
    /** @brief Removes and destroys the backend body from the world. */
    void destroy(const std::shared_ptr<PhysicsWorld> &world);
};

/**
 * @brief Physics world owning the backend simulation state.
 */
class PhysicsWorld {
#ifndef BEZEL_NATIVE
    std::unique_ptr<JPH::TempAllocatorMalloc> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;

    BroadPhaseLayerImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;
    std::shared_ptr<CollisionDispatcher> collisionDispatcher;

    std::vector<BodyIdentifier> bodies;

#endif
  public:
#ifndef BEZEL_NATIVE

    JPH::PhysicsSystem physicsSystem;
    struct BreakableConstraint {
        JPH::Ref<JPH::Constraint> constraint;
        float breakForce = 0.0f;
        float breakTorque = 0.0f;
    };

    std::vector<BreakableConstraint> breakableConstraints;
#endif
    bool initialized = false;

    /** @brief Initializes the backend physics system. */
    void init();

    /** @brief Advances the physics simulation by `dt` seconds. */
    void update(float dt);

    RaycastResult raycast(const Position3d &origin, const Position3d &direction,
                          float maxDistance,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID) const;
    RaycastResult raycastAll(const Position3d &origin,
                             const Position3d &direction, float maxDistance,
                             uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    OverlapResult overlap(const std::shared_ptr<PhysicsWorld> &world,
                          const std::shared_ptr<Collider> &collider,
                          const Position3d &position,
                          const Rotation3d &rotation,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    SweepResult sweep(const std::shared_ptr<PhysicsWorld> &world,
                      const std::shared_ptr<Collider> &collider,
                      const Position3d &startPosition,
                      const Rotation3d &startRotation,
                      const Position3d &direction, Position3d &endPosition,
                      uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    SweepResult sweepAll(const std::shared_ptr<PhysicsWorld> &world,
                         const std::shared_ptr<Collider> &collider,
                         const Position3d &startPosition,
                         const Rotation3d &startRotation,
                         const Position3d &direction, Position3d &endPosition,
                         uint32_t ignoreBodyId = INVALID_JOLT_ID) const;

    /** @brief Adds a rigidbody to the simulation. */
    void addBody(const std::shared_ptr<bezel::Rigidbody> &body);

    /** @brief Sets the global gravity vector. */
    void setGravity(const Position3d &gravity);

    ~PhysicsWorld();
};

struct SoftbodyMaterial {
    float stiffness = 0.8f;
    float volumeStiffness = 1.0f;

    float damping = 0.1f;

    float friction = 0.2f;
    float restitution = 0.0f;
};

struct SoftbodyVertex {
    Position3d position;
    float inverseMass = 1.0f;
};

struct SoftbodyTetrahedron {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};

struct SoftbodyTriangle {
    uint32_t a;
    uint32_t b;
    uint32_t c;
};

struct SoftbodyMesh {
    std::vector<SoftbodyVertex> vertices;
    std::vector<SoftbodyTetrahedron> tetrahedra;
    std::vector<SoftbodyTriangle> surface;
};

struct SoftbodyRenderBinding {
    uint32_t a;
    uint32_t b;
    uint32_t c;

    glm::vec3 weights;
};

struct Softbody {
    Position3d position;
    Rotation3d rotation;
    glm::quat rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool isSensor = false;
    std::string sensorSignal;

    std::vector<std::string> tags;

    float mass = 1.0f;

    SoftbodyMaterial material;

    uint32_t solverIterations = 8;
    float gravityFactor = 1.0f;
    bool allowSleeping = true;

    Position3d force = {0.0f, 0.0f, 0.0f};
    Position3d forcePoint = {0.0f, 0.0f, 0.0f};
    Position3d impulse = {0.0f, 0.0f, 0.0f};

    BodyIdentifier id = {
        .joltId = INVALID_JOLT_ID,
        .atlasId = 0,
    };

    Velocity3d
    getLinearVelocity(const std::shared_ptr<PhysicsWorld> &world) const;
    Velocity3d getVelocity(const std::shared_ptr<PhysicsWorld> &world) const;

    void create(const std::shared_ptr<PhysicsWorld> &world);

    void destroy(const std::shared_ptr<PhysicsWorld> &world);

    void createMesh();
    void setObject(CoreObject *object);

    void updateVertices(const std::shared_ptr<PhysicsWorld> &world);

    CoreObject *object = nullptr;

    bool isMeshCreated = false;

  private:
    SoftbodyMesh mesh;

    std::vector<SoftbodyRenderBinding> renderBindings;
    void createRenderBindings();
};

enum class ClothBendType {
    None,
    Distance,
    Dihedral,
};

enum class AnchorPoint {
    TopLeft,
    TopCenter,
    TopRight,

    CenterLeft,
    Center,
    CenterRight,

    BottomLeft,
    BottomCenter,
    BottomRight,
};

struct ClothMaterial {
    float stretchCompliance = 0.0f;
    float shearCompliance = 0.0f;
    float bendCompliance = 0.0f;

    float damping = 0.05f;

    float friction = 0.2f;
    float restitution = 0.0f;
};

struct ClothMesh {
    std::vector<SoftbodyVertex> vertices;
    std::vector<SoftbodyTriangle> surface;
};

class Cloth {
  public:
    Position3d position;
    Rotation3d rotation;
    glm::quat rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    float mass = 1.0f;

    ClothMaterial material;

    uint32_t solverIterations = 8;
    float gravityFactor = 1.0f;

    bool allowSleeping = true;
    bool doubleSided = true;

    float vertexRadius = 0.0f;

    ClothBendType bendType = ClothBendType::Dihedral;

    std::vector<AnchorPoint> anchors;

    BodyIdentifier id = {
        .joltId = INVALID_JOLT_ID,
        .atlasId = 0,
    };

    Cloth() = default;

    void setObject(CoreObject *object);

    void setAnchors(const std::vector<AnchorPoint> &anchors);

    void addAnchor(AnchorPoint anchor);
    void removeAnchor(AnchorPoint anchor);
    void clearAnchors();

    void pinVertex(uint32_t vertexIndex);
    void unpinVertex(uint32_t vertexIndex);
    void clearPinnedVertices();

    void createMesh();

    void create(const std::shared_ptr<PhysicsWorld> &world);
    void destroy(const std::shared_ptr<PhysicsWorld> &world);
    void updateVertices(const std::shared_ptr<PhysicsWorld> &world) const;

    bool isCreated() const;

    CoreObject *object = nullptr;
    bool isMeshCreated = false;

  private:
    ClothMesh mesh;

    std::vector<uint32_t> manuallyPinnedVertices;
    std::vector<uint32_t> resolvedPinnedVertices;

    void resolveAnchors();
};

} // namespace bezel

#endif
