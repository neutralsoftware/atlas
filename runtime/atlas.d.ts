//
// atlas.d.ts
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Declarations for the Atlas Library for scripting
// Copyright (c) 2026 Max Van den Eynde
//

/**
 * Runtime logging.
 */
declare module "atlas/log" {
    /**
     * Writes messages to the Atlas runtime log at the selected severity.
     */
    export const Debug: {
        /**
         * Writes an informational message to the Atlas log.
         *
         * @param message - The message to write.
         */
        print(message: string): void;
        /**
         * Writes a warning message to the Atlas log.
         *
         * @param message - The message to write.
         */
        warning(message: string): void;
        /**
         * Writes an error message to the Atlas log.
         *
         * @param message - The message to write.
         */
        error(message: string): void;
    };
}

/**
 * Scene, object, component, resource, camera, and window APIs.
 */
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

    /**
     * Fog color and intensity used by the scene environment.
     */
    export type Fog = {
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The effect intensity.
         */
        intensity: number;
    };

    /**
     * Controls volumetric light scattering and its accumulation parameters.
     */
    export type VolumetricLighting = {
        /**
         * Whether this feature is enabled.
         */
        enabled: boolean;
        /**
         * The density.
         */
        density: number;
        /**
         * The weight.
         */
        weight: number;
        /**
         * The decay.
         */
        decay: number;
        /**
         * The exposure.
         */
        exposure: number;
    };

    /**
     * Brightness threshold, filter radius, and sample budget for light bloom.
     */
    export type LightBloomConfiguration = {
        /**
         * The threshold.
         */
        threshold: number;
        /**
         * The radius.
         */
        radius: number;
        /**
         * The maximum samples.
         */
        maxSamples: number;
    };

    /**
     * Color and strength of the rim-lighting effect.
     */
    export type RimLightingConfiguration = {
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The effect intensity.
         */
        intensity: number;
    };

    /**
     * Scene-wide fog, lighting effects, and color lookup texture.
     */
    export type Environment = {
        /**
         * The fog.
         */
        fog: Fog;
        /**
         * The volumetric lighting.
         */
        volumetricLighting: VolumetricLighting;
        /**
         * The light bloom.
         */
        lightBloom: LightBloomConfiguration;
        /**
         * The rim lighting.
         */
        rimLighting: RimLightingConfiguration;
        /**
         * The lookup texture.
         */
        lookupTexture: Texture;
    };

    /**
     * Access to the active scene, its lighting, atmosphere, camera, and window.
     */
    export class Scene {
        /**
         * The human-readable name.
         */
        name: string;

        /**
         * Sets ambient intensity.
         *
         * @param intensity - The effect intensity.
         */
        setAmbientIntensity(intensity: number): void;
        /**
         * Sets automatic ambient.
         *
         * @param enabled - Whether to enable the feature.
         */
        setAutomaticAmbient(enabled: boolean): void;

        /**
         * Sets skybox.
         *
         * @param skybox - The skybox to apply.
         */
        setSkybox(skybox: Skybox): void;
        /**
         * Configures the use of atmosphere skybox.
         *
         * @param enabled - Whether to enable the feature.
         */
        useAtmosphereSkybox(enabled: boolean): void;

        /**
         * Sets environment.
         *
         * @param environment - The complete environment configuration.
         */
        setEnvironment(environment: Environment): void;

        /**
         * Sets ambient color.
         *
         * @param color - The color to apply.
         */
        setAmbientColor(color: Color): void;
        /**
         * Sets ambient intensity.
         *
         * @param intensity - The effect intensity.
         */
        setAmbientIntensity(intensity: number): void;
        /**
         * Adds directional light.
         *
         * @param light - The light to add.
         */
        addDirectionalLight(light: DirectionalLight): void;
        /**
         * Adds light.
         *
         * @param light - The light to add.
         */
        addLight(light: Light): void;
        /**
         * Adds spot light.
         *
         * @param light - The light to add.
         */
        addSpotLight(light: SpotLight): void;
        /**
         * Adds area light.
         *
         * @param light - The light to add.
         */
        addAreaLight(light: AreaLight): void;

        /**
         * Returns the active camera.
         *
         * @returns The camera.
         */
        getCamera(): Camera;
        /**
         * Returns the active runtime window.
         *
         * @returns The window.
         */
        getWindow(): Window;

        /**
         * The scene atmosphere controller.
         */
        atmosphere: Atmosphere;
    }

    /**
     * Base class for behavior attached to a GameObject. Override lifecycle and event hooks to implement scripts.
     *
     * @example
     * ```ts
     * import { Component } from "atlas";
     * import { Position3d } from "atlas/units";
     * class Mover extends Component {
     *     init(): void {}
     *     update(deltaTime: number): void {
     *         this.getParent().move(new Position3d(deltaTime, 0, 0));
     *     }
     * }
     * ```
     */
    export abstract class Component {
        /**
         * The runtime identifier of the parent object.
         */
        parentId: number;

        /**
         * Initialization hook for attached script behavior.
         */
        abstract init(): void;
        /**
         * Per-frame update hook.
         *
         * @param deltaTime - Elapsed time since the previous frame, in seconds.
         */
        abstract update(deltaTime: number): void;
        /**
         * Hook called before physics simulation.
         */
        beforePhysics(): void;
        /**
         * Hook called when the component is attached to its parent.
         */
        atAttach(): void;

        /**
         * Called when contact with another object begins.
         *
         * @param other - The other object involved in the collision.
         */
        onCollisionEnter(other: GameObject): void;
        /**
         * Called while contact with another object continues.
         *
         * @param other - The other object involved in the collision.
         */
        onCollisionStay(other: GameObject): void;
        /**
         * Called when contact with another object ends.
         *
         * @param other - The other object involved in the collision.
         */
        onCollisionExit(other: GameObject): void;
        /**
         * Declared signal callback. The current runtime dispatches the legacy spelling `onSignalRecieve`.
         *
         * @param signal - The signal name.
         * @param sender - The object that sent the signal or query.
         */
        onSignalReceive(signal: string, sender: GameObject): void;
        /**
         * Called when a signal from the sending object ends.
         *
         * @param signal - The signal name.
         * @param sender - The object that sent the signal or query.
         */
        onSignalEnd(signal: string, sender: GameObject): void;
        /**
         * Receives a physics query result and the object owning this component.
         *
         * @param query - The physics query result.
         * @param sender - The object that sent the signal or query.
         */
        onQueryReceive(query: QueryResult, sender: GameObject): void;

        /**
         * Returns the parent object.
         *
         * @returns The parent object or matching parent component.
         */
        getParent(): GameObject;
        /**
         * Returns a matching component on the parent, or null if none is attached.
         *
         * @param type - The type or constructor used for the operation.
         * @returns The parent object or matching parent component.
         */
        getParent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;
        /**
         * Looks up a scene object by numeric ID or string name.
         *
         * @param identifier - The numeric object ID or string object name.
         * @returns The matching scene object.
         */
        getObject(identifier: number | string): CoreObject;
        /**
         * Returns the active scene.
         *
         * @returns The scene.
         */
        getScene(): Scene;
        /**
         * Returns the active runtime window.
         *
         * @returns The window.
         */
        getWindow(): Window;
    }

    /**
     * Physically based surface properties. Defaults include white albedo, metallic 0, roughness 0.5, and AO 1.
     */
    export class Material {
        /**
         * Creates a new material.
         */
        constructor();

        /**
         * The albedo.
         */
        albedo: Color;
        /**
         * The metallic.
         *
         * @defaultValue 0
         */
        metallic: number;
        /**
         * The roughness.
         *
         * @defaultValue 0.5
         */
        roughness: number;
        /**
         * Ambient occlusion factor; defaults to 1.
         *
         * @defaultValue 1
         */
        ao: number;
        /**
         * The reflectivity.
         */
        reflectivity: number;
        /**
         * The emissive color.
         */
        emissiveColor: Color;
        /**
         * The emissive intensity.
         */
        emissiveIntensity: number;
        /**
         * The normal map strength.
         */
        normalMapStrength: number;
        /**
         * Whether normal-map shading is enabled.
         */
        useNormalMap: boolean;
        /**
         * The transmittance.
         */
        transmittance: number;
        /**
         * Index of refraction; defaults to 1.
         *
         * @defaultValue 1
         */
        ior: number;
    }

    /**
     * Mesh vertex with position, color, UV coordinates, and a tangent-space basis.
     */
    export class CoreVertex {
        /**
         * Creates a new core vertex.
         *
         * @param position - The vertex position. Defaults to `Position3d.zero()`.
         * @param color - The vertex color. Defaults to `Color.white()`.
         * @param textureCoord - The UV coordinate. Defaults to `Position2d.zero()`.
         * @param normal - The surface normal. Defaults to `Position3d.zero()`.
         * @param tangent - The tangent vector. Defaults to `Position3d.zero()`.
         * @param bitangent - The bitangent vector. Defaults to `Position3d.zero()`.
         */
        constructor(
            position?: Position3d,
            color?: Color,
            textureCoord?: Position2d,
            normal?: Normal3d,
            tangent?: Normal3d,
            bitangent?: Normal3d,
        );

        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The texture coord.
         */
        textureCoord: Position2d;
        /**
         * The normalized surface direction.
         */
        normal: Normal3d;
        /**
         * The tangent.
         */
        tangent: Normal3d;
        /**
         * The bitangent.
         */
        bitangent: Normal3d;
    }

    /**
     * A transform for an instance of a CoreObject. Transform methods commit changes to the runtime.
     */
    export class Instance {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The Euler rotation in degrees.
         */
        rotation: Rotation3d;
        /**
         * The scale along each axis.
         */
        scale: Scale3d;

        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        move(position: Position3d): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        setRotation(rotation: Rotation3d): void;
        /**
         * Applies an incremental rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        rotate(rotation: Rotation3d): void;
        /**
         * Replaces the scale factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        setScale(scale: Scale3d): void;
        /**
         * Multiplies the current scale component-wise by the supplied factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        scaleBy(scale: Scale3d): void;

        /**
         * Equals.
         *
         * @param other - The value to compare against.
         * @returns `true` when the values are equal; otherwise `false`.
         */
        equals(other: Instance): boolean;
    }

    /**
     * Base class for scene objects with a transform, visibility, and attached components.
     */
    export abstract class GameObject {
        /**
         * The runtime identifier for this object.
         */
        id: number;
        /**
         * The components attached to this object.
         */
        components: Component[];
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The Euler rotation in degrees.
         */
        rotation: Rotation3d;
        /**
         * The scale along each axis.
         */
        scale: Scale3d;
        /**
         * The human-readable name.
         */
        name: string;

        /**
         * Creates a new game object.
         */
        constructor();

        /**
         * Attaches a texture to this object.
         *
         * @param texture - The texture to use.
         */
        attachTexture(texture: Texture): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        move(position: Position3d): void;
        /**
         * Orients toward the target position.
         *
         * @param target - The target position.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        lookAt(target: Position3d, up?: Normal3d): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        setRotation(rotation: Rotation3d): void;
        /**
         * Applies an incremental rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        rotate(rotation: Rotation3d): void;
        /**
         * Replaces the scale factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        setScale(scale: Scale3d): void;
        /**
         * Multiplies the current scale component-wise by the supplied factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        scaleBy(scale: Scale3d): void;
        /**
         * Makes the object visible.
         */
        show(): void;
        /**
         * Makes the object invisible.
         */
        hide(): void;

        /**
         * As.
         *
         * @param type - The type or constructor used for the operation.
         * @returns The requested as, or `null` when unavailable.
         */
        as<T extends GameObject>(type: new (...args: any[]) => T): T | null;

        /**
         * Attaches a component to this object.
         *
         * @param component - The component to attach.
         */
        addComponent<T extends Component>(component: T): void;
    }

    /**
     * Base class for UI elements with screen-space positioning and measured size.
     */
    export abstract class UIObject extends GameObject {
        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        getSize(): Size2d;
        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        abstract setScreenPosition(position: Position2d): void;
    }

    /**
     * Renderable mesh with vertices, indices, textures, material, and optional instances.
     */
    export class CoreObject extends GameObject {
        /**
         * The mesh vertices.
         */
        vertices: CoreVertex[];
        /**
         * The mesh indices defining its primitives.
         */
        indices: number[];
        /**
         * The textures attached to this object.
         */
        textures: Texture[];
        /**
         * The material used to shade this object.
         */
        material: Material;
        /**
         * The instances of this mesh.
         */
        instances: Instance[];
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The Euler rotation in degrees.
         */
        rotation: Rotation3d;
        /**
         * The scale along each axis.
         */
        scale: Scale3d;
        /**
         * Whether this object contributes to shadow maps.
         */
        castsShadows: boolean;
        /**
         * The human-readable name.
         */
        name: string;

        /**
         * Creates a new core object.
         */
        constructor();

        /**
         * Make emissive.
         *
         * @param color - The color to apply.
         * @param intensity - The effect intensity.
         */
        makeEmissive(color: Color, intensity: number): void;
        /**
         * Attach vertices.
         *
         * @param vertices - The collection of vertices.
         */
        attachVertices(vertices: CoreVertex[]): void;
        /**
         * Attach indices.
         *
         * @param indices - The collection of indices.
         */
        attachIndices(indices: number[]): void;
        /**
         * Attaches a texture to this object.
         *
         * @param texture - The texture to use.
         */
        attachTexture(texture: Texture): void;

        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        move(position: Position3d): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        setRotation(rotation: Rotation3d): void;
        /**
         * Sets rotation quaternion.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        setRotationQuaternion(rotation: Quaternion): void;
        /**
         * Applies an incremental rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        rotate(rotation: Rotation3d): void;
        /**
         * Orients toward the target position.
         *
         * @param target - The target position.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        lookAt(target: Position3d, up?: Normal3d): void;
        /**
         * Replaces the scale factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        setScale(scale: Scale3d): void;
        /**
         * Multiplies the current scale component-wise by the supplied factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        scaleBy(scale: Scale3d): void;

        /**
         * Creates a copy of this mesh object.
         *
         * @returns The newly created value.
         */
        clone(): CoreObject;

        /**
         * Makes the object visible.
         */
        show(): void;
        /**
         * Makes the object invisible.
         */
        hide(): void;

        /**
         * Attaches a component to this object.
         *
         * @param component - The component to attach.
         */
        addComponent<T extends Component>(component: T): void;

        /**
         * Enables deferred rendering.
         */
        enableDeferredRendering(): void;
        /**
         * Disables deferred rendering.
         */
        disableDeferredRendering(): void;

        /**
         * Creates an additional instance of this mesh and returns its transform handle.
         *
         * @returns The newly created value.
         */
        createInstance(): Instance;

        /**
         * Returns the component matching the supplied constructor, or null if none is attached.
         *
         * @param type - The type or constructor used for the operation.
         * @returns The matching component, or `null` when none is attached.
         */
        getComponent<T extends Component>(
            type: new (...args: any[]) => T,
        ): T | null;

        /**
         * Box.
         *
         * @param size - The size or dimensions.
         * @returns The newly created mesh object.
         */
        static box(size: Size3d): CoreObject;
        /**
         * Plane.
         *
         * @param size - The size or dimensions.
         * @returns The newly created mesh object.
         */
        static plane(size: Size2d): CoreObject;
        /**
         * Pyramid.
         *
         * @param size - The size or dimensions.
         * @returns The newly created mesh object.
         */
        static pyramid(size: Size3d): CoreObject;
        /**
         * Sphere.
         *
         * @param radius - The radius.
         * @param sectorCount - The number of longitudinal sphere segments.
         * @param stackCount - The number of latitudinal sphere segments.
         * @returns The newly created mesh object.
         */
        static sphere(
            radius: number,
            sectorCount: number,
            stackCount: number,
        ): CoreObject;
    }

    /**
     * A resource-backed model containing multiple CoreObjects.
     */
    export class Model extends GameObject {
        /**
         * Creates a value from resource.
         *
         * @param path - The asset path.
         * @returns The newly created value.
         */
        static fromResource(path: string): Model;

        /**
         * Returns the mesh objects that make up the model.
         *
         * @returns The objects.
         */
        getObjects(): CoreObject[];

        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        override move(position: Position3d): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        override setPosition(position: Position3d): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        override setRotation(rotation: Rotation3d): void;
        /**
         * Orients toward the target position.
         *
         * @param target - The target position.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        override lookAt(target: Position3d, up?: Normal3d): void;
        /**
         * Applies an incremental rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        override rotate(rotation: Rotation3d): void;
        /**
         * Replaces the scale factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        override setScale(scale: Scale3d): void;
        /**
         * Multiplies the current scale component-wise by the supplied factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        override scaleBy(scale: Scale3d): void;

        /**
         * Makes the object visible.
         */
        override show(): void;
        /**
         * Makes the object invisible.
         */
        override hide(): void;
        /**
         * Attaches a texture to this object.
         *
         * @param texture - The texture to use.
         */
        override attachTexture(texture: Texture): void;
    }

    /**
     * Asset category used when loading or looking up a resource.
     */
    export enum ResourceType {
        /**
         * Selects file for resource type.
         */
        File,
        /**
         * Selects texture for resource type.
         */
        Texture,
        /**
         * Selects specular map for resource type.
         */
        SpecularMap,
        /**
         * Selects audio for resource type.
         */
        Audio,
        /**
         * Selects font for resource type.
         */
        Font,
        /**
         * Selects model for resource type.
         */
        Model,
    }

    /**
     * Reference to a named asset and its resource type.
     */
    export class Resource {
        /**
         * The category or discriminator for this value.
         */
        type: ResourceType;
        /**
         * The path.
         */
        path: string;
        /**
         * The human-readable name.
         */
        name: string;

        /**
         * Creates a new resource.
         *
         * @param type - The type or constructor used for the operation.
         * @param path - The asset path.
         * @param name - The name used to identify the value.
         */
        constructor(type: ResourceType, path: string, name: string);

        /**
         * Creates a resource reference from an asset path, type, and optional name.
         *
         * @param path - The asset path.
         * @param type - The type or constructor used for the operation.
         * @param name - The name used to identify the value.
         * @returns The newly created value.
         */
        static fromAssetPath(
            path: string,
            type: ResourceType,
            name?: string,
        ): Resource;

        /**
         * Looks up a resource by name and type; returns null if no matching resource is found.
         *
         * @param name - The name used to identify the value.
         * @param type - The type or constructor used for the operation.
         * @returns The requested name, or `null` when unavailable.
         */
        static fromName(name: string, type: ResourceType): Resource | null;
    }

    /**
     * Named collection of resources with lookup by resource name.
     */
    export class ResourceGroup {
        /**
         * The resources in this group.
         */
        resources: Resource[];
        /**
         * The human-readable name.
         */
        name: string;

        /**
         * Creates a new resource group.
         *
         * @param resources - The resources to include.
         * @param name - The name used to identify the value.
         */
        constructor(resources: Resource[], name: string);

        /**
         * Adds resource.
         *
         * @param resource - The resource to use.
         */
        addResource(resource: Resource): void;
        /**
         * Returns the first resource with this name, or null if none exists.
         *
         * @param name - The name used to identify the value.
         * @returns The requested resource by name, or `null` when unavailable.
         */
        getResourceByName(name: string): Resource | null;
    }

    /**
     * Active camera controls for projection, orientation, movement, and depth of field.
     */
    export class Camera {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The point toward which the camera is oriented.
         */
        target: Point3d;
        /**
         * Perspective field of view in degrees.
         *
         * @defaultValue 45 degrees
         */
        fov: number;
        /**
         * The near clip.
         *
         * @defaultValue 0.5
         */
        nearClip: number;
        /**
         * The far clip.
         *
         * @defaultValue 1000
         */
        farClip: number;
        /**
         * The orthographic size.
         */
        orthographicSize: number;
        /**
         * The movement speed.
         */
        movementSpeed: number;
        /**
         * The mouse sensitivity.
         */
        mouseSensitivity: number;
        /**
         * The controller look sensitivity.
         */
        controllerLookSensitivity: number;
        /**
         * The look smoothness.
         */
        lookSmoothness: number;
        /**
         * Whether the camera uses orthographic projection.
         */
        useOrthographic: boolean;
        /**
         * The focus depth.
         */
        focusDepth: number;
        /**
         * The focus range.
         */
        focusRange: number;

        /**
         * Creates a new camera.
         */
        constructor();

        /**
         * Translates by the supplied offset.
         *
         * @param offset - The translation offset.
         */
        move(offset: Position3d): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Moves the camera while preserving its viewing orientation.
         *
         * @param position - The position or translation offset.
         */
        setPositionKeepingOrientation(position: Position3d): void;
        /**
         * Orients toward the target position.
         *
         * @param target - The target position.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        lookAt(target: Point3d, up?: Normal3d): void;
        /**
         * Move to.
         *
         * @param target - The target position.
         * @param speed - The movement speed.
         */
        moveTo(target: Point3d, speed: number): void;
        /**
         * Returns direction.
         *
         * @returns The direction.
         */
        getDirection(): Normal3d;
    }

    /**
     * Camera position, target, and timing information supplied to frame-dependent delegates.
     */
    export type ViewInformation = {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The point toward which the camera is oriented.
         */
        target: Point3d;
        /**
         * The elapsed runtime time, in seconds.
         */
        time: number;
        /**
         * The elapsed time since the previous frame, in seconds.
         */
        deltaTime: number;
    };

    /**
     * Window geometry, presentation flags, and rendering configuration for windowed mode.
     */
    export type WindowConfiguration = {
        /**
         * The title.
         */
        title: string;
        /**
         * The width.
         */
        width: number;
        /**
         * The height.
         */
        height: number;
        /**
         * The render scale.
         */
        renderScale: number;
        /**
         * Whether the mouse starts captured by the window.
         */
        mouseCaptured: boolean;
        /**
         * The position X.
         */
        posX: number;
        /**
         * The position Y.
         */
        posY: number;
        /**
         * Whether multisample antialiasing is enabled.
         */
        multisampling: boolean;
        /**
         * Whether built-in editor camera controls are enabled.
         */
        editorControls: boolean;
        /**
         * Whether the operating-system window decorations are shown.
         */
        decorations: boolean;
        /**
         * Whether the user can resize the window.
         */
        resizable: boolean;
        /**
         * Whether the window framebuffer supports transparency.
         */
        transparent: boolean;
        /**
         * Whether the window remains above other windows.
         */
        alwaysOnTop: boolean;
        /**
         * The opacity.
         */
        opacity: number;
        /**
         * The aspect ratio X.
         */
        aspectRatioX: number;
        /**
         * The aspect ratio Y.
         */
        aspectRatioY: number;
        /**
         * The screen-space ambient occlusion scale.
         */
        ssaoScale: number;
    };

    /**
     * Monitor resolution in pixels and refresh rate in hertz.
     */
    export type VideoMode = {
        /**
         * The width.
         */
        width: number;
        /**
         * The height.
         */
        height: number;
        /**
         * The display refresh rate in hertz.
         */
        refreshRate: number;
    };

    /**
     * Connected display information and supported video modes.
     */
    export class Monitor {
        /**
         * The monitor identifier.
         */
        monitorId: number;
        /**
         * Whether this is the primary monitor.
         */
        primary: boolean;

        /**
         * Query video modes.
         *
         * @returns The video modes.
         */
        queryVideoModes(): VideoMode[];
        /**
         * Returns current video mode.
         *
         * @returns The current video mode.
         */
        getCurrentVideoMode(): VideoMode;
        /**
         * Returns the physical display dimensions in millimeters.
         *
         * @returns The physical size.
         */
        getPhysicalSize(): Size2d;
        /**
         * Returns the current position.
         *
         * @returns The position.
         */
        getPosition(): Position2d;
        /**
         * Returns content scale.
         *
         * @returns The content scale.
         */
        getContentScale(): number;
        /**
         * Returns name.
         *
         * @returns The name.
         */
        getName(): string;
    }

    /**
     * Logical gamepad axes, including paired sticks and individual axis selections.
     */
    export enum ControllerAxis {
        /**
         * Selects left stick for controller axis.
         */
        LeftStick,
        /**
         * Selects left stick X for controller axis.
         */
        LeftStickX,
        /**
         * Selects left stick Y for controller axis.
         */
        LeftStickY,
        /**
         * Selects right stick for controller axis.
         */
        RightStick,
        /**
         * Selects right stick X for controller axis.
         */
        RightStickX,
        /**
         * Selects right stick Y for controller axis.
         */
        RightStickY,
        /**
         * Selects trigger for controller axis.
         */
        Trigger,
        /**
         * Selects trigger left for controller axis.
         */
        TriggerLeft,
        /**
         * Selects trigger right for controller axis.
         */
        TriggerRight,
    }

    /**
     * Standard gamepad button identifiers.
     */
    export enum ControllerButton {
        /**
         * Identifies the a gamepad button.
         */
        A = 0,
        /**
         * Identifies the b gamepad button.
         */
        B,
        /**
         * Identifies the X gamepad button.
         */
        X,
        /**
         * Identifies the Y gamepad button.
         */
        Y,
        /**
         * Identifies the left bumper gamepad button.
         */
        LeftBumper,
        /**
         * Identifies the right bumper gamepad button.
         */
        RightBumper,
        /**
         * Identifies the back gamepad button.
         */
        Back,
        /**
         * Identifies the start gamepad button.
         */
        Start,
        /**
         * Identifies the guide gamepad button.
         */
        Guide,
        /**
         * Identifies the left thumb gamepad button.
         */
        LeftThumb,
        /**
         * Identifies the right thumb gamepad button.
         */
        RightThumb,
        /**
         * Identifies the d pad up gamepad button.
         */
        DPadUp,
        /**
         * Identifies the d pad right gamepad button.
         */
        DPadRight,
        /**
         * Identifies the d pad down gamepad button.
         */
        DPadDown,
        /**
         * Identifies the d pad left gamepad button.
         */
        DPadLeft,
        /**
         * Identifies the button count gamepad button.
         */
        ButtonCount,
    }

    /**
     * Button identifiers using Nintendo controller labels.
     */
    export enum NintendoControllerButton {
        /**
         * Identifies the Nintendo b button.
         */
        B = 0,
        /**
         * Identifies the Nintendo a button.
         */
        A,
        /**
         * Identifies the Nintendo Y button.
         */
        Y,
        /**
         * Identifies the Nintendo X button.
         */
        X,
        /**
         * Identifies the Nintendo l button.
         */
        L,
        /**
         * Identifies the Nintendo r button.
         */
        R,
        /**
         * Identifies the Nintendo zl button.
         */
        ZL,
        /**
         * Identifies the Nintendo zr button.
         */
        ZR,
        /**
         * Identifies the Nintendo minus button.
         */
        Minus,
        /**
         * Identifies the Nintendo plus button.
         */
        Plus,
        /**
         * Identifies the Nintendo left stick button.
         */
        LeftStick,
        /**
         * Identifies the Nintendo right stick button.
         */
        RightStick,
        /**
         * Identifies the Nintendo d pad up button.
         */
        DPadUp,
        /**
         * Identifies the Nintendo d pad right button.
         */
        DPadRight,
        /**
         * Identifies the Nintendo d pad down button.
         */
        DPadDown,
        /**
         * Identifies the Nintendo d pad left button.
         */
        DPadLeft,
        /**
         * Identifies the Nintendo button count button.
         */
        ButtonCount,
    }

    /**
     * Button identifiers using Sony controller labels.
     */
    export enum SonyControllerButton {
        /**
         * Identifies the Sony cross button.
         */
        Cross = 0,
        /**
         * Identifies the Sony circle button.
         */
        Circle,
        /**
         * Identifies the Sony square button.
         */
        Square,
        /**
         * Identifies the Sony triangle button.
         */
        Triangle,
        /**
         * Identifies the Sony l 1 button.
         */
        L1,
        /**
         * Identifies the Sony r 1 button.
         */
        R1,
        /**
         * Identifies the Sony l 2 button.
         */
        L2,
        /**
         * Identifies the Sony r 2 button.
         */
        R2,
        /**
         * Identifies the Sony share button.
         */
        Share,
        /**
         * Identifies the Sony options button.
         */
        Options,
        /**
         * Identifies the Sony left stick button.
         */
        LeftStick,
        /**
         * Identifies the Sony right stick button.
         */
        RightStick,
        /**
         * Identifies the Sony d pad up button.
         */
        DPadUp,
        /**
         * Identifies the Sony d pad right button.
         */
        DPadRight,
        /**
         * Identifies the Sony d pad down button.
         */
        DPadDown,
        /**
         * Identifies the Sony d pad left button.
         */
        DPadLeft,
        /**
         * Identifies the Sony button count button.
         */
        ButtonCount,
    }

    /**
     * Sentinel used by global controller triggers to avoid selecting a specific controller.
     */
    export const CONTROLLER_UNDEFINED = -2;

    /**
     * Connected gamepad access, trigger creation, and vibration control.
     */
    export class Gamepad {
        /**
         * The runtime controller identifier.
         */
        controllerId: number;
        /**
         * The human-readable name.
         */
        name: string;
        /**
         * Whether the input device is currently connected.
         */
        connected: boolean;

        /**
         * Returns axis trigger.
         *
         * @param axis - The logical controller axis.
         * @returns The axis trigger.
         */
        getAxisTrigger(axis: ControllerAxis): AxisTrigger;
        /**
         * Creates an axis binding without selecting a specific controller.
         *
         * @param axis - The logical controller axis.
         * @returns The global axis trigger.
         */
        static getGlobalAxisTrigger(axis: ControllerAxis): AxisTrigger;
        /**
         * Returns button trigger.
         *
         * @param button - The mouse or controller button.
         * @returns The button trigger.
         */
        getButtonTrigger(button: ControllerButton): Trigger;
        /**
         * Creates a button binding without selecting a specific controller.
         *
         * @param button - The mouse or controller button.
         * @returns The global button trigger.
         */
        static getGlobalButtonTrigger(button: ControllerButton): Trigger;

        /**
         * Requests controller vibration. The method name retains the spelling used by this declaration.
         *
         * @param strength - The effect strength.
         * @param duration - The duration in seconds.
         */
        runble(strength: number, duration: number): void;
    }

    /**
     * Type alias for Gamepad.
     */
    export type Controller = Gamepad;

    /**
     * Joystick access using device-specific axis and button indices.
     */
    export class Joystick {
        /**
         * The runtime joystick identifier.
         */
        joystickId: number;
        /**
         * The human-readable name.
         */
        name: string;
        /**
         * Whether the input device is currently connected.
         */
        connected: boolean;

        /**
         * Returns single axis trigger.
         *
         * @param axisIndex - The zero-based controller axis index.
         * @returns The single axis trigger.
         */
        getSingleAxisTrigger(axisIndex: number): AxisTrigger;
        /**
         * Returns dual axis trigger.
         *
         * @param axisIndexX - The zero-based controller axis used for X.
         * @param axisIndexY - The zero-based controller axis used for Y.
         * @returns The dual axis trigger.
         */
        getDualAxisTrigger(axisIndexX: number, axisIndexY: number): AxisTrigger;
        /**
         * Returns button trigger.
         *
         * @param buttonIndex - The zero-based device button index.
         * @returns The button trigger.
         */
        getButtonTrigger(buttonIndex: number): Trigger;

        /**
         * Returns axis count.
         *
         * @returns The axis count.
         */
        getAxisCount(): number;
        /**
         * Returns button count.
         *
         * @returns The button count.
         */
        getButtonCount(): number;
    }

    /**
     * Device descriptor returned by Window.getControllers; isJoystick distinguishes joystick devices.
     */
    export type ControllerID = {
        /**
         * The runtime identifier for this object.
         */
        id: number;
        /**
         * The human-readable name.
         */
        name: string;
        /**
         * Whether the device exposes raw joystick axes and buttons.
         */
        isJoystick: boolean;
    };

    /**
     * Active runtime window: scene objects, input, audio, display configuration, and frame timing.
     */
    export class Window {
        /**
         * The title.
         */
        title: string;
        /**
         * The width.
         */
        width: number;
        /**
         * The height.
         */
        height: number;
        /**
         * The zero-based frame counter.
         */
        currentFrame: number;

        /**
         * The audio engine.
         */
        audioEngine: AudioEngine;

        /**
         * Sets clear color.
         *
         * @param color - The color to apply.
         */
        setClearColor(color: Color): void;
        /**
         * Close.
         */
        close(): void;
        /**
         * Sets fullscreen.
         *
         * @param enabled - Whether to enable the feature.
         */
        setFullscreen(enabled: boolean): void;
        /**
         * Sets fullscreen.
         *
         * @param monitor - The monitor on which to enter fullscreen mode.
         */
        setFullscreen(monitor: Monitor): void;
        /**
         * Sets windowed.
         *
         * @param config - The complete configuration to apply.
         */
        setWindowed(config: WindowConfiguration): void;

        /**
         * Enumerate monitors.
         *
         * @returns The monitors.
         */
        enumerateMonitors(): Monitor[];
        /**
         * Returns controllers.
         *
         * @returns The controllers.
         */
        getControllers(): ControllerID[];
        /**
         * Returns controller.
         *
         * @param id - A controller descriptor returned by `getControllers()`.
         * @returns The requested controller, or `null` when unavailable.
         */
        getController(id: ControllerID): Controller | null;
        /**
         * Returns joystick.
         *
         * @param id - A controller descriptor returned by `getControllers()`.
         * @returns The requested joystick, or `null` when unavailable.
         */
        getJoystick(id: ControllerID): Joystick | null;

        /**
         * Adds an object to the runtime window.
         *
         * @param object - The object to operate on.
         */
        instantiate(object: GameObject): void;
        /**
         * Removes an object from the runtime window.
         *
         * @param object - The object to operate on.
         */
        destroy(object: GameObject): void;

        /**
         * Adds a UI element to the window.
         *
         * @param object - The object to operate on.
         */
        addUIObject(object: UIObject): void;
        /**
         * Sets camera.
         *
         * @param camera - The camera to make active.
         */
        setCamera(camera: Camera): void;
        /**
         * Sets scene.
         *
         * @param scene - The scene to make active.
         */
        setScene(scene: Scene): void;
        /**
         * Returns time.
         *
         * @returns The time.
         */
        getTime(): number;
        /**
         * Reports whether key active.
         *
         * @param key - The keyboard key.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isKeyActive(key: Key): boolean;

        /**
         * Reports whether mouse button active.
         *
         * @param button - The mouse or controller button.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isMouseButtonActive(button: MouseButton): boolean;
        /**
         * Reports whether mouse button pressed.
         *
         * @param button - The mouse or controller button.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isMouseButtonPressed(button: MouseButton): boolean;
        /**
         * Returns text collected by the runtime's text input system.
         *
         * @returns The text input.
         */
        getTextInput(): string;
        /**
         * Enables text input collection.
         */
        startTextInput(): void;
        /**
         * Disables text input collection.
         */
        stopTextInput(): void;
        /**
         * Reports whether text input collection is enabled.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isTextInputActive(): boolean;

        /**
         * Reports whether controller button pressed.
         *
         * @param controllerID - The runtime controller identifier.
         * @param buttonIndex - The zero-based device button index.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isControllerButtonPressed(
            controllerID: number,
            buttonIndex: number,
        ): boolean;
        /**
         * Returns controller axis value.
         *
         * @param controllerID - The runtime controller identifier.
         * @param axisIndex - The zero-based controller axis index.
         * @returns The controller axis value.
         */
        getControllerAxisValue(controllerID: number, axisIndex: number): number;
        /**
         * Returns controller axis pair value.
         *
         * @param controllerID - The runtime controller identifier.
         * @param axisIndexX - The zero-based controller axis used for X.
         * @param axisIndexY - The zero-based controller axis used for Y.
         * @returns The controller axis pair value.
         */
        getControllerAxisPairValue(
            controllerID: number,
            axisIndexX: number,
            axisIndexY: number,
        ): Position2d;

        /**
         * Releases mouse capture.
         */
        releaseMouse(): void;
        /**
         * Captures the mouse for relative input.
         */
        captureMouse(): void;
        /**
         * Returns cursor position.
         *
         * @returns The cursor position.
         */
        getCursorPosition(): Position2d;

        /**
         * The main.
         */
        main: Window;

        /**
         * Returns current scene.
         *
         * @returns The current scene.
         */
        getCurrentScene(): Scene;
        /**
         * Returns the active camera.
         *
         * @returns The camera.
         */
        getCamera(): Camera;
        /**
         * Adds render target.
         *
         * @returns The render target attached to the window.
         */
        addRenderTarget(): RenderTarget;
        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        getSize(): Size2d;
        /**
         * Activate debug.
         */
        activateDebug(): void;
        /**
         * Disables the debug display. The method name retains the spelling used by this declaration.
         */
        desactivateDebug(): void;

        /**
         * Returns delta time.
         *
         * @returns The delta time.
         */
        getDeltaTime(): number;
        /**
         * Returns frames per second.
         *
         * @returns The frames per second.
         */
        getFramesPerSecond(): number;
        /**
         * The gravitational acceleration applied by the simulation.
         */
        gravity: number;

        /**
         * Configures the use of atlas tracer.
         *
         * @param enabled - Whether to enable the feature.
         */
        useAtlasTracer(enabled: boolean): void;
        /**
         * Sets log output.
         *
         * @param showLogs - Whether informational messages are shown.
         * @param showWarnings - Whether warning messages are shown.
         * @param showErrors - Whether error messages are shown.
         */
        setLogOutput(
            showLogs: boolean,
            showWarnings: boolean,
            showErrors: boolean,
        ): void;

        /**
         * Whether the window uses deferred rendering.
         */
        usesDeferred: boolean;

        /**
         * Returns render scale.
         *
         * @returns The render scale.
         */
        getRenderScale(): number;
        /**
         * Configures the use of metal upscaling.
         *
         * @param ratio - The scale or distribution ratio.
         */
        useMetalUpscaling(ratio: number): void;
        /**
         * Reports whether metal upscaling enabled.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isMetalUpscalingEnabled(): boolean;
        /**
         * Returns metal upscaling ratio.
         *
         * @returns The metal upscaling ratio.
         */
        getMetalUpscalingRatio(): number;

        /**
         * Returns screen-space ambient occlusion render scale.
         *
         * @returns The screen-space ambient occlusion render scale.
         */
        getSSAORenderScale(): number;

        /**
         * Registers a named input action for later polling.
         *
         * @param action - The named input action to register.
         */
        addInputAction(action: InputAction): void;
        /**
         * Clears the registered input actions.
         */
        resetInputActions(): void;
        /**
         * Returns the registered action with this name, or null if it is missing.
         *
         * @param name - The name used to identify the value.
         * @returns The requested input action, or `null` when unavailable.
         */
        getInputAction(name: string): InputAction | null;

        /**
         * Reports whether action triggered.
         *
         * @param name - The name used to identify the value.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isActionTriggered(name: string): boolean;
        /**
         * Reports whether action currently active.
         *
         * @param name - The name used to identify the value.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isActionCurrentlyActive(name: string): boolean;
        /**
         * Returns action axis value.
         *
         * @param name - The name used to identify the value.
         * @returns The action axis value.
         */
        getActionAxisValue(name: string): AxisPacket;
    }
}

/**
 * Textures, render targets, post-processing, and lighting.
 */
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

    /**
     * Texture role used by the renderer, such as albedo, normal, depth, or material data.
     */
    export enum TextureType {
        /**
         * A texture containing color data.
         */
        Color,
        /**
         * A texture containing specular data.
         */
        Specular,
        /**
         * A texture containing cubemap data.
         */
        Cubemap,
        /**
         * A texture containing depth data.
         */
        Depth,
        /**
         * A texture containing depth cube data.
         */
        DepthCube,
        /**
         * A texture containing normal data.
         */
        Normal,
        /**
         * A texture containing parallax data.
         */
        Parallax,
        /**
         * A texture containing screen-space ambient occlusion noise data.
         */
        SSAONoise,
        /**
         * A texture containing screen-space ambient occlusion data.
         */
        SSAO,
        /**
         * A texture containing metallic data.
         */
        Metallic,
        /**
         * A texture containing roughness data.
         */
        Roughness,
        /**
         * A texture containing ambient occlusion data.
         */
        AO,
        /**
         * A texture containing opacity data.
         */
        Opacity,
        /**
         * A texture containing HDR data.
         */
        HDR,
        /**
         * A texture containing PBR pack data.
         */
        PBRPack,
    }

    /**
     * GPU texture loaded from a resource or created procedurally.
     */
    export class Texture {
        /**
         * The category or discriminator for this value.
         */
        type: TextureType;
        /**
         * The resource backing this object.
         */
        resource: Resource;
        /**
         * The width.
         */
        width: number;
        /**
         * The height.
         */
        height: number;
        /**
         * The channels.
         */
        channels: number;
        /**
         * The runtime identifier for this object.
         */
        id: number;
        /**
         * The border color.
         */
        borderColor: Color;

        /**
         * Creates a value from resource.
         *
         * @param resource - The resource to use.
         * @param type - The type or constructor used for the operation.
         * @returns The newly created value.
         */
        static fromResource(
            resource: Resource | string,
            type: TextureType,
        ): Texture;

        /**
         * Creates empty.
         *
         * @param width - The width.
         * @param height - The height.
         * @param type - The type or constructor used for the operation.
         * @param borderColor - The color sampled outside the texture boundary.
         * @returns The newly created value.
         */
        static createEmpty(
            width: number,
            height: number,
            type: TextureType,
            borderColor?: Color,
        ): Texture;

        /**
         * Creates color.
         *
         * @param color - The color to apply.
         * @param type - The type or constructor used for the operation.
         * @param width - The width.
         * @param height - The height.
         * @returns The newly created value.
         */
        static createColor(
            color: Color,
            type: TextureType,
            width: number,
            height: number,
        ): Texture;

        /**
         * Creates checkerboard.
         *
         * @param width - The width.
         * @param height - The height.
         * @param checkSize - The width and height of each checkerboard cell, in pixels.
         * @param color1 - The first color, selected when `t` is `0`.
         * @param color2 - The second color, selected when `t` is `1`.
         */
        createCheckerboard(
            width: number,
            height: number,
            checkSize: number,
            color1: Color,
            color2: Color,
        ): void;

        /**
         * Creates double checkerboard.
         *
         * @param width - The width.
         * @param height - The height.
         * @param checkSizeBig - The size of each large checkerboard cell, in pixels.
         * @param checkSizeSmall - The size of each small checkerboard cell, in pixels.
         * @param color1 - The first color, selected when `t` is `0`.
         * @param color2 - The second color, selected when `t` is `1`.
         * @param color3 - The third color used by the nested checkerboard pattern.
         */
        createDoubleCheckerboard(
            width: number,
            height: number,
            checkSizeBig: number,
            checkSizeSmall: number,
            color1: Color,
            color2: Color,
            color3: Color,
        ): void;

        /**
         * Display to window.
         */
        displayToWindow(): void;
    }

    /**
     * Cube texture assembled from face resources, with helpers for updating face colors.
     */
    export class Cubemap {
        /**
         * The resources in this group.
         */
        resources: Resource[];
        /**
         * The runtime identifier for this object.
         */
        id: number;

        /**
         * Creates a new cubemap.
         *
         * @param resources - The resources to include.
         */
        constructor(resources: Resource[]);

        /**
         * Returns average color.
         *
         * @returns The average color.
         */
        getAverageColor(): Color;
        /**
         * Creates a value from resource group.
         *
         * @param resourceGroup - The group containing the cubemap face resources.
         * @returns The requested resource group, or `null` when unavailable.
         */
        static fromResourceGroup(resourceGroup: ResourceGroup): Cubemap | null;
        /**
         * Update with colors.
         *
         * @param colors - The collection of colors.
         */
        updateWithColors(colors: Color[]): void;
    }

    /**
     * Framebuffer purpose and attachment configuration.
     */
    export enum RenderTargetType {
        /**
         * Selects scene for render target type.
         */
        Scene,
        /**
         * Selects multisampled for render target type.
         */
        Multisampled,
        /**
         * Selects shadow for render target type.
         */
        Shadow,
        /**
         * Selects cube shadow for render target type.
         */
        CubeShadow,
        /**
         * Selects g buffer for render target type.
         */
        GBuffer,
        /**
         * Selects screen-space ambient occlusion for render target type.
         */
        SSAO,
        /**
         * Selects screen-space ambient occlusion blur for render target type.
         */
        SSAOBlur,
    }

    /**
     * Rendering pipeline pass to enqueue for a render target.
     */
    export enum RenderPassType {
        /**
         * Selects deferred for render pass type.
         */
        Deferred,
        /**
         * Selects forward for render pass type.
         */
        Forward,
        /**
         * Selects path tracing for render pass type.
         */
        PathTracing,
    }

    /**
     * Built-in post-processing effect descriptors. Copy a descriptor to customize its parameters.
     */
    export const Effects: {
        /**
         * The inversion.
         */
        Inversion: {
            /**
             * The category or discriminator for this value.
             */
            type: "Inversion"
        };
        /**
         * The grayscale.
         */
        Grayscale: {
            /**
             * The category or discriminator for this value.
             */
            type: "Grayscale"
        };
        /**
         * The sharpen.
         */
        Sharpen: {
            /**
             * The category or discriminator for this value.
             */
            type: "Sharpen"
        };
        /**
         * The blur.
         */
        Blur: {
            /**
             * The category or discriminator for this value.
             */
            type: "Blur";
            /**
             * The magnitude.
             */
            magnitude: number
        };
        /**
         * The edge detection.
         */
        EdgeDetection: {
            /**
             * The category or discriminator for this value.
             */
            type: "EdgeDetection"
        };
        /**
         * The color correction.
         */
        ColorCorrection: {
            /**
             * The category or discriminator for this value.
             */
            type: "ColorCorrection";
            /**
             * The exposure.
             */
            exposure: number;
            /**
             * The contrast.
             */
            contrast: number;
            /**
             * The saturation.
             */
            saturation: number;
            /**
             * The gamma.
             */
            gamma: number;
            /**
             * The temperature.
             */
            temperature: number;
            /**
             * The tint.
             */
            tint: number;
        };
        /**
         * The motion blur.
         */
        MotionBlur: {
            /**
             * The category or discriminator for this value.
             */
            type: "MotionBlur";
            /**
             * The dimensions.
             */
            size: number;
            /**
             * The separation.
             */
            separation: number
        };
        /**
         * The chromatic aberration.
         */
        ChromaticAberration: {
            /**
             * The category or discriminator for this value.
             */
            type: "ChromaticAberration";
            /**
             * The red.
             */
            red: number;
            /**
             * The green.
             */
            green: number;
            /**
             * The blue.
             */
            blue: number;
            /**
             * The normalized direction vector.
             */
            direction: Magnitude2d;
        };
        /**
         * The posterization.
         */
        Posterization: {
            /**
             * The category or discriminator for this value.
             */
            type: "Posterization";
            /**
             * The levels.
             */
            levels: number
        };
        /**
         * The pixelation.
         */
        Pixelation: {
            /**
             * The category or discriminator for this value.
             */
            type: "Pixelation";
            /**
             * The pixel size.
             */
            pixelSize: number
        };
        /**
         * The dialation.
         */
        Dialation: {
            /**
             * The category or discriminator for this value.
             */
            type: "Dilation";
            /**
             * The dimensions.
             */
            size: number;
            /**
             * The separation.
             */
            separation: number
        };
        /**
         * The dilation.
         */
        Dilation: {
            /**
             * The category or discriminator for this value.
             */
            type: "Dilation";
            /**
             * The dimensions.
             */
            size: number;
            /**
             * The separation.
             */
            separation: number
        };
        /**
         * The film grain.
         */
        FilmGrain: {
            /**
             * The category or discriminator for this value.
             */
            type: "FilmGrain";
            /**
             * The amount.
             */
            amount: number
        };
    };

    /**
     * A built-in effect name or an effect descriptor containing its parameters.
     */
    export type Effect =
        | keyof typeof Effects
        | (typeof Effects)[keyof typeof Effects];

    /**
     * Offscreen rendering destination with output textures, render passes, and post-processing effects.
     */
    export class RenderTarget {
        /**
         * The category or discriminator for this value.
         */
        type: RenderTargetType;
        /**
         * The resolution.
         */
        resolution: number;
        /**
         * The collection of out textures.
         */
        outTextures: Texture[];
        /**
         * The depth texture, or `null` when unavailable.
         */
        depthTexture: Texture | null;

        /**
         * Creates a new render target.
         *
         * @param type - The type or constructor used for the operation.
         * @param resolution - The texture or shadow-map resolution in pixels.
         */
        constructor(type: RenderTargetType, resolution: number);

        /**
         * Adds a post-processing effect by name or configured descriptor.
         *
         * @param effect - The audio or post-processing effect to apply.
         */
        addEffect(effect: Effect): void;

        /**
         * Enqueues the selected rendering pass for this target.
         *
         * @param type - The type or constructor used for the operation.
         */
        addToPassQueue(type: RenderPassType): void;
        /**
         * Display.
         */
        display(): void;
    }

    /**
     * Scene background rendered from a cubemap.
     */
    export class Skybox {
        /**
         * The cubemap.
         */
        cubemap: Cubemap;

        /**
         * Creates a new skybox.
         *
         * @param cubemap - The cubemap containing the six skybox faces.
         */
        constructor(cubemap: Cubemap);
    }

    /**
     * Uniform ambient illumination described by color and intensity.
     */
    export class AmbientLight {
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The effect intensity.
         */
        intensity: number;

        /**
         * Creates a new ambient light.
         *
         * @param color - The color to apply.
         * @param intensity - The effect intensity.
         */
        constructor(color?: Color, intensity?: number);
    }

    /**
     * Point light with position, range, diffuse color, and specular shine color.
     */
    export class Light {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The shine color.
         */
        shineColor: Color;
        /**
         * The effect intensity.
         */
        intensity: number;
        /**
         * The distance.
         */
        distance: number;

        /**
         * Creates a new light.
         *
         * @param position - The position or translation offset.
         * @param color - The color to apply.
         * @param distance - The light's effective range.
         * @param shineColor - The light color used for specular highlights.
         * @param intensity - The effect intensity.
         */
        constructor(
            position?: Position3d,
            color?: Color,
            distance?: number,
            shineColor?: Color,
            intensity?: number,
        );

        /**
         * Updates the color used by this object.
         *
         * @param color - The color to apply.
         */
        setColor(color: Color): void;
        /**
         * Creates a visual representation of the light for debugging.
         */
        createDebugObject(): void;
        /**
         * Enables shadow casting with the requested shadow-map resolution.
         *
         * @param resolution - The texture or shadow-map resolution in pixels.
         */
        castShadows(resolution: number): void;
    }

    /**
     * Light with a direction and no positional origin, suitable for sunlight.
     */
    export class DirectionalLight {
        /**
         * The normalized direction vector.
         */
        direction: Magnitude3d;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The shine color.
         */
        shineColor: Color;
        /**
         * The effect intensity.
         */
        intensity: number;

        /**
         * Creates a new directional light.
         *
         * @param direction - The direction vector.
         * @param color - The color to apply.
         * @param shineColor - The light color used for specular highlights.
         * @param intensity - The effect intensity.
         */
        constructor(
            direction?: Magnitude3d,
            color?: Color,
            shineColor?: Color,
            intensity?: number,
        );

        /**
         * Updates the color used by this object.
         *
         * @param color - The color to apply.
         */
        setColor(color: Color): void;
        /**
         * Enables shadow casting with the requested shadow-map resolution.
         *
         * @param resolution - The texture or shadow-map resolution in pixels.
         */
        castShadows(resolution: number): void;
    }

    /**
     * Positioned cone light with inner and outer cutoffs, direction, and range.
     */
    export class SpotLight {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The normalized direction vector.
         */
        direction: Magnitude3d;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The shine color.
         */
        shineColor: Color;
        /**
         * The range.
         */
        range: number;
        /**
         * The cut off.
         */
        cutOff: number;
        /**
         * The outer cut off.
         */
        outerCutOff: number;
        /**
         * The effect intensity.
         */
        intensity: number;

        /**
         * Creates a new spot light.
         *
         * @param position - The position or translation offset.
         * @param direction - The direction vector.
         * @param color - The color to apply.
         * @param cutOff - The spotlight's inner cone cutoff angle.
         * @param outerCutOff - The spotlight's outer cone cutoff angle.
         * @param shineColor - The light color used for specular highlights.
         * @param intensity - The effect intensity.
         * @param range - The effective range of the light.
         */
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

        /**
         * Updates the color used by this object.
         *
         * @param color - The color to apply.
         */
        setColor(color: Color): void;
        /**
         * Creates a visual representation of the light for debugging.
         */
        createDebugObject(): void;
        /**
         * Orients toward the target position.
         *
         * @param target - The target position.
         */
        lookAt(target: Position3d): void;
        /**
         * Enables shadow casting with the requested shadow-map resolution.
         *
         * @param resolution - The texture or shadow-map resolution in pixels.
         */
        castShadows(resolution: number): void;
    }

    /**
     * Oriented area light with dimensions, emission range, and optional two-sided emission.
     */
    export class AreaLight {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The right.
         */
        right: Magnitude3d;
        /**
         * The up.
         */
        up: Magnitude3d;
        /**
         * The dimensions.
         */
        size: Size2d;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The shine color.
         */
        shineColor: Color;
        /**
         * The effect intensity.
         */
        intensity: number;
        /**
         * The range.
         */
        range: number;
        /**
         * The angle.
         */
        angle: number;
        /**
         * Whether the area light emits from both faces.
         */
        castsBothSides: boolean;
        /**
         * The Euler rotation in degrees.
         */
        rotation: Rotation3d;

        /**
         * Creates a new area light.
         *
         * @param position - The position or translation offset.
         * @param right - The area's local right direction.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         * @param size - The size or dimensions.
         * @param color - The color to apply.
         * @param shineColor - The light color used for specular highlights.
         * @param intensity - The effect intensity.
         * @param range - The effective range of the light.
         * @param angle - The light's emission angle, in degrees.
         * @param castsBothSides - Whether the area light emits from both faces.
         * @param rotation - The Euler rotation, in degrees.
         */
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

        /**
         * Returns normal.
         *
         * @returns The normal.
         */
        getNormal(): Magnitude3d;
        /**
         * Updates the color used by this object.
         *
         * @param color - The color to apply.
         */
        setColor(color: Color): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        setRotation(rotation: Rotation3d): void;
        /**
         * Applies an incremental rotation.
         *
         * @param delta - The incremental rotation to apply, in degrees.
         */
        rotate(delta: Rotation3d): void;
        /**
         * Creates a visual representation of the light for debugging.
         */
        createDebugObject(): void;
        /**
         * Enables shadow casting with the requested shadow-map resolution.
         *
         * @param resolution - The texture or shadow-map resolution in pixels.
         */
        castShadows(resolution: number): void;
    }
}

/**
 * Value types for coordinates, dimensions, rotations, and colors.
 */
declare module "atlas/units" {
    /**
     * Three-component value. Arithmetic returns new values; up is +Y and forward is +Z.
     */
    export class Position3d {
        /**
         * The X.
         */
        x: number;
        /**
         * The Y.
         */
        y: number;
        /**
         * The Z.
         */
        z: number;

        /**
         * Creates a new position 3D.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @param z - The Z coordinate or component.
         */
        constructor(x: number, y: number, z: number);

        /**
         * Zero.
         *
         * @returns The resulting `Position3d` value.
         */
        static zero(): Position3d;
        /**
         * Down.
         *
         * @returns The resulting `Position3d` value.
         */
        static down(): Position3d;
        /**
         * Up.
         *
         * @returns The resulting `Position3d` value.
         */
        static up(): Position3d;
        /**
         * Forward.
         *
         * @returns The resulting `Position3d` value.
         */
        static forward(): Position3d;
        /**
         * Back.
         *
         * @returns The resulting `Position3d` value.
         */
        static back(): Position3d;
        /**
         * Right.
         *
         * @returns The resulting `Position3d` value.
         */
        static right(): Position3d;
        /**
         * Left.
         *
         * @returns The resulting `Position3d` value.
         */
        static left(): Position3d;
        /**
         * Returns a value whose coordinate components are NaN.
         *
         * @returns The resulting `Position3d` value.
         */
        static invalid(): Position3d;

        /**
         * Returns a new value by adding the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position3d` value.
         */
        add(other: Position3d | number): Position3d;
        /**
         * Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position3d` value.
         */
        subtract(other: Position3d | number): Position3d;
        /**
         * Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position3d` value.
         */
        multiply(other: Position3d | number): Position3d;
        /**
         * Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position3d` value.
         */
        divide(other: Position3d | number): Position3d;
        /**
         * Tests exact equality of all components.
         *
         * @param other - The value to compare against.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        is(other: Position3d): boolean;

        /**
         * Returns a unit-length copy, or a zero vector if the length is zero.
         *
         * @returns A normalized copy of this value.
         */
        normalized(): Position3d;
        /**
         * Returns a human-readable representation.
         *
         * @returns A human-readable string representation.
         */
        toString(): string;
    }

    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Scale3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Size3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Point3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Normal3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Magnitude3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Impulse3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Force3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Vector3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Velocity3d = Position3d;
    /**
     * Semantic type alias for Position3d; it does not introduce a distinct value type.
     */
    export type Rotation3d = Position3d;

    /**
     * Axis-aligned bounds with inclusive point containment and intersection checks.
     */
    export class BoundingBox {
        /**
         * The minimum.
         */
        min: Position3d;
        /**
         * The maximum.
         */
        max: Position3d;

        /**
         * Creates a new bounding box.
         *
         * @param min - The minimum corner of the bounds.
         * @param max - The maximum corner of the bounds.
         */
        constructor(min: Position3d, max: Position3d);

        /**
         * Returns a human-readable representation.
         *
         * @returns A human-readable string representation.
         */
        toString(): string;
        /**
         * Tests whether the point lies inside or on the bounds.
         *
         * @param point - The world-space point where the force is applied.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        contains(point: Position3d): boolean;
        /**
         * Tests whether the bounds overlap or touch.
         *
         * @param other - The other bounding box.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        intersects(other: BoundingBox): boolean;
    }

    /**
     * Four-component orientation with conversion to and from Euler rotations.
     */
    export class Quaternion {
        /**
         * The X.
         */
        x: number;
        /**
         * The Y.
         */
        y: number;
        /**
         * The Z.
         */
        z: number;
        /**
         * The quaternion scalar component.
         */
        w: number;

        /**
         * Creates a new quaternion.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @param z - The Z coordinate or component.
         * @param w - The quaternion scalar component.
         */
        constructor(x: number, y: number, z: number, w: number);
        /**
         * Creates a new quaternion.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        constructor(rotation: Rotation3d);

        /**
         * To euler.
         *
         * @returns The resulting `Rotation3d` value.
         */
        toEuler(): Rotation3d;
        /**
         * Creates a value from euler.
         *
         * @param rotation - The Euler rotation, in degrees.
         * @returns The newly created value.
         */
        static fromEuler(rotation: Rotation3d): Quaternion;
    }

    /**
     * RGBA color, conventionally using 0–1 channels. Arithmetic returns new colors and includes alpha without clamping.
     */
    export class Color {
        /**
         * The red channel.
         */
        r: number;
        /**
         * The green channel.
         */
        g: number;
        /**
         * The blue channel.
         */
        b: number;
        /**
         * Alpha channel; the constructor defaults to 1 (opaque).
         */
        a: number;

        /**
         * Creates a new color.
         *
         * @param r - The red channel.
         * @param g - The green channel.
         * @param b - The blue channel.
         * @param a - The alpha channel. Defaults to `1`.
         */
        constructor(r: number, g: number, b: number, a?: number);

        /**
         * Returns a new value by adding the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting or predefined color.
         */
        add(other: Color | number): Color;
        /**
         * Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting or predefined color.
         */
        subtract(other: Color | number): Color;
        /**
         * Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting or predefined color.
         */
        multiply(other: Color | number): Color;
        /**
         * Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting or predefined color.
         */
        divide(other: Color | number): Color;
        /**
         * Tests exact equality of all components.
         *
         * @param other - The value to compare against.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        is(other: Color): boolean;

        /**
         * White.
         *
         * @returns The resulting or predefined color.
         */
        static white(): Color;
        /**
         * Black.
         *
         * @returns The resulting or predefined color.
         */
        static black(): Color;
        /**
         * Red.
         *
         * @returns The resulting or predefined color.
         */
        static red(): Color;
        /**
         * Green.
         *
         * @returns The resulting or predefined color.
         */
        static green(): Color;
        /**
         * Blue.
         *
         * @returns The resulting or predefined color.
         */
        static blue(): Color;
        /**
         * Transparent.
         *
         * @returns The resulting or predefined color.
         */
        static transparent(): Color;
        /**
         * Yellow.
         *
         * @returns The resulting or predefined color.
         */
        static yellow(): Color;
        /**
         * Cyan.
         *
         * @returns The resulting or predefined color.
         */
        static cyan(): Color;
        /**
         * Magenta.
         *
         * @returns The resulting or predefined color.
         */
        static magenta(): Color;
        /**
         * Gray.
         *
         * @returns The resulting or predefined color.
         */
        static gray(): Color;
        /**
         * Orange.
         *
         * @returns The resulting or predefined color.
         */
        static orange(): Color;
        /**
         * Purple.
         *
         * @returns The resulting or predefined color.
         */
        static purple(): Color;
        /**
         * Brown.
         *
         * @returns The resulting or predefined color.
         */
        static brown(): Color;
        /**
         * Pink.
         *
         * @returns The resulting or predefined color.
         */
        static pink(): Color;
        /**
         * Lime.
         *
         * @returns The resulting or predefined color.
         */
        static lime(): Color;
        /**
         * Navy.
         *
         * @returns The resulting or predefined color.
         */
        static navy(): Color;
        /**
         * Teal.
         *
         * @returns The resulting or predefined color.
         */
        static teal(): Color;
        /**
         * Olive.
         *
         * @returns The resulting or predefined color.
         */
        static olive(): Color;
        /**
         * Maroon.
         *
         * @returns The resulting or predefined color.
         */
        static maroon(): Color;

        /**
         * Creates an opaque color from RGB or RRGGBB hexadecimal text, with an optional leading #.
         *
         * @param hex - An RGB or RRGGBB hexadecimal color, optionally prefixed with `#`.
         * @returns The newly created value.
         */
        static fromHex(hex: string): Color;
        /**
         * Linearly interpolates all RGBA channels; t = 0 selects color1 and t = 1 selects color2. The factor is not clamped.
         *
         * @param color1 - The first color, selected when `t` is `0`.
         * @param color2 - The second color, selected when `t` is `1`.
         * @param t - The interpolation factor; values are not clamped.
         * @returns The resulting or predefined color.
         */
        static mix(color1: Color, color2: Color, t: number): Color;
    }

    /**
     * Named directions along the three coordinate axes.
     */
    export enum Direction3d {
        /**
         * The positive Y direction.
         */
        Up,
        /**
         * The negative Y direction.
         */
        Down,
        /**
         * The negative X direction.
         */
        Left,
        /**
         * The positive X direction.
         */
        Right,
        /**
         * The positive Z direction.
         */
        Forward,
        /**
         * The negative Z direction.
         */
        Backward,
    }

    /**
     * Two-component value. Arithmetic returns new values; up is +Y.
     */
    export class Position2d {
        /**
         * The X.
         */
        x: number;
        /**
         * The Y.
         */
        y: number;

        /**
         * Creates a new position 2D.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         */
        constructor(x: number, y: number);

        /**
         * Zero.
         *
         * @returns The resulting `Position2d` value.
         */
        static zero(): Position2d;
        /**
         * Up.
         *
         * @returns The resulting `Position2d` value.
         */
        static up(): Position2d;
        /**
         * Down.
         *
         * @returns The resulting `Position2d` value.
         */
        static down(): Position2d;
        /**
         * Left.
         *
         * @returns The resulting `Position2d` value.
         */
        static left(): Position2d;
        /**
         * Right.
         *
         * @returns The resulting `Position2d` value.
         */
        static right(): Position2d;
        /**
         * Returns a value whose coordinate components are NaN.
         *
         * @returns The resulting `Position2d` value.
         */
        static invalid(): Position2d;

        /**
         * Returns a new value by adding the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position2d` value.
         */
        add(other: Position2d | number): Position2d;
        /**
         * Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position2d` value.
         */
        subtract(other: Position2d | number): Position2d;
        /**
         * Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position2d` value.
         */
        multiply(other: Position2d | number): Position2d;
        /**
         * Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Position2d` value.
         */
        divide(other: Position2d | number): Position2d;

        /**
         * Tests exact equality of all components.
         *
         * @param other - The value to compare against.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        is(other: Position2d): boolean;
    }

    /**
     * Semantic type alias for Position2d; it does not introduce a distinct value type.
     */
    export type Scale2d = Position2d;
    /**
     * Semantic type alias for Position2d; it does not introduce a distinct value type.
     */
    export type Point2d = Position2d;
    /**
     * Semantic type alias for Position2d; it does not introduce a distinct value type.
     */
    export type Movement2d = Position2d;
    /**
     * Semantic type alias for Position2d; it does not introduce a distinct value type.
     */
    export type Magnitude2d = Position2d;

    /**
     * Angle stored in radians, with degree conversion and arithmetic returning new values.
     */
    export class Radians {
        /**
         * The value.
         */
        value: number;

        /**
         * Creates a new radians.
         *
         * @param value - The value to apply.
         */
        constructor(value: number);

        /**
         * Returns a new value by adding the operand.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Radians` value.
         */
        add(other: Radians): Radians;
        /**
         * Returns a new value by subtracting the operand.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Radians` value.
         */
        subtract(other: Radians): Radians;
        /**
         * Returns a new value by multiplying by the operand.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Radians` value.
         */
        multiply(other: Radians | number): Radians;
        /**
         * Returns a new value by dividing by the operand.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Radians` value.
         */
        divide(other: Radians | number): Radians;

        /**
         * Returns the stored numeric value.
         *
         * @returns The stored number.
         */
        toNumber(): number;
        /**
         * Creates a value from degrees.
         *
         * @param degrees - The angle in degrees.
         * @returns The newly created value.
         */
        static fromDegrees(degrees: number): Radians;
        /**
         * To degrees.
         *
         * @returns The angle in degrees.
         */
        toDegrees(): number;
    }

    /**
     * Width and height value with component-wise arithmetic returning new values.
     */
    export class Size2d {
        /**
         * The width.
         */
        width: number;
        /**
         * The height.
         */
        height: number;

        /**
         * Creates a new size 2D.
         *
         * @param width - The width.
         * @param height - The height.
         */
        constructor(width: number, height: number);

        /**
         * Zero.
         *
         * @returns The resulting `Size2d` value.
         */
        static zero(): Size2d;

        /**
         * Returns a human-readable representation.
         *
         * @returns A human-readable string representation.
         */
        toString(): string;

        /**
         * Returns a new value by adding the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Size2d` value.
         */
        add(other: Size2d | number): Size2d;
        /**
         * Returns a new value by subtracting the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Size2d` value.
         */
        subtract(other: Size2d | number): Size2d;
        /**
         * Returns a new value by multiplying by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Size2d` value.
         */
        multiply(other: Size2d | number): Size2d;
        /**
         * Returns a new value by dividing by the operand component-wise; a numeric operand applies to every component.
         *
         * @param other - The scalar or same-shaped value applied component-wise.
         * @returns The resulting `Size2d` value.
         */
        divide(other: Size2d | number): Size2d;

        /**
         * Tests exact equality of all components.
         *
         * @param other - The value to compare against.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        is(other: Size2d): boolean;
    }
}

/**
 * Audio playback components.
 */
declare module "atlas/audio" {
    import { Component, Resource } from "atlas";
    import { Color, Position3d } from "atlas/units";
    import { AudioSource } from "finewave";

    /**
     * Component that plays an audio source and supports positional audio.
     */
    export class AudioPlayer extends Component {
        /**
         * Creates a new audio player.
         */
        constructor();

        /**
         * Initializes the component after it is attached.
         */
        override init(): void;
        /**
         * Starts or resumes playback.
         */
        play(): void;
        /**
         * Pauses playback.
         */
        pause(): void;
        /**
         * Stops playback.
         */
        stop(): void;
        /**
         * Sets volume.
         *
         * @param volume - The playback or master volume level.
         */
        setVolume(volume: number): void;
        /**
         * Enables or disables repeated playback.
         *
         * @param loop - Whether playback repeats after reaching the end.
         */
        setLoop(loop: boolean): void;

        /**
         * Loads the audio resource for playback.
         *
         * @param resource - The resource to use.
         */
        setSource(resource: Resource): void;

        /**
         * Updates the component for the current frame.
         *
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        override update(dt: number): void;

        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Enables or disables positional audio for this player.
         *
         * @param enabled - Whether to enable the feature.
         */
        useSpatialAudio(enabled: boolean): void;

        /**
         * The audio source controlled by this component.
         */
        source: AudioSource;
    }
}

/**
 * Input devices, bindings, action polling, and interaction callbacks.
 */
declare module "atlas/input" {
    import { Position2d } from "atlas/units";

    /**
     * Keyboard identifiers used by input polling and trigger bindings.
     */
    export enum Key {
        /**
         * Identifies the Unknown keyboard key.
         */
        Unknown,
        /**
         * Identifies the Space keyboard key.
         */
        Space,
        /**
         * Identifies the Apostrophe keyboard key.
         */
        Apostrophe,
        /**
         * Identifies the Comma keyboard key.
         */
        Comma,
        /**
         * Identifies the Minus keyboard key.
         */
        Minus,
        /**
         * Identifies the Period keyboard key.
         */
        Period,
        /**
         * Identifies the Slash keyboard key.
         */
        Slash,
        /**
         * Identifies the 0 keyboard key.
         */
        Key0,
        /**
         * Identifies the 1 keyboard key.
         */
        Key1,
        /**
         * Identifies the 2 keyboard key.
         */
        Key2,
        /**
         * Identifies the 3 keyboard key.
         */
        Key3,
        /**
         * Identifies the 4 keyboard key.
         */
        Key4,
        /**
         * Identifies the 5 keyboard key.
         */
        Key5,
        /**
         * Identifies the 6 keyboard key.
         */
        Key6,
        /**
         * Identifies the 7 keyboard key.
         */
        Key7,
        /**
         * Identifies the 8 keyboard key.
         */
        Key8,
        /**
         * Identifies the 9 keyboard key.
         */
        Key9,
        /**
         * Identifies the Semicolon keyboard key.
         */
        Semicolon,
        /**
         * Identifies the Equal keyboard key.
         */
        Equal,
        /**
         * Identifies the A keyboard key.
         */
        A,
        /**
         * Identifies the B keyboard key.
         */
        B,
        /**
         * Identifies the C keyboard key.
         */
        C,
        /**
         * Identifies the D keyboard key.
         */
        D,
        /**
         * Identifies the E keyboard key.
         */
        E,
        /**
         * Identifies the F keyboard key.
         */
        F,
        /**
         * Identifies the G keyboard key.
         */
        G,
        /**
         * Identifies the H keyboard key.
         */
        H,
        /**
         * Identifies the I keyboard key.
         */
        I,
        /**
         * Identifies the J keyboard key.
         */
        J,
        /**
         * Identifies the K keyboard key.
         */
        K,
        /**
         * Identifies the L keyboard key.
         */
        L,
        /**
         * Identifies the M keyboard key.
         */
        M,
        /**
         * Identifies the N keyboard key.
         */
        N,
        /**
         * Identifies the O keyboard key.
         */
        O,
        /**
         * Identifies the P keyboard key.
         */
        P,
        /**
         * Identifies the Q keyboard key.
         */
        Q,
        /**
         * Identifies the R keyboard key.
         */
        R,
        /**
         * Identifies the S keyboard key.
         */
        S,
        /**
         * Identifies the T keyboard key.
         */
        T,
        /**
         * Identifies the U keyboard key.
         */
        U,
        /**
         * Identifies the V keyboard key.
         */
        V,
        /**
         * Identifies the W keyboard key.
         */
        W,
        /**
         * Identifies the X keyboard key.
         */
        X,
        /**
         * Identifies the Y keyboard key.
         */
        Y,
        /**
         * Identifies the Z keyboard key.
         */
        Z,
        /**
         * Identifies the LeftBracket keyboard key.
         */
        LeftBracket,
        /**
         * Identifies the Backslash keyboard key.
         */
        Backslash,
        /**
         * Identifies the RightBracket keyboard key.
         */
        RightBracket,
        /**
         * Identifies the GraveAccent keyboard key.
         */
        GraveAccent,
        /**
         * Identifies the Escape keyboard key.
         */
        Escape,
        /**
         * Identifies the Enter keyboard key.
         */
        Enter,
        /**
         * Identifies the Tab keyboard key.
         */
        Tab,
        /**
         * Identifies the Backspace keyboard key.
         */
        Backspace,
        /**
         * Identifies the Insert keyboard key.
         */
        Insert,
        /**
         * Identifies the Delete keyboard key.
         */
        Delete,
        /**
         * Identifies the Right keyboard key.
         */
        Right,
        /**
         * Identifies the Left keyboard key.
         */
        Left,
        /**
         * Identifies the Down keyboard key.
         */
        Down,
        /**
         * Identifies the Up keyboard key.
         */
        Up,
        /**
         * Identifies the PageUp keyboard key.
         */
        PageUp,
        /**
         * Identifies the PageDown keyboard key.
         */
        PageDown,
        /**
         * Identifies the Home keyboard key.
         */
        Home,
        /**
         * Identifies the End keyboard key.
         */
        End,
        /**
         * Identifies the CapsLock keyboard key.
         */
        CapsLock,
        /**
         * Identifies the ScrollLock keyboard key.
         */
        ScrollLock,
        /**
         * Identifies the NumLock keyboard key.
         */
        NumLock,
        /**
         * Identifies the PrintScreen keyboard key.
         */
        PrintScreen,
        /**
         * Identifies the Pause keyboard key.
         */
        Pause,
        /**
         * Identifies the F1 keyboard key.
         */
        F1,
        /**
         * Identifies the F2 keyboard key.
         */
        F2,
        /**
         * Identifies the F3 keyboard key.
         */
        F3,
        /**
         * Identifies the F4 keyboard key.
         */
        F4,
        /**
         * Identifies the F5 keyboard key.
         */
        F5,
        /**
         * Identifies the F6 keyboard key.
         */
        F6,
        /**
         * Identifies the F7 keyboard key.
         */
        F7,
        /**
         * Identifies the F8 keyboard key.
         */
        F8,
        /**
         * Identifies the F9 keyboard key.
         */
        F9,
        /**
         * Identifies the F10 keyboard key.
         */
        F10,
        /**
         * Identifies the F11 keyboard key.
         */
        F11,
        /**
         * Identifies the F12 keyboard key.
         */
        F12,
        /**
         * Identifies the F13 keyboard key.
         */
        F13,
        /**
         * Identifies the F14 keyboard key.
         */
        F14,
        /**
         * Identifies the F15 keyboard key.
         */
        F15,
        /**
         * Identifies the F16 keyboard key.
         */
        F16,
        /**
         * Identifies the F17 keyboard key.
         */
        F17,
        /**
         * Identifies the F18 keyboard key.
         */
        F18,
        /**
         * Identifies the F19 keyboard key.
         */
        F19,
        /**
         * Identifies the F20 keyboard key.
         */
        F20,
        /**
         * Identifies the F21 keyboard key.
         */
        F21,
        /**
         * Identifies the F22 keyboard key.
         */
        F22,
        /**
         * Identifies the F23 keyboard key.
         */
        F23,
        /**
         * Identifies the F24 keyboard key.
         */
        F24,
        /**
         * Identifies the F25 keyboard key.
         */
        F25,
        /**
         * Identifies the KP0 keyboard key.
         */
        KP0,
        /**
         * Identifies the KP1 keyboard key.
         */
        KP1,
        /**
         * Identifies the KP2 keyboard key.
         */
        KP2,
        /**
         * Identifies the KP3 keyboard key.
         */
        KP3,
        /**
         * Identifies the KP4 keyboard key.
         */
        KP4,
        /**
         * Identifies the KP5 keyboard key.
         */
        KP5,
        /**
         * Identifies the KP6 keyboard key.
         */
        KP6,
        /**
         * Identifies the KP7 keyboard key.
         */
        KP7,
        /**
         * Identifies the KP8 keyboard key.
         */
        KP8,
        /**
         * Identifies the KP9 keyboard key.
         */
        KP9,
        /**
         * Identifies the KPDecimal keyboard key.
         */
        KPDecimal,
        /**
         * Identifies the KPDivide keyboard key.
         */
        KPDivide,
        /**
         * Identifies the KPMultiply keyboard key.
         */
        KPMultiply,
        /**
         * Identifies the KPSubtract keyboard key.
         */
        KPSubtract,
        /**
         * Identifies the KPAdd keyboard key.
         */
        KPAdd,
        /**
         * Identifies the KPEnter keyboard key.
         */
        KPEnter,
        /**
         * Identifies the KPEqual keyboard key.
         */
        KPEqual,
        /**
         * Identifies the LeftShift keyboard key.
         */
        LeftShift,
        /**
         * Identifies the LeftControl keyboard key.
         */
        LeftControl,
        /**
         * Identifies the LeftAlt keyboard key.
         */
        LeftAlt,
        /**
         * Identifies the LeftSuper keyboard key.
         */
        LeftSuper,
        /**
         * Identifies the RightShift keyboard key.
         */
        RightShift,
        /**
         * Identifies the RightControl keyboard key.
         */
        RightControl,
        /**
         * Identifies the RightAlt keyboard key.
         */
        RightAlt,
        /**
         * Identifies the RightSuper keyboard key.
         */
        RightSuper,
        /**
         * Identifies the Menu keyboard key.
         */
        Menu,
    }

    /**
     * Mouse button identifiers used by input polling and trigger bindings.
     */
    export enum MouseButton {
        /**
         * Identifies the left mouse button.
         */
        Left,
        /**
         * Identifies the right mouse button.
         */
        Right,
        /**
         * Identifies the middle mouse button.
         */
        Middle,
        /**
         * Identifies the X 1 mouse button.
         */
        X1,
        /**
         * Identifies the X 2 mouse button.
         */
        X2,
        /**
         * Identifies the button 6 mouse button.
         */
        Button6,
        /**
         * Identifies the button 7 mouse button.
         */
        Button7,
        /**
         * Identifies the button 8 mouse button.
         */
        Button8,
        /**
         * Identifies the last mouse button.
         */
        Last,
    }

    /**
     * Discriminator selecting the active field of a button trigger.
     */
    export enum TriggerType {
        /**
         * Selects mouse button for trigger type.
         */
        MouseButton,
        /**
         * Selects key for trigger type.
         */
        Key,
        /**
         * Selects controller button for trigger type.
         */
        ControllerButton,
    }

    /**
     * Controller identifier and device button index for a button binding.
     */
    export type ControllerButtonTrigger = {
        /**
         * The controller identifier.
         */
        controllerID: number;
        /**
         * The button index.
         */
        buttonIndex: number;
    };

    /**
     * Digital input binding. Use a factory to populate the fields corresponding to its type.
     */
    export class Trigger {
        /**
         * The category or discriminator for this value.
         */
        type: TriggerType;
        /**
         * The optional mouse button.
         */
        mouseButton?: MouseButton;
        /**
         * The optional key.
         */
        key?: Key;
        /**
         * The optional controller button.
         */
        controllerButton?: ControllerButtonTrigger;

        /**
         * Creates a value from key.
         *
         * @param key - The keyboard key.
         * @returns The newly created value.
         */
        static fromKey(key: Key): Trigger;
        /**
         * Creates a value from mouse button.
         *
         * @param mouseButton - The mouse button to bind.
         * @returns The newly created value.
         */
        static fromMouseButton(mouseButton: MouseButton): Trigger;
        /**
         * Creates a value from controller button.
         *
         * @param controllerID - The runtime controller identifier.
         * @param buttonIndex - The zero-based device button index.
         * @returns The newly created value.
         */
        static fromControllerButton(
            controllerID: number,
            buttonIndex: number,
        ): Trigger;
    }

    /**
     * Source of axis input: mouse motion, directional keys, or controller axes.
     */
    export enum AxisTriggerType {
        /**
         * Selects mouse axis for axis trigger type.
         */
        MouseAxis,
        /**
         * Selects key custom for axis trigger type.
         */
        KeyCustom,
        /**
         * Selects controller axis for axis trigger type.
         */
        ControllerAxis,
    }

    /**
     * Axis binding built from mouse motion, four directional keys, or one or two controller axes.
     */
    export class AxisTrigger {
        /**
         * The category or discriminator for this value.
         */
        type: AxisTriggerType;

        /**
         * The positive X.
         */
        positiveX: Trigger;
        /**
         * The negative X.
         */
        negativeX: Trigger;
        /**
         * The positive Y.
         */
        positiveY: Trigger;
        /**
         * The negative Y.
         */
        negativeY: Trigger;

        /**
         * The runtime controller identifier.
         */
        controllerId?: number;
        /**
         * Whether this binding reads one controller axis.
         */
        controllerAxisSingle: boolean;
        /**
         * The optional axis index.
         */
        axisIndex?: number;
        /**
         * The axis index Y.
         */
        axisIndexY: number;

        /**
         * Whether the device exposes raw joystick axes and buttons.
         */
        isJoystick: boolean;

        /**
         * Creates an axis binding driven by mouse motion.
         *
         * @returns The newly created value.
         */
        static fromMouse(): AxisTrigger;
        /**
         * Creates a two-axis binding; arguments are positive X, negative X, positive Y, then negative Y.
         *
         * @param positiveX - The key bound to positive horizontal movement.
         * @param negativeX - The key bound to negative horizontal movement.
         * @param positiveY - The key bound to positive vertical movement.
         * @param negativeY - The key bound to negative vertical movement.
         * @returns The newly created value.
         */
        static fromKeys(
            positiveX: Key,
            negativeX: Key,
            positiveY: Key,
            negativeY: Key,
        ): AxisTrigger;
        /**
         * Binds a controller axis or pair; single selects one axis, otherwise axisIndexY supplies the second axis.
         *
         * @param controllerId - The runtime controller identifier.
         * @param axisIndex - The zero-based controller axis index.
         * @param single - Whether to read a single axis instead of an axis pair.
         * @param axisIndexY - The zero-based controller axis used for Y.
         * @returns The newly created value.
         */
        static fromControllerAxis(
            controllerId: number,
            axisIndex: number,
            single: boolean,
            axisIndexY?: number,
        ): AxisTrigger;
    }

    /**
     * Axis action sample containing value-based input and motion deltas with flags identifying the available sources.
     */
    export type AxisPacket = {
        /**
         * The horizontal motion delta.
         */
        deltaX: number;
        /**
         * The vertical motion delta.
         */
        deltaY: number;
        /**
         * The X.
         */
        x: number;
        /**
         * The Y.
         */
        y: number;
        /**
         * The horizontal value-based input.
         */
        valueX: number;
        /**
         * The vertical value-based input.
         */
        valueY: number;
        /**
         * The horizontal delta-based input.
         */
        inputDeltaX: number;
        /**
         * The vertical delta-based input.
         */
        inputDeltaY: number;
        /**
         * Whether value-based input contributed to this sample.
         */
        hasValueInput: boolean;
        /**
         * Whether delta-based input contributed to this sample.
         */
        hasDeltaInput: boolean;
    };

    /**
     * Mouse position and motion offsets passed to interactive callbacks.
     */
    export type MousePacket = {
        /**
         * The cursor's horizontal position.
         */
        xpos: number;
        /**
         * The cursor's vertical position.
         */
        ypos: number;
        /**
         * The horizontal input offset since the previous event.
         */
        xoffset: number;
        /**
         * The vertical input offset since the previous event.
         */
        yoffset: number;
        /**
         * Whether mouse pitch is constrained.
         */
        constrainPitch: boolean;
        /**
         * Whether this is the first mouse sample after initialization or capture.
         */
        firstMouse: boolean;
    };

    /**
     * Horizontal and vertical scroll offsets.
     */
    export type MouseScrollPacket = {
        /**
         * The horizontal input offset since the previous event.
         */
        xoffset: number;
        /**
         * The vertical input offset since the previous event.
         */
        yoffset: number;
    };

    /**
     * Named button or axis mapping. Register it before querying it.
     *
     * @example
     * ```ts
     * import { Input, InputAction, Key, Trigger } from "atlas/input";
     * Input.addAction(InputAction.createButtonAction("jump", [Trigger.fromKey(Key.Space)]));
     * ```
     */
    export class InputAction {
        /**
         * The digital bindings assigned to this action.
         */
        triggers: Trigger[];
        /**
         * The axis bindings assigned to this action.
         */
        axisTriggers: AxisTrigger[];
        /**
         * The human-readable name.
         */
        name: string;
        /**
         * Whether this action produces axis data.
         */
        isAxis: boolean;
        /**
         * Whether this action represents a single scalar axis.
         */
        isAxisSingle: boolean;
        /**
         * Whether to normalize the two-dimensional action value.
         */
        normalized: boolean;
        /**
         * Whether to invert the action Y axis.
         */
        invertY: boolean;

        /**
         * Creates a named digital action from button bindings; register the returned action before polling it.
         *
         * @param name - The name used to identify the value.
         * @param triggers - The collection of triggers.
         * @returns The newly created value.
         */
        static createButtonAction(
            name: string,
            triggers: Trigger[],
        ): InputAction;
        /**
         * Creates a named axis action from axis bindings; register the returned action before polling it.
         *
         * @param name - The name used to identify the value.
         * @param axisTriggers - The collection of axis triggers.
         * @returns The newly created value.
         */
        static createAxisAction(
            name: string,
            axisTriggers: AxisTrigger[],
        ): InputAction;
        /**
         * Creates a single axis using positive and negative button bindings.
         *
         * @param name - The name used to identify the value.
         * @param positiveTrigger - The digital binding that drives the axis toward `1`.
         * @param negativeTrigger - The digital binding that drives the axis toward `-1`.
         * @returns The newly created value.
         */
        static createSingleAxisAction(
            name: string,
            positiveTrigger: Trigger,
            negativeTrigger: Trigger,
        ): InputAction;
    }

    /**
     * Global input polling, text input, mouse capture, and named action registration.
     */
    export const Input: {
        /**
         * Registers an input action and returns the registered action.
         *
         * @param action - The named input action to register.
         * @returns The registered input action.
         */
        addAction(action: InputAction): InputAction;
        /**
         * Clears the registered input actions.
         */
        resetActions(): void;

        /**
         * Reports whether key active.
         *
         * @param key - The keyboard key.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isKeyActive(key: Key): boolean;
        /**
         * Reports whether key pressed.
         *
         * @param key - The keyboard key.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isKeyPressed(key: Key): boolean;
        /**
         * Reports whether mouse button active.
         *
         * @param button - The mouse or controller button.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isMouseButtonActive(button: MouseButton): boolean;
        /**
         * Reports whether mouse button pressed.
         *
         * @param button - The mouse or controller button.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isMouseButtonPressed(button: MouseButton): boolean;

        /**
         * Returns text collected by the runtime's text input system.
         *
         * @returns The text input.
         */
        getTextInput(): string;
        /**
         * Enables text input collection.
         */
        startTextInput(): void;
        /**
         * Disables text input collection.
         */
        stopTextInput(): void;
        /**
         * Reports whether text input collection is enabled.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isTextInputActive(): boolean;

        /**
         * Reports whether controller button pressed.
         *
         * @param controllerID - The runtime controller identifier.
         * @param buttonIndex - The zero-based device button index.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isControllerButtonPressed(
            controllerID: number,
            buttonIndex: number,
        ): boolean;
        /**
         * Returns controller axis value.
         *
         * @param controllerID - The runtime controller identifier.
         * @param axisIndex - The zero-based controller axis index.
         * @returns The controller axis value.
         */
        getControllerAxisValue(controllerID: number, axisIndex: number): number;
        /**
         * Returns controller axis pair value.
         *
         * @param controllerID - The runtime controller identifier.
         * @param axisIndexX - The zero-based controller axis used for X.
         * @param axisIndexY - The zero-based controller axis used for Y.
         * @returns The controller axis pair value.
         */
        getControllerAxisPairValue(
            controllerID: number,
            axisIndexX: number,
            axisIndexY: number,
        ): Position2d;

        /**
         * Captures the mouse for relative input.
         */
        captureMouse(): void;
        /**
         * Releases mouse capture.
         */
        releaseMouse(): void;
        /**
         * Returns mouse position.
         *
         * @returns The mouse position.
         */
        getMousePosition(): Position2d;

        /**
         * Reports whether action triggered.
         *
         * @param name - The name used to identify the value.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isActionTriggered(name: string): boolean;
        /**
         * Reports whether action currently active.
         *
         * @param name - The name used to identify the value.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isActionCurrentlyActive(name: string): boolean;
        /**
         * Returns axis action value.
         *
         * @param name - The name used to identify the value.
         * @returns The axis action value.
         */
        getAxisActionValue(name: string): AxisPacket;
    };

    /**
     * Callback interface for keyboard, mouse, scroll, and per-frame interaction.
     */
    export abstract class Interactive {
        /**
         * Called when key press occurs.
         *
         * @param key - The keyboard key.
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        abstract onKeyPress(key: Key, dt: number): void;
        /**
         * Called when key release occurs.
         *
         * @param key - The keyboard key.
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        abstract onKeyRelease(key: Key, dt: number): void;
        /**
         * Called when mouse move occurs.
         *
         * @param packet - The mouse movement or scroll event data.
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        abstract onMouseMove(packet: MousePacket, dt: number): void;
        /**
         * Called when mouse button press occurs.
         *
         * @param button - The mouse or controller button.
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        abstract onMouseButtonPress(button: MouseButton, dt: number): void;
        /**
         * Called when mouse scroll occurs.
         *
         * @param packet - The mouse movement or scroll event data.
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        abstract onMouseScroll(packet: MouseScrollPacket, dt: number): void;
        /**
         * Called when each frame occurs.
         *
         * @param dt - Elapsed time since the previous frame, in seconds.
         */
        abstract onEachFrame(dt: number): void;
    }
}

/**
 * Particle simulation and emission controls.
 */
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

    /**
     * Selects fountain-style or ambient particle emission.
     */
    export enum ParticleEmissionType {
        /**
         * Selects fountain for particle emission type.
         */
        Fountain,
        /**
         * Selects ambient for particle emission type.
         */
        Ambient,
    }

    /**
     * Lifetime, size, fading, gravity, spread, and speed variation for emitted particles.
     */
    export type ParticleSettings = {
        /**
         * The minimum lifetime.
         */
        minLifetime: number;
        /**
         * The particle's total lifetime, in seconds.
         */
        maxLifetime: number;
        /**
         * The minimum size.
         */
        minSize: number;
        /**
         * The maximum size.
         */
        maxSize: number;
        /**
         * The fade speed.
         */
        fadeSpeed: number;
        /**
         * The gravitational acceleration applied by the simulation.
         */
        gravity: number;
        /**
         * The spread.
         */
        spread: number;
        /**
         * The speed variation.
         */
        speedVariation: number;
    };

    /**
     * Particle simulation state including remaining lifetime, velocity, appearance, and activity.
     */
    export type Particle = {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The velocity.
         */
        velocity: Magnitude3d;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * The particle's remaining lifetime, in seconds.
         */
        lifetime: number;
        /**
         * The particle's total lifetime, in seconds.
         */
        maxLifetime: number;
        /**
         * The dimensions.
         */
        size: number;
        /**
         * Whether this particle is active.
         */
        active: boolean;
    };

    /**
     * Particle system with a fixed capacity, configurable spawning, and optional texturing.
     */
    export class ParticleEmitter extends GameObject {
        /**
         * The settings.
         */
        settings: ParticleSettings;
        /**
         * Creates a new particle emitter.
         *
         * @param maxParticles - The maximum number of particles that can exist simultaneously.
         */
        constructor(maxParticles: number);

        /**
         * Attaches a texture to this object.
         *
         * @param texture - The texture to use.
         */
        override attachTexture(texture: Texture): void;
        /**
         * Updates the color used by this object.
         *
         * @param color - The color to apply.
         */
        setColor(color: Color): void;
        /**
         * Enables texture.
         */
        enableTexture(): void;
        /**
         * Disables texture.
         */
        disableTexture(): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        override setPosition(position: Position3d): void;
        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        override move(position: Position3d): void;
        /**
         * Returns the current position.
         *
         * @returns The position.
         */
        getPosition(): Position3d;

        /**
         * Sets emission type.
         *
         * @param type - The type or constructor used for the operation.
         */
        setEmissionType(type: ParticleEmissionType): void;
        /**
         * Sets the particle emission direction.
         *
         * @param direction - The direction vector.
         */
        setDirection(direction: Magnitude3d): void;
        /**
         * Sets spawn radius.
         *
         * @param radius - The radius.
         */
        setSpawnRadius(radius: number): void;
        /**
         * Sets spawn rate.
         *
         * @param rate - The number of particles spawned per unit of emission time.
         */
        setSpawnRate(rate: number): void;
        /**
         * Replaces the emitter's particle settings and sends them to the runtime.
         *
         * @param settings - The complete configuration to apply.
         */
        setParticleSettings(settings: ParticleSettings): void;

        /**
         * Emit once.
         */
        emitOnce(): void;
        /**
         * Emit continuous.
         */
        emitContinuous(): void;
        /**
         * Starts particle emission.
         */
        startEmission(): void;
        /**
         * Stops spawning new particles.
         */
        stopEmission(): void;
        /**
         * Requests emission of the specified number of particles.
         *
         * @param count - The number of items or particles.
         */
        emitBurst(count: number): void;

        /**
         * This transform operation is unsupported by particle emitters and has no effect.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        override setRotation(rotation: Rotation3d): void;
        /**
         * This transform operation is unsupported by particle emitters and has no effect.
         *
         * @param scale - The scale or scale multiplier.
         */
        override setScale(scale: Scale3d): void;
        /**
         * This transform operation is unsupported by particle emitters and has no effect.
         *
         * @param target - The target position.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        override lookAt(target: Position3d, up?: Normal3d): void;
        /**
         * This transform operation is unsupported by particle emitters and has no effect.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        override rotate(rotation: Rotation3d): void;
        /**
         * This transform operation is unsupported by particle emitters and has no effect.
         *
         * @param scale - The scale or scale multiplier.
         */
        override scaleBy(scale: Scale3d): void;
        /**
         * Makes the object visible.
         */
        override show(): void;
        /**
         * Makes the object invisible.
         */
        override hide(): void;
    }
}

/**
 * Physics bodies, constraints, vehicles, and spatial query results.
 */
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

    /**
     * Ray intersection information, including the hit object and surface normal.
     */
    export type RaycastHit = {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The normalized surface direction.
         */
        normal: Normal3d;
        /**
         * The distance.
         */
        distance: number;
        /**
         * The object.
         */
        object: GameObject;
        /**
         * Whether the ray intersected an object.
         */
        didHit: boolean;
    };

    /**
     * Ray query hits and nearest-hit information; hit is null when no closest hit exists.
     */
    export type RaycastResult = {
        /**
         * The collection of hits.
         */
        hits: RaycastHit[];
        /**
         * The hit, or `null` when unavailable.
         */
        hit: RaycastHit | null;
        /**
         * The closest distance.
         */
        closestDistance: number;
    };

    /**
     * Overlap contact information. penerationAxis retains the spelling used by this API.
     */
    export type OverlapHit = {
        /**
         * The contact point.
         */
        contactPoint: Position3d;
        /**
         * The peneration axis.
         */
        penerationAxis: Point3d;
        /**
         * The penetration depth.
         */
        penetrationDepth: number;
        /**
         * The object.
         */
        object: GameObject;
    };

    /**
     * Overlap contacts and a flag indicating whether any were found.
     */
    export type OverlapResult = {
        /**
         * The collection of hits.
         */
        hits: OverlapHit[];
        /**
         * Whether the query found at least one contact.
         */
        hitAny: boolean;
    };

    /**
     * Contact encountered while sweeping a collider along a movement path.
     */
    export type SweepHit = {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The normalized surface direction.
         */
        normal: Normal3d;
        /**
         * The distance.
         */
        distance: number;
        /**
         * The percentage.
         */
        percentage: number;
        /**
         * The object.
         */
        object: GameObject;
    };

    /**
     * Movement sweep contacts, nearest contact, and resulting end position.
     */
    export type SweepResult = {
        /**
         * The collection of hits.
         */
        hits: SweepHit[];
        /**
         * The closest, or `null` when unavailable.
         */
        closest: SweepHit | null;
        /**
         * Whether the query found at least one contact.
         */
        hitAny: boolean;
        /**
         * The end position.
         */
        endPosition: Position3d;
    };

    /**
     * Identifies the physics query represented by a QueryResult.
     */
    export enum QueryOperation {
        /**
         * Selects raycast all for query operation.
         */
        RaycastAll,
        /**
         * Selects raycast for query operation.
         */
        Raycast,
        /**
         * Selects rasycast world for query operation.
         */
        RasycastWorld,
        /**
         * Selects raycast world all for query operation.
         */
        RaycastWorldAll,
        /**
         * Selects raycast tagged for query operation.
         */
        RaycastTagged,
        /**
         * Selects raycast tagged all for query operation.
         */
        RaycastTaggedAll,
        /**
         * Selects movement for query operation.
         */
        Movement,
        /**
         * Selects overlap for query operation.
         */
        Overlap,
        /**
         * Selects movement all for query operation.
         */
        MovementAll,
    }

    /**
     * Physics query payload. Inspect operation before reading the corresponding result field.
     */
    export type QueryResult = {
        /**
         * The operation.
         */
        operation: QueryOperation;
        /**
         * The optional raycast result.
         */
        raycastResult?: RaycastResult;
        /**
         * The optional overlap result.
         */
        overlapResult?: OverlapResult;
        /**
         * The optional sweep result.
         */
        sweepResult?: SweepResult;
    };

    /**
     * World attachment marker type used as a joint endpoint.
     */
    export type WorldBody = {};

    /**
     * An object or world attachment used as one endpoint of a joint.
     */
    export type JointMember = GameObject | WorldBody;

    /**
     * Selects frequency/damping-ratio or stiffness/damping spring configuration.
     */
    export enum SpringMode {
        /**
         * Selects frequency and damping for spring mode.
         */
        FrequencyAndDamping,
        /**
         * Selects stiffness and damping for spring mode.
         */
        StiffnessAndDamping,
    }

    /**
     * Coordinate space used for joint configuration.
     */
    export enum Space {
        /**
         * Selects local for space.
         */
        Local,
        /**
         * Selects global for space.
         */
        Global,
    }

    /**
     * Optional spring behavior with parameters interpreted according to mode.
     */
    export type Spring = {
        /**
         * Whether this feature is enabled.
         */
        enabled: boolean;
        /**
         * The mode.
         */
        mode: SpringMode;
        /**
         * The frequency.
         */
        frequency: number;
        /**
         * The damping ratio.
         */
        dampingRatio: number;
        /**
         * The stiffness.
         */
        stiffness: number;
        /**
         * The damping.
         */
        damping: number;
    };

    /**
     * Optional minimum and maximum joint angles.
     */
    export type AngleLimits = {
        /**
         * Whether this feature is enabled.
         */
        enabled: boolean;
        /**
         * The minimum angle.
         */
        minAngle: number;
        /**
         * The maximum angle.
         */
        maxAngle: number;
    };

    /**
     * Optional joint motor with force and torque limits.
     */
    export type Motor = {
        /**
         * Whether this feature is enabled.
         */
        enabled: boolean;
        /**
         * The maximum force.
         */
        maxForce: number;
        /**
         * The maximum torque.
         */
        maxTorque: number;
    };

    /**
     * Base component for a physical constraint between two joint members.
     */
    export abstract class Joint extends Component {
        /**
         * The parent.
         */
        parent: JointMember;
        /**
         * The child.
         */
        child: JointMember;
        /**
         * The space.
         */
        space: Space;
        /**
         * The anchor.
         */
        anchor: Position3d;
        /**
         * The break force.
         */
        breakForce: number;
        /**
         * The break torque.
         */
        breakTorque: number;

        /**
         * Initializes the component after it is attached.
         */
        override init(): void;
        /**
         * Updates the component for the current frame.
         *
         * @param deltaTime - Elapsed time since the previous frame, in seconds.
         */
        override update(deltaTime: number): void;

        /**
         * Runs immediately before the physics simulation step.
         */
        abstract override beforePhysics(): void;
        /**
         * Breaks the physical constraint between the joint members.
         */
        abstract breakJoint(): void;
    }

    /**
     * Constraint that fixes the relative transform of its two members.
     */
    export class FixedJoint extends Joint {
        /**
         * Runs immediately before the physics simulation step.
         */
        override beforePhysics(): void;
        /**
         * Breaks the physical constraint between the joint members.
         */
        override breakJoint(): void;
    }

    /**
     * Constraint allowing rotation about a hinge axis, with optional limits and motor.
     */
    export class HingeJoint extends Joint {
        /**
         * The primary hinge axis.
         */
        axis1: Normal3d;
        /**
         * The secondary hinge axis.
         */
        axis2: Normal3d;
        /**
         * The angle limits.
         */
        angleLimits: AngleLimits;
        /**
         * The motor.
         */
        motor: Motor;

        /**
         * Runs immediately before the physics simulation step.
         */
        override beforePhysics(): void;
        /**
         * Breaks the physical constraint between the joint members.
         */
        override breakJoint(): void;
    }

    /**
     * Distance constraint with spring behavior and optional length limits.
     */
    export class SpringJoint extends Joint {
        /**
         * The anchor b.
         */
        anchorB: Position3d;
        /**
         * The rest length.
         */
        restLength: number;
        /**
         * Whether minimum and maximum length limits are enforced.
         */
        useLimits: boolean;
        /**
         * The minimum length.
         */
        minLength: number;
        /**
         * The maximum length.
         */
        maxLength: number;

        /**
         * The spring.
         */
        spring: Spring;

        /**
         * Runs immediately before the physics simulation step.
         */
        override beforePhysics(): void;
        /**
         * Breaks the physical constraint between the joint members.
         */
        override breakJoint(): void;
    }

    /**
     * Wheel geometry, suspension, steering, and braking parameters.
     */
    export type VehicleWheelSettings = {
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * Whether suspension force is applied at a custom point.
         */
        enableSuspensionForcePoint: boolean;
        /**
         * The suspension force point.
         */
        suspensionForcePoint: Position3d;

        /**
         * The suspension direction.
         */
        suspensionDirection: Normal3d;
        /**
         * The steering axis.
         */
        steeringAxis: Normal3d;
        /**
         * The wheel up.
         */
        wheelUp: Normal3d;
        /**
         * The wheel forward.
         */
        wheelForward: Normal3d;

        /**
         * The suspension minimum length.
         */
        suspensionMinLength: number;
        /**
         * The suspension maximum length.
         */
        suspensionMaxLength: number;
        /**
         * The suspension preload length.
         */
        suspensionPreloadLength: number;
        /**
         * The suspension frequency hz.
         */
        suspensionFrequencyHz: number;
        /**
         * The suspension damping ratio.
         */
        suspensionDampingRatio: number;

        /**
         * The radius.
         */
        radius: number;
        /**
         * The width.
         */
        width: number;

        /**
         * The inertia.
         */
        inertia: number;
        /**
         * The angular damping.
         */
        angularDamping: number;
        /**
         * The maximum steer angle degrees.
         */
        maxSteerAngleDegrees: number;
        /**
         * The maximum brake torque.
         */
        maxBrakeTorque: number;
        /**
         * The maximum hand brake torque.
         */
        maxHandBrakeTorque: number;
    };

    /**
     * Torque distribution and differential settings for a pair of wheel indices.
     */
    export type VehicleDifferential = {
        /**
         * The left wheel.
         */
        leftWheel: number;
        /**
         * The right wheel.
         */
        rightWheel: number;
        /**
         * The differential ratio.
         */
        differentialRatio: number;
        /**
         * The left right split.
         */
        leftRightSplit: number;
        /**
         * The limited slip ratio.
         */
        limitedSlipRatio: number;
        /**
         * The engine torque ratio.
         */
        engineTorqueRatio: number;
    };

    /**
     * Engine torque, RPM limits, inertia, and angular damping.
     */
    export type VehicleEngine = {
        /**
         * The maximum torque.
         */
        maxTorque: number;
        /**
         * The minimum RPM.
         */
        minRPM: number;
        /**
         * The maximum RPM.
         */
        maxRPM: number;
        /**
         * The inertia.
         */
        inertia: number;
        /**
         * The angular damping.
         */
        angularDamping: number;
    };

    /**
     * Automatic or manual vehicle gear selection.
     */
    export enum VehicleTransmissionMode {
        /**
         * Selects auto for vehicle transmission mode.
         */
        Auto,
        /**
         * Selects manual for vehicle transmission mode.
         */
        Manual,
    }

    /**
     * Forward/reverse gear ratios, clutch behavior, and shift timing.
     */
    export type VehicleTransmission = {
        /**
         * The mode.
         */
        mode: VehicleTransmissionMode;
        /**
         * The collection of gear ratios.
         */
        gearRatios: number[];
        /**
         * The collection of reverse gear ratios.
         */
        reverseGearRatios: number[];
        /**
         * The switch time.
         */
        switchTime: number;
        /**
         * The clutch release time.
         */
        clutchReleaseTime: number;
        /**
         * The switch latency.
         */
        switchLatency: number;
        /**
         * The shift up RPM.
         */
        shiftUpRPM: number;
        /**
         * The shift down RPM.
         */
        shiftDownRPM: number;
        /**
         * The clutch strength.
         */
        clutchStrength: number;
    };

    /**
     * Engine, transmission, and differential configuration for a vehicle.
     */
    export type VehicleControllerSettings = {
        /**
         * The engine.
         */
        engine: VehicleEngine;
        /**
         * The transmission.
         */
        transmission: VehicleTransmission;
        /**
         * The collection of differentials.
         */
        differentials: VehicleDifferential[];
        /**
         * The differential limited slip ratio.
         */
        differentialLimitedSlipRatio: number;
    };

    /**
     * Vehicle axes, wheel configurations, controller settings, and stability limits.
     */
    export type VehicleSettings = {
        /**
         * The up.
         */
        up: Normal3d;
        /**
         * The forward.
         */
        forward: Normal3d;

        /**
         * The maximum pitch roll angle deg.
         */
        maxPitchRollAngleDeg: number;

        /**
         * The collection of wheels.
         */
        wheels: VehicleWheelSettings[];
        /**
         * The controller.
         */
        controller: VehicleControllerSettings;

        /**
         * The maximum slop angle deg.
         */
        maxSlopAngleDeg: number;
    };

    /**
     * Physics vehicle component with driving inputs and configurable wheels and drivetrain.
     */
    export class Vehicle extends Component {
        /**
         * The settings.
         */
        settings: VehicleSettings;
        /**
         * The forward.
         */
        forward: number;
        /**
         * The right.
         */
        right: number;
        /**
         * The brake.
         */
        brake: number;
        /**
         * The hand brake.
         */
        handBrake: number;

        /**
         * Runs when the component is attached to its parent object.
         */
        override atAttach(): void;
        /**
         * Runs immediately before the physics simulation step.
         */
        override beforePhysics(): void;

        /**
         * Requests recreation of the vehicle physics state after configuration changes.
         */
        requestRecreate(): void;

        /**
         * Initializes the component after it is attached.
         */
        override init(): void;
        /**
         * Updates the component for the current frame.
         *
         * @param deltaTime - Elapsed time since the previous frame, in seconds.
         */
        override update(deltaTime: number): void;
    }

    /**
     * Capsule collision shape specified by radius and height.
     */
    export type CapsuleCollider = {
        /**
         * The radius.
         */
        radius: number;
        /**
         * The height.
         */
        height: number;
    };

    /**
     * Box collision shape specified by its three-dimensional size.
     */
    export type BoxCollider = {
        /**
         * The dimensions.
         */
        size: Size3d;
    };

    /**
     * Sphere collision shape specified by radius.
     */
    export type SphereCollider = {
        /**
         * The radius.
         */
        radius: number;
    };

    /**
     * Mesh collision shape descriptor.
     */
    export type MeshCollider = {};

    /**
     * Supported collision shape descriptors for rigidbodies and physics queries.
     */
    export type Collider =
        | CapsuleCollider
        | BoxCollider
        | SphereCollider
        | MeshCollider;

    /**
     * Physics component providing colliders, forces, velocities, tags, and spatial queries.
     */
    export class Rigidbody extends Component {
        /**
         * The send signal.
         */
        sendSignal: string;
        /**
         * Whether the body detects contacts without producing a physical collision response.
         */
        isSensor: boolean;

        /**
         * Runs when the component is attached to its parent object.
         */
        override atAttach(): void;
        /**
         * Initializes the component after it is attached.
         */
        override init(): void;
        /**
         * Runs immediately before the physics simulation step.
         */
        override beforePhysics(): void;
        /**
         * Updates the component for the current frame.
         *
         * @param deltaTime - Elapsed time since the previous frame, in seconds.
         */
        override update(deltaTime: number): void;

        /**
         * Creates a copy of this object.
         *
         * @returns The newly created value.
         */
        clone(): Rigidbody;

        /**
         * Adds collider.
         *
         * @param collider - The collision shape used by the query.
         */
        addCollider(collider: Collider): void;

        /**
         * Sets friction.
         *
         * @param friction - The surface friction coefficient.
         */
        setFriction(friction: number): void;
        /**
         * Applies a force to the body.
         *
         * @param force - The force vector to apply.
         */
        applyForce(force: Force3d): void;
        /**
         * Applies a force at the specified position, allowing an off-center force to produce torque.
         *
         * @param force - The force vector to apply.
         * @param point - The world-space point where the force is applied.
         */
        applyForceAtPoint(force: Force3d, point: Position3d): void;
        /**
         * Applies an instantaneous impulse to the body.
         *
         * @param impulse - The impulse vector to apply.
         */
        applyImpulse(impulse: Impulse3d): void;

        /**
         * Sets linear velocity.
         *
         * @param velocity - The velocity vector.
         */
        setLinearVelocity(velocity: Velocity3d): void;
        /**
         * Adds to the body's current linear velocity.
         *
         * @param velocity - The velocity vector.
         */
        addLinearVelocity(velocity: Velocity3d): void;
        /**
         * Sets angular velocity.
         *
         * @param velocity - The velocity vector.
         */
        setAngularVelocity(velocity: Velocity3d): void;
        /**
         * Adds to the body's current angular velocity.
         *
         * @param velocity - The velocity vector.
         */
        addAngularVelocity(velocity: Velocity3d): void;

        /**
         * Sets maximum linear velocity.
         *
         * @param velocity - The velocity vector.
         */
        setMaxLinearVelocity(velocity: number): void;
        /**
         * Sets maximum angular velocity.
         *
         * @param velocity - The velocity vector.
         */
        setMaxAngularVelocity(velocity: number): void;

        /**
         * Returns linear velocity.
         *
         * @returns The linear velocity.
         */
        getLinearVelocity(): Velocity3d;
        /**
         * Returns angular velocity.
         *
         * @returns The angular velocity.
         */
        getAngularVelocity(): Velocity3d;
        /**
         * Returns velocity.
         *
         * @returns The velocity.
         */
        getVelocity(): Velocity3d;

        /**
         * Raycast.
         *
         * @param direction - The direction vector.
         * @param maxDistance - The maximum query distance.
         * @returns The raycast query result.
         */
        raycast(direction: Normal3d, maxDistance: number): RaycastResult;
        /**
         * Raycast all.
         *
         * @param direction - The direction vector.
         * @param maxDistance - The maximum query distance.
         * @returns The raycast query result.
         */
        raycastAll(direction: Normal3d, maxDistance: number): RaycastResult;
        /**
         * Raycast world.
         *
         * @param origin - The query origin in world space.
         * @param direction - The direction vector.
         * @param maxDistance - The maximum query distance.
         * @returns The raycast query result.
         */
        raycastWorld(
            origin: Position3d,
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;
        /**
         * Raycast world all.
         *
         * @param origin - The query origin in world space.
         * @param direction - The direction vector.
         * @param maxDistance - The maximum query distance.
         * @returns The raycast query result.
         */
        raycastWorldAll(
            origin: Position3d,
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;
        /**
         * Raycast tagged.
         *
         * @param tags - The tags used to filter objects.
         * @param direction - The direction vector.
         * @param maxDistance - The maximum query distance.
         * @returns The raycast query result.
         */
        raycastTagged(
            tags: string[],
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;
        /**
         * Raycast tagged all.
         *
         * @param tags - The tags used to filter objects.
         * @param direction - The direction vector.
         * @param maxDistance - The maximum query distance.
         * @returns The raycast query result.
         */
        raycastTaggedAll(
            tags: string[],
            direction: Normal3d,
            maxDistance: number,
        ): RaycastResult;

        /**
         * Overlap.
         *
         * @returns The overlap query result.
         */
        overlap(): OverlapResult;
        /**
         * Overlap with collider.
         *
         * @param collider - The collision shape used by the query.
         * @returns The overlap query result.
         */
        overlapWithCollider(collider: Collider): OverlapResult;
        /**
         * Overlap with collider world.
         *
         * @param collider - The collision shape used by the query.
         * @param position - The position or translation offset.
         * @returns The overlap query result.
         */
        overlapWithColliderWorld(
            collider: Collider,
            position: Position3d,
        ): OverlapResult;

        /**
         * Predict movement with collider.
         *
         * @param endPosition - The requested sweep destination in world space.
         * @param collider - The collision shape used by the query.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementWithCollider(
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        /**
         * Predict movement.
         *
         * @param endPosition - The requested sweep destination in world space.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovement(endPosition: Position3d): SweepResult;
        /**
         * Predict movement with collider world.
         *
         * @param startPosition - The sweep origin in world space.
         * @param endPosition - The requested sweep destination in world space.
         * @param collider - The collision shape used by the query.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementWithColliderWorld(
            startPosition: Position3d,
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        /**
         * Predict movement world.
         *
         * @param startPosition - The sweep origin in world space.
         * @param endPosition - The requested sweep destination in world space.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementWorld(
            startPosition: Position3d,
            endPosition: Position3d,
        ): SweepResult;
        /**
         * Predict movement with collider all.
         *
         * @param endPosition - The requested sweep destination in world space.
         * @param collider - The collision shape used by the query.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementWithColliderAll(
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        /**
         * Predict movement all.
         *
         * @param endPosition - The requested sweep destination in world space.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementAll(endPosition: Position3d): SweepResult;
        /**
         * Predict movement with collider all world.
         *
         * @param startPosition - The sweep origin in world space.
         * @param endPosition - The requested sweep destination in world space.
         * @param collider - The collision shape used by the query.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementWithColliderAllWorld(
            startPosition: Position3d,
            endPosition: Position3d,
            collider: Collider,
        ): SweepResult;
        /**
         * Predict movement all world.
         *
         * @param startPosition - The sweep origin in world space.
         * @param endPosition - The requested sweep destination in world space.
         * @returns The predicted movement and its sweep contacts.
         */
        predictMovementAllWorld(
            startPosition: Position3d,
            endPosition: Position3d,
        ): SweepResult;

        /**
         * Reports whether this value has tag.
         *
         * @param tag - The object tag to test, add, or remove.
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        hasTag(tag: string): boolean;
        /**
         * Adds tag.
         *
         * @param tag - The object tag to test, add, or remove.
         */
        addTag(tag: string): void;
        /**
         * Removes tag.
         *
         * @param tag - The object tag to test, add, or remove.
         */
        removeTag(tag: string): void;

        /**
         * Sets damping.
         *
         * @param linearDamping - The resistance applied to linear motion.
         * @param angularDamping - The resistance applied to angular motion.
         */
        setDamping(linearDamping: number, angularDamping: number): void;
        /**
         * Sets mass.
         *
         * @param mass - The body mass used by the physics simulation.
         */
        setMass(mass: number): void;
        /**
         * Sets collision restitution (bounciness); the method name retains its existing spelling.
         *
         * @param restitution - The collision bounciness coefficient.
         */
        setRestituition(restitution: number): void;
        /**
         * Selects a static, dynamically simulated, or kinematic body.
         *
         * @param motionType - The body's simulation mode.
         */
        setMotionType(motionType: "Static" | "Dynamic" | "Kinematic"): void;
    }

    /**
     * Rigidbody initialized as a sensor for detecting contacts and sending signals.
     */
    export class Sensor extends Rigidbody {
        /**
         * Creates a new sensor.
         */
        constructor(); // sets isSensor to true

        /**
         * Sets signal.
         *
         * @param signal - The signal name.
         */
        setSignal(signal: string): void;
    }
}

/**
 * Noise, procedural terrain, and biome APIs.
 */
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

    /**
     * Two-dimensional Perlin noise generator with optional seed.
     */
    export class PerlinNoise {
        /**
         * Creates a new perlin noise.
         *
         * @param seed - The deterministic noise seed. When omitted, the runtime chooses its default seed.
         */
        constructor(seed?: number);
        /**
         * Samples the noise function at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The sampled noise value.
         */
        noise(x: number, y: number): number;
    }

    /**
     * Two-dimensional simplex noise sampling.
     */
    export class SimplexNoise {
        /**
         * Samples the noise function at the supplied coordinates.
         *
         * @param xin - The X coordinate at which to sample simplex noise.
         * @param yin - The Y coordinate at which to sample simplex noise.
         * @returns The sampled noise value.
         */
        static noise(xin: number, yin: number): number;
    }

    /**
     * Cellular noise generator configured by point count and optional seed.
     */
    export class WorleyNoise {
        /**
         * Creates a new worley noise.
         *
         * @param numPoints - The number of feature points used by the cellular noise.
         * @param seed - The deterministic noise seed. When omitted, the runtime chooses its default seed.
         */
        constructor(numPoints: number, seed?: number);
        /**
         * Samples the noise function at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The sampled noise value.
         */
        noise(x: number, y: number): number;
    }

    /**
     * Layered noise generator configured by octave count and persistence.
     */
    export class FractalNoise {
        /**
         * Creates a new fractal noise.
         *
         * @param o - The number of noise octaves.
         * @param p - The persistence applied between noise octaves.
         */
        constructor(o: number, p: number);
        /**
         * Samples the noise function at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The sampled noise value.
         */
        noise(x: number, y: number): number;
    }

    /**
     * Convenience entry points for the supported two-dimensional noise functions.
     */
    export class Noise {
        /**
         * Perlin.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The sampled noise value.
         */
        static perlin(x: number, y: number): number;
        /**
         * Simplex.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The sampled noise value.
         */
        static simplex(x: number, y: number): number;
        /**
         * Worley.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The sampled noise value.
         */
        static worley(x: number, y: number): number;
        /**
         * Fractal.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @param octaves - The number of layered noise octaves.
         * @param persistence - The amplitude multiplier between octaves.
         * @returns The sampled noise value.
         */
        static fractal(
            x: number,
            y: number,
            octaves: number,
            persistence: number,
        ): number;
        static seed: number;
        static initializedSeed: boolean;
    }

    /**
     * Terrain appearance and height, moisture, and temperature selection settings.
     */
    export class Biome {
        /**
         * The human-readable name.
         */
        name: string;
        /**
         * The texture used for rendering.
         */
        texture: Texture;
        /**
         * The RGBA color.
         */
        color: Color;
        /**
         * Whether the texture is used instead of the fallback color.
         */
        useTexture: boolean;

        /**
         * Attaches a texture to this object.
         *
         * @param texture - The texture to use.
         */
        attachTexture(texture: Texture): void;

        /**
         * The minimum height.
         */
        minHeight: number;
        /**
         * The maximum height.
         */
        maxHeight: number;
        /**
         * The minimum moisture.
         */
        minMoisture: number;
        /**
         * The maximum moisture.
         */
        maxMoisture: number;
        /**
         * The minimum temperature.
         */
        minTemperature: number;
        /**
         * The maximum temperature.
         */
        maxTemperature: number;

        /**
         * Creates a new biome.
         *
         * @param name - The name used to identify the value.
         * @param texture - The texture to use.
         * @param color - The color to apply.
         * @param useTexture - Whether the biome uses its texture.
         */
        constructor(
            name: string,
            texture: Texture,
            color: Color,
            useTexture: boolean,
        );

        /**
         * The current weather condition.
         */
        condition: BiomeFunction;
    }

    /**
     * Callback receiving a biome for custom biome configuration.
     *
     * @param biome - The biome to add or configure.
     */
    export type BiomeFunction = (biome: Biome) => void;

    /**
     * Terrain object created from a heightmap or procedural generator, with biome support.
     */
    export class Terrain extends GameObject {
        /**
         * Attaches a texture to this object.
         *
         * @param texture - The texture to use.
         */
        attachTexture(texture: Texture): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        move(position: Position3d): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        setRotation(rotation: Rotation3d): void;
        /**
         * Orients toward the target position.
         *
         * @param target - The target position.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        lookAt(target: Position3d, up?: Normal3d): void;
        /**
         * Applies an incremental rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        rotate(rotation: Rotation3d): void;
        /**
         * Replaces the scale factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        setScale(scale: Scale3d): void;
        /**
         * Multiplies the current scale component-wise by the supplied factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        scaleBy(scale: Scale3d): void;
        /**
         * Makes the object visible.
         */
        show(): void;
        /**
         * Makes the object invisible.
         */
        hide(): void;

        /**
         * The heightmap.
         */
        heightmap: Resource;
        /**
         * The moisture texture.
         */
        moistureTexture: Texture;
        /**
         * The temperature texture.
         */
        temperatureTexture: Texture;
        /**
         * The generator.
         */
        generator: TerrainGenerator;

        /**
         * Whether the terrain was created from a heightmap resource.
         */
        createdWithMap: boolean;
        /**
         * The width.
         */
        width: number;
        /**
         * The length.
         */
        length: number;
        /**
         * The height.
         */
        height: number;

        /**
         * Adds a biome to the terrain.
         *
         * @param biome - The biome to add or configure.
         */
        addBiome(biome: Biome): void;

        /**
         * Creates terrain using the supplied height generator.
         *
         * @param generator - The terrain generator to use.
         * @returns The newly created value.
         */
        static fromGenerator<T extends TerrainGenerator>(generator: T): Terrain;
        /**
         * Creates terrain from a heightmap resource.
         *
         * @param heightmap - The image resource containing terrain heights.
         * @returns The newly created value.
         */
        static fromHeightmap(heightmap: Resource): Terrain;

        /**
         * The maximum peak.
         */
        maxPeak: number;
        /**
         * The sea level.
         */
        seaLevel: number;
    }

    /**
     * Base class for a height function that can be applied to terrain.
     */
    export abstract class TerrainGenerator {
        /**
         * Samples the procedural terrain height at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The generated terrain height.
         */
        abstract generateHeight(x: number, y: number): number;
        /**
         * Applies this generator to a terrain object.
         *
         * @param terrain - The terrain to modify.
         */
        applyTo(terrain: Terrain): void;
    }

    /**
     * Procedural hill height generator configured by scale and amplitude.
     */
    export class HillGenerator extends TerrainGenerator {
        /**
         * Creates a new hill generator.
         *
         * @param scale - The scale or scale multiplier.
         * @param amplitude - The maximum height contribution of the generated noise.
         */
        constructor(scale: number, amplitude: number);

        /**
         * Samples the procedural terrain height at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The generated terrain height.
         */
        override generateHeight(x: number, y: number): number;
    }

    /**
     * Layered mountain height generator configured by scale, amplitude, octaves, and persistence.
     */
    export class MountainGenerator extends TerrainGenerator {
        /**
         * Creates a new mountain generator.
         *
         * @param scale - The scale or scale multiplier.
         * @param amplitude - The maximum height contribution of the generated noise.
         * @param octaves - The number of layered noise octaves.
         * @param persistance - The amplitude multiplier between octaves; the name retains the API's spelling.
         */
        constructor(
            scale: number,
            amplitude: number,
            octaves: number,
            persistance: number,
        );

        /**
         * Samples the procedural terrain height at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The generated terrain height.
         */
        override generateHeight(x: number, y: number): number;
    }

    /**
     * Procedural plain height generator configured by scale and amplitude.
     */
    export class PlainGenerator extends TerrainGenerator {
        /**
         * Creates a new plain generator.
         *
         * @param scale - The scale or scale multiplier.
         * @param amplitude - The maximum height contribution of the generated noise.
         */
        constructor(scale: number, amplitude: number);

        /**
         * Samples the procedural terrain height at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The generated terrain height.
         */
        override generateHeight(x: number, y: number): number;
    }

    /**
     * Procedural island height generator configured by feature count and scale.
     */
    export class IslandGenerator extends TerrainGenerator {
        /**
         * Creates a new island generator.
         *
         * @param numFeatures - The number of procedural island features to generate.
         * @param scale - The scale or scale multiplier.
         */
        constructor(numFeatures: number, scale: number);

        /**
         * Samples the procedural terrain height at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The generated terrain height.
         */
        override generateHeight(x: number, y: number): number;
    }

    /**
     * Terrain generator that combines multiple child generators.
     */
    export class CompoundGenerator extends TerrainGenerator {
        /**
         * Adds generator.
         *
         * @param generator - The terrain generator to use.
         */
        addGenerator<T extends TerrainGenerator>(generator: T): void;
        /**
         * Samples the procedural terrain height at the supplied coordinates.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @returns The generated terrain height.
         */
        override generateHeight(x: number, y: number): number;
    }
}

/**
 * Audio playback, spatial listeners, and sound effects.
 */
declare module "finewave" {
    import { Position3d } from "atlas/units";
    import { Resource } from "atlas";

    /**
     * Audio device controls, master volume, and spatial listener state.
     */
    export class AudioEngine {
        /**
         * Sets listener position.
         *
         * @param position - The position or translation offset.
         */
        setListenerPosition(position: Position3d): void;
        /**
         * Sets listener orientation.
         *
         * @param forward - The listener's forward direction.
         * @param up - The upward reference direction. Defaults to the positive Y axis where supported.
         */
        setListenerOrientation(forward: Position3d, up: Position3d): void;
        /**
         * Sets listener velocity.
         *
         * @param velocity - The velocity vector.
         */
        setListenerVelocity(velocity: Position3d): void;
        /**
         * Sets master volume.
         *
         * @param volume - The playback or master volume level.
         */
        setMasterVolume(volume: number): void;
        /**
         * The active audio output device name.
         */
        deviceName: string;
    }

    /**
     * Audio data loaded from a resource for use by an AudioSource.
     */
    export class AudioData {
        /**
         * Creates a value from resource.
         *
         * @param resource - The resource to use.
         * @returns The newly created value.
         */
        static fromResource(resource: Resource): AudioData;
        /**
         * Whether the audio data contains one channel.
         */
        isMono: boolean;
        /**
         * The resource backing this object.
         */
        resource: Resource;
    }

    /**
     * Playback controls for audio data, including spatialization and effects.
     */
    export class AudioSource {
        /**
         * Sets data.
         *
         * @param data - The decoded audio data to play.
         */
        setData(data: AudioData): void;
        /**
         * Creates a value from file.
         *
         * @param resource - The resource to use.
         */
        fromFile(resource: Resource): void;
        /**
         * Starts or resumes playback.
         */
        play(): void;
        /**
         * Pauses playback.
         */
        pause(): void;
        /**
         * Stops playback.
         */
        stop(): void;
        /**
         * Enables or disables repeated playback.
         *
         * @param loop - Whether playback repeats after reaching the end.
         */
        setLoop(loop: boolean): void;
        /**
         * Sets volume.
         *
         * @param volume - The playback or master volume level.
         */
        setVolume(volume: number): void;
        /**
         * Sets pitch.
         *
         * @param pitch - The playback-rate multiplier.
         */
        setPitch(pitch: number): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        setPosition(position: Position3d): void;
        /**
         * Sets velocity.
         *
         * @param velocity - The velocity vector.
         */
        setVelocity(velocity: Position3d): void;

        /**
         * Reports whether playing.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isPlaying(): boolean;
        /**
         * Starts playback at the supplied offset in seconds.
         *
         * @param position - The position or translation offset.
         */
        playFrom(position: number): void;
        /**
         * Disables positional audio processing for this source.
         */
        disableSpatialization(): void;
        /**
         * Applies an audio effect to this source.
         *
         * @param effect - The audio or post-processing effect to apply.
         */
        applyEffect(effect: AudioEffect): void;
        /**
         * Returns the current position.
         *
         * @returns The position.
         */
        getPosition(): Position3d;
        /**
         * Returns listener position.
         *
         * @returns The listener position.
         */
        getListenerPosition(): Position3d;
        /**
         * Enables positional audio processing for this source.
         */
        useSpatialization(): void;
    }

    /**
     * Base type for effects that can be applied to an AudioSource.
     */
    export abstract class AudioEffect {}

    /**
     * Reverberation effect with room, damping, stereo width, and wet/dry controls.
     */
    export class Reverb extends AudioEffect {
        /**
         * Sets room size.
         *
         * @param size - The size or dimensions.
         */
        setRoomSize(size: number): void;
        /**
         * Sets damping.
         *
         * @param damping - The amount by which the effect attenuates over time.
         */
        setDamping(damping: number): void;
        /**
         * Sets wet level.
         *
         * @param level - The wet or dry signal level.
         */
        setWetLevel(level: number): void;
        /**
         * Sets dry level.
         *
         * @param level - The wet or dry signal level.
         */
        setDryLevel(level: number): void;
        /**
         * Sets width.
         *
         * @param width - The width.
         */
        setWidth(width: number): void;
    }

    /**
     * Delay effect with decay and wet/dry controls.
     */
    export class Echo extends AudioEffect {
        /**
         * Sets the echo delay in seconds.
         *
         * @param delay - The echo delay in seconds.
         */
        setDelay(delay: number): void;
        /**
         * Sets decay.
         *
         * @param decay - The rate at which the repeated signal loses energy.
         */
        setDecay(decay: number): void;
        /**
         * Sets wet level.
         *
         * @param level - The wet or dry signal level.
         */
        setWetLevel(level: number): void;
        /**
         * Sets dry level.
         *
         * @param level - The wet or dry signal level.
         */
        setDryLevel(level: number): void;
    }

    /**
     * Distortion effect with edge, gain, and low-pass filtering controls.
     */
    export class Distortion extends AudioEffect {
        /**
         * Sets edge.
         *
         * @param edge - The distortion edge intensity.
         */
        setEdge(edge: number): void;
        /**
         * Sets gain.
         *
         * @param gain - The output gain applied by the distortion effect.
         */
        setGain(gain: number): void;
        /**
         * Sets lowpass cutoff.
         *
         * @param cutoff - The low-pass filter cutoff frequency.
         */
        setLowpassCutoff(cutoff: number): void;
    }
}

/**
 * UI elements, layouts, fonts, styles, and themes.
 */
declare module "graphite" {
    import { UIObject, Resource } from "atlas";
    import { Texture } from "atlas/graphics";
    import { Position2d, Position3d, Color, Size2d, Size3d } from "atlas/units";

    /**
     * Textured UI element with size, tint, and styling.
     */
    export class Image extends UIObject {
        /**
         * The texture used for rendering.
         */
        texture: Texture;
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The dimensions.
         */
        size: Size2d;
        /**
         * The tint.
         */
        tint: Color;

        /**
         * Creates a new image.
         */
        constructor();
        /**
         * Creates a new image.
         *
         * @param texture - The texture to use.
         * @param size - The size or dimensions.
         * @param position - The position or translation offset.
         * @param tint - The image tint color.
         */
        constructor(
            texture: Texture,
            size: Size2d,
            position: Position2d,
            tint: Color,
        );

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;
        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * Returns the element's style.
         *
         * @returns The element's current style.
         */
        style(): UIStyle;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns This `Image` instance, allowing method chaining.
         */
        setStyle(style: UIStyle): Image;

        /**
         * Sets texture.
         *
         * @param texture - The texture to use.
         */
        setTexture(texture: Texture): void;
        /**
         * Sets size.
         *
         * @param size - The size or dimensions.
         */
        setSize(size: Size2d): void;
    }

    /**
     * Text field state supplied to a change callback.
     */
    export type TextFieldChangeEvent = {
        /**
         * The text.
         */
        text: string;
        /**
         * The cursor position.
         */
        cursorPosition: number;
        /**
         * Whether the text field has keyboard focus.
         */
        focused: boolean;
    };

    /**
     * Button label supplied to a click callback.
     */
    export type ButtonClickEvent = {
        /**
         * The label.
         */
        label: string;
    };

    /**
     * Checkbox label and checked state supplied to a toggle callback.
     */
    export type CheckboxToggleEvent = {
        /**
         * The label.
         */
        label: string;
        /**
         * Whether the checkbox is checked.
         */
        checked: boolean;
    };

    /**
     * Callback types associated with TextField.
     */
    export namespace TextField {
        /**
         * Defines change callback.
         *
         * @param event - The event data.
         */
        export type ChangeCallback = (event: TextFieldChangeEvent) => void;
    }

    /**
     * Editable text input with focus, cursor, placeholder, and change callbacks.
     */
    export class TextField extends UIObject {
        /**
         * The text.
         */
        text: string;
        /**
         * The placeholder.
         */
        placeholder: string;
        /**
         * The font.
         */
        font: Font;
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The font size.
         */
        fontSize: number;
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The maximum width.
         */
        maximumWidth: number;
        /**
         * The text color.
         */
        textColor: Color;
        /**
         * The placeholder color.
         */
        placeholderColor: Color;
        /**
         * The background color.
         */
        backgroundColor: Color;
        /**
         * The border color.
         */
        borderColor: Color;
        /**
         * The focused border color.
         */
        focusedBorderColor: Color;
        /**
         * The cursor color.
         */
        cursorColor: Color;

        /**
         * Creates a new text field.
         */
        constructor();

        /**
         * Creates a new text field.
         *
         * @param font - The font to use.
         * @param maximumWidth - The maximum text field width.
         * @param position - The position or translation offset.
         * @param text - The text content.
         * @param placeholder - The text shown while the field is empty.
         */
        constructor(
            font: Font,
            maximumWidth: number,
            position: Position2d,
            text: string,
            placeholder: string,
        );

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;
        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * Returns text.
         *
         * @returns The text.
         */
        getText(): string;
        /**
         * Reports whether focused.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isFocused(): boolean;
        /**
         * Returns cursor index.
         *
         * @returns The cursor index.
         */
        getCursorIndex(): number;
        /**
         * Returns the element's style.
         *
         * @returns The element's current style.
         */
        style(): UIStyle;

        /**
         * Sets text.
         *
         * @param text - The text content.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setText(text: string): TextField;
        /**
         * Sets placeholder.
         *
         * @param placeholder - The text shown while the field is empty.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setPlaceholder(placeholder: string): TextField;
        /**
         * Sets padding.
         *
         * @param padding - The inner spacing around the content.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setPadding(padding: Size2d): TextField;
        /**
         * Sets maximum width.
         *
         * @param width - The width.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setMaximumWidth(width: number): TextField;
        /**
         * Sets font size.
         *
         * @param size - The size or dimensions.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setFontSize(size: number): TextField;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setStyle(style: UIStyle): TextField;
        /**
         * Registers the text-change callback and returns this text field for chaining.
         *
         * @param callback - The callback to invoke.
         * @returns This `TextField` instance, allowing method chaining.
         */
        setOnChange(callback: TextField.ChangeCallback): TextField;

        /**
         * Gives keyboard focus to the text field.
         */
        focus(): void;
        /**
         * Removes keyboard focus from the text field.
         */
        blur(): void;
    }

    /**
     * Callback types associated with Button.
     */
    export namespace Button {
        /**
         * Defines click callback.
         *
         * @param event - The event data.
         */
        export type ClickCallback = (event: ButtonClickEvent) => void;
    }

    /**
     * Clickable text button with hover, enabled state, and click callback support.
     */
    export class Button extends UIObject {
        /**
         * The label.
         */
        label: string;
        /**
         * The font.
         */
        font: Font;
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The font size.
         */
        fontSize: number;
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The minimum size.
         */
        minimumSize: Size2d;
        /**
         * The text color.
         */
        textColor: Color;
        /**
         * The background color.
         */
        backgroundColor: Color;
        /**
         * The hover background color.
         */
        hoverBackgroundColor: Color;
        /**
         * The pressed background color.
         */
        pressedBackgroundColor: Color;
        /**
         * The border color.
         */
        borderColor: Color;
        /**
         * The hover border color.
         */
        hoverBorderColor: Color;
        /**
         * Whether this feature is enabled.
         */
        enabled: boolean;

        /**
         * Creates a new button.
         */
        constructor();

        /**
         * Creates a new button.
         *
         * @param font - The font to use.
         * @param label - The displayed label.
         * @param position - The position or translation offset.
         */
        constructor(font: Font, label: string, position: Position2d);

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;
        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * Returns label.
         *
         * @returns The label.
         */
        getLabel(): string;
        /**
         * Reports whether hovered.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isHovered(): boolean;
        /**
         * Reports whether enabled.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isEnabled(): boolean;

        /**
         * Returns the element's style.
         *
         * @returns The element's current style.
         */
        style(): UIStyle;

        /**
         * Sets label.
         *
         * @param label - The displayed label.
         * @returns This `Button` instance, allowing method chaining.
         */
        setLabel(label: string): Button;
        /**
         * Sets padding.
         *
         * @param padding - The inner spacing around the content.
         * @returns This `Button` instance, allowing method chaining.
         */
        setPadding(padding: Size2d): Button;
        /**
         * Sets minimum size.
         *
         * @param size - The size or dimensions.
         * @returns This `Button` instance, allowing method chaining.
         */
        setMinimumSize(size: Size2d): Button;
        /**
         * Sets font size.
         *
         * @param size - The size or dimensions.
         * @returns This `Button` instance, allowing method chaining.
         */
        setFontSize(size: number): Button;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns This `Button` instance, allowing method chaining.
         */
        setStyle(style: UIStyle): Button;
        /**
         * Registers the click callback and returns this button for chaining.
         *
         * @param callback - The callback to invoke.
         * @returns This `Button` instance, allowing method chaining.
         */
        setOnClick(callback: Button.ClickCallback): Button;
        /**
         * Sets enabled.
         *
         * @param enabled - Whether to enable the feature.
         */
        setEnabled(enabled: boolean): void;
    }

    /**
     * Callback types associated with Checkbox.
     */
    export namespace Checkbox {
        /**
         * Defines toggle callback.
         *
         * @param event - The event data.
         */
        export type ToggleCallback = (event: CheckboxToggleEvent) => void;
    }

    /**
     * Toggleable checkbox with a label and change callback support.
     */
    export class Checkbox extends UIObject {
        /**
         * The label.
         */
        label: string;
        /**
         * The font.
         */
        font: Font;
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The font size.
         */
        fontSize: number;
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The box size.
         */
        boxSize: number;
        /**
         * The spacing.
         */
        spacing: number;
        /**
         * Whether the checkbox is checked.
         */
        checked: boolean;
        /**
         * Whether this feature is enabled.
         */
        enabled: boolean;
        /**
         * The text color.
         */
        textColor: Color;
        /**
         * The box background color.
         */
        boxBackgroundColor: Color;
        /**
         * The hover box background color.
         */
        hoverBoxBackgroundColor: Color;
        /**
         * The border color.
         */
        borderColor: Color;
        /**
         * The active border color.
         */
        activeBorderColor: Color;
        /**
         * The check color.
         */
        checkColor: Color;

        /**
         * Creates a new checkbox.
         */
        constructor();

        /**
         * Creates a new checkbox.
         *
         * @param font - The font to use.
         * @param label - The displayed label.
         * @param position - The position or translation offset.
         */
        constructor(font: Font, label: string, position: Position2d);

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;
        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * Returns label.
         *
         * @returns The label.
         */
        getLabel(): string;
        /**
         * Reports whether checked.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isChecked(): boolean;
        /**
         * Reports whether hovered.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isHovered(): boolean;
        /**
         * Reports whether enabled.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isEnabled(): boolean;

        /**
         * Returns the element's style.
         *
         * @returns The element's current style.
         */
        style(): UIStyle;

        /**
         * Sets label.
         *
         * @param label - The displayed label.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setLabel(label: string): Checkbox;
        /**
         * Sets padding.
         *
         * @param padding - The inner spacing around the content.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setPadding(padding: Size2d): Checkbox;
        /**
         * Sets font size.
         *
         * @param size - The size or dimensions.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setFontSize(size: number): Checkbox;
        /**
         * Sets box size.
         *
         * @param size - The size or dimensions.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setBoxSize(size: number): Checkbox;
        /**
         * Sets spacing.
         *
         * @param spacing - The space between adjacent children.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setSpacing(spacing: number): Checkbox;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setStyle(style: UIStyle): Checkbox;
        /**
         * Registers the toggle callback and returns this checkbox for chaining.
         *
         * @param callback - The callback to invoke.
         * @returns This `Checkbox` instance, allowing method chaining.
         */
        setOnToggle(callback: Checkbox.ToggleCallback): Checkbox;
        /**
         * Sets checked.
         *
         * @param checked - The new checked state.
         */
        setChecked(checked: boolean): void;
        /**
         * Sets enabled.
         *
         * @param enabled - Whether to enable the feature.
         */
        setEnabled(enabled: boolean): void;
        /**
         * Toggle.
         */
        toggle(): void;
    }

    /**
     * Alignment of children within a layout.
     */
    export enum ElementAlignment {
        /**
         * Selects top for element alignment.
         */
        Top,
        /**
         * Selects center for element alignment.
         */
        Center,
        /**
         * Selects bottom for element alignment.
         */
        Bottom,
    }

    /**
     * Anchor used to place a layout relative to its position.
     */
    export enum LayoutAnchor {
        /**
         * Selects top left for layout anchor.
         */
        TopLeft,
        /**
         * Selects top center for layout anchor.
         */
        TopCenter,
        /**
         * Selects top right for layout anchor.
         */
        TopRight,
        /**
         * Selects center left for layout anchor.
         */
        CenterLeft,
        /**
         * Selects center for layout anchor.
         */
        Center,
        /**
         * Selects center right for layout anchor.
         */
        CenterRight,
        /**
         * Selects bottom left for layout anchor.
         */
        BottomLeft,
        /**
         * Selects bottom center for layout anchor.
         */
        BottomCenter,
        /**
         * Selects bottom right for layout anchor.
         */
        BottomRight,
    }

    /**
     * Vertical layout of UI children with spacing, padding, alignment, and an anchor.
     */
    export class Column extends UIObject {
        /**
         * Creates a new column.
         *
         * @param position - The position or translation offset.
         */
        constructor(position: Position2d);
        /**
         * Creates a new column.
         *
         * @param children - The child elements.
         * @param spacing - The space between adjacent children.
         * @param padding - The inner spacing around the content.
         * @param position - The position or translation offset.
         */
        constructor(
            children: UIObject[],
            spacing: number,
            padding: Size2d,
            position: Position2d,
        );

        /**
         * The spacing.
         */
        spacing: number;
        /**
         * The maximum size.
         */
        maxSize: Size2d;
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The child elements managed by this layout.
         */
        children: UIObject[];
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The alignment.
         */
        alignment: ElementAlignment;
        /**
         * The anchor.
         */
        anchor: LayoutAnchor;

        /**
         * Appends a child element to the layout.
         *
         * @param child - The UI element to append to the layout.
         */
        addChild(child: UIObject): void;
        /**
         * Replaces the layout's child elements.
         *
         * @param children - The child elements.
         */
        setChildren(children: UIObject[]): void;

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;

        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * The style applied to this layout.
         */
        style: UIStyle;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns This `Column` instance, allowing method chaining.
         */
        setStyle(style: UIStyle): Column;
    }

    /**
     * Horizontal layout of UI children with spacing, padding, alignment, and an anchor.
     */
    export class Row extends UIObject {
        /**
         * Creates a new row.
         *
         * @param position - The position or translation offset.
         */
        constructor(position: Position2d);
        /**
         * Creates a new row.
         *
         * @param children - The child elements.
         * @param spacing - The space between adjacent children.
         * @param padding - The inner spacing around the content.
         * @param position - The position or translation offset.
         */
        constructor(
            children: UIObject[],
            spacing: number,
            padding: Size2d,
            position: Position2d,
        );

        /**
         * The spacing.
         */
        spacing: number;
        /**
         * The maximum size.
         */
        maxSize: Size2d;
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The child elements managed by this layout.
         */
        children: UIObject[];
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The alignment.
         */
        alignment: ElementAlignment;
        /**
         * The anchor.
         */
        anchor: LayoutAnchor;

        /**
         * Appends a child element to the layout.
         *
         * @param child - The UI element to append to the layout.
         */
        addChild(child: UIObject): void;
        /**
         * Replaces the layout's child elements.
         *
         * @param children - The child elements.
         */
        setChildren(children: UIObject[]): void;

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;

        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * The style applied to this layout.
         */
        style: UIStyle;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns The resulting `Column` value.
         */
        setStyle(style: UIStyle): Column;
    }

    /**
     * Overlapping layout of UI children with horizontal and vertical alignment.
     */
    export class Stack extends UIObject {
        /**
         * Creates a new stack.
         *
         * @param position - The position or translation offset.
         */
        constructor(position: Position2d);
        /**
         * Creates a new stack.
         *
         * @param children - The child elements.
         * @param padding - The inner spacing around the content.
         * @param position - The position or translation offset.
         */
        constructor(
            children: UIObject[],
            padding: Size2d,
            position: Position2d,
        );

        /**
         * The maximum size.
         */
        maxSize: Size2d;
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The child elements managed by this layout.
         */
        children: UIObject[];
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The horizontal alignment.
         */
        horizontalAlignment: ElementAlignment;
        /**
         * The vertical alignment.
         */
        verticalAlignment: ElementAlignment;
        /**
         * The anchor.
         */
        anchor: LayoutAnchor;

        /**
         * Appends a child element to the layout.
         *
         * @param child - The UI element to append to the layout.
         */
        addChild(child: UIObject): void;
        /**
         * Replaces the layout's child elements.
         *
         * @param children - The child elements.
         */
        setChildren(children: UIObject[]): void;

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;

        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * The style applied to this layout.
         */
        style: UIStyle;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns The resulting `Column` value.
         */
        setStyle(style: UIStyle): Column;
    }

    /**
     * Interaction state selecting a style variant.
     */
    export enum UIStyleState {
        /**
         * Selects normal for ui style state.
         */
        Normal,
        /**
         * Selects hovered for ui style state.
         */
        Hovered,
        /**
         * Selects pressed for ui style state.
         */
        Pressed,
        /**
         * Selects disabled for ui style state.
         */
        Disabled,
        /**
         * Selects focused for ui style state.
         */
        Focused,
        /**
         * Selects checked for ui style state.
         */
        Checked,
    }

    /**
     * Current interaction flags used when resolving a UI style.
     */
    export type UIStyleStateSnapshot = {
        /**
         * Whether the pointer is hovering over the element.
         */
        hovered: boolean;
        /**
         * Whether the element is currently pressed.
         */
        pressed: boolean;
        /**
         * Whether the element is disabled.
         */
        disabled: boolean;
        /**
         * Whether the text field has keyboard focus.
         */
        focused: boolean;
        /**
         * Whether the checkbox is checked.
         */
        checked: boolean;
    };

    /**
     * Optional style overrides for one state; setter methods support chaining.
     */
    export class UIStyleVariant {
        /**
         * The optional padding value.
         */
        paddingValue?: number;
        /**
         * The optional corner radius value.
         */
        cornerRadiusValue?: number;
        /**
         * The optional border width value.
         */
        borderWidthValue?: number;
        /**
         * The optional background color value.
         */
        backgroundColorValue?: Color;
        /**
         * The optional border color value.
         */
        borderColorValue?: Color;
        /**
         * The optional foreground color value.
         */
        foregroundColorValue?: Color;
        /**
         * The optional tint color value.
         */
        tintColorValue?: Color;
        /**
         * The optional font value.
         */
        fontValue?: Font;
        /**
         * The optional font size value.
         */
        fontSizeValue?: number;

        /**
         * Padding.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        padding(value: Size2d): UIStyleVariant;
        /**
         * Corner radius.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        cornerRadius(value: number): UIStyleVariant;
        /**
         * Border width.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        borderWidth(value: number): UIStyleVariant;
        /**
         * Background color.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        backgroundColor(value: Color): UIStyleVariant;
        /**
         * Border color.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        borderColor(value: Color): UIStyleVariant;
        /**
         * Foreground color.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        foregroundColor(value: Color): UIStyleVariant;
        /**
         * Tint color.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        tintColor(value: Color): UIStyleVariant;
        /**
         * Font.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        font(value: Font): UIStyleVariant;
        /**
         * Font size.
         *
         * @param value - The value to apply.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        fontSize(value: number): UIStyleVariant;
    }

    /**
     * Concrete visual properties produced by style resolution.
     */
    export type UIResolvedStyle = {
        /**
         * The padding.
         */
        padding: Size2d;
        /**
         * The corner radius.
         */
        cornerRadius: number;
        /**
         * The border width.
         */
        borderWidth: number;
        /**
         * The background color.
         */
        backgroundColor: Color;
        /**
         * The border color.
         */
        borderColor: Color;
        /**
         * The foreground color.
         */
        foregroundColor: Color;
        /**
         * The tint color.
         */
        tintColor: Color;
        /**
         * The font.
         */
        font: Font;
        /**
         * The font size.
         */
        fontSize: number;
    };

    /**
     * Collection of visual overrides for normal and interactive states.
     */
    export class UIStyle {
        /**
         * Normal.
         *
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        normal(): UIStyleVariant;
        /**
         * Hovered.
         *
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        hovered(): UIStyleVariant;
        /**
         * Pressed.
         *
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        pressed(): UIStyleVariant;
        /**
         * Disables d.
         *
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        disabled(): UIStyleVariant;
        /**
         * Focused.
         *
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        focused(): UIStyleVariant;
        /**
         * Checked.
         *
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        checked(): UIStyleVariant;
        /**
         * Returns the style variant associated with the selected interaction state.
         *
         * @param state - The interaction state whose style variant is requested.
         * @returns This `UIStyleVariant` instance, allowing method chaining.
         */
        variant(state: UIStyleState): UIStyleVariant;
    }

    /**
     * Shared default styles for UI element types, with access to the current theme.
     */
    export class Theme {
        /**
         * The text.
         */
        text: UIStyle;
        /**
         * The image.
         */
        image: UIStyle;
        /**
         * The text field.
         */
        textField: UIStyle;
        /**
         * The button.
         */
        button: UIStyle;
        /**
         * The checkbox.
         */
        checkbox: UIStyle;
        /**
         * The row.
         */
        row: UIStyle;
        /**
         * The column.
         */
        column: UIStyle;
        /**
         * The stack.
         */
        stack: UIStyle;

        /**
         * Returns the current shared UI theme.
         *
         * @returns This `Theme` instance, allowing method chaining.
         */
        static current(): Theme;
        /**
         * Replaces the current shared UI theme.
         *
         * @param theme - The theme to make globally active.
         */
        static set(theme: Theme): void;
        /**
         * Restores the default UI theme.
         */
        static reset(): void;
    }

    /**
     * Glyph dimensions, bearing, advance, and texture atlas UV bounds.
     */
    export type Character = {
        /**
         * The dimensions.
         */
        size: Size2d;
        /**
         * The bearing.
         */
        bearing: Position2d;
        /**
         * The advance.
         */
        advance: number;
        /**
         * The UV minimum.
         */
        uvMin: Position2d;
        /**
         * The UV maximum.
         */
        uvMax: Position2d;
    };

    /**
     * Map from character strings to glyph metrics and atlas coordinates.
     */
    export type FontAtlas = Map<string, Character>;

    /**
     * Resource-backed font with atlas texture and size controls.
     */
    export class Font {
        /**
         * The human-readable name.
         */
        name: string;
        /**
         * The atlas.
         */
        atlas: Texture;
        /**
         * The dimensions.
         */
        size: number;
        /**
         * The resource backing this object.
         */
        resource: Resource;
        /**
         * The texture used for rendering.
         */
        texture: Texture;

        /**
         * Creates a value from resource.
         *
         * @param resource - The resource to use.
         * @returns The newly created value.
         */
        static fromResource(resource: Resource): Font;
        /**
         * Returns font.
         *
         * @param name - The name used to identify the value.
         * @returns The font.
         */
        static getFont(name: string): Font;

        /**
         * Change size.
         *
         * @param size - The size or dimensions.
         * @returns This `Font` instance, allowing method chaining.
         */
        changeSize(size: number): Font;
    }

    /**
     * UI text rendered with a font, color, screen position, and optional style.
     */
    export class Text extends UIObject {
        /**
         * The content.
         */
        content: string;
        /**
         * The font.
         */
        font: Font;
        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The font size.
         */
        fontSize: number;
        /**
         * The RGBA color.
         */
        color: Color;

        /**
         * Creates a new text.
         */
        constructor();
        /**
         * Creates a new text.
         *
         * @param text - The text content.
         * @param font - The font to use.
         * @param color - The color to apply.
         * @param position - The position or translation offset.
         */
        constructor(
            text: string,
            font: Font,
            color: Color,
            position: Position2d,
        );

        /**
         * Returns the current dimensions.
         *
         * @returns The size.
         */
        override getSize(): Size2d;
        /**
         * Returns the UI element's screen-space position.
         *
         * @returns The screen position.
         */
        override getScreenPosition(): Position2d;
        /**
         * Sets the UI element's screen-space position.
         *
         * @param position - The position or translation offset.
         */
        override setScreenPosition(position: Position2d): void;

        /**
         * Returns the element's style.
         *
         * @returns The element's current style.
         */
        style(): UIStyle;
        /**
         * Applies a style to the element.
         *
         * @param style - The style to apply.
         * @returns This `Text` instance, allowing method chaining.
         */
        setStyle(style: UIStyle): Text;
        /**
         * Sets font size.
         *
         * @param size - The size or dimensions.
         * @returns This `Text` instance, allowing method chaining.
         */
        setFontSize(size: number): Text;
    }
}

/**
 * Atmosphere, weather, clouds, and water rendering.
 */
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

    /**
     * Three-dimensional cellular noise with helpers producing texture handles.
     */
    export class WorleyNoise3D {
        /**
         * Creates a new worley noise 3 d.
         *
         * @param frequency - The noise or oscillation frequency.
         * @param numDivisions - The number of subdivisions used to generate the noise.
         */
        constructor(frequency: number, numDivisions: number);

        /**
         * Returns value.
         *
         * @param x - The X coordinate or component.
         * @param y - The Y coordinate or component.
         * @param z - The Z coordinate or component.
         * @returns The value.
         */
        getValue(x: number, y: number, z: number): number;

        /**
         * Returns 3D texture.
         *
         * @param size - The size or dimensions.
         * @returns The 3D texture.
         */
        get3dTexture(size: number): number;
        /**
         * Returns detail texture.
         *
         * @param size - The size or dimensions.
         * @returns The detail texture.
         */
        getDetailTexture(size: number): number;
        /**
         * Returns 3D texture at all channels.
         *
         * @param size - The size or dimensions.
         * @returns The 3D texture at all channels.
         */
        get3dTextureAtAllChannels(size: number): number;
    }

    /**
     * Volumetric cloud noise and rendering controls, including density, lighting steps, and wind.
     */
    export class Clouds {
        /**
         * Creates a new clouds.
         *
         * @param frequency - The noise or oscillation frequency.
         * @param numDivisions - The number of subdivisions used to generate the noise.
         */
        constructor(frequency: number, numDivisions: number);

        /**
         * Returns cloud texture.
         *
         * @param size - The size or dimensions.
         * @returns The cloud texture.
         */
        getCloudTexture(size: number): number;

        /**
         * The position in world space.
         */
        position: Position3d;
        /**
         * The dimensions.
         */
        size: Size3d;
        /**
         * The scale along each axis.
         */
        scale: number;
        /**
         * The offset.
         */
        offset: Position3d;
        /**
         * The density.
         */
        density: number;
        /**
         * The density multiplier.
         */
        densityMultiplier: number;
        /**
         * The absorption.
         */
        absorption: number;
        /**
         * The scattering.
         */
        scattering: number;
        /**
         * The phase.
         */
        phase: number;
        /**
         * The cluster strength.
         */
        clusterStrength: number;
        /**
         * The primary step count.
         */
        primaryStepCount: number;
        /**
         * The light step count.
         */
        lightStepCount: number;
        /**
         * The light step multiplier.
         */
        lightStepMultiplier: number;
        /**
         * The minimum step length.
         */
        minStepLength: number;
        /**
         * The wind direction and strength.
         */
        wind: Force3d;
    }

    /**
     * Weather preset represented by a WeatherState.
     */
    export enum WeatherCondition {
        /**
         * Selects clear weather.
         */
        Clear,
        /**
         * Selects rain weather.
         */
        Rain,
        /**
         * Selects snow weather.
         */
        Snow,
        /**
         * Selects storm weather.
         */
        Storm,
    }

    /**
     * Weather condition, intensity, and wind returned by a weather delegate.
     */
    export type WeatherState = {
        /**
         * The current weather condition.
         */
        condition: WeatherCondition;
        /**
         * The effect intensity.
         */
        intensity: number;
        /**
         * The wind direction and strength.
         */
        wind: Force3d;
    };

    /**
     * Computes weather from the current view and frame information.
     *
     * @param information - The current camera position, target, time, and frame delta.
     * @returns The weather state for the current view and time.
     */
    export type WeatherDelegate = (
        information: ViewInformation,
    ) => WeatherState;

    /**
     * Day/night cycle, sky colors, celestial lighting, weather, and clouds.
     */
    export class Atmosphere {
        /**
         * The time of day.
         */
        timeOfDay: number;
        /**
         * The seconds per hour.
         */
        secondsPerHour: number;
        /**
         * The wind direction and strength.
         */
        wind: Magnitude3d;
        /**
         * The callback used to compute the current weather state.
         */
        weatherDelegate: WeatherDelegate;

        /**
         * Enable.
         */
        enable(): void;
        /**
         * Disable.
         */
        disable(): void;
        /**
         * Reports whether enabled.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isEnabled(): boolean;
        /**
         * Enables weather.
         */
        enableWeather(): void;
        /**
         * Disables weather.
         */
        disableWeather(): void;

        /**
         * Returns normalized time.
         *
         * @returns The normalized time.
         */
        getNormalizedTime(): number;
        /**
         * Returns sun angle.
         *
         * @returns The sun angle.
         */
        getSunAngle(): Magnitude3d;
        /**
         * Returns moon angle.
         *
         * @returns The moon angle.
         */
        getMoonAngle(): Magnitude3d;
        /**
         * Returns light intensity.
         *
         * @returns The light intensity.
         */
        getLightIntensity(): number;
        /**
         * Returns light color.
         *
         * @returns The light color.
         */
        getLightColor(): Color;

        /**
         * The cloud system, when clouds have been added.
         */
        clouds?: Clouds;

        /**
         * Returns skybox colors.
         *
         * @returns The skybox colors.
         */
        getSkyboxColors(): Color[];
        /**
         * Creates a sky cubemap with the requested face size.
         *
         * @param size - The size or dimensions.
         * @returns The newly created value.
         */
        createSkyCubemap(size: number): Cubemap;
        /**
         * Updates an existing cubemap with the current sky colors.
         *
         * @param cubemap - The cubemap containing the six skybox faces.
         */
        updateSkyCubemap(cubemap: Cubemap): void;

        /**
         * Cast shadows from sunlight.
         *
         * @param resolution - The texture or shadow-map resolution in pixels.
         */
        castShadowsFromSunlight(resolution: number): void;
        /**
         * Configures the use of global light.
         */
        useGlobalLight(): void;

        /**
         * The sun color.
         */
        sunColor: Color;
        /**
         * The moon color.
         */
        moonColor: Color;

        /**
         * The sun size.
         */
        sunSize: number;
        /**
         * The moon size.
         */
        moonSize: number;
        /**
         * The sun tint strength.
         */
        sunTintStrength: number;
        /**
         * The moon tint strength.
         */
        moonTintStrength: number;
        /**
         * The star intensity.
         */
        starIntensity: number;

        /**
         * Reports whether daytime.
         *
         * @returns `true` when the condition is satisfied; otherwise `false`.
         */
        isDaytime(): boolean;
        /**
         * Sets the atmosphere clock using hours, minutes, and seconds.
         *
         * @param hours - The hour component.
         * @param minutes - The minute component.
         * @param seconds - The second component.
         */
        setTime(hours: number, minutes: number, seconds: number): void;

        /**
         * Creates clouds using the supplied noise frequency and division count.
         *
         * @param frequency - The noise or oscillation frequency.
         * @param numDivisions - The number of subdivisions used to generate the noise.
         */
        addClouds(frequency: number, numDivisions: number): void;

        /**
         * Whether the atmosphere advances its time-of-day cycle automatically.
         */
        cycle: boolean;
        /**
         * Resets runtime state to its initial state.
         */
        resetRuntimeState(): void;
    }

    /**
     * Renderable water surface with configurable extent, color, wave motion, and textures.
     */
    export class Fluid extends GameObject {
        /**
         * The wave velocity.
         */
        waveVelocity: number;

        /**
         * Creates a new fluid.
         */
        constructor();
        /**
         * Create.
         *
         * @param extent - The water surface width and height.
         * @param color - The color to apply.
         */
        create(extent: Size2d, color: Color): void;

        /**
         * Translates by the supplied offset.
         *
         * @param position - The position or translation offset.
         */
        override move(position: Position3d): void;
        /**
         * Sets the position to an absolute value.
         *
         * @param position - The position or translation offset.
         */
        override setPosition(position: Position3d): void;
        /**
         * Replaces the rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        override setRotation(rotation: Rotation3d): void;
        /**
         * Applies an incremental rotation.
         *
         * @param rotation - The Euler rotation, in degrees.
         */
        override rotate(rotation: Rotation3d): void;
        /**
         * Replaces the scale factors.
         *
         * @param scale - The scale or scale multiplier.
         */
        override setScale(scale: Scale3d): void;

        /**
         * Sets extent.
         *
         * @param extent - The water surface width and height.
         */
        setExtent(extent: Size2d): void;
        /**
         * Sets wave velocity.
         *
         * @param velocity - The velocity vector.
         */
        setWaveVelocity(velocity: number): void;
        /**
         * Sets water color.
         *
         * @param color - The color to apply.
         */
        setWaterColor(color: Color): void;
        /**
         * Returns the current position.
         *
         * @returns The position.
         */
        getPosition(): Position3d;
        /**
         * Returns scale.
         *
         * @returns The scale.
         */
        getScale(): Scale3d;

        /**
         * The normal texture.
         */
        normalTexture: Texture;
        /**
         * The movement texture.
         */
        movementTexture: Texture;
    }
}
