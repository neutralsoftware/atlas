import { AxisTrigger, InputAction, Key, MouseButton, Trigger } from "atlas/input";
import { Color, Position2d, Position3d } from "atlas/units";

const windowConstants = globalThis.__atlasGetWindowConstants?.() ?? {};

export const ControllerAxis = Object.freeze(windowConstants.ControllerAxis ?? {});
export const ControllerButton = Object.freeze(
    windowConstants.ControllerButton ?? {},
);
export const NintendoControllerButton = Object.freeze(
    windowConstants.NintendoControllerButton ?? {},
);
export const SonyControllerButton = Object.freeze(
    windowConstants.SonyControllerButton ?? {},
);
export const CONTROLLER_UNDEFINED =
    windowConstants.CONTROLLER_UNDEFINED ?? -2;

function makeControllerAxisTrigger(controllerId, axis) {
    switch (axis) {
        case ControllerAxis.LeftStick:
            return AxisTrigger.fromControllerAxis(controllerId, 0, false, 1);
        case ControllerAxis.LeftStickX:
            return AxisTrigger.fromControllerAxis(controllerId, 0, true);
        case ControllerAxis.LeftStickY:
            return AxisTrigger.fromControllerAxis(controllerId, 1, true);
        case ControllerAxis.RightStick:
            return AxisTrigger.fromControllerAxis(controllerId, 2, false, 3);
        case ControllerAxis.RightStickX:
            return AxisTrigger.fromControllerAxis(controllerId, 2, true);
        case ControllerAxis.RightStickY:
            return AxisTrigger.fromControllerAxis(controllerId, 3, true);
        case ControllerAxis.Trigger:
            return AxisTrigger.fromControllerAxis(controllerId, 4, false, 5);
        case ControllerAxis.TriggerLeft:
        case ControllerAxis.LeftTrigger:
            return AxisTrigger.fromControllerAxis(controllerId, 4, true);
        case ControllerAxis.TriggerRight:
        case ControllerAxis.RightTrigger:
            return AxisTrigger.fromControllerAxis(controllerId, 5, true);
        default:
            return new AxisTrigger();
    }
}

export class Scene {
    constructor() {
        this.name = "";
        this.atmosphere = null;
        return globalThis.__atlasGetScene() ?? this;
    }

    setAmbientIntensity(intensity) {
        return globalThis.__atlasSetSceneAmbientIntensity(this, intensity);
    }

    setAutomaticAmbient(enabled) {
        return globalThis.__atlasSetSceneAutomaticAmbient(this, enabled);
    }

    setSkybox(skybox) {
        return globalThis.__atlasSetSceneSkybox(this, skybox);
    }

    useAtmosphereSkybox(enabled) {
        return globalThis.__atlasUseAtmosphereSkybox(this, enabled);
    }

    setEnvironment(environment) {
        return globalThis.__atlasSetSceneEnvironment(this, environment);
    }

    setAmbientColor(color) {
        return globalThis.__atlasSetSceneAmbientColor(this, color);
    }

    addDirectionalLight(light) {
        return globalThis.__atlasSceneAddDirectionalLight(this, light);
    }

    addLight(light) {
        return globalThis.__atlasSceneAddLight(this, light);
    }

    addSpotLight(light) {
        return globalThis.__atlasSceneAddSpotLight(this, light);
    }

    addAreaLight(light) {
        return globalThis.__atlasSceneAddAreaLight(this, light);
    }

    getCamera() {
        return globalThis.__atlasGetCamera();
    }

    getWindow() {
        return globalThis.__atlasGetWindow();
    }
}

export class Component {
    init() {}
    beforePhysics() {}
    atAttach() {}
    update(deltaTime) {}
    onCollisionEnter(other) {}
    onCollisionStay(other) {}
    onCollisionExit(other) {}
    onSignalRecieve(signal, sender) {}
    onSignalEnd(signal, sender) {}
    onQueryRecieve(query, sender) {}
    onQueryReceive(query, sender) {}

    getParent(type) {
        const parent = globalThis.__atlasGetObjectById(this.parentId);
        if (type == null) {
            return parent;
        }
        return parent?.getComponent(type) ?? null;
    }

    getObject(identifier) {
        if (typeof identifier === "number") {
            return globalThis.__atlasGetObjectById(identifier);
        }
        if (typeof identifier === "string") {
            return globalThis.__atlasGetObjectByName(identifier);
        }
        return null;
    }

    getScene() {
        return globalThis.__atlasGetScene();
    }

    getWindow() {
        return globalThis.__atlasGetWindow();
    }

    getCamera() {
        return this.getScene()?.getCamera() ?? null;
    }
}

export class Material {
    constructor() {
        this.albedo = Color.white();
        this.metallic = 0;
        this.roughness = 0.5;
        this.ao = 1;
        this.reflectivity = 0;
        this.emissiveColor = Color.black();
        this.emissiveIntensity = 0;
        this.normalMapStrength = 1;
        this.useNormalMap = true;
        this.transmittance = 0;
        this.ior = 1;
        this.abbeNumber = 0;
        this.attenuationColor = Color.white();
        this.attenuationDistance = 1;
        this.isVolume = false;
        this.volumeDensity = 1;
        this.volumeAbsorptionColor = Color.white();
        this.volumeAbsorptionStrength = 0;
        this.volumeScatteringColor = Color.white();
        this.volumeScatteringStrength = 0;
        this.volumeAnisotropy = 0;
        this.volumeEmissionColor = Color.black();
        this.volumeEmissionStrength = 0;
    }
}

export class CoreVertex {
    constructor(
        position = Position3d.zero(),
        color = Color.white(),
        textureCoord = Position2d.zero(),
        normal = Position3d.zero(),
        tangent = Position3d.zero(),
        bitangent = Position3d.zero(),
    ) {
        this.position = position;
        this.color = color;
        this.textureCoord = textureCoord;
        this.normal = normal;
        this.tangent = tangent;
        this.bitangent = bitangent;
    }
}

export class Instance {
    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasCommitInstance(this);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasCommitInstance(this);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasCommitInstance(this);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        globalThis.__atlasCommitInstance(this);
    }

    setScale(scale) {
        this.scale = scale;
        globalThis.__atlasCommitInstance(this);
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        globalThis.__atlasCommitInstance(this);
    }

    equals(other) {
        return (
            this.position.is(other.position) &&
            this.rotation.is(other.rotation) &&
            this.scale.is(other.scale)
        );
    }
}

export class GameObject {
    constructor() {
        this.id = -1;
        this.components = [];
        this.position = Position3d.zero();
        this.rotation = Position3d.zero();
        this.scale = new Position3d(1, 1, 1);
        this.name = "";
    }

    attachTexture(texture) {
        return globalThis.__atlasAttachTexture(this, texture);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasUpdateObject(this);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasUpdateObject(this);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasUpdateObject(this);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtObject(this, target, up);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        globalThis.__atlasUpdateObject(this);
    }

    setScale(scale) {
        this.scale = scale;
        globalThis.__atlasUpdateObject(this);
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        globalThis.__atlasUpdateObject(this);
    }

    show() {
        globalThis.__atlasShowObject(this.id);
    }

    hide() {
        globalThis.__atlasHideObject(this.id);
    }

    as(type) {
        if (type == null) {
            return null;
        }
        const object = globalThis.__atlasGetObjectById(this.id) ?? this;
        return object instanceof type ? object : null;
    }

    addComponent(component) {
        return globalThis.__atlasAddComponent(this.id, component);
    }
}

export class UIObject extends GameObject {
    getSize() {
        return globalThis.__graphiteGetUISize(this);
    }

    getScreenPosition() {
        return globalThis.__graphiteGetUIScreenPosition(this);
    }

    setScreenPosition(position) {
        this.position = new Position3d(position.x, position.y, 0);
        return globalThis.__graphiteSetUIScreenPosition(this, position);
    }

    setPosition(position) {
        this.position = position;
        return globalThis.__graphiteSetUIScreenPosition(
            this,
            new Position2d(position.x, position.y),
        );
    }

    move(position) {
        const screenPosition = this.getScreenPosition();
        return this.setScreenPosition(
            new Position2d(
                screenPosition.x + position.x,
                screenPosition.y + position.y,
            ),
        );
    }
}

export class CoreObject extends GameObject {
    constructor() {
        super();
        this.vertices = [];
        this.indices = [];
        this.textures = [];
        this.material = new Material();
        this.instances = [];
        this.castsShadows = true;
        globalThis.__atlasCreateCoreObject(this);
    }

    makeEmissive(color, intensity) {
        globalThis.__atlasMakeEmissive(this.id, color, intensity);
    }

    attachVertices(vertices) {
        this.vertices = vertices;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    attachIndices(indices) {
        this.indices = indices;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setPosition(position) {
        this.position = position;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setRotationQuaternion(rotation) {
        return globalThis.__atlasSetRotationQuaternion(this.id, rotation);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtObject(this, target, up);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    setScale(scale) {
        this.scale = scale;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        globalThis.__atlasUpdateCoreObject(this, this.id);
    }

    clone() {
        return globalThis.__atlasCloneCoreObject(this);
    }

    enableDeferredRendering() {
        globalThis.__atlasEnableDeferredRendering(this.id);
    }

    disableDeferredRendering() {
        globalThis.__atlasDisableDeferredRendering(this.id);
    }

    createInstance() {
        return globalThis.__atlasCreateInstance(this.id);
    }

    getComponent(type) {
        if (type == null) {
            return null;
        }
        return globalThis.__atlasGetComponentByName(this.id, type.name);
    }

    static box(size) {
        return globalThis.__atlasCreateBox(size);
    }

    static plane(size) {
        return globalThis.__atlasCreatePlane(size);
    }

    static pyramid(size) {
        return globalThis.__atlasCreatePyramid(size);
    }

    static sphere(radius, sectorCount = 36, stackCount = 18) {
        return globalThis.__atlasCreateSphere(radius, sectorCount, stackCount);
    }
}

export class Model extends GameObject {
    static fromResource(path) {
        return globalThis.__atlasCreateModel(path);
    }

    getObjects() {
        return globalThis.__atlasGetModelObjects(this);
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        return globalThis.__atlasMoveModel(this, position);
    }

    setPosition(position) {
        this.position = position;
        return globalThis.__atlasSetModelPosition(this, position);
    }

    setRotation(rotation) {
        this.rotation = rotation;
        return globalThis.__atlasSetModelRotation(this, rotation);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtModel(this, target, up);
    }

    rotate(rotation) {
        this.rotation.x += rotation.x;
        this.rotation.y += rotation.y;
        this.rotation.z += rotation.z;
        return globalThis.__atlasRotateModel(this, rotation);
    }

    setScale(scale) {
        this.scale = scale;
        return globalThis.__atlasSetModelScale(this, scale);
    }

    scaleBy(scale) {
        this.scale.x *= scale.x;
        this.scale.y *= scale.y;
        this.scale.z *= scale.z;
        return globalThis.__atlasScaleModelBy(this, scale);
    }

    show() {
        return globalThis.__atlasShowModel(this.id);
    }

    hide() {
        return globalThis.__atlasHideModel(this.id);
    }

    attachTexture(texture) {
        return globalThis.__atlasAttachTexture(this, texture);
    }
}

export const ResourceType = Object.freeze({
    File: 0,
    Texture: 1,
    SpecularMap: 2,
    Audio: 3,
    Font: 4,
    Model: 5,
});

export class Resource {
    constructor(type, path, name) {
        this.type = type;
        this.path = path;
        this.name = name;
    }

    static fromAssetPath(path, type, name) {
        return globalThis.__atlasLoadResource(path, type, name);
    }

    static fromName(name, type) {
        return globalThis.__atlasGetResourceByName(name, type);
    }
}

export class ResourceGroup {
    constructor(resources, name) {
        this.resources = resources;
        this.name = name;
    }

    addResource(resource) {
        this.resources.push(resource);
    }

    getResourceByName(name) {
        return this.resources.find((r) => r.name === name) || null;
    }
}

export class Monitor {
    constructor() {
        this.monitorId = -1;
        this.primary = false;
    }

    queryVideoModes() {
        return globalThis.__atlasMonitorQueryVideoModes(this);
    }

    getCurrentVideoMode() {
        return globalThis.__atlasMonitorGetCurrentVideoMode(this);
    }

    getPhysicalSize() {
        return globalThis.__atlasMonitorGetPhysicalSize(this);
    }

    getPosition() {
        return globalThis.__atlasMonitorGetPosition(this);
    }

    getContentScale() {
        return globalThis.__atlasMonitorGetContentScale(this);
    }

    getName() {
        return globalThis.__atlasMonitorGetName(this);
    }
}

export class Gamepad {
    constructor() {
        this.controllerId = CONTROLLER_UNDEFINED;
        this.name = "";
        this.connected = false;
    }

    getAxisTrigger(axis) {
        return makeControllerAxisTrigger(this.controllerId, axis);
    }

    static getGlobalAxisTrigger(axis) {
        return makeControllerAxisTrigger(CONTROLLER_UNDEFINED, axis);
    }

    getButtonTrigger(button) {
        return Trigger.fromControllerButton(this.controllerId, button);
    }

    static getGlobalButtonTrigger(button) {
        return Trigger.fromControllerButton(CONTROLLER_UNDEFINED, button);
    }

    runble(strength, duration) {
        globalThis.__atlasGamepadRumble(this, strength, duration);
    }

    rumble(strength, duration) {
        this.runble(strength, duration);
    }
}

export class Joystick {
    constructor() {
        this.joystickId = -1;
        this.name = "";
        this.connected = false;
    }

    getSingleAxisTrigger(axisIndex) {
        return AxisTrigger.fromControllerAxis(this.joystickId, axisIndex, true);
    }

    getDualAxisTrigger(axisIndexX, axisIndexY) {
        return AxisTrigger.fromControllerAxis(
            this.joystickId,
            axisIndexX,
            false,
            axisIndexY,
        );
    }

    getButtonTrigger(buttonIndex) {
        return Trigger.fromControllerButton(this.joystickId, buttonIndex);
    }

    getAxisCount() {
        return globalThis.__atlasJoystickGetAxisCount(this);
    }

    getButtonCount() {
        return globalThis.__atlasJoystickGetButtonCount(this);
    }
}

export class Window {
    constructor() {
        this._title = "";
        this._width = 0;
        this._height = 0;
        this._currentFrame = 0;
        this._gravity = 9.81;
        this._usesDeferred = false;
        this.audioEngine = null;
        return globalThis.__atlasGetWindow() ?? this;
    }

    get title() {
        return this._title;
    }

    set title(value) {
        this._title = value;
    }

    get width() {
        return this._width;
    }

    set width(value) {
        this._width = value;
    }

    get height() {
        return this._height;
    }

    set height(value) {
        this._height = value;
    }

    get currentFrame() {
        return this._currentFrame;
    }

    set currentFrame(value) {
        this._currentFrame = value;
    }

    get main() {
        return globalThis.__atlasGetWindow() ?? this;
    }

    get gravity() {
        return this._gravity;
    }

    set gravity(value) {
        this._gravity = value;
        globalThis.__atlasSetWindowGravity?.(this, value);
    }

    get usesDeferred() {
        return this._usesDeferred;
    }

    set usesDeferred(value) {
        this._usesDeferred = value;
    }

    setClearColor(color) {
        globalThis.__atlasSetWindowClearColor(this, color);
    }

    close() {
        globalThis.__atlasCloseWindow(this);
    }

    setFullscreen(value = true) {
        if (value != null && typeof value === "object") {
            globalThis.__atlasSetWindowFullscreenMonitor(this, value);
            return;
        }
        globalThis.__atlasSetWindowFullscreen(this, value !== false);
    }

    setWindowed(config) {
        globalThis.__atlasSetWindowed(this, config);
    }

    enumerateMonitors() {
        return globalThis.__atlasEnumerateMonitors(this);
    }

    getControllers() {
        return globalThis.__atlasGetControllers(this);
    }

    getController(id) {
        return globalThis.__atlasGetController(this, id);
    }

    getJoystick(id) {
        return globalThis.__atlasGetJoystick(this, id);
    }

    instantiate(object) {
        globalThis.__atlasInstantiateObject(this, object);
    }

    destroy(object) {
        globalThis.__atlasDestroyObject(this, object);
    }

    addUIObject(object) {
        globalThis.__atlasAddUIObject(this, object);
    }

    setCamera(camera) {
        globalThis.__atlasSetWindowCamera(this, camera);
    }

    setScene(scene) {
        globalThis.__atlasSetWindowScene(this, scene);
    }

    getTime() {
        return globalThis.__atlasGetWindowTime(this);
    }

    isKeyActive(key) {
        return globalThis.__atlasIsKeyActive(key);
    }

    isMouseButtonActive(button) {
        return globalThis.__atlasIsMouseButtonActive(button);
    }

    isMouseButtonPressed(button) {
        return globalThis.__atlasIsMouseButtonPressed(button);
    }

    getTextInput() {
        return globalThis.__atlasGetTextInput();
    }

    startTextInput() {
        globalThis.__atlasStartTextInput();
    }

    stopTextInput() {
        globalThis.__atlasStopTextInput();
    }

    isTextInputActive() {
        return globalThis.__atlasIsTextInputActive();
    }

    isControllerButtonPressed(controllerID, buttonIndex) {
        return globalThis.__atlasIsControllerButtonPressed(
            controllerID,
            buttonIndex,
        );
    }

    getControllerAxisValue(controllerID, axisIndex) {
        return globalThis.__atlasGetControllerAxisValue(controllerID, axisIndex);
    }

    getControllerAxisPairValue(controllerID, axisIndexX, axisIndexY) {
        return globalThis.__atlasGetControllerAxisPairValue(
            controllerID,
            axisIndexX,
            axisIndexY,
        );
    }

    releaseMouse() {
        globalThis.__atlasReleaseMouse();
    }

    captureMouse() {
        globalThis.__atlasCaptureMouse();
    }

    getCursorPosition() {
        return globalThis.__atlasGetMousePosition();
    }

    getCurrentScene() {
        return globalThis.__atlasGetScene();
    }

    getCamera() {
        return globalThis.__atlasGetCamera();
    }

    addRenderTarget(target) {
        const renderTarget =
            target ?? globalThis.__atlasCreateRenderTarget(1, 1024);
        return globalThis.__atlasAddWindowRenderTarget(this, renderTarget);
    }

    getSize() {
        return globalThis.__atlasGetWindowSize(this);
    }

    activateDebug() {
        globalThis.__atlasActivateWindowDebug(this);
    }

    desactivateDebug() {
        globalThis.__atlasDeactivateWindowDebug(this);
    }

    deactivateDebug() {
        this.desactivateDebug();
    }

    getDeltaTime() {
        return globalThis.__atlasGetWindowDeltaTime(this);
    }

    getFramesPerSecond() {
        return globalThis.__atlasGetWindowFramesPerSecond(this);
    }

    useAtlasTracer(enabled) {
        globalThis.__atlasUseWindowTracer(this, enabled);
    }

    setLogOutput(showLogs, showWarnings, showErrors) {
        globalThis.__atlasSetWindowLogOutput(
            this,
            showLogs,
            showWarnings,
            showErrors,
        );
    }

    getRenderScale() {
        return globalThis.__atlasGetWindowRenderScale(this);
    }

    useMetalUpscaling(ratio) {
        globalThis.__atlasUseWindowMetalUpscaling(this, ratio);
    }

    isMetalUpscalingEnabled() {
        return globalThis.__atlasIsWindowMetalUpscalingEnabled(this);
    }

    getMetalUpscalingRatio() {
        return globalThis.__atlasGetWindowMetalUpscalingRatio(this);
    }

    getSSAORenderScale() {
        return globalThis.__atlasGetWindowSSAORenderScale(this);
    }

    addInputAction(action) {
        return globalThis.__atlasRegisterInputAction(action);
    }

    resetInputActions() {
        globalThis.__atlasResetInputActions();
    }

    getInputAction(name) {
        return globalThis.__atlasGetWindowInputAction(this, name);
    }

    isActionTriggered(name) {
        return globalThis.__atlasIsActionTriggered(name);
    }

    isActionCurrentlyActive(name) {
        return globalThis.__atlasIsActionCurrentlyActive(name);
    }

    getActionAxisValue(name) {
        return globalThis.__atlasGetAxisActionValue(name);
    }
}

export class Camera {
    constructor() {
        this.position = new Position3d(0, 0, 3);
        this.target = Position3d.zero();
        this.fov = 45;
        this.nearClip = 0.5;
        this.farClip = 1000;
        this.orthographicSize = 5;
        this.movementSpeed = 2;
        this.mouseSensitivity = 0.1;
        this.controllerLookSensitivity = 180;
        this.lookSmoothness = 0.15;
        this.useOrthographic = false;
        this.focusDepth = 20;
        this.focusRange = 10;
        return globalThis.__atlasGetCamera() ?? this;
    }

    move(position) {
        this.position.x += position.x;
        this.position.y += position.y;
        this.position.z += position.z;
        return globalThis.__atlasUpdateCamera(this);
    }

    setPosition(position) {
        this.position = position;
        return globalThis.__atlasUpdateCamera(this);
    }

    setPositionKeepingOrientation(position) {
        return globalThis.__atlasSetPositionKeepingOrientation(this, position);
    }

    lookAt(target, up = Position3d.up()) {
        return globalThis.__atlasLookAtCamera(this, target, up);
    }

    moveTo(position, speed) {
        return globalThis.__atlasMoveCameraTo(this, position, speed);
    }

    getDirection() {
        return globalThis.__atlasGetCameraDirection(this);
    }
}
