//
// atlas.d.ts
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Declarations for the Atlas Library for scripting
// Copyright (c) 2026 Max Van den Eynde
//

declare module "atlas/log" {
    export const Debug: {
        print(message: string): void;
        warning(message: string): void;
        error(message: string): void;
    };
}

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

    export type Fog = {
        color: Color;
        intensity: number;
    };

    export type VolumetricLighting = {
        enabled: boolean;
        density: number;
        weight: number;
        decay: number;
        exposure: number;
    };

    export type LightBloomConfiguration = {
        radius: number;
        maxSamples: number;
    };

    export type RimLightingConfiguration = {
        color: Color;
        intensity: number;
    };

    export type Environment = {
        fog: Fog;
        volumetricLighting: VolumetricLighting;
        lightBloom: LightBloomConfiguration;
        rimLighting: RimLightingConfiguration;
        lookupTexture: Texture;
    };

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

        getCamera(): Camera;
        getWindow(): Window;

        atmosphere: Atmosphere;
    }

    export abstract class Component {
        parentId: number;

        abstract init(): void;
        abstract update(deltaTime: number): void;
        beforePhysics(): void;
        atAttach(): void;

        onCollisionEnter(other: GameObject): void;
        onCollisionStay(other: GameObject): void;
        onCollisionExit(other: GameObject): void;
        onSignalReceive(signal: string, sender: GameObject): void;
        onSignalEnd(signal: string, sender: GameObject): void;
        onQueryReceive(query: QueryResult, sender: GameObject): void;

        getParent(): GameObject;
        getParent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;
        getObject(identifier: number | string): CoreObject;
        getScene(): Scene;
        getWindow(): Window;
    }

    export class Material {
        constructor();

        albedo: Color;
        metallic: number;
        roughness: number;
        ao: number;
        reflectivity: number;
        emissiveColor: Color;
        emissiveIntensity: number;
        normalMapStrength: number;
        useNormalMap: boolean;
        transmittance: number;
        ior: number;
    }

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

    export class Instance {
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;

        move(position: Position3d): void;
        setPosition(position: Position3d): void;
        setRotation(rotation: Rotation3d): void;
        rotate(rotation: Rotation3d): void;
        setScale(scale: Scale3d): void;
        scaleBy(scale: Scale3d): void;

        equals(other: Instance): boolean;
    }

    export abstract class GameObject {
        id: number;
        components: Component[];
        position: Position3d;
        rotation: Rotation3d;
        scale: Scale3d;
        name: string;

        constructor();

        attachTexture(texture: Texture): void;
        setPosition(position: Position3d): void;
        move(position: Position3d): void;
        lookAt(target: Position3d, up?: Normal3d): void;
        setRotation(rotation: Rotation3d): void;
        rotate(rotation: Rotation3d): void;
        setScale(scale: Scale3d): void;
        scaleBy(scale: Scale3d): void;
        show(): void;
        hide(): void;

        as<T extends GameObject>(type: new (...args: any[]) => T): T | null;

        addComponent<T extends Component>(component: T): void;
    }

    export abstract class UIObject extends GameObject {
        getSize(): Size2d;
        getScreenPosition(): Position2d;
        abstract setScreenPosition(position: Position2d): void;
    }

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
        attachTexture(texture: Texture): void;

        setPosition(position: Position3d): void;
        move(position: Position3d): void;
        setRotation(rotation: Rotation3d): void;
        setRotationQuaternion(rotation: Quaternion): void;
        rotate(rotation: Rotation3d): void;
        lookAt(target: Position3d, up?: Normal3d): void;
        setScale(scale: Scale3d): void;
        scaleBy(scale: Scale3d): void;

        clone(): CoreObject;

        show(): void;
        hide(): void;

        addComponent<T extends Component>(component: T): void;

        enableDeferredRendering(): void;
        disableDeferredRendering(): void;

        createInstance(): Instance;

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

    export class Model extends GameObject {
        static fromResource(path: string): Model;

        getObjects(): CoreObject[];

        override move(position: Position3d): void;
        override setPosition(position: Position3d): void;
        override setRotation(rotation: Rotation3d): void;
        override lookAt(target: Position3d, up?: Normal3d): void;
        override rotate(rotation: Rotation3d): void;
        override setScale(scale: Scale3d): void;
        override scaleBy(scale: Scale3d): void;

        override show(): void;
        override hide(): void;
        override attachTexture(texture: Texture): void;
    }

    export enum ResourceType {
        File,
        Texture,
        SpecularMap,
        Audio,
        Font,
        Model,
    }

    export class Resource {
        type: ResourceType;
        path: string;
        name: string;

        constructor(type: ResourceType, path: string, name: string);

        static fromAssetPath(
            path: string,
            type: ResourceType,
            name?: string,
        ): Resource;

        static fromName(name: string, type: ResourceType): Resource | null;
    }

    export class ResourceGroup {
        resources: Resource[];
        name: string;

        constructor(resources: Resource[], name: string);

        addResource(resource: Resource): void;
        getResourceByName(name: string): Resource | null;
    }

    export class Camera {
        position: Position3d;
        target: Point3d;
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

        move(offset: Position3d): void;
        setPosition(position: Position3d): void;
        setPositionKeepingOrientation(position: Position3d): void;
        lookAt(target: Point3d, up?: Normal3d): void;
        moveTo(target: Point3d, speed: number): void;
        getDirection(): Normal3d;
    }

    export type ViewInformation = {
        position: Position3d;
        target: Point3d;
        time: number;
        deltaTime: number;
    };

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

    export type VideoMode = {
        width: number;
        height: number;
        refreshRate: number;
    };

    export class Monitor {
        monitorId: number;
        primary: boolean;

        queryVideoModes(): VideoMode[];
        getCurrentVideoMode(): VideoMode;
        getPhysicalSize(): Size2d;
        getPosition(): Position2d;
        getContentScale(): number;
        getName(): string;
    }

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

    export const CONTROLLER_UNDEFINED = -2;

    export class Gamepad {
        controllerId: number;
        name: string;
        connected: boolean;

        getAxisTrigger(axis: ControllerAxis): AxisTrigger;
        static getGlobalAxisTrigger(axis: ControllerAxis): AxisTrigger;
        getButtonTrigger(button: ControllerButton): Trigger;
        static getGlobalButtonTrigger(button: ControllerButton): Trigger;

        runble(strength: number, duration: number): void;
    }

    export type Controller = Gamepad;

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

    export type ControllerID = {
        id: number;
        name: string;
        isJoystick: boolean;
    };

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

        instantiate(object: GameObject): void;
        destroy(object: GameObject): void;

        addUIObject(object: UIObject): void;
        setCamera(camera: Camera): void;
        setScene(scene: Scene): void;
        getTime(): number;
        isKeyActive(key: Key): boolean;

        isMouseButtonActive(button: MouseButton): boolean;
        isMouseButtonPressed(button: MouseButton): boolean;
        getTextInput(): string;
        startTextInput(): void;
        stopTextInput(): void;
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

        releaseMouse(): void;
        captureMouse(): void;
        getCursorPosition(): Position2d;

        main: Window;

        getCurrentScene(): Scene;
        getCamera(): Camera;
        addRenderTarget(): RenderTarget;
        getSize(): Size2d;
        activateDebug(): void;
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

        addInputAction(action: InputAction): void;
        resetInputActions(): void;
        getInputAction(name: string): InputAction | null;

        isActionTriggered(name: string): boolean;
        isActionCurrentlyActive(name: string): boolean;
        getActionAxisValue(name: string): AxisPacket;
    }
}

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
    }

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

    export class Cubemap {
        resources: Resource[];
        id: number;

        constructor(resources: Resource[]);

        getAverageColor(): Color;
        static fromResourceGroup(resourceGroup: ResourceGroup): Cubemap | null;
        updateWithColors(colors: Color[]): void;
    }

    export enum RenderTargetType {
        Scene,
        Multisampled,
        Shadow,
        CubeShadow,
        GBuffer,
        SSAO,
        SSAOBlur,
    }

    export enum RenderPassType {
        Deferred,
        Forward,
        PathTracing,
    }

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

    export type Effect =
        | keyof typeof Effects
        | (typeof Effects)[keyof typeof Effects];

    export class RenderTarget {
        type: RenderTargetType;
        resolution: number;
        outTextures: Texture[];
        depthTexture: Texture | null;

        constructor(type: RenderTargetType, resolution: number);

        addEffect(effect: Effect): void;

        addToPassQueue(type: RenderPassType): void;
        display(): void;
    }

    export class Skybox {
        cubemap: Cubemap;

        constructor(cubemap: Cubemap);
    }

    export class AmbientLight {
        color: Color;
        intensity: number;

        constructor(color?: Color, intensity?: number);
    }

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

        setColor(color: Color): void;
        // Also calls addDebugObject(Window&);
        createDebugObject(): void;
        castShadows(resolution: number): void;
    }

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

        setColor(color: Color): void;
        castShadows(resolution: number): void;
    }

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

        setColor(color: Color): void;
        // Also calls addDebugObject(Window&);
        createDebugObject(): void;
        lookAt(target: Position3d): void;
        castShadows(resolution: number): void;
    }

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
        setColor(color: Color): void;
        setRotation(rotation: Rotation3d): void;
        rotate(delta: Rotation3d): void;
        // Also calls addDebugObject(Window&);
        createDebugObject(): void;
        castShadows(resolution: number): void;
    }
}

declare module "atlas/units" {
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
        static invalid(): Position3d;

        add(other: Position3d | number): Position3d;
        subtract(other: Position3d | number): Position3d;
        multiply(other: Position3d | number): Position3d;
        divide(other: Position3d | number): Position3d;
        is(other: Position3d): boolean;

        normalized(): Position3d;
        toString(): string;
    }

    export type Scale3d = Position3d;
    export type Size3d = Position3d;
    export type Point3d = Position3d;
    export type Normal3d = Position3d;
    export type Magnitude3d = Position3d;
    export type Impulse3d = Position3d;
    export type Force3d = Position3d;
    export type Vector3d = Position3d;
    export type Velocity3d = Position3d;
    export type Rotation3d = Position3d;

    export class BoundingBox {
        min: Position3d;
        max: Position3d;

        constructor(min: Position3d, max: Position3d);

        toString(): string;
        contains(point: Position3d): boolean;
        intersects(other: BoundingBox): boolean;
    }

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

    export class Color {
        r: number;
        g: number;
        b: number;
        a: number;

        constructor(r: number, g: number, b: number, a?: number);

        add(other: Color | number): Color;
        subtract(other: Color | number): Color;
        multiply(other: Color | number): Color;
        divide(other: Color | number): Color;
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

        static fromHex(hex: string): Color;
        static mix(color1: Color, color2: Color, t: number): Color;
    }

    export enum Direction3d {
        Up,
        Down,
        Left,
        Right,
        Forward,
        Backward,
    }

    export class Position2d {
        x: number;
        y: number;

        constructor(x: number, y: number);

        static zero(): Position2d;
        static up(): Position2d;
        static down(): Position2d;
        static left(): Position2d;
        static right(): Position2d;
        static invalid(): Position2d;

        add(other: Position2d | number): Position2d;
        subtract(other: Position2d | number): Position2d;
        multiply(other: Position2d | number): Position2d;
        divide(other: Position2d | number): Position2d;

        is(other: Position2d): boolean;
    }

    export type Scale2d = Position2d;
    export type Point2d = Position2d;
    export type Movement2d = Position2d;
    export type Magnitude2d = Position2d;

    export class Radians {
        value: number;

        constructor(value: number);

        add(other: Radians): Radians;
        subtract(other: Radians): Radians;
        multiply(other: Radians | number): Radians;
        divide(other: Radians | number): Radians;

        toNumber(): number;
        static fromDegrees(degrees: number): Radians;
        toDegrees(): number;
    }

    export class Size2d {
        width: number;
        height: number;

        constructor(width: number, height: number);

        static zero(): Size2d;

        toString(): string;

        add(other: Size2d | number): Size2d;
        subtract(other: Size2d | number): Size2d;
        multiply(other: Size2d | number): Size2d;
        divide(other: Size2d | number): Size2d;

        is(other: Size2d): boolean;
    }
}

declare module "atlas/audio" {
    import { Component, Resource } from "atlas";
    import { Color, Position3d } from "atlas/units";
    import { AudioSource } from "finewave";

    export class AudioPlayer extends Component {
        constructor();

        override init(): void;
        play(): void;
        pause(): void;
        stop(): void;
        setVolume(volume: number): void;
        setLoop(loop: boolean): void;

        setSource(resource: Resource): void;

        override update(dt: number): void;

        setPosition(position: Position3d): void;
        useSpatialAudio(enabled: boolean): void;

        source: AudioSource;
    }
}

declare module "atlas/input" {
    import { Position2d } from "atlas/units";

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

    export enum TriggerType {
        MouseButton,
        Key,
        ControllerButton,
    }

    export type ControllerButtonTrigger = {
        controllerID: number;
        buttonIndex: number;
    };

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

    export enum AxisTriggerType {
        MouseAxis,
        KeyCustom,
        ControllerAxis,
    }

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

        static fromMouse(): AxisTrigger;
        static fromKeys(
            positiveX: Key,
            negativeX: Key,
            positiveY: Key,
            negativeY: Key,
        ): AxisTrigger;
        static fromControllerAxis(
            controllerId: number,
            axisIndex: number,
            single: boolean,
            axisIndexY?: number,
        ): AxisTrigger;
    }

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

    export type MousePacket = {
        xpos: number;
        ypos: number;
        xoffset: number;
        yoffset: number;
        constrainPitch: boolean;
        firstMouse: boolean;
    };

    export type MouseScrollPacket = {
        xoffset: number;
        yoffset: number;
    };

    export class InputAction {
        triggers: Trigger[];
        axisTriggers: AxisTrigger[];
        name: string;
        isAxis: boolean;
        isAxisSingle: boolean;
        normalized: boolean;
        invertY: boolean;

        static createButtonAction(
            name: string,
            triggers: Trigger[],
        ): InputAction;
        static createAxisAction(
            name: string,
            axisTriggers: AxisTrigger[],
        ): InputAction;
        static createSingleAxisAction(
            name: string,
            positiveTrigger: Trigger,
            negativeTrigger: Trigger,
        ): InputAction;
    }

    export const Input: {
        addAction(action: InputAction): InputAction;
        resetActions(): void;

        isKeyActive(key: Key): boolean;
        isKeyPressed(key: Key): boolean;
        isMouseButtonActive(button: MouseButton): boolean;
        isMouseButtonPressed(button: MouseButton): boolean;

        getTextInput(): string;
        startTextInput(): void;
        stopTextInput(): void;
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

        captureMouse(): void;
        releaseMouse(): void;
        getMousePosition(): Position2d;

        isActionTriggered(name: string): boolean;
        isActionCurrentlyActive(name: string): boolean;
        getAxisActionValue(name: string): AxisPacket;
    };

    export abstract class Interactive {
        abstract onKeyPress(key: Key, dt: number): void;
        abstract onKeyRelease(key: Key, dt: number): void;
        abstract onMouseMove(packet: MousePacket, dt: number): void;
        abstract onMouseButtonPress(button: MouseButton, dt: number): void;
        abstract onMouseScroll(packet: MouseScrollPacket, dt: number): void;
        abstract onEachFrame(dt: number): void;
    }
}

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

    export enum ParticleEmissionType {
        Fountain,
        Ambient,
    }

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

    export type Particle = {
        position: Position3d;
        velocity: Magnitude3d;
        color: Color;
        lifetime: number;
        maxLifetime: number;
        size: number;
        active: boolean;
    };

    export class ParticleEmitter extends GameObject {
        settings: ParticleSettings;
        constructor(maxParticles: number);

        override attachTexture(texture: Texture): void;
        setColor(color: Color): void;
        enableTexture(): void;
        disableTexture(): void;
        override setPosition(position: Position3d): void;
        override move(position: Position3d): void;
        getPosition(): Position3d;

        setEmissionType(type: ParticleEmissionType): void;
        setDirection(direction: Magnitude3d): void;
        setSpawnRadius(radius: number): void;
        setSpawnRate(rate: number): void;
        setParticleSettings(settings: ParticleSettings): void;

        emitOnce(): void;
        emitContinuous(): void;
        startEmission(): void;
        stopEmission(): void;
        emitBurst(count: number): void;

        // Disabled
        override setRotation(rotation: Rotation3d): void;
        override setScale(scale: Scale3d): void;
        override lookAt(target: Position3d, up?: Normal3d): void;
        override rotate(rotation: Rotation3d): void;
        override scaleBy(scale: Scale3d): void;
        override show(): void;
        override hide(): void;
    }
}

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

    export type RaycastHit = {
        position: Position3d;
        normal: Normal3d;
        distance: number;
        object: GameObject;
        didHit: boolean;
    };

    export type RaycastResult = {
        hits: RaycastHit[];
        hit: RaycastHit | null;
        closestDistance: number;
    };

    export type OverlapHit = {
        contactPoint: Position3d;
        penerationAxis: Point3d;
        penetrationDepth: number;
        object: GameObject;
    };

    export type OverlapResult = {
        hits: OverlapHit[];
        hitAny: boolean;
    };

    export type SweepHit = {
        position: Position3d;
        normal: Normal3d;
        distance: number;
        percentage: number;
        object: GameObject;
    };

    export type SweepResult = {
        hits: SweepHit[];
        closest: SweepHit | null;
        hitAny: boolean;
        endPosition: Position3d;
    };

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

    export type QueryResult = {
        operation: QueryOperation;
        raycastResult?: RaycastResult;
        overlapResult?: OverlapResult;
        sweepResult?: SweepResult;
    };

    export type WorldBody = {};

    export type JointMember = GameObject | WorldBody;

    export enum SpringMode {
        FrequencyAndDamping,
        StiffnessAndDamping,
    }

    export enum Space {
        Local,
        Global,
    }

    export type Spring = {
        enabled: boolean;
        mode: SpringMode;
        frequency: number;
        dampingRatio: number;
        stiffness: number;
        damping: number;
    };

    export type AngleLimits = {
        enabled: boolean;
        minAngle: number;
        maxAngle: number;
    };

    export type Motor = {
        enabled: boolean;
        maxForce: number;
        maxTorque: number;
    };

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
        abstract breakJoint(): void;
    }

    export class FixedJoint extends Joint {
        override beforePhysics(): void;
        override breakJoint(): void;
    }

    export class HingeJoint extends Joint {
        axis1: Normal3d;
        axis2: Normal3d;
        angleLimits: AngleLimits;
        motor: Motor;

        override beforePhysics(): void;
        override breakJoint(): void;
    }

    export class SpringJoint extends Joint {
        anchorB: Position3d;
        restLength: number;
        useLimits: boolean;
        minLength: number;
        maxLength: number;

        spring: Spring;

        override beforePhysics(): void;
        override breakJoint(): void;
    }

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

    export type VehicleDifferential = {
        leftWheel: number;
        rightWheel: number;
        differentialRatio: number;
        leftRightSplit: number;
        limitedSlipRatio: number;
        engineTorqueRatio: number;
    };

    export type VehicleEngine = {
        maxTorque: number;
        minRPM: number;
        maxRPM: number;
        inertia: number;
        angularDamping: number;
    };

    export enum VehicleTransmissionMode {
        Auto,
        Manual,
    }

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

    export type VehicleControllerSettings = {
        engine: VehicleEngine;
        transmission: VehicleTransmission;
        differentials: VehicleDifferential[];
        differentialLimitedSlipRatio: number;
    };

    export type VehicleSettings = {
        up: Normal3d;
        forward: Normal3d;

        maxPitchRollAngleDeg: number;

        wheels: VehicleWheelSettings[];
        controller: VehicleControllerSettings;

        maxSlopAngleDeg: number;
    };

    export class Vehicle extends Component {
        settings: VehicleSettings;
        forward: number;
        right: number;
        brake: number;
        handBrake: number;

        override atAttach(): void;
        override beforePhysics(): void;

        requestRecreate(): void;

        override init(): void;
        override update(deltaTime: number): void;
    }

    export type CapsuleCollider = {
        radius: number;
        height: number;
    };

    export type BoxCollider = {
        size: Size3d;
    };

    export type SphereCollider = {
        radius: number;
    };

    export type MeshCollider = {};

    export type Collider =
        | CapsuleCollider
        | BoxCollider
        | SphereCollider
        | MeshCollider;

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
        applyForce(force: Force3d): void;
        applyForceAtPoint(force: Force3d, point: Position3d): void;
        applyImpulse(impulse: Impulse3d): void;

        setLinearVelocity(velocity: Velocity3d): void;
        addLinearVelocity(velocity: Velocity3d): void;
        setAngularVelocity(velocity: Velocity3d): void;
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
        setRestituition(restitution: number): void;
        setMotionType(motionType: "Static" | "Dynamic" | "Kinematic"): void;
    }

    export class Sensor extends Rigidbody {
        constructor(); // sets isSensor to true

        setSignal(signal: string): void;
    }
}

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

    export class PerlinNoise {
        constructor(seed?: number);
        noise(x: number, y: number): number;
    }

    export class SimplexNoise {
        static noise(xin: number, yin: number): number;
    }

    export class WorleyNoise {
        constructor(numPoints: number, seed?: number);
        noise(x: number, y: number): number;
    }

    export class FractalNoise {
        constructor(o: number, p: number);
        noise(x: number, y: number): number;
    }

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

    export class Biome {
        name: string;
        texture: Texture;
        color: Color;
        useTexture: boolean;

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

    export type BiomeFunction = (biome: Biome) => void;

    export class Terrain extends GameObject {
        attachTexture(texture: Texture): void;
        setPosition(position: Position3d): void;
        move(position: Position3d): void;
        setRotation(rotation: Rotation3d): void;
        lookAt(target: Position3d, up?: Normal3d): void;
        rotate(rotation: Rotation3d): void;
        setScale(scale: Scale3d): void;
        scaleBy(scale: Scale3d): void;
        show(): void;
        hide(): void;

        heightmap: Resource;
        moistureTexture: Texture;
        temperatureTexture: Texture;
        generator: TerrainGenerator;

        createdWithMap: boolean;
        width: number;
        length: number;
        height: number;

        addBiome(biome: Biome): void;

        static fromGenerator<T extends TerrainGenerator>(generator: T): Terrain;
        static fromHeightmap(heightmap: Resource): Terrain;

        maxPeak: number;
        seaLevel: number;
    }

    export abstract class TerrainGenerator {
        abstract generateHeight(x: number, y: number): number;
        applyTo(terrain: Terrain): void;
    }

    export class HillGenerator extends TerrainGenerator {
        constructor(scale: number, amplitude: number);

        override generateHeight(x: number, y: number): number;
    }

    export class MountainGenerator extends TerrainGenerator {
        constructor(
            scale: number,
            amplitude: number,
            octaves: number,
            persistance: number,
        );

        override generateHeight(x: number, y: number): number;
    }

    export class PlainGenerator extends TerrainGenerator {
        constructor(scale: number, amplitude: number);

        override generateHeight(x: number, y: number): number;
    }

    export class IslandGenerator extends TerrainGenerator {
        constructor(numFeatures: number, scale: number);

        override generateHeight(x: number, y: number): number;
    }

    export class CompoundGenerator extends TerrainGenerator {
        addGenerator<T extends TerrainGenerator>(generator: T): void;
        override generateHeight(x: number, y: number): number;
    }
}

declare module "finewave" {
    import { Position3d } from "atlas/units";
    import { Resource } from "atlas";

    export class AudioEngine {
        setListenerPosition(position: Position3d): void;
        setListenerOrientation(forward: Position3d, up: Position3d): void;
        setListenerVelocity(velocity: Position3d): void;
        setMasterVolume(volume: number): void;
        deviceName: string;
    }

    export class AudioData {
        static fromResource(resource: Resource): AudioData;
        isMono: boolean;
        resource: Resource;
    }

    export class AudioSource {
        setData(data: AudioData): void;
        fromFile(resource: Resource): void;
        play(): void;
        pause(): void;
        stop(): void;
        setLoop(loop: boolean): void;
        setVolume(volume: number): void;
        setPitch(pitch: number): void;
        setPosition(position: Position3d): void;
        setVelocity(velocity: Position3d): void;

        isPlaying(): boolean;
        playFrom(position: number): void;
        disableSpatialization(): void;
        applyEffect(effect: AudioEffect): void;
        getPosition(): Position3d;
        getListenerPosition(): Position3d;
        useSpatialization(): void;
    }

    export abstract class AudioEffect {}

    export class Reverb extends AudioEffect {
        setRoomSize(size: number): void;
        setDamping(damping: number): void;
        setWetLevel(level: number): void;
        setDryLevel(level: number): void;
        setWidth(width: number): void;
    }

    export class Echo extends AudioEffect {
        setDelay(delay: number): void;
        setDecay(decay: number): void;
        setWetLevel(level: number): void;
        setDryLevel(level: number): void;
    }

    export class Distortion extends AudioEffect {
        setEdge(edge: number): void;
        setGain(gain: number): void;
        setLowpassCutoff(cutoff: number): void;
    }
}

declare module "graphite" {
    import { UIObject, Resource } from "atlas";
    import { Texture } from "atlas/graphics";
    import { Position2d, Position3d, Color, Size2d, Size3d } from "atlas/units";

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

        override getSize(): Size2d;
        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        style(): UIStyle;
        setStyle(style: UIStyle): Image;

        setTexture(texture: Texture): void;
        setSize(size: Size2d): void;
    }

    export type TextFieldChangeEvent = {
        text: string;
        cursorPosition: number;
        focused: boolean;
    };

    export type ButtonClickEvent = {
        label: string;
    };

    export type CheckboxToggleEvent = {
        label: string;
        checked: boolean;
    };

    export namespace TextField {
        export type ChangeCallback = (event: TextFieldChangeEvent) => void;
    }

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

        override getSize(): Size2d;
        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        getText(): string;
        isFocused(): boolean;
        getCursorIndex(): number;
        style(): UIStyle;

        setText(text: string): TextField;
        setPlaceholder(placeholder: string): TextField;
        setPadding(padding: Size2d): TextField;
        setMaximumWidth(width: number): TextField;
        setFontSize(size: number): TextField;
        setStyle(style: UIStyle): TextField;
        setOnChange(callback: TextField.ChangeCallback): TextField;

        focus(): void;
        blur(): void;
    }

    export namespace Button {
        export type ClickCallback = (event: ButtonClickEvent) => void;
    }

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

        override getSize(): Size2d;
        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        getLabel(): string;
        isHovered(): boolean;
        isEnabled(): boolean;

        style(): UIStyle;

        setLabel(label: string): Button;
        setPadding(padding: Size2d): Button;
        setMinimumSize(size: Size2d): Button;
        setFontSize(size: number): Button;
        setStyle(style: UIStyle): Button;
        setOnClick(callback: Button.ClickCallback): Button;
        setEnabled(enabled: boolean): void;
    }

    export namespace Checkbox {
        export type ToggleCallback = (event: CheckboxToggleEvent) => void;
    }

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

        override getSize(): Size2d;
        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        getLabel(): string;
        isChecked(): boolean;
        isHovered(): boolean;
        isEnabled(): boolean;

        style(): UIStyle;

        setLabel(label: string): Checkbox;
        setPadding(padding: Size2d): Checkbox;
        setFontSize(size: number): Checkbox;
        setBoxSize(size: number): Checkbox;
        setSpacing(spacing: number): Checkbox;
        setStyle(style: UIStyle): Checkbox;
        setOnToggle(callback: Checkbox.ToggleCallback): Checkbox;
        setChecked(checked: boolean): void;
        setEnabled(enabled: boolean): void;
        toggle(): void;
    }

    export enum ElementAlignment {
        Top,
        Center,
        Bottom,
    }

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

        addChild(child: UIObject): void;
        setChildren(children: UIObject[]): void;

        override getSize(): Size2d;

        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        style: UIStyle;
        setStyle(style: UIStyle): Column;
    }

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

        addChild(child: UIObject): void;
        setChildren(children: UIObject[]): void;

        override getSize(): Size2d;

        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        style: UIStyle;
        setStyle(style: UIStyle): Column;
    }

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

        addChild(child: UIObject): void;
        setChildren(children: UIObject[]): void;

        override getSize(): Size2d;

        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        style: UIStyle;
        setStyle(style: UIStyle): Column;
    }

    export enum UIStyleState {
        Normal,
        Hovered,
        Pressed,
        Disabled,
        Focused,
        Checked,
    }

    export type UIStyleStateSnapshot = {
        hovered: boolean;
        pressed: boolean;
        disabled: boolean;
        focused: boolean;
        checked: boolean;
    };

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

    export class UIStyle {
        normal(): UIStyleVariant;
        hovered(): UIStyleVariant;
        pressed(): UIStyleVariant;
        disabled(): UIStyleVariant;
        focused(): UIStyleVariant;
        checked(): UIStyleVariant;
        variant(state: UIStyleState): UIStyleVariant;
    }

    export class Theme {
        text: UIStyle;
        image: UIStyle;
        textField: UIStyle;
        button: UIStyle;
        checkbox: UIStyle;
        row: UIStyle;
        column: UIStyle;
        stack: UIStyle;

        static current(): Theme;
        static set(theme: Theme): void;
        static reset(): void;
    }

    export type Character = {
        size: Size2d;
        bearing: Position2d;
        advance: number;
        uvMin: Position2d;
        uvMax: Position2d;
    };

    export type FontAtlas = Map<string, Character>;

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

        override getSize(): Size2d;
        override getScreenPosition(): Position2d;
        override setScreenPosition(position: Position2d): void;

        style(): UIStyle;
        setStyle(style: UIStyle): Text;
        setFontSize(size: number): Text;
    }
}

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

    export class WorleyNoise3D {
        constructor(frequency: number, numDivisions: number);

        getValue(x: number, y: number, z: number): number;

        get3dTexture(size: number): number;
        getDetailTexture(size: number): number;
        get3dTextureAtAllChannels(size: number): number;
    }

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

    export enum WeatherCondition {
        Clear,
        Rain,
        Snow,
        Storm,
    }

    export type WeatherState = {
        condition: WeatherCondition;
        intensity: number;
        wind: Force3d;
    };

    export type WeatherDelegate = (
        information: ViewInformation,
    ) => WeatherState;

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
        createSkyCubemap(size: number): Cubemap;
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
        setTime(hours: number, minutes: number, seconds: number): void;

        addClouds(frequency: number, numDivisions: number): void;

        cycle: boolean;
        resetRuntimeState(): void;
    }

    export class Fluid extends GameObject {
        waveVelocity: number;

        constructor();
        create(extent: Size2d, color: Color): void;

        override move(position: Position3d): void;
        override setPosition(position: Position3d): void;
        override setRotation(rotation: Rotation3d): void;
        override rotate(rotation: Rotation3d): void;
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
