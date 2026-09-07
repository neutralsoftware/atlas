//
// atlas.d.ts
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Declarations for the Atlas Library for scripting
// Copyright (c) 2026 Max Van den Eynde
//

/** Runtime logging. */
declare module "atlas/log" {
    /** Writes messages to the Atlas runtime log at the selected severity. */
    export const Debug: {
        print(message: string): void;
        warning(message: string): void;
        error(message: string): void;
    };
}

/** Scene, object, component, resource, camera, and window APIs. */
declare module "atlas" {
    import {
        Position3d,
        Color,
        Position2d,
        Size2d,
        Quaternion,
        Size3d,
        Point3d,
        Normal3d,
        Rotation3d,
        Scale3d,
    } from "atlas/units";
    import {
        Skybox,
        Light,
        SpotLight,
        DirectionalLight,
        AreaLight,
        RenderTarget,
        Texture,
        Cubemap,
    } from "atlas/graphics";
    import {
        AxisTrigger,
        Trigger,
        Key,
        MouseButton,
        InputAction,
        AxisPacket,
    } from "atlas/input";
    import { QueryResult } from "bezel";
    import { AudioEngine } from "finewave";
    import { Atmosphere } from "hydra";

    /** Fog color and intensity used by the scene environment. */
    export type Fog = {
        color: Color;
        intensity: number;
    };

    /** Controls volumetric light scattering and its accumulation parameters. */
    export type VolumetricLighting = {
        enabled: boolean;
        density: number;
        weight: number;
        decay: number;
        exposure: number;
    };

    /** Brightness threshold, filter radius, and sample budget for light bloom. */
    export type LightBloomConfiguration = {
        threshold: number;
        radius: number;
        maxSamples: number;
    };

    /** Color and strength of the rim-lighting effect. */
    export type RimLightingConfiguration = {
        color: Color;
        intensity: number;
    };

    /** Scene-wide fog, lighting effects, and color lookup texture. */
    export type Environment = {
        fog: Fog;
        volumetricLighting: VolumetricLighting;
        lightBloom: LightBloomConfiguration;
        rimLighting: RimLightingConfiguration;
        lookupTexture: Texture;
    };

    /** Access to the active scene, its lighting, atmosphere, camera, and window. */
    export class Scene {
        name: string;

        setAmbientIntensity(intensity: number): void;
        setAutomaticAmbient(enabled: boolean): void;

        setSkybox(skybox: Skybox): void;
        useAtmosphereSkybox(enabled: boolean): void;

        setEnvironment(environment: Environment): void;

        setAmbientColor(color: Color): void;
        setAmbientIntensity(intensity: number): void;
        addDirectionalLight(light: DirectionalLight): void;
        addLight(light: Light): void;
        addSpotLight(light: SpotLight): void;
        addAreaLight(light: AreaLight): void;

        /** Returns the active camera. */
        getCamera(): Camera;
        /** Returns the active runtime window. */
        getWindow(): Window;

        atmosphere: Atmosphere;
    }

    /**
     * Base class for behavior attached to a GameObject. Override lifecycle and event hooks to implement scripts.
     * @example
     * ```ts
     * import { Component } from "atlas";
     * import { Position3d } from "atlas/units";
     *
     * class Mover extends Component {
     *     init(): void {}
     *     update(deltaTime: number): void {
     *         this.getParent().move(new Position3d(deltaTime, 0, 0));
     *     }
     * }
     * ```
     */
    export abstract class Component {
        parentId: number;

        /** Initialization hook for attached script behavior. */
        abstract init(): void;
        /**
         * Per-frame update hook.
         * @param deltaTime Elapsed frame time in seconds.
         */
        abstract update(deltaTime: number): void;
        /** Hook called before physics simulation. */
        beforePhysics(): void;
        /** Hook called when the component is attached to its parent. */
        atAttach(): void;

        /** Called when contact with another object begins. */
        onCollisionEnter(other: GameObject): void;
        /** Called while contact with another object continues. */
        onCollisionStay(other: GameObject): void;
        /** Called when contact with another object ends. */
        onCollisionExit(other: GameObject): void;
        /** Declared signal callback. The current runtime dispatches the legacy spelling `onSignalRecieve`. */
        onSignalReceive(signal: string, sender: GameObject): void;
        /** Called when a signal from the sending object ends. */
        onSignalEnd(signal: string, sender: GameObject): void;
        /** Receives a physics query result and the object owning this component. */
        onQueryReceive(query: QueryResult, sender: GameObject): void;

        /** Returns the parent object. */
        getParent(): GameObject;
        /** Returns a matching component on the parent, or null if none is attached. */
        getParent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;
        /** Looks up a scene object by numeric ID or string name. */
        getObject(identifier: number | string): CoreObject;
        /** Returns the active scene. */
        getScene(): Scene;
        /** Returns the active runtime window. */
        getWindow(): Window;
    }

    /** Physically based surface properties. Defaults include white albedo, metallic 0, roughness 0.5, and AO 1. */
    export class Material {
        constructor();

        albedo: Color;
        metallic: number;
        roughness: number;
        /** Ambient occlusion factor; defaults to 1. */
        ao: number;
        reflectivity: number;
        emissiveColor: Color;
        emissiveIntensity: number;
        normalMapStrength: number;
        useNormalMap: boolean;
        transmittance: number;
        /** Index of refraction; defaults to 1. */
        ior: number;
    }

    /** Mesh vertex with position, color, UV coordinates, and a tangent-space basis. */
    export class CoreVertex {
        constructor(
            position?: Position3d,
            color?: Color,
            textureCoord?: Position2d,
            normal?: Normal3d,
            tangent?: Normal3d,
            bitangent?: Normal3d,
        );

        position: Position3d;
        color: Color;
        textureCoord: Position2d;
        normal: Normal3d;
        tangent: Normal3d;
        bitangent: Normal3d;
    }

    /** A transform for an instance of a CoreObject. Transform methods commit changes to the runtime. */
    export class Instance {
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;

        /** Translates by the supplied offset. */
        move(position: Position3d): void;
        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        /** Replaces the rotation. */
        setRotation(rotation: Rotation3d): void;
        /** Applies an incremental rotation. */
        rotate(rotation: Rotation3d): void;
        /** Replaces the scale factors. */
        setScale(scale: Scale3d): void;
        /** Multiplies the current scale component-wise by the supplied factors. */
        scaleBy(scale: Scale3d): void;

        equals(other: Instance): boolean;
    }

    /** Base class for scene objects with a transform, visibility, and attached components. */
    export abstract class GameObject {
        id: number;
        components: Component[];
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;
        name: string;

        constructor();

        /** Attaches a texture to this object. */
        attachTexture(texture: Texture): void;
        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        /** Translates by the supplied offset. */
        move(position: Position3d): void;
        /** Orients toward the target position. */
        lookAt(target: Position3d, up?: Normal3d): void;
        /** Replaces the rotation. */
        setRotation(rotation: Rotation3d): void;
        /** Applies an incremental rotation. */
        rotate(rotation: Rotation3d): void;
        /** Replaces the scale factors. */
        setScale(scale: Scale3d): void;
        /** Multiplies the current scale component-wise by the supplied factors. */
        scaleBy(scale: Scale3d): void;
        /** Makes the object visible. */
        show(): void;
        /** Makes the object invisible. */
        hide(): void;

        as<T extends GameObject>(type: new (...args: any[]) => T): T | null;

        /** Attaches a component to this object. */
        addComponent<T extends Component>(component: T): void;
    }

    /** Base class for UI elements with screen-space positioning and measured size. */
    export abstract class UIObject extends GameObject {
        /** Returns the current dimensions. */
        getSize(): Size2d;
        /** Returns the UI element's screen-space position. */
        getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        abstract setScreenPosition(position: Position2d): void;
    }

    /** Renderable mesh with vertices, indices, textures, material, and optional instances. */
    export class CoreObject extends GameObject {
        vertices: CoreVertex[];
        indices: number[];
        textures: Texture[];
        material: Material;
        instances: Instance[];
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;
        castsShadows: boolean;
        name: string;

        constructor();

        makeEmissive(color: Color, intensity: number): void;
        attachVertices(vertices: CoreVertex[]): void;
        attachIndices(indices: number[]): void;
        /** Attaches a texture to this object. */
        attachTexture(texture: Texture): void;

        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        /** Translates by the supplied offset. */
        move(position: Position3d): void;
        /** Replaces the rotation. */
        setRotation(rotation: Rotation3d): void;
        setRotationQuaternion(rotation: Quaternion): void;
        /** Applies an incremental rotation. */
        rotate(rotation: Rotation3d): void;
        /** Orients toward the target position. */
        lookAt(target: Position3d, up?: Normal3d): void;
        /** Replaces the scale factors. */
        setScale(scale: Scale3d): void;
        /** Multiplies the current scale component-wise by the supplied factors. */
        scaleBy(scale: Scale3d): void;

        /** Creates a copy of this mesh object. */
        clone(): CoreObject;

        /** Makes the object visible. */
        show(): void;
        /** Makes the object invisible. */
        hide(): void;

        /** Attaches a component to this object. */
        addComponent<T extends Component>(component: T): void;

        enableDeferredRendering(): void;
        disableDeferredRendering(): void;

        /** Creates an additional instance of this mesh and returns its transform handle. */
        createInstance(): Instance;

        /** Returns the component matching the supplied constructor, or null if none is attached. */
        getComponent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;

        static box(size: Size3d): CoreObject;
        static plane(size: Size2d): CoreObject;
        static pyramid(size: Size3d): CoreObject;
        static sphere(
            radius: number,
            sectorCount: number,
            stackCount: number,
        ): CoreObject;
    }

    /** A resource-backed model containing multiple CoreObjects. */
    export class Model extends GameObject {
        static fromResource(path: string): Model;

        /** Returns the mesh objects that make up the model. */
        getObjects(): CoreObject[];

        /** Translates by the supplied offset. */
        override move(position: Position3d): void;
        /** Sets the position to an absolute value. */
        override setPosition(position: Position3d): void;
        /** Replaces the rotation. */
        override setRotation(rotation: Rotation3d): void;
        /** Orients toward the target position. */
        override lookAt(target: Position3d, up?: Normal3d): void;
        /** Applies an incremental rotation. */
        override rotate(rotation: Rotation3d): void;
        /** Replaces the scale factors. */
        override setScale(scale: Scale3d): void;
        /** Multiplies the current scale component-wise by the supplied factors. */
        override scaleBy(scale: Scale3d): void;

        /** Makes the object visible. */
        override show(): void;
        /** Makes the object invisible. */
        override hide(): void;
        /** Attaches a texture to this object. */
        override attachTexture(texture: Texture): void;
    }

    /** Asset category used when loading or looking up a resource. */
    export enum ResourceType {
        File,
        Texture,
        SpecularMap,
        Audio,
        Font,
        Model,
    }

    /** Reference to a named asset and its resource type. */
    export class Resource {
        type: ResourceType;
        path: string;
        name: string;

        constructor(type: ResourceType, path: string, name: string);

        /** Creates a resource reference from an asset path, type, and optional name. */
        static fromAssetPath(
            path: string,
            type: ResourceType,
            name?: string,
        ): Resource;

        /** Looks up a resource by name and type; returns null if no matching resource is found. */
        static fromName(name: string, type: ResourceType): Resource | null;
    }

    /** Named collection of resources with lookup by resource name. */
    export class ResourceGroup {
        resources: Resource[];
        name: string;

        constructor(resources: Resource[], name: string);

        addResource(resource: Resource): void;
        /** Returns the first resource with this name, or null if none exists. */
        getResourceByName(name: string): Resource | null;
    }

    /** Active camera controls for projection, orientation, movement, and depth of field. */
    export class Camera {
        position: Position3d;
        target: Point3d;
        /** Perspective field of view in degrees. */
        fov: number;
        nearClip: number;
        farClip: number;
        orthographicSize: number;
        movementSpeed: number;
        mouseSensitivity: number;
        controllerLookSensitivity: number;
        lookSmoothness: number;
        useOrthographic: boolean;
        focusDepth: number;
        focusRange: number;

        constructor();

        /** Translates by the supplied offset. */
        move(offset: Position3d): void;
        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        /** Moves the camera while preserving its viewing orientation. */
        setPositionKeepingOrientation(position: Position3d): void;
        /** Orients toward the target position. */
        lookAt(target: Point3d, up?: Normal3d): void;
        moveTo(target: Point3d, speed: number): void;
        getDirection(): Normal3d;
    }

    /** Camera position, target, and timing information supplied to frame-dependent delegates. */
    export type ViewInformation = {
        position: Position3d;
        target: Point3d;
        time: number;
        deltaTime: number;
    };

    /** Window geometry, presentation flags, and rendering configuration for windowed mode. */
    export type WindowConfiguration = {
        title: string;
        width: number;
        height: number;
        renderScale: number;
        mouseCaptured: boolean;
        posX: number;
        posY: number;
        multisampling: boolean;
        editorControls: boolean;
        decorations: boolean;
        resizable: boolean;
        transparent: boolean;
        alwaysOnTop: boolean;
        opacity: number;
        aspectRatioX: number;
        aspectRatioY: number;
        ssaoScale: number;
    };

    /** Monitor resolution in pixels and refresh rate in hertz. */
    export type VideoMode = {
        width: number;
        height: number;
        refreshRate: number;
    };

    /** Connected display information and supported video modes. */
    export class Monitor {
        monitorId: number;
        primary: boolean;

        queryVideoModes(): VideoMode[];
        getCurrentVideoMode(): VideoMode;
        /** Returns the physical display dimensions in millimeters. */
        getPhysicalSize(): Size2d;
        getPosition(): Position2d;
        getContentScale(): number;
        getName(): string;
    }

    /** Logical gamepad axes, including paired sticks and individual axis selections. */
    export enum ControllerAxis {
        LeftStick,
        LeftStickX,
        LeftStickY,
        RightStick,
        RightStickX,
        RightStickY,
        Trigger,
        TriggerLeft,
        TriggerRight,
    }

    /** Standard gamepad button identifiers. */
    export enum ControllerButton {
        A = 0,
        B,
        X,
        Y,
        LeftBumper,
        RightBumper,
        Back,
        Start,
        Guide,
        LeftThumb,
        RightThumb,
        DPadUp,
        DPadRight,
        DPadDown,
        DPadLeft,
        ButtonCount,
    }

    /** Button identifiers using Nintendo controller labels. */
    export enum NintendoControllerButton {
        B = 0,
        A,
        Y,
        X,
        L,
        R,
        ZL,
        ZR,
        Minus,
        Plus,
        LeftStick,
        RightStick,
        DPadUp,
        DPadRight,
        DPadDown,
        DPadLeft,
        ButtonCount,
    }

    /** Button identifiers using Sony controller labels. */
    export enum SonyControllerButton {
        Cross = 0,
        Circle,
        Square,
        Triangle,
        L1,
        R1,
        L2,
        R2,
        Share,
        Options,
        LeftStick,
        RightStick,
        DPadUp,
        DPadRight,
        DPadDown,
        DPadLeft,
        ButtonCount,
    }

    /** Sentinel used by global controller triggers to avoid selecting a specific controller. */
    export const CONTROLLER_UNDEFINED = -2;

    /** Connected gamepad access, trigger creation, and vibration control. */
    export class Gamepad {
        controllerId: number;
        name: string;
        connected: boolean;

        getAxisTrigger(axis: ControllerAxis): AxisTrigger;
        /** Creates an axis binding without selecting a specific controller. */
        static getGlobalAxisTrigger(axis: ControllerAxis): AxisTrigger;
        getButtonTrigger(button: ControllerButton): Trigger;
        /** Creates a button binding without selecting a specific controller. */
        static getGlobalButtonTrigger(button: ControllerButton): Trigger;

        /** Requests controller vibration. The method name retains the spelling used by this declaration. */
        runble(strength: number, duration: number): void;
    }

    /** Type alias for Gamepad. */
    export type Controller = Gamepad;

    /** Joystick access using device-specific axis and button indices. */
    export class Joystick {
        joystickId: number;
        name: string;
        connected: boolean;

        getSingleAxisTrigger(axisIndex: number): AxisTrigger;
        getDualAxisTrigger(axisIndexX: number, axisIndexY: number): AxisTrigger;
        getButtonTrigger(buttonIndex: number): Trigger;

        getAxisCount(): number;
        getButtonCount(): number;
    }

    /** Device descriptor returned by Window.getControllers; isJoystick distinguishes joystick devices. */
    export type ControllerID = {
        id: number;
        name: string;
        isJoystick: boolean;
    };

    /** Active runtime window: scene objects, input, audio, display configuration, and frame timing. */
    export class Window {
        title: string;
        width: number;
        height: number;
        currentFrame: number;

        audioEngine: AudioEngine;

        setClearColor(color: Color): void;
        close(): void;
        setFullscreen(enabled: boolean): void;
        setFullscreen(monitor: Monitor): void;
        setWindowed(config: WindowConfiguration): void;

        enumerateMonitors(): Monitor[];
        getControllers(): ControllerID[];
        getController(id: ControllerID): Controller | null;
        getJoystick(id: ControllerID): Joystick | null;

        /** Adds an object to the runtime window. */
        instantiate(object: GameObject): void;
        /** Removes an object from the runtime window. */
        destroy(object: GameObject): void;

        /** Adds a UI element to the window. */
        addUIObject(object: UIObject): void;
        setCamera(camera: Camera): void;
        setScene(scene: Scene): void;
        getTime(): number;
        isKeyActive(key: Key): boolean;

        isMouseButtonActive(button: MouseButton): boolean;
        isMouseButtonPressed(button: MouseButton): boolean;
        /** Returns text collected by the runtime's text input system. */
        getTextInput(): string;
        /** Enables text input collection. */
        startTextInput(): void;
        /** Disables text input collection. */
        stopTextInput(): void;
        /** Reports whether text input collection is enabled. */
        isTextInputActive(): boolean;

        isControllerButtonPressed(
            controllerID: number,
            buttonIndex: number,
        ): boolean;
        getControllerAxisValue(controllerID: number, axisIndex: number): number;
        getControllerAxisPairValue(
            controllerID: number,
            axisIndexX: number,
            axisIndexY: number,
        ): Position2d;

        /** Releases mouse capture. */
        releaseMouse(): void;
        /** Captures the mouse for relative input. */
        captureMouse(): void;
        getCursorPosition(): Position2d;

        main: Window;

        getCurrentScene(): Scene;
        /** Returns the active camera. */
        getCamera(): Camera;
        addRenderTarget(): RenderTarget;
        /** Returns the current dimensions. */
        getSize(): Size2d;
        activateDebug(): void;
        /** Disables the debug display. The method name retains the spelling used by this declaration. */
        desactivateDebug(): void;

        getDeltaTime(): number;
        getFramesPerSecond(): number;
        gravity: number;

        useAtlasTracer(enabled: boolean): void;
        setLogOutput(
            showLogs: boolean,
            showWarnings: boolean,
            showErrors: boolean,
        ): void;

        usesDeferred: boolean;

        getRenderScale(): number;
        useMetalUpscaling(ratio: number): void;
        isMetalUpscalingEnabled(): boolean;
        getMetalUpscalingRatio(): number;

        getSSAORenderScale(): number;

        /** Registers a named input action for later polling. */
        addInputAction(action: InputAction): void;
        /** Clears the registered input actions. */
        resetInputActions(): void;
        /** Returns the registered action with this name, or null if it is missing. */
        getInputAction(name: string): InputAction | null;

        isActionTriggered(name: string): boolean;
        isActionCurrentlyActive(name: string): boolean;
        getActionAxisValue(name: string): AxisPacket;
    }
}

/** Textures, render targets, post-processing, and lighting. */
declare module "atlas/graphics" {
    import { Resource, ResourceGroup } from "atlas";
    import {
        Color,
        Magnitude2d,
        Position3d,
        Magnitude3d,
        Rotation3d,
        Size2d,
    } from "atlas/units";

    /** Texture role used by the renderer, such as albedo, normal, depth, or material data. */
    export enum TextureType {
        Color,
        Specular,
        Cubemap,
        Depth,
        DepthCube,
        Normal,
        Parallax,
        SSAONoise,
        SSAO,
        Metallic,
        Roughness,
        AO,
        Opacity,
        HDR,
        PBRPack,
    }

    /** GPU texture loaded from a resource or created procedurally. */
    export class Texture {
        type: TextureType;
        resource: Resource;
        width: number;
        height: number;
        channels: number;
        id: number;
        borderColor: Color;

        static fromResource(
            resource: Resource | string,
            type: TextureType,
        ): Texture;

        static createEmpty(
            width: number,
            height: number,
            type: TextureType,
            borderColor?: Color,
        ): Texture;

        static createColor(
            color: Color,
            type: TextureType,
            width: number,
            height: number,
        ): Texture;

        createCheckerboard(
            width: number,
            height: number,
            checkSize: number,
            color1: Color,
            color2: Color,
        ): void;

        createDoubleCheckerboard(
            width: number,
            height: number,
            checkSizeBig: number,
            checkSizeSmall: number,
            color1: Color,
            color2: Color,
            color3: Color,
        ): void;

        displayToWindow(): void;
    }

    /** Cube texture assembled from face resources, with helpers for updating face colors. */
    export class Cubemap {
        resources: Resource[];
        id: number;

        constructor(resources: Resource[]);

        getAverageColor(): Color;
        static fromResourceGroup(resourceGroup: ResourceGroup): Cubemap | null;
        updateWithColors(colors: Color[]): void;
    }

    /** Framebuffer purpose and attachment configuration. */
    export enum RenderTargetType {
        Scene,
        Multisampled,
        Shadow,
        CubeShadow,
        GBuffer,
        SSAO,
        SSAOBlur,
    }

    /** Rendering pipeline pass to enqueue for a render target. */
    export enum RenderPassType {
        Deferred,
        Forward,
        PathTracing,
    }

    /** Built-in post-processing effect descriptors. Copy a descriptor to customize its parameters. */
    export const Effects: {
        Inversion: { type: "Inversion" };
        Grayscale: { type: "Grayscale" };
        Sharpen: { type: "Sharpen" };
        Blur: { type: "Blur"; magnitude: number };
        EdgeDetection: { type: "EdgeDetection" };
        ColorCorrection: {
            type: "ColorCorrection";
            exposure: number;
            contrast: number;
            saturation: number;
            gamma: number;
            temperature: number;
            tint: number;
        };
        MotionBlur: { type: "MotionBlur"; size: number; separation: number };
        ChromaticAberration: {
            type: "ChromaticAberration";
            red: number;
            green: number;
            blue: number;
            direction: Magnitude2d;
        };
        Posterization: { type: "Posterization"; levels: number };
        Pixelation: { type: "Pixelation"; pixelSize: number };
        Dialation: { type: "Dilation"; size: number; separation: number };
        Dilation: { type: "Dilation"; size: number; separation: number };
        FilmGrain: { type: "FilmGrain"; amount: number };
    };

    /** A built-in effect name or an effect descriptor containing its parameters. */
    export type Effect =
        | keyof typeof Effects
        | (typeof Effects)[keyof typeof Effects];

    /** Offscreen rendering destination with output textures, render passes, and post-processing effects. */
    export class RenderTarget {
        type: RenderTargetType;
        resolution: number;
        outTextures: Texture[];
        depthTexture: Texture | null;

        constructor(type: RenderTargetType, resolution: number);

        /** Adds a post-processing effect by name or configured descriptor. */
        addEffect(effect: Effect): void;

        /** Enqueues the selected rendering pass for this target. */
        addToPassQueue(type: RenderPassType): void;
        display(): void;
    }

    /** Scene background rendered from a cubemap. */
    export class Skybox {
        cubemap: Cubemap;

        constructor(cubemap: Cubemap);
    }

    /** Uniform ambient illumination described by color and intensity. */
    export class AmbientLight {
        color: Color;
        intensity: number;

        constructor(color?: Color, intensity?: number);
    }

    /** Point light with position, range, diffuse color, and specular shine color. */
    export class Light {
        position: Position3d;
        color: Color;
        shineColor: Color;
        intensity: number;
        distance: number;

        constructor(
            position?: Position3d,
            color?: Color,
            distance?: number,
            shineColor?: Color,
            intensity?: number,
        );

        /** Updates the color used by this object. */
        setColor(color: Color): void;
        // Also calls addDebugObject(Window&);
        /** Creates a visual representation of the light for debugging. */
        createDebugObject(): void;
        /** Enables shadow casting with the requested shadow-map resolution. */
        castShadows(resolution: number): void;
    }

    /** Light with a direction and no positional origin, suitable for sunlight. */
    export class DirectionalLight {
        direction: Magnitude3d;
        color: Color;
        shineColor: Color;
        intensity: number;

        constructor(
            direction?: Magnitude3d,
            color?: Color,
            shineColor?: Color,
            intensity?: number,
        );

        /** Updates the color used by this object. */
        setColor(color: Color): void;
        /** Enables shadow casting with the requested shadow-map resolution. */
        castShadows(resolution: number): void;
    }

    /** Positioned cone light with inner and outer cutoffs, direction, and range. */
    export class SpotLight {
        position: Position3d;
        direction: Magnitude3d;
        color: Color;
        shineColor: Color;
        range: number;
        cutOff: number;
        outerCutOff: number;
        intensity: number;

        constructor(
            position?: Position3d,
            direction?: Magnitude3d,
            color?: Color,
            cutOff?: number,
            outerCutOff?: number,
            shineColor?: Color,
            intensity?: number,
            range?: number,
        );

        /** Updates the color used by this object. */
        setColor(color: Color): void;
        // Also calls addDebugObject(Window&);
        /** Creates a visual representation of the light for debugging. */
        createDebugObject(): void;
        /** Orients toward the target position. */
        lookAt(target: Position3d): void;
        /** Enables shadow casting with the requested shadow-map resolution. */
        castShadows(resolution: number): void;
    }

    /** Oriented area light with dimensions, emission range, and optional two-sided emission. */
    export class AreaLight {
        position: Position3d;
        right: Magnitude3d;
        up: Magnitude3d;
        size: Size2d;
        color: Color;
        shineColor: Color;
        intensity: number;
        range: number;
        angle: number;
        castsBothSides: boolean;
        rotation: Rotation3d;

        constructor(
            position?: Position3d,
            right?: Magnitude3d,
            up?: Magnitude3d,
            size?: Size2d,
            color?: Color,
            shineColor?: Color,
            intensity?: number,
            range?: number,
            angle?: number,
            castsBothSides?: boolean,
            rotation?: Rotation3d,
        );

        getNormal(): Magnitude3d;
        /** Updates the color used by this object. */
        setColor(color: Color): void;
        /** Replaces the rotation. */
        setRotation(rotation: Rotation3d): void;
        /** Applies an incremental rotation. */
        rotate(delta: Rotation3d): void;
        // Also calls addDebugObject(Window&);
        /** Creates a visual representation of the light for debugging. */
        createDebugObject(): void;
        /** Enables shadow casting with the requested shadow-map resolution. */
        castShadows(resolution: number): void;
    }
}

/** Value types for coordinates, dimensions, rotations, and colors. */
declare module "atlas/units" {
    /** Three-component value. Arithmetic returns new values; up is +Y and forward is +Z. */
    export class Position3d {
        x: number;
        y: number;
        z: number;

        constructor(x: number, y: number, z: number);

        static zero(): Position3d;
        static down(): Position3d;
        static up(): Position3d;
        static forward(): Position3d;
        static back(): Position3d;
        static right(): Position3d;
        static left(): Position3d;
        /** Returns a value whose coordinate components are NaN. */
        static invalid(): Position3d;

        /** Returns a new value by adding the operand component-wise; a numeric operand applies to every component. */
        add(other: Position3d | number): Position3d;
        /** Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component. */
        subtract(other: Position3d | number): Position3d;
        /** Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component. */
        multiply(other: Position3d | number): Position3d;
        /** Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component. */
        divide(other: Position3d | number): Position3d;
        /** Tests exact equality of all components. */
        is(other: Position3d): boolean;

        /** Returns a unit-length copy, or a zero vector if the length is zero. */
        normalized(): Position3d;
        toString(): string;
    }

    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Scale3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Size3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Point3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Normal3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Magnitude3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Impulse3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Force3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Vector3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Velocity3d = Position3d;
    /** Semantic type alias for Position3d; it does not introduce a distinct value type. */
    export type Rotation3d = Position3d;

    /** Axis-aligned bounds with inclusive point containment and intersection checks. */
    export class BoundingBox {
        min: Position3d;
        max: Position3d;

        constructor(min: Position3d, max: Position3d);

        toString(): string;
        /** Tests whether the point lies inside or on the bounds. */
        contains(point: Position3d): boolean;
        /** Tests whether the bounds overlap or touch. */
        intersects(other: BoundingBox): boolean;
    }

    /** Four-component orientation with conversion to and from Euler rotations. */
    export class Quaternion {
        x: number;
        y: number;
        z: number;
        w: number;

        constructor(x: number, y: number, z: number, w: number);
        constructor(rotation: Rotation3d);

        toEuler(): Rotation3d;
        static fromEuler(rotation: Rotation3d): Quaternion;
    }

    /** RGBA color, conventionally using 0–1 channels. Arithmetic returns new colors and includes alpha without clamping. */
    export class Color {
        r: number;
        g: number;
        b: number;
        /** Alpha channel; the constructor defaults to 1 (opaque). */
        a: number;

        constructor(r: number, g: number, b: number, a?: number);

        /** Returns a new value by adding the operand component-wise; a numeric operand applies to every component. */
        add(other: Color | number): Color;
        /** Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component. */
        subtract(other: Color | number): Color;
        /** Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component. */
        multiply(other: Color | number): Color;
        /** Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component. */
        divide(other: Color | number): Color;
        /** Tests exact equality of all components. */
        is(other: Color): boolean;

        static white(): Color;
        static black(): Color;
        static red(): Color;
        static green(): Color;
        static blue(): Color;
        static transparent(): Color;
        static yellow(): Color;
        static cyan(): Color;
        static magenta(): Color;
        static gray(): Color;
        static orange(): Color;
        static purple(): Color;
        static brown(): Color;
        static pink(): Color;
        static lime(): Color;
        static navy(): Color;
        static teal(): Color;
        static olive(): Color;
        static maroon(): Color;

        /** Creates an opaque color from RGB or RRGGBB hexadecimal text, with an optional leading #. */
        static fromHex(hex: string): Color;
        /** Linearly interpolates all RGBA channels; t = 0 selects color1 and t = 1 selects color2. The factor is not clamped. */
        static mix(color1: Color, color2: Color, t: number): Color;
    }

    /** Named directions along the three coordinate axes. */
    export enum Direction3d {
        Up,
        Down,
        Left,
        Right,
        Forward,
        Backward,
    }

    /** Two-component value. Arithmetic returns new values; up is +Y. */
    export class Position2d {
        x: number;
        y: number;

        constructor(x: number, y: number);

        static zero(): Position2d;
        static up(): Position2d;
        static down(): Position2d;
        static left(): Position2d;
        static right(): Position2d;
        /** Returns a value whose coordinate components are NaN. */
        static invalid(): Position2d;

        /** Returns a new value by adding the operand component-wise; a numeric operand applies to every component. */
        add(other: Position2d | number): Position2d;
        /** Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component. */
        subtract(other: Position2d | number): Position2d;
        /** Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component. */
        multiply(other: Position2d | number): Position2d;
        /** Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component. */
        divide(other: Position2d | number): Position2d;

        /** Tests exact equality of all components. */
        is(other: Position2d): boolean;
    }

    /** Semantic type alias for Position2d; it does not introduce a distinct value type. */
    export type Scale2d = Position2d;
    /** Semantic type alias for Position2d; it does not introduce a distinct value type. */
    export type Point2d = Position2d;
    /** Semantic type alias for Position2d; it does not introduce a distinct value type. */
    export type Movement2d = Position2d;
    /** Semantic type alias for Position2d; it does not introduce a distinct value type. */
    export type Magnitude2d = Position2d;

    /** Angle stored in radians, with degree conversion and arithmetic returning new values. */
    export class Radians {
        value: number;

        constructor(value: number);

        /** Returns a new value by adding the operand. */
        add(other: Radians): Radians;
        /** Returns a new value by subtracting the operand. */
        subtract(other: Radians): Radians;
        /** Returns a new value by multiplying by the operand. */
        multiply(other: Radians | number): Radians;
        /** Returns a new value by dividing by the operand. */
        divide(other: Radians | number): Radians;

        toNumber(): number;
        static fromDegrees(degrees: number): Radians;
        toDegrees(): number;
    }

    /** Width and height value with component-wise arithmetic returning new values. */
    export class Size2d {
        width: number;
        height: number;

        constructor(width: number, height: number);

        static zero(): Size2d;

        toString(): string;

        /** Returns a new value by adding the operand component-wise; a numeric operand applies to every component. */
        add(other: Size2d | number): Size2d;
        /** Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component. */
        subtract(other: Size2d | number): Size2d;
        /** Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component. */
        multiply(other: Size2d | number): Size2d;
        /** Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component. */
        divide(other: Size2d | number): Size2d;

        /** Tests exact equality of all components. */
        is(other: Size2d): boolean;
    }
}

/** Audio playback components. */
declare module "atlas/audio" {
    import { Component, Resource } from "atlas";
    import { Color, Position3d } from "atlas/units";
    import { AudioSource } from "finewave";

    /** Component that plays an audio source and supports positional audio. */
    export class AudioPlayer extends Component {
        constructor();

        override init(): void;
        /** Starts or resumes playback. */
        play(): void;
        /** Pauses playback. */
        pause(): void;
        /** Stops playback. */
        stop(): void;
        setVolume(volume: number): void;
        /** Enables or disables repeated playback. */
        setLoop(loop: boolean): void;

        /** Loads the audio resource for playback. */
        setSource(resource: Resource): void;

        override update(dt: number): void;

        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        /** Enables or disables positional audio for this player. */
        useSpatialAudio(enabled: boolean): void;

        source: AudioSource;
    }
}

/** Input devices, bindings, action polling, and interaction callbacks. */
declare module "atlas/input" {
    import { Position2d } from "atlas/units";

    /** Keyboard identifiers used by input polling and trigger bindings. */
    export enum Key {
        Unknown,
        Space,
        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,
        Key0,
        Key1,
        Key2,
        Key3,
        Key4,
        Key5,
        Key6,
        Key7,
        Key8,
        Key9,
        Semicolon,
        Equal,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        LeftBracket,
        Backslash,
        RightBracket,
        GraveAccent,
        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,
        Right,
        Left,
        Down,
        Up,
        PageUp,
        PageDown,
        Home,
        End,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        KP0,
        KP1,
        KP2,
        KP3,
        KP4,
        KP5,
        KP6,
        KP7,
        KP8,
        KP9,
        KPDecimal,
        KPDivide,
        KPMultiply,
        KPSubtract,
        KPAdd,
        KPEnter,
        KPEqual,
        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
        Menu,
    }

    /** Mouse button identifiers used by input polling and trigger bindings. */
    export enum MouseButton {
        Left,
        Right,
        Middle,
        X1,
        X2,
        Button6,
        Button7,
        Button8,
        Last,
    }

    /** Discriminator selecting the active field of a button trigger. */
    export enum TriggerType {
        MouseButton,
        Key,
        ControllerButton,
    }

    /** Controller identifier and device button index for a button binding. */
    export type ControllerButtonTrigger = {
        controllerID: number;
        buttonIndex: number;
    };

    /** Digital input binding. Use a factory to populate the fields corresponding to its type. */
    export class Trigger {
        type: TriggerType;
        mouseButton?: MouseButton;
        key?: Key;
        controllerButton?: ControllerButtonTrigger;

        static fromKey(key: Key): Trigger;
        static fromMouseButton(mouseButton: MouseButton): Trigger;
        static fromControllerButton(
            controllerID: number,
            buttonIndex: number,
        ): Trigger;
    }

    /** Source of axis input: mouse motion, directional keys, or controller axes. */
    export enum AxisTriggerType {
        MouseAxis,
        KeyCustom,
        ControllerAxis,
    }

    /** Axis binding built from mouse motion, four directional keys, or one or two controller axes. */
    export class AxisTrigger {
        type: AxisTriggerType;

        positiveX: Trigger;
        negativeX: Trigger;
        positiveY: Trigger;
        negativeY: Trigger;

        controllerId?: number;
        controllerAxisSingle: boolean;
        axisIndex?: number;
        axisIndexY: number;

        isJoystick: boolean;

        /** Creates an axis binding driven by mouse motion. */
        static fromMouse(): AxisTrigger;
        /** Creates a two-axis binding; arguments are positive X, negative X, positive Y, then negative Y. */
        static fromKeys(
            positiveX: Key,
            negativeX: Key,
            positiveY: Key,
            negativeY: Key,
        ): AxisTrigger;
        /** Binds a controller axis or pair; single selects one axis, otherwise axisIndexY supplies the second axis. */
        static fromControllerAxis(
            controllerId: number,
            axisIndex: number,
            single: boolean,
            axisIndexY?: number,
        ): AxisTrigger;
    }

    /** Axis action sample containing value-based input and motion deltas with flags identifying the available sources. */
    export type AxisPacket = {
        deltaX: number;
        deltaY: number;
        x: number;
        y: number;
        valueX: number;
        valueY: number;
        inputDeltaX: number;
        inputDeltaY: number;
        hasValueInput: boolean;
        hasDeltaInput: boolean;
    };

    /** Mouse position and motion offsets passed to interactive callbacks. */
    export type MousePacket = {
        xpos: number;
        ypos: number;
        xoffset: number;
        yoffset: number;
        constrainPitch: boolean;
        firstMouse: boolean;
    };

    /** Horizontal and vertical scroll offsets. */
    export type MouseScrollPacket = {
        xoffset: number;
        yoffset: number;
    };

    /**
     * Named button or axis mapping. Register it before querying it.
     * @example
     * ```ts
     * import { Input, InputAction, Key, Trigger } from "atlas/input";
     *
     * Input.addAction(InputAction.createButtonAction("jump", [Trigger.fromKey(Key.Space)]));
     * ```
     */
    export class InputAction {
        triggers: Trigger[];
        axisTriggers: AxisTrigger[];
        name: string;
        isAxis: boolean;
        isAxisSingle: boolean;
        /** Whether to normalize the two-dimensional action value. */
        normalized: boolean;
        /** Whether to invert the action Y axis. */
        invertY: boolean;

        /** Creates a named digital action from button bindings; register the returned action before polling it. */
        static createButtonAction(
            name: string,
            triggers: Trigger[],
        ): InputAction;
        /** Creates a named axis action from axis bindings; register the returned action before polling it. */
        static createAxisAction(
            name: string,
            axisTriggers: AxisTrigger[],
        ): InputAction;
        /** Creates a single axis using positive and negative button bindings. */
        static createSingleAxisAction(
            name: string,
            positiveTrigger: Trigger,
            negativeTrigger: Trigger,
        ): InputAction;
    }

    /** Global input polling, text input, mouse capture, and named action registration. */
    export const Input: {
        /** Registers an input action and returns the registered action. */
        addAction(action: InputAction): InputAction;
        /** Clears the registered input actions. */
        resetActions(): void;

        isKeyActive(key: Key): boolean;
        isKeyPressed(key: Key): boolean;
        isMouseButtonActive(button: MouseButton): boolean;
        isMouseButtonPressed(button: MouseButton): boolean;

        /** Returns text collected by the runtime's text input system. */
        getTextInput(): string;
        /** Enables text input collection. */
        startTextInput(): void;
        /** Disables text input collection. */
        stopTextInput(): void;
        /** Reports whether text input collection is enabled. */
        isTextInputActive(): boolean;

        isControllerButtonPressed(
            controllerID: number,
            buttonIndex: number,
        ): boolean;
        getControllerAxisValue(controllerID: number, axisIndex: number): number;
        getControllerAxisPairValue(
            controllerID: number,
            axisIndexX: number,
            axisIndexY: number,
        ): Position2d;

        /** Captures the mouse for relative input. */
        captureMouse(): void;
        /** Releases mouse capture. */
        releaseMouse(): void;
        getMousePosition(): Position2d;

        isActionTriggered(name: string): boolean;
        isActionCurrentlyActive(name: string): boolean;
        getAxisActionValue(name: string): AxisPacket;
    };

    /** Callback interface for keyboard, mouse, scroll, and per-frame interaction. */
    export abstract class Interactive {
        abstract onKeyPress(key: Key, dt: number): void;
        abstract onKeyRelease(key: Key, dt: number): void;
        abstract onMouseMove(packet: MousePacket, dt: number): void;
        abstract onMouseButtonPress(button: MouseButton, dt: number): void;
        abstract onMouseScroll(packet: MouseScrollPacket, dt: number): void;
        abstract onEachFrame(dt: number): void;
    }
}

/** Particle simulation and emission controls. */
declare module "atlas/particle" {
    import {
        Position3d,
        Color,
        Magnitude3d,
        Rotation3d,
        Scale3d,
        Normal3d,
    } from "atlas/units";
    import { GameObject } from "atlas";
    import { Texture } from "atlas/graphics";

    /** Selects fountain-style or ambient particle emission. */
    export enum ParticleEmissionType {
        Fountain,
        Ambient,
    }

    /** Lifetime, size, fading, gravity, spread, and speed variation for emitted particles. */
    export type ParticleSettings = {
        minLifetime: number;
        maxLifetime: number;
        minSize: number;
        maxSize: number;
        fadeSpeed: number;
        gravity: number;
        spread: number;
        speedVariation: number;
    };

    /** Particle simulation state including remaining lifetime, velocity, appearance, and activity. */
    export type Particle = {
        position: Position3d;
        velocity: Magnitude3d;
        color: Color;
        lifetime: number;
        maxLifetime: number;
        size: number;
        active: boolean;
    };

    /** Particle system with a fixed capacity, configurable spawning, and optional texturing. */
    export class ParticleEmitter extends GameObject {
        settings: ParticleSettings;
        constructor(maxParticles: number);

        /** Attaches a texture to this object. */
        override attachTexture(texture: Texture): void;
        /** Updates the color used by this object. */
        setColor(color: Color): void;
        enableTexture(): void;
        disableTexture(): void;
        /** Sets the position to an absolute value. */
        override setPosition(position: Position3d): void;
        /** Translates by the supplied offset. */
        override move(position: Position3d): void;
        getPosition(): Position3d;

        setEmissionType(type: ParticleEmissionType): void;
        /** Sets the particle emission direction. */
        setDirection(direction: Magnitude3d): void;
        setSpawnRadius(radius: number): void;
        setSpawnRate(rate: number): void;
        /** Replaces the emitter's particle settings and sends them to the runtime. */
        setParticleSettings(settings: ParticleSettings): void;

        emitOnce(): void;
        emitContinuous(): void;
        /** Starts particle emission. */
        startEmission(): void;
        /** Stops spawning new particles. */
        stopEmission(): void;
        /** Requests emission of the specified number of particles. */
        emitBurst(count: number): void;

        /** Unsupported for particle emitters; the runtime method does nothing. */
        override setRotation(rotation: Rotation3d): void;
        /** Unsupported for particle emitters; the runtime method does nothing. */
        override setScale(scale: Scale3d): void;
        /** Unsupported for particle emitters; the runtime method does nothing. */
        override lookAt(target: Position3d, up?: Normal3d): void;
        /** Unsupported for particle emitters; the runtime method does nothing. */
        override rotate(rotation: Rotation3d): void;
        /** Unsupported for particle emitters; the runtime method does nothing. */
        override scaleBy(scale: Scale3d): void;
        /** Makes the object visible. */
        override show(): void;
        /** Makes the object invisible. */
        override hide(): void;
    }
}

/** Physics bodies, constraints, vehicles, and spatial query results. */
declare module "bezel" {
    import {
        Position3d,
        Normal3d,
        Point3d,
        Size3d,
        Force3d,
        Impulse3d,
        Velocity3d,
    } from "atlas/units";
    import { GameObject, Component } from "atlas";

    /** Ray intersection information, including the hit object and surface normal. */
    export type RaycastHit = {
        position: Position3d;
        normal: Normal3d;
        distance: number;
        object: GameObject;
        didHit: boolean;
    };

    /** Ray query hits and nearest-hit information; hit is null when no closest hit exists. */
    export type RaycastResult = {
        hits: RaycastHit[];
        hit: RaycastHit | null;
        closestDistance: number;
    };

    /** Overlap contact information. penerationAxis retains the spelling used by this API. */
    export type OverlapHit = {
        contactPoint: Position3d;
        penerationAxis: Point3d;
        penetrationDepth: number;
        object: GameObject;
    };

    /** Overlap contacts and a flag indicating whether any were found. */
    export type OverlapResult = {
        hits: OverlapHit[];
        hitAny: boolean;
    };

    /** Contact encountered while sweeping a collider along a movement path. */
    export type SweepHit = {
        position: Position3d;
        normal: Normal3d;
        distance: number;
        percentage: number;
        object: GameObject;
    };

    /** Movement sweep contacts, nearest contact, and resulting end position. */
    export type SweepResult = {
        hits: SweepHit[];
        closest: SweepHit | null;
        hitAny: boolean;
        endPosition: Position3d;
    };

    /** Identifies the physics query represented by a QueryResult. */
    export enum QueryOperation {
        RaycastAll,
        Raycast,
        RasycastWorld,
        RaycastWorldAll,
        RaycastTagged,
        RaycastTaggedAll,
        Movement,
        Overlap,
        MovementAll,
    }

    /** Physics query payload. Inspect operation before reading the corresponding result field. */
    export type QueryResult = {
        operation: QueryOperation;
        raycastResult?: RaycastResult;
        overlapResult?: OverlapResult;
        sweepResult?: SweepResult;
    };

    /** World attachment marker type used as a joint endpoint. */
    export type WorldBody = {};

    /** An object or world attachment used as one endpoint of a joint. */
    export type JointMember = GameObject | WorldBody;

    /** Selects frequency/damping-ratio or stiffness/damping spring configuration. */
    export enum SpringMode {
        FrequencyAndDamping,
        StiffnessAndDamping,
    }

    /** Coordinate space used for joint configuration. */
    export enum Space {
        Local,
        Global,
    }

    /** Optional spring behavior with parameters interpreted according to mode. */
    export type Spring = {
        enabled: boolean;
        mode: SpringMode;
        frequency: number;
        dampingRatio: number;
        stiffness: number;
        damping: number;
    };

    /** Optional minimum and maximum joint angles. */
    export type AngleLimits = {
        enabled: boolean;
        minAngle: number;
        maxAngle: number;
    };

    /** Optional joint motor with force and torque limits. */
    export type Motor = {
        enabled: boolean;
        maxForce: number;
        maxTorque: number;
    };

    /** Base component for a physical constraint between two joint members. */
    export abstract class Joint extends Component {
        parent: JointMember;
        child: JointMember;
        space: Space;
        anchor: Position3d;
        breakForce: number;
        breakTorque: number;

        override init(): void;
        override update(deltaTime: number): void;

        abstract override beforePhysics(): void;
        /** Breaks the physical constraint between the joint members. */
        abstract breakJoint(): void;
    }

    /** Constraint that fixes the relative transform of its two members. */
    export class FixedJoint extends Joint {
        override beforePhysics(): void;
        /** Breaks the physical constraint between the joint members. */
        override breakJoint(): void;
    }

    /** Constraint allowing rotation about a hinge axis, with optional limits and motor. */
    export class HingeJoint extends Joint {
        axis1: Normal3d;
        axis2: Normal3d;
        angleLimits: AngleLimits;
        motor: Motor;

        override beforePhysics(): void;
        /** Breaks the physical constraint between the joint members. */
        override breakJoint(): void;
    }

    /** Distance constraint with spring behavior and optional length limits. */
    export class SpringJoint extends Joint {
        anchorB: Position3d;
        restLength: number;
        useLimits: boolean;
        minLength: number;
        maxLength: number;

        spring: Spring;

        override beforePhysics(): void;
        /** Breaks the physical constraint between the joint members. */
        override breakJoint(): void;
    }

    /** Wheel geometry, suspension, steering, and braking parameters. */
    export type VehicleWheelSettings = {
        position: Position3d;
        enableSuspensionForcePoint: boolean;
        suspensionForcePoint: Position3d;

        suspensionDirection: Normal3d;
        steeringAxis: Normal3d;
        wheelUp: Normal3d;
        wheelForward: Normal3d;

        suspensionMinLength: number;
        suspensionMaxLength: number;
        suspensionPreloadLength: number;
        suspensionFrequencyHz: number;
        suspensionDampingRatio: number;

        radius: number;
        width: number;

        inertia: number;
        angularDamping: number;
        maxSteerAngleDegrees: number;
        maxBrakeTorque: number;
        maxHandBrakeTorque: number;
    };

    /** Torque distribution and differential settings for a pair of wheel indices. */
    export type VehicleDifferential = {
        leftWheel: number;
        rightWheel: number;
        differentialRatio: number;
        leftRightSplit: number;
        limitedSlipRatio: number;
        engineTorqueRatio: number;
    };

    /** Engine torque, RPM limits, inertia, and angular damping. */
    export type VehicleEngine = {
        maxTorque: number;
        minRPM: number;
        maxRPM: number;
        inertia: number;
        angularDamping: number;
    };

    /** Automatic or manual vehicle gear selection. */
    export enum VehicleTransmissionMode {
        Auto,
        Manual,
    }

    /** Forward/reverse gear ratios, clutch behavior, and shift timing. */
    export type VehicleTransmission = {
        mode: VehicleTransmissionMode;
        gearRatios: number[];
        reverseGearRatios: number[];
        switchTime: number;
        clutchReleaseTime: number;
        switchLatency: number;
        shiftUpRPM: number;
        shiftDownRPM: number;
        clutchStrength: number;
    };

    /** Engine, transmission, and differential configuration for a vehicle. */
    export type VehicleControllerSettings = {
        engine: VehicleEngine;
        transmission: VehicleTransmission;
        differentials: VehicleDifferential[];
        differentialLimitedSlipRatio: number;
    };

    /** Vehicle axes, wheel configurations, controller settings, and stability limits. */
    export type VehicleSettings = {
        up: Normal3d;
        forward: Normal3d;

        maxPitchRollAngleDeg: number;

        wheels: VehicleWheelSettings[];
        controller: VehicleControllerSettings;

        maxSlopAngleDeg: number;
    };

    /** Physics vehicle component with driving inputs and configurable wheels and drivetrain. */
    export class Vehicle extends Component {
        settings: VehicleSettings;
        forward: number;
        right: number;
        brake: number;
        handBrake: number;

        override atAttach(): void;
        override beforePhysics(): void;

        /** Requests recreation of the vehicle physics state after configuration changes. */
        requestRecreate(): void;

        override init(): void;
        override update(deltaTime: number): void;
    }

    /** Capsule collision shape specified by radius and height. */
    export type CapsuleCollider = {
        radius: number;
        height: number;
    };

    /** Box collision shape specified by its three-dimensional size. */
    export type BoxCollider = {
        size: Size3d;
    };

    /** Sphere collision shape specified by radius. */
    export type SphereCollider = {
        radius: number;
    };

    /** Mesh collision shape descriptor. */
    export type MeshCollider = {};

    /** Supported collision shape descriptors for rigidbodies and physics queries. */
    export type Collider =
        | CapsuleCollider
        | BoxCollider
        | SphereCollider
        | MeshCollider;

    /** Physics component providing colliders, forces, velocities, tags, and spatial queries. */
    export class Rigidbody extends Component {
        sendSignal: string;
        isSensor: boolean;

        override atAttach(): void;
        override init(): void;
        override beforePhysics(): void;
        override update(deltaTime: number): void;

        clone(): Rigidbody;

        addCollider(collider: Collider): void;

        setFriction(friction: number): void;
        /** Applies a force to the body. */
        applyForce(force: Force3d): void;
        /** Applies a force at the specified position, allowing an off-center force to produce torque. */
        applyForceAtPoint(force: Force3d, point: Position3d): void;
        /** Applies an instantaneous impulse to the body. */
        applyImpulse(impulse: Impulse3d): void;

        setLinearVelocity(velocity: Velocity3d): void;
        /** Adds to the body's current linear velocity. */
        addLinearVelocity(velocity: Velocity3d): void;
        setAngularVelocity(velocity: Velocity3d): void;
        /** Adds to the body's current angular velocity. */
        addAngularVelocity(velocity: Velocity3d): void;

        setMaxLinearVelocity(velocity: number): void;
        setMaxAngularVelocity(velocity: number): void;

        getLinearVelocity(): Velocity3d;
        getAngularVelocity(): Velocity3d;
        getVelocity(): Velocity3d;

        raycast(direction: Normal3d, maxDistance: number): RaycastResult;
        raycastAll(direction: Normal3d, maxDistance: number): RaycastResult;
        raycastWorld(
            origin: Position3d,
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;
        raycastWorldAll(
            origin: Position3d,
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;
        raycastTagged(
            tags: string[],
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;
        raycastTaggedAll(
            tags: string[],
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;

        overlap(): OverlapResult;
        overlapWithCollider(collider: Collider): OverlapResult;
        overlapWithColliderWorld(
            collider: Collider,
            position: Position3d,
        ): OverlapResult;

        predictMovementWithCollider(
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        predictMovement(endPosition: Position3d): SweepResult;
        predictMovementWithColliderWorld(
            startPosition: Position3d,
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        predictMovementWorld(
            startPosition: Position3d,
            endPosition: Position3d,
        ): SweepResult;
        predictMovementWithColliderAll(
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        predictMovementAll(endPosition: Position3d): SweepResult;
        predictMovementWithColliderAllWorld(
            startPosition: Position3d,
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        predictMovementAllWorld(
            startPosition: Position3d,
            endPosition: Position3d,
        ): SweepResult;

        hasTag(tag: string): boolean;
        addTag(tag: string): void;
        removeTag(tag: string): void;

        setDamping(linearDamping: number, angularDamping: number): void;
        setMass(mass: number): void;
        /** Sets collision restitution (bounciness); the method name retains its existing spelling. */
        setRestituition(restitution: number): void;
        /** Selects a static, dynamically simulated, or kinematic body. */
        setMotionType(motionType: "Static" | "Dynamic" | "Kinematic"): void;
    }

    /** Rigidbody initialized as a sensor for detecting contacts and sending signals. */
    export class Sensor extends Rigidbody {
        constructor(); // sets isSensor to true

        setSignal(signal: string): void;
    }
}

/** Noise, procedural terrain, and biome APIs. */
declare module "aurora" {
    import { GameObject, Resource } from "atlas";
    import { Texture } from "atlas/graphics";
    import {
        Color,
        Position3d,
        Rotation3d,
        Normal3d,
        Scale3d,
    } from "atlas/units";

    /** Two-dimensional Perlin noise generator with optional seed. */
    export class PerlinNoise {
        constructor(seed?: number);
        noise(x: number, y: number): number;
    }

    /** Two-dimensional simplex noise sampling. */
    export class SimplexNoise {
        static noise(xin: number, yin: number): number;
    }

    /** Cellular noise generator configured by point count and optional seed. */
    export class WorleyNoise {
        constructor(numPoints: number, seed?: number);
        noise(x: number, y: number): number;
    }

    /** Layered noise generator configured by octave count and persistence. */
    export class FractalNoise {
        constructor(o: number, p: number);
        noise(x: number, y: number): number;
    }

    /** Convenience entry points for the supported two-dimensional noise functions. */
    export class Noise {
        static perlin(x: number, y: number): number;
        static simplex(x: number, y: number): number;
        static worley(x: number, y: number): number;
        static fractal(
            x: number,
            y: number,
            octaves: number,
            persistence: number,
        ): number;
        static seed: number;
        static initializedSeed: boolean;
    }

    /** Terrain appearance and height, moisture, and temperature selection settings. */
    export class Biome {
        name: string;
        texture: Texture;
        color: Color;
        useTexture: boolean;

        /** Attaches a texture to this object. */
        attachTexture(texture: Texture): void;

        minHeight: number;
        maxHeight: number;
        minMoisture: number;
        maxMoisture: number;
        minTemperature: number;
        maxTemperature: number;

        constructor(
            name: string,
            texture: Texture,
            color: Color,
            useTexture: boolean,
        );

        condition: BiomeFunction;
    }

    /** Callback receiving a biome for custom biome configuration. */
    export type BiomeFunction = (biome: Biome) => void;

    /** Terrain object created from a heightmap or procedural generator, with biome support. */
    export class Terrain extends GameObject {
        /** Attaches a texture to this object. */
        attachTexture(texture: Texture): void;
        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        /** Translates by the supplied offset. */
        move(position: Position3d): void;
        /** Replaces the rotation. */
        setRotation(rotation: Rotation3d): void;
        /** Orients toward the target position. */
        lookAt(target: Position3d, up?: Normal3d): void;
        /** Applies an incremental rotation. */
        rotate(rotation: Rotation3d): void;
        /** Replaces the scale factors. */
        setScale(scale: Scale3d): void;
        /** Multiplies the current scale component-wise by the supplied factors. */
        scaleBy(scale: Scale3d): void;
        /** Makes the object visible. */
        show(): void;
        /** Makes the object invisible. */
        hide(): void;

        heightmap: Resource;
        moistureTexture: Texture;
        temperatureTexture: Texture;
        generator: TerrainGenerator;

        createdWithMap: boolean;
        width: number;
        length: number;
        height: number;

        /** Adds a biome to the terrain. */
        addBiome(biome: Biome): void;

        /** Creates terrain using the supplied height generator. */
        static fromGenerator<T extends TerrainGenerator>(generator: T): Terrain;
        /** Creates terrain from a heightmap resource. */
        static fromHeightmap(heightmap: Resource): Terrain;

        maxPeak: number;
        seaLevel: number;
    }

    /** Base class for a height function that can be applied to terrain. */
    export abstract class TerrainGenerator {
        /** Samples the procedural terrain height at the supplied coordinates. */
        abstract generateHeight(x: number, y: number): number;
        applyTo(terrain: Terrain): void;
    }

    /** Procedural hill height generator configured by scale and amplitude. */
    export class HillGenerator extends TerrainGenerator {
        constructor(scale: number, amplitude: number);

        /** Samples the procedural terrain height at the supplied coordinates. */
        override generateHeight(x: number, y: number): number;
    }

    /** Layered mountain height generator configured by scale, amplitude, octaves, and persistence. */
    export class MountainGenerator extends TerrainGenerator {
        constructor(
            scale: number,
            amplitude: number,
            octaves: number,
            persistance: number,
        );

        /** Samples the procedural terrain height at the supplied coordinates. */
        override generateHeight(x: number, y: number): number;
    }

    /** Procedural plain height generator configured by scale and amplitude. */
    export class PlainGenerator extends TerrainGenerator {
        constructor(scale: number, amplitude: number);

        /** Samples the procedural terrain height at the supplied coordinates. */
        override generateHeight(x: number, y: number): number;
    }

    /** Procedural island height generator configured by feature count and scale. */
    export class IslandGenerator extends TerrainGenerator {
        constructor(numFeatures: number, scale: number);

        /** Samples the procedural terrain height at the supplied coordinates. */
        override generateHeight(x: number, y: number): number;
    }

    /** Terrain generator that combines multiple child generators. */
    export class CompoundGenerator extends TerrainGenerator {
        addGenerator<T extends TerrainGenerator>(generator: T): void;
        /** Samples the procedural terrain height at the supplied coordinates. */
        override generateHeight(x: number, y: number): number;
    }
}

/** Audio playback, spatial listeners, and sound effects. */
declare module "finewave" {
    import { Position3d } from "atlas/units";
    import { Resource } from "atlas";

    /** Audio device controls, master volume, and spatial listener state. */
    export class AudioEngine {
        setListenerPosition(position: Position3d): void;
        setListenerOrientation(forward: Position3d, up: Position3d): void;
        setListenerVelocity(velocity: Position3d): void;
        setMasterVolume(volume: number): void;
        deviceName: string;
    }

    /** Audio data loaded from a resource for use by an AudioSource. */
    export class AudioData {
        static fromResource(resource: Resource): AudioData;
        isMono: boolean;
        resource: Resource;
    }

    /** Playback controls for audio data, including spatialization and effects. */
    export class AudioSource {
        setData(data: AudioData): void;
        fromFile(resource: Resource): void;
        /** Starts or resumes playback. */
        play(): void;
        /** Pauses playback. */
        pause(): void;
        /** Stops playback. */
        stop(): void;
        /** Enables or disables repeated playback. */
        setLoop(loop: boolean): void;
        setVolume(volume: number): void;
        setPitch(pitch: number): void;
        /** Sets the position to an absolute value. */
        setPosition(position: Position3d): void;
        setVelocity(velocity: Position3d): void;

        isPlaying(): boolean;
        /** Starts playback at the supplied offset in seconds. */
        playFrom(position: number): void;
        /** Disables positional audio processing for this source. */
        disableSpatialization(): void;
        /** Applies an audio effect to this source. */
        applyEffect(effect: AudioEffect): void;
        getPosition(): Position3d;
        getListenerPosition(): Position3d;
        /** Enables positional audio processing for this source. */
        useSpatialization(): void;
    }

    /** Base type for effects that can be applied to an AudioSource. */
    export abstract class AudioEffect {}

    /** Reverberation effect with room, damping, stereo width, and wet/dry controls. */
    export class Reverb extends AudioEffect {
        setRoomSize(size: number): void;
        setDamping(damping: number): void;
        setWetLevel(level: number): void;
        setDryLevel(level: number): void;
        setWidth(width: number): void;
    }

    /** Delay effect with decay and wet/dry controls. */
    export class Echo extends AudioEffect {
        /** Sets the echo delay in seconds. */
        setDelay(delay: number): void;
        setDecay(decay: number): void;
        setWetLevel(level: number): void;
        setDryLevel(level: number): void;
    }

    /** Distortion effect with edge, gain, and low-pass filtering controls. */
    export class Distortion extends AudioEffect {
        setEdge(edge: number): void;
        setGain(gain: number): void;
        setLowpassCutoff(cutoff: number): void;
    }
}

/** UI elements, layouts, fonts, styles, and themes. */
declare module "graphite" {
    import { UIObject, Resource } from "atlas";
    import { Texture } from "atlas/graphics";
    import { Position2d, Position3d, Color, Size2d, Size3d } from "atlas/units";

    /** Textured UI element with size, tint, and styling. */
    export class Image extends UIObject {
        texture: Texture;
        position: Position3d;
        size: Size2d;
        tint: Color;

        constructor();
        constructor(
            texture: Texture,
            size: Size2d,
            position: Position2d,
            tint: Color,
        );

        /** Returns the current dimensions. */
        override getSize(): Size2d;
        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        /** Returns the element's style. */
        style(): UIStyle;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Image;

        setTexture(texture: Texture): void;
        setSize(size: Size2d): void;
    }

    /** Text field state supplied to a change callback. */
    export type TextFieldChangeEvent = {
        text: string;
        cursorPosition: number;
        focused: boolean;
    };

    /** Button label supplied to a click callback. */
    export type ButtonClickEvent = {
        label: string;
    };

    /** Checkbox label and checked state supplied to a toggle callback. */
    export type CheckboxToggleEvent = {
        label: string;
        checked: boolean;
    };

    /** Callback types associated with TextField. */
    export namespace TextField {
        export type ChangeCallback = (event: TextFieldChangeEvent) => void;
    }

    /** Editable text input with focus, cursor, placeholder, and change callbacks. */
    export class TextField extends UIObject {
        text: string;
        placeholder: string;
        font: Font;
        position: Position3d;
        fontSize: number;
        padding: Size2d;
        maximumWidth: number;
        textColor: Color;
        placeholderColor: Color;
        backgroundColor: Color;
        borderColor: Color;
        focusedBorderColor: Color;
        cursorColor: Color;

        constructor();

        constructor(
            font: Font,
            maximumWidth: number,
            position: Position2d,
            text: string,
            placeholder: string,
        );

        /** Returns the current dimensions. */
        override getSize(): Size2d;
        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        getText(): string;
        isFocused(): boolean;
        getCursorIndex(): number;
        /** Returns the element's style. */
        style(): UIStyle;

        setText(text: string): TextField;
        setPlaceholder(placeholder: string): TextField;
        setPadding(padding: Size2d): TextField;
        setMaximumWidth(width: number): TextField;
        setFontSize(size: number): TextField;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): TextField;
        /** Registers the text-change callback and returns this text field for chaining. */
        setOnChange(callback: TextField.ChangeCallback): TextField;

        /** Gives keyboard focus to the text field. */
        focus(): void;
        /** Removes keyboard focus from the text field. */
        blur(): void;
    }

    /** Callback types associated with Button. */
    export namespace Button {
        export type ClickCallback = (event: ButtonClickEvent) => void;
    }

    /** Clickable text button with hover, enabled state, and click callback support. */
    export class Button extends UIObject {
        label: string;
        font: Font;
        position: Position3d;
        fontSize: number;
        padding: Size2d;
        minimumSize: Size2d;
        textColor: Color;
        backgroundColor: Color;
        hoverBackgroundColor: Color;
        pressedBackgroundColor: Color;
        borderColor: Color;
        hoverBorderColor: Color;
        enabled: boolean;

        constructor();

        constructor(font: Font, label: string, position: Position2d);

        /** Returns the current dimensions. */
        override getSize(): Size2d;
        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        getLabel(): string;
        isHovered(): boolean;
        isEnabled(): boolean;

        /** Returns the element's style. */
        style(): UIStyle;

        setLabel(label: string): Button;
        setPadding(padding: Size2d): Button;
        setMinimumSize(size: Size2d): Button;
        setFontSize(size: number): Button;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Button;
        /** Registers the click callback and returns this button for chaining. */
        setOnClick(callback: Button.ClickCallback): Button;
        setEnabled(enabled: boolean): void;
    }

    /** Callback types associated with Checkbox. */
    export namespace Checkbox {
        export type ToggleCallback = (event: CheckboxToggleEvent) => void;
    }

    /** Toggleable checkbox with a label and change callback support. */
    export class Checkbox extends UIObject {
        label: string;
        font: Font;
        position: Position3d;
        fontSize: number;
        padding: Size2d;
        boxSize: number;
        spacing: number;
        checked: boolean;
        enabled: boolean;
        textColor: Color;
        boxBackgroundColor: Color;
        hoverBoxBackgroundColor: Color;
        borderColor: Color;
        activeBorderColor: Color;
        checkColor: Color;

        constructor();

        constructor(font: Font, label: string, position: Position2d);

        /** Returns the current dimensions. */
        override getSize(): Size2d;
        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        getLabel(): string;
        isChecked(): boolean;
        isHovered(): boolean;
        isEnabled(): boolean;

        /** Returns the element's style. */
        style(): UIStyle;

        setLabel(label: string): Checkbox;
        setPadding(padding: Size2d): Checkbox;
        setFontSize(size: number): Checkbox;
        setBoxSize(size: number): Checkbox;
        setSpacing(spacing: number): Checkbox;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Checkbox;
        /** Registers the toggle callback and returns this checkbox for chaining. */
        setOnToggle(callback: Checkbox.ToggleCallback): Checkbox;
        setChecked(checked: boolean): void;
        setEnabled(enabled: boolean): void;
        toggle(): void;
    }

    /** Alignment of children within a layout. */
    export enum ElementAlignment {
        Top,
        Center,
        Bottom,
    }

    /** Anchor used to place a layout relative to its position. */
    export enum LayoutAnchor {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    }

    /** Vertical layout of UI children with spacing, padding, alignment, and an anchor. */
    export class Column extends UIObject {
        constructor(position: Position2d);
        constructor(
            children: UIObject[],
            spacing: number,
            padding: Size2d,
            position: Position2d,
        );

        spacing: number;
        maxSize: Size2d;
        padding: Size2d;
        children: UIObject[];
        position: Position3d;
        alignment: ElementAlignment;
        anchor: LayoutAnchor;

        /** Appends a child element to the layout. */
        addChild(child: UIObject): void;
        /** Replaces the layout's child elements. */
        setChildren(children: UIObject[]): void;

        /** Returns the current dimensions. */
        override getSize(): Size2d;

        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        style: UIStyle;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Column;
    }

    /** Horizontal layout of UI children with spacing, padding, alignment, and an anchor. */
    export class Row extends UIObject {
        constructor(position: Position2d);
        constructor(
            children: UIObject[],
            spacing: number,
            padding: Size2d,
            position: Position2d,
        );

        spacing: number;
        maxSize: Size2d;
        padding: Size2d;
        children: UIObject[];
        position: Position3d;
        alignment: ElementAlignment;
        anchor: LayoutAnchor;

        /** Appends a child element to the layout. */
        addChild(child: UIObject): void;
        /** Replaces the layout's child elements. */
        setChildren(children: UIObject[]): void;

        /** Returns the current dimensions. */
        override getSize(): Size2d;

        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        style: UIStyle;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Column;
    }

    /** Overlapping layout of UI children with horizontal and vertical alignment. */
    export class Stack extends UIObject {
        constructor(position: Position2d);
        constructor(
            children: UIObject[],
            padding: Size2d,
            position: Position2d,
        );

        maxSize: Size2d;
        padding: Size2d;
        children: UIObject[];
        position: Position3d;
        horizontalAlignment: ElementAlignment;
        verticalAlignment: ElementAlignment;
        anchor: LayoutAnchor;

        /** Appends a child element to the layout. */
        addChild(child: UIObject): void;
        /** Replaces the layout's child elements. */
        setChildren(children: UIObject[]): void;

        /** Returns the current dimensions. */
        override getSize(): Size2d;

        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        style: UIStyle;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Column;
    }

    /** Interaction state selecting a style variant. */
    export enum UIStyleState {
        Normal,
        Hovered,
        Pressed,
        Disabled,
        Focused,
        Checked,
    }

    /** Current interaction flags used when resolving a UI style. */
    export type UIStyleStateSnapshot = {
        hovered: boolean;
        pressed: boolean;
        disabled: boolean;
        focused: boolean;
        checked: boolean;
    };

    /** Optional style overrides for one state; setter methods support chaining. */
    export class UIStyleVariant {
        paddingValue?: number;
        cornerRadiusValue?: number;
        borderWidthValue?: number;
        backgroundColorValue?: Color;
        borderColorValue?: Color;
        foregroundColorValue?: Color;
        tintColorValue?: Color;
        fontValue?: Font;
        fontSizeValue?: number;

        padding(value: Size2d): UIStyleVariant;
        cornerRadius(value: number): UIStyleVariant;
        borderWidth(value: number): UIStyleVariant;
        backgroundColor(value: Color): UIStyleVariant;
        borderColor(value: Color): UIStyleVariant;
        foregroundColor(value: Color): UIStyleVariant;
        tintColor(value: Color): UIStyleVariant;
        font(value: Font): UIStyleVariant;
        fontSize(value: number): UIStyleVariant;
    }

    /** Concrete visual properties produced by style resolution. */
    export type UIResolvedStyle = {
        padding: Size2d;
        cornerRadius: number;
        borderWidth: number;
        backgroundColor: Color;
        borderColor: Color;
        foregroundColor: Color;
        tintColor: Color;
        font: Font;
        fontSize: number;
    };

    /** Collection of visual overrides for normal and interactive states. */
    export class UIStyle {
        normal(): UIStyleVariant;
        hovered(): UIStyleVariant;
        pressed(): UIStyleVariant;
        disabled(): UIStyleVariant;
        focused(): UIStyleVariant;
        checked(): UIStyleVariant;
        /** Returns the style variant associated with the selected interaction state. */
        variant(state: UIStyleState): UIStyleVariant;
    }

    /** Shared default styles for UI element types, with access to the current theme. */
    export class Theme {
        text: UIStyle;
        image: UIStyle;
        textField: UIStyle;
        button: UIStyle;
        checkbox: UIStyle;
        row: UIStyle;
        column: UIStyle;
        stack: UIStyle;

        /** Returns the current shared UI theme. */
        static current(): Theme;
        /** Replaces the current shared UI theme. */
        static set(theme: Theme): void;
        /** Restores the default UI theme. */
        static reset(): void;
    }

    /** Glyph dimensions, bearing, advance, and texture atlas UV bounds. */
    export type Character = {
        size: Size2d;
        bearing: Position2d;
        advance: number;
        uvMin: Position2d;
        uvMax: Position2d;
    };

    /** Map from character strings to glyph metrics and atlas coordinates. */
    export type FontAtlas = Map<string, Character>;

    /** Resource-backed font with atlas texture and size controls. */
    export class Font {
        name: string;
        atlas: Texture;
        size: number;
        resource: Resource;
        texture: Texture;

        static fromResource(resource: Resource): Font;
        static getFont(name: string): Font;

        changeSize(size: number): Font;
    }

    /** UI text rendered with a font, color, screen position, and optional style. */
    export class Text extends UIObject {
        content: string;
        font: Font;
        position: Position3d;
        fontSize: number;
        color: Color;

        constructor();
        constructor(
            text: string,
            font: Font,
            color: Color,
            position: Position2d,
        );

        /** Returns the current dimensions. */
        override getSize(): Size2d;
        /** Returns the UI element's screen-space position. */
        override getScreenPosition(): Position2d;
        /** Sets the UI element's screen-space position. */
        override setScreenPosition(position: Position2d): void;

        /** Returns the element's style. */
        style(): UIStyle;
        /** Applies a style to the element. */
        setStyle(style: UIStyle): Text;
        setFontSize(size: number): Text;
    }
}

/** Atmosphere, weather, clouds, and water rendering. */
declare module "hydra" {
    import {
        Position3d,
        Size3d,
        Force3d,
        Magnitude3d,
        Color,
        Scale3d,
        Rotation3d,
        Size2d,
    } from "atlas/units";
    import { Cubemap } from "atlas/graphics";
    import { ViewInformation, GameObject } from "atlas";
    import { Texture } from "atlas/graphics";

    /** Three-dimensional cellular noise with helpers producing texture handles. */
    export class WorleyNoise3D {
        constructor(frequency: number, numDivisions: number);

        getValue(x: number, y: number, z: number): number;

        get3dTexture(size: number): number;
        getDetailTexture(size: number): number;
        get3dTextureAtAllChannels(size: number): number;
    }

    /** Volumetric cloud noise and rendering controls, including density, lighting steps, and wind. */
    export class Clouds {
        constructor(frequency: number, numDivisions: number);

        getCloudTexture(size: number): number;

        position: Position3d;
        size: Size3d;
        scale: number;
        offset: Position3d;
        density: number;
        densityMultiplier: number;
        absorption: number;
        scattering: number;
        phase: number;
        clusterStrength: number;
        primaryStepCount: number;
        lightStepCount: number;
        lightStepMultiplier: number;
        minStepLength: number;
        wind: Force3d;
    }

    /** Weather preset represented by a WeatherState. */
    export enum WeatherCondition {
        Clear,
        Rain,
        Snow,
        Storm,
    }

    /** Weather condition, intensity, and wind returned by a weather delegate. */
    export type WeatherState = {
        condition: WeatherCondition;
        intensity: number;
        wind: Force3d;
    };

    /** Computes weather from the current view and frame information. */
    export type WeatherDelegate = (
        information: ViewInformation,
    ) => WeatherState;

    /** Day/night cycle, sky colors, celestial lighting, weather, and clouds. */
    export class Atmosphere {
        timeOfDay: number;
        secondsPerHour: number;
        wind: Magnitude3d;
        weatherDelegate: WeatherDelegate;

        enable(): void;
        disable(): void;
        isEnabled(): boolean;
        enableWeather(): void;
        disableWeather(): void;

        getNormalizedTime(): number;
        getSunAngle(): Magnitude3d;
        getMoonAngle(): Magnitude3d;
        getLightIntensity(): number;
        getLightColor(): Color;

        clouds?: Clouds;

        getSkyboxColors(): Color[];
        /** Creates a sky cubemap with the requested face size. */
        createSkyCubemap(size: number): Cubemap;
        /** Updates an existing cubemap with the current sky colors. */
        updateSkyCubemap(cubemap: Cubemap): void;

        castShadowsFromSunlight(resolution: number): void;
        useGlobalLight(): void;

        sunColor: Color;
        moonColor: Color;

        sunSize: number;
        moonSize: number;
        sunTintStrength: number;
        moonTintStrength: number;
        starIntensity: number;

        isDaytime(): boolean;
        /** Sets the atmosphere clock using hours, minutes, and seconds. */
        setTime(hours: number, minutes: number, seconds: number): void;

        /** Creates clouds using the supplied noise frequency and division count. */
        addClouds(frequency: number, numDivisions: number): void;

        cycle: boolean;
        resetRuntimeState(): void;
    }

    /** Renderable water surface with configurable extent, color, wave motion, and textures. */
    export class Fluid extends GameObject {
        waveVelocity: number;

        constructor();
        create(extent: Size2d, color: Color): void;

        /** Translates by the supplied offset. */
        override move(position: Position3d): void;
        /** Sets the position to an absolute value. */
        override setPosition(position: Position3d): void;
        /** Replaces the rotation. */
        override setRotation(rotation: Rotation3d): void;
        /** Applies an incremental rotation. */
        override rotate(rotation: Rotation3d): void;
        /** Replaces the scale factors. */
        override setScale(scale: Scale3d): void;

        setExtent(extent: Size2d): void;
        setWaveVelocity(velocity: number): void;
        setWaterColor(color: Color): void;
        getPosition(): Position3d;
        getScale(): Scale3d;

        normalTexture: Texture;
        movementTexture: Texture;
    }
}
