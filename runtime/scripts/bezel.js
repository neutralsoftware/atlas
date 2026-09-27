import { Component } from "atlas";
import { Position3d } from "atlas/units";

export const QueryOperation = Object.freeze({
    RaycastAll: 0,
    Raycast: 1,
    RasycastWorld: 2,
    RaycastWorld: 2,
    RaycastWorldAll: 3,
    RaycastTagged: 4,
    RaycastTaggedAll: 5,
    Movement: 6,
    Overlap: 7,
    MovementAll: 8,
});

export const SpringMode = Object.freeze({
    FrequencyAndDamping: 0,
    StiffnessAndDamping: 1,
});

export const Space = Object.freeze({
    Local: 0,
    Global: 1,
});

export const VehicleTransmissionMode = Object.freeze({
    Auto: 0,
    Manual: 1,
});

function emptyRaycastResult() {
    return {
        hits: [],
        hit: null,
        closestDistance: -1,
    };
}

function emptyOverlapResult() {
    return {
        hits: [],
        hitAny: false,
    };
}

function emptySweepResult(endPosition = Position3d.zero()) {
    return {
        hits: [],
        closest: null,
        hitAny: false,
        endPosition,
    };
}

function defaultWheelSettings() {
    return {
        position: Position3d.zero(),
        enableSuspensionForcePoint: false,
        suspensionForcePoint: Position3d.zero(),
        suspensionDirection: Position3d.down(),
        steeringAxis: Position3d.up(),
        wheelUp: Position3d.up(),
        wheelForward: Position3d.forward(),
        suspensionMinLength: 0.3,
        suspensionMaxLength: 0.5,
        suspensionPreloadLength: 0,
        suspensionFrequencyHz: 1.5,
        suspensionDampingRatio: 0.5,
        radius: 0.3,
        width: 0.1,
        inertia: 0.9,
        angularDamping: 0.2,
        maxSteerAngleDegrees: 70,
        maxBrakeTorque: 1500,
        maxHandBrakeTorque: 4000,
    };
}

function defaultVehicleSettings() {
    return {
        up: Position3d.up(),
        forward: Position3d.forward(),
        maxPitchRollAngleDeg: 180,
        wheels: [defaultWheelSettings(), defaultWheelSettings()],
        controller: {
            engine: {
                maxTorque: 500,
                minRPM: 1000,
                maxRPM: 6000,
                inertia: 0.5,
                angularDamping: 0.2,
            },
            transmission: {
                mode: VehicleTransmissionMode.Auto,
                gearRatios: [2.66, 1.78, 1.3, 1.0, 0.74],
                reverseGearRatios: [-2.9],
                switchTime: 0.5,
                clutchReleaseTime: 0.3,
                switchLatency: 0.5,
                shiftUpRPM: 4000,
                shiftDownRPM: 2000,
                clutchStrength: 10,
            },
            differentials: [],
            differentialLimitedSlipRatio: 1.4,
        },
        maxSlopAngleDeg: 80,
    };
}

export class Joint extends Component {
    constructor() {
        super();
        this.parent = {};
        this.child = {};
        this.space = Space.Global;
        this.anchor = Position3d.invalid();
        this.breakForce = 0;
        this.breakTorque = 0;
    }

    init() {}
    update(deltaTime) {}
    beforePhysics() {}
    breakJoint() {}
}

export class FixedJoint extends Joint {
    constructor() {
        super();
        globalThis.__atlasCreateFixedJoint(this);
    }

    beforePhysics() {
        return globalThis.__atlasFixedJointBeforePhysics(this);
    }

    breakJoint() {
        return globalThis.__atlasFixedJointBreak(this);
    }
}

export class HingeJoint extends Joint {
    constructor() {
        super();
        this.axis1 = Position3d.up();
        this.axis2 = Position3d.up();
        this.angleLimits = {
            enabled: false,
            minAngle: 0,
            maxAngle: 0,
        };
        this.motor = {
            enabled: false,
            maxForce: 0,
            maxTorque: 0,
        };
        globalThis.__atlasCreateHingeJoint(this);
    }

    beforePhysics() {
        return globalThis.__atlasHingeJointBeforePhysics(this);
    }

    breakJoint() {
        return globalThis.__atlasHingeJointBreak(this);
    }
}

export class SpringJoint extends Joint {
    constructor() {
        super();
        this.anchorB = Position3d.invalid();
        this.restLength = 1;
        this.useLimits = false;
        this.minLength = 0;
        this.maxLength = 0;
        this.spring = {
            enabled: false,
            mode: SpringMode.FrequencyAndDamping,
            frequency: 0,
            dampingRatio: 0,
            stiffness: 0,
            damping: 0,
        };
        globalThis.__atlasCreateSpringJoint(this);
    }

    beforePhysics() {
        return globalThis.__atlasSpringJointBeforePhysics(this);
    }

    breakJoint() {
        return globalThis.__atlasSpringJointBreak(this);
    }
}

export class Vehicle extends Component {
    constructor() {
        super();
        this.settings = defaultVehicleSettings();
        this.forward = 0;
        this.right = 0;
        this.brake = 0;
        this.handBrake = 0;
        globalThis.__atlasCreateVehicle(this);
    }

    atAttach() {}

    beforePhysics() {
        return globalThis.__atlasVehicleBeforePhysics(this);
    }

    requestRecreate() {
        return globalThis.__atlasVehicleRequestRecreate(this);
    }

    init() {}
    update(deltaTime) {}
}

export class Rigidbody extends Component {
    constructor() {
        super();
        this.sendSignal = "";
        this.isSensor = false;
        globalThis.__atlasCreateRigidbody(this);
    }

    init() {
        return globalThis.__atlasInitRigidbody(this);
    }

    beforePhysics() {
        return globalThis.__atlasBeforePhysicsRigidbody(this);
    }

    update(deltaTime) {
        return globalThis.__atlasUpdateRigidbody(this, deltaTime);
    }

    clone() {
        return globalThis.__atlasCloneRigidbody(this);
    }

    addCollider(collider) {
        return globalThis.__atlasRigidbodyAddCollider(this, collider);
    }

    setFriction(friction) {
        return globalThis.__atlasRigidbodySetFriction(this, friction);
    }

    applyForce(force) {
        return globalThis.__atlasRigidbodyApplyForce(this, force);
    }

    applyForceAtPoint(force, point) {
        return globalThis.__atlasRigidbodyApplyForceAtPoint(this, force, point);
    }

    applyImpulse(impulse) {
        return globalThis.__atlasRigidbodyApplyImpulse(this, impulse);
    }

    setLinearVelocity(velocity) {
        return globalThis.__atlasRigidbodySetLinearVelocity(this, velocity);
    }

    addLinearVelocity(velocity) {
        return globalThis.__atlasRigidbodyAddLinearVelocity(this, velocity);
    }

    setAngularVelocity(velocity) {
        return globalThis.__atlasRigidbodySetAngularVelocity(this, velocity);
    }

    addAngularVelocity(velocity) {
        return globalThis.__atlasRigidbodyAddAngularVelocity(this, velocity);
    }

    setMaxLinearVelocity(velocity) {
        return globalThis.__atlasRigidbodySetMaxLinearVelocity(this, velocity);
    }

    setMaxAngularVelocity(velocity) {
        return globalThis.__atlasRigidbodySetMaxAngularVelocity(this, velocity);
    }

    getLinearVelocity() {
        return globalThis.__atlasRigidbodyGetLinearVelocity(this);
    }

    getAngularVelocity() {
        return globalThis.__atlasRigidbodyGetAngularVelocity(this);
    }

    getVelocity() {
        return globalThis.__atlasRigidbodyGetVelocity(this);
    }

    raycast(direction, maxDistance) {
        return (
            globalThis.__atlasRigidbodyRaycast(this, direction, maxDistance)
                ?.raycastResult ?? emptyRaycastResult()
        );
    }

    raycastAll(direction, maxDistance) {
        return (
            globalThis.__atlasRigidbodyRaycastAll(this, direction, maxDistance)
                ?.raycastResult ?? emptyRaycastResult()
        );
    }

    raycastWorld(origin, direction, maxDistance) {
        return (
            globalThis.__atlasRigidbodyRaycastWorld(
                this,
                origin,
                direction,
                maxDistance,
            )?.raycastResult ?? emptyRaycastResult()
        );
    }

    raycastWorldAll(origin, direction, maxDistance) {
        return (
            globalThis.__atlasRigidbodyRaycastWorldAll(
                this,
                origin,
                direction,
                maxDistance,
            )?.raycastResult ?? emptyRaycastResult()
        );
    }

    raycastTagged(tags, direction, maxDistance) {
        return (
            globalThis.__atlasRigidbodyRaycastTagged(
                this,
                tags,
                direction,
                maxDistance,
            )?.raycastResult ?? emptyRaycastResult()
        );
    }

    raycastTaggedAll(tags, direction, maxDistance) {
        return (
            globalThis.__atlasRigidbodyRaycastTaggedAll(
                this,
                tags,
                direction,
                maxDistance,
            )?.raycastResult ?? emptyRaycastResult()
        );
    }

    overlap() {
        return (
            globalThis.__atlasRigidbodyOverlap(this)?.overlapResult ??
            emptyOverlapResult()
        );
    }

    overlapWithCollider(collider) {
        return (
            globalThis.__atlasRigidbodyOverlapWithCollider(this, collider)
                ?.overlapResult ?? emptyOverlapResult()
        );
    }

    overlapWithColliderWorld(collider, position) {
        return (
            globalThis.__atlasRigidbodyOverlapWithColliderWorld(
                this,
                collider,
                position,
            )?.overlapResult ?? emptyOverlapResult()
        );
    }

    predictMovementWithCollider(endPosition, collider) {
        return (
            globalThis.__atlasRigidbodyPredictMovementWithCollider(
                this,
                endPosition,
                collider,
            )?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovement(endPosition) {
        return (
            globalThis.__atlasRigidbodyPredictMovement(this, endPosition)
                ?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovementWithColliderWorld(startPosition, endPosition, collider) {
        return (
            globalThis.__atlasRigidbodyPredictMovementWithColliderWorld(
                this,
                startPosition,
                endPosition,
                collider,
            )?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovementWorld(startPosition, endPosition) {
        return (
            globalThis.__atlasRigidbodyPredictMovementWorld(
                this,
                startPosition,
                endPosition,
            )?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovementWithColliderAll(endPosition, collider) {
        return (
            globalThis.__atlasRigidbodyPredictMovementWithColliderAll(
                this,
                endPosition,
                collider,
            )?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovementAll(endPosition) {
        return (
            globalThis.__atlasRigidbodyPredictMovementAll(this, endPosition)
                ?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovementWithColliderAllWorld(startPosition, endPosition, collider) {
        return (
            globalThis.__atlasRigidbodyPredictMovementWithColliderAllWorld(
                this,
                startPosition,
                endPosition,
                collider,
            )?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    predictMovementAllWorld(startPosition, endPosition) {
        return (
            globalThis.__atlasRigidbodyPredictMovementAllWorld(
                this,
                startPosition,
                endPosition,
            )?.sweepResult ?? emptySweepResult(endPosition)
        );
    }

    hasTag(tag) {
        return globalThis.__atlasRigidbodyHasTag(this, tag);
    }

    addTag(tag) {
        return globalThis.__atlasRigidbodyAddTag(this, tag);
    }

    removeTag(tag) {
        return globalThis.__atlasRigidbodyRemoveTag(this, tag);
    }

    setDamping(linearDamping, angularDamping) {
        return globalThis.__atlasRigidbodySetDamping(
            this,
            linearDamping,
            angularDamping,
        );
    }

    setMass(mass) {
        return globalThis.__atlasRigidbodySetMass(this, mass);
    }

    setRestituition(restitution) {
        return globalThis.__atlasRigidbodySetRestitution(this, restitution);
    }

    setRestitution(restitution) {
        return this.setRestituition(restitution);
    }

    setMotionType(motionType) {
        return globalThis.__atlasRigidbodySetMotionType(this, motionType);
    }
}

export class Softbody extends Component {
    constructor() {
        super();
        this.sendSignal = "";
        this.isSensor = false;
        this.mass = 1;
        this.stiffness = 0.8;
        this.volumeStiffness = 1;
        this.damping = 0.1;
        this.friction = 0.2;
        this.restitution = 0;
        this.gravityFactor = 1;
        this.solverIterations = 8;
        this.allowSleeping = true;
        globalThis.__atlasCreateSoftbody(this);
    }

    init() {
        return globalThis.__atlasInitSoftbody(this);
    }

    beforePhysics() {
        return globalThis.__atlasBeforePhysicsSoftbody(this);
    }

    update(deltaTime) {
        return globalThis.__atlasUpdateSoftbody(this, deltaTime);
    }

    clone() {
        return globalThis.__atlasCloneSoftbody(this);
    }

    setMass(mass) {
        this.mass = mass;
        return globalThis.__atlasSoftbodySetMass(this, mass);
    }

    setStiffness(stiffness) {
        this.stiffness = stiffness;
        return globalThis.__atlasSoftbodySetStiffness(this, stiffness);
    }

    setVolumeStiffness(volumeStiffness) {
        this.volumeStiffness = volumeStiffness;
        return globalThis.__atlasSoftbodySetVolumeStiffness(
            this,
            volumeStiffness,
        );
    }

    setDamping(damping) {
        this.damping = damping;
        return globalThis.__atlasSoftbodySetDamping(this, damping);
    }

    setFriction(friction) {
        this.friction = friction;
        return globalThis.__atlasSoftbodySetFriction(this, friction);
    }

    setRestitution(restitution) {
        this.restitution = restitution;
        return globalThis.__atlasSoftbodySetRestitution(this, restitution);
    }

    setGravityFactor(gravityFactor) {
        this.gravityFactor = gravityFactor;
        return globalThis.__atlasSoftbodySetGravityFactor(this, gravityFactor);
    }

    setSolverIterations(solverIterations) {
        this.solverIterations = solverIterations;
        return globalThis.__atlasSoftbodySetSolverIterations(
            this,
            solverIterations,
        );
    }

    setAllowSleeping(allowSleeping) {
        this.allowSleeping = allowSleeping;
        return globalThis.__atlasSoftbodySetAllowSleeping(this, allowSleeping);
    }

    getLinearVelocity() {
        return globalThis.__atlasSoftbodyGetLinearVelocity(this);
    }

    getVelocity() {
        return globalThis.__atlasSoftbodyGetVelocity(this);
    }
}

export class Sensor extends Rigidbody {
    constructor() {
        super();
        this.isSensor = true;
    }

    setSignal(signal) {
        this.sendSignal = signal;
        return globalThis.__atlasSensorSetSignal(this, signal);
    }
}
