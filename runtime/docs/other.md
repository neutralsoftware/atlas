# Other things that may appear in the format

## Lights

Lights are defined in the `lights` section of the scene format file, which is an array of light objects. Each light object has a `type` property that specifies the type of the light (e.g., point light, directional light, spotlight, etc.), and other properties that define the values for that light.

### Ambient Light (`type = "ambient_light"`)
An ambient light is a light that illuminates the entire scene uniformly, without any direction or position. It has the following properties:

* `intensity`: The intensity of the ambient light, defined as a single number representing the strength of the light (values between 0 and 1). A higher value means that the light will be brighter.
* `color`: The color of the ambient light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).

### Directional Light (`type = "directionalLight"`)

A directional light is a light that illuminates the scene from a specific direction, without any position. It has the following properties:

* `direction`: The direction of the directional light, defined as an array of three numbers representing the x, y and z components of the direction vector.
* `color`: The color of the directional light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `shineColor`: The color of the specular highlights produced by the directional light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `intensity`: The intensity of the directional light, defined as a single number representing the strength of the light (values between 0 and 1). A higher value means that the light will be brighter.
* `castsShadows`: A boolean value that indicates whether the directional light casts shadows or not. If `true`, the directional light will cast shadows from objects in the scene. If `false`, the directional light will not cast shadows.
* `shadowResolution`: The resolution of the shadows cast by the directional light, defined as a single number representing the size of the shadow map in pixels (e.g., 1024, 2048, etc.). A higher value means that the shadows will be sharper and more detailed, but it may also impact performance.

### Point Light (`type = "pointLight"`)

A point light is a light that illuminates the scene from a specific position, emitting light in all directions. It has the following properties:

* `position`: The position of the point light in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `color`: The color of the point light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `shineColor`: The color of the specular highlights produced by the point light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `intensity`: The intensity of the point light, defined as a single number representing the strength of the light (values between 0 and 1). A higher value means that the light will be brighter.
* `distance`: The distance at which the point light's intensity will be reduced to zero, defined as a single number representing the distance in units. A higher value means that the light will affect objects that are farther away.
* `castsShadows`: A boolean value that indicates whether the point light casts shadows or not. If `true`, the point light will cast shadows from objects in the scene. If `false`, the point light will not cast shadows.
* `shadowResolution`: The resolution of the shadows cast by the point light, defined as a single number representing the size of the shadow map in pixels (e.g., 1024, 2048, etc.). A higher value means that the shadows will be sharper and more detailed, but it may also impact performance.
* `addDebugObject`: A boolean value that indicates whether to add a debug object for the light in the scene or not. If `true`, a debug object will be added to the scene to visualize the position and orientation of the light. If `false`, no debug object will be added for the light.

### Spotlight (`type = "spotlight"`)

A spotlight is a light that illuminates the scene from a specific position and direction, emitting light in a cone shape. It has the following properties:

* `position`: The position of the spotlight in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `direction`: The direction of the spotlight, defined as an array of three numbers representing the x, y and z components of the direction vector.
* `color`: The color of the spotlight, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `shineColor`: The color of the specular highlights produced by the spotlight, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `intensity`: The intensity of the spotlight, defined as a single number representing the strength of the light (values between 0 and 1). A higher value means that the light will be brighter.
* `range`: The distance at which the spotlight's intensity will be reduced to zero, defined as a single number representing the distance in units. A higher value means that the light will affect objects that are farther away.
* `cutoff`: The cutoff angle of the spotlight, defined as a single number representing the angle in degrees that defines the cone of light emitted by the spotlight. A smaller value means that the light will be more focused, while a larger value means that the light will be more spread out.
* `outerCutoff`: The outer cutoff angle of the spotlight, defined as a single number representing the angle in degrees that defines the outer cone of light emitted by the spotlight. This is used to create a smooth transition between the fully lit area and the area that is not affected by the spotlight. A smaller value means that the transition will be sharper, while a larger value means that the transition will be smoother.
* `castsShadows`: A boolean value that indicates whether the spotlight casts shadows or not. If `true`, the spotlight will cast shadows from objects in the scene. If `false`, the spotlight will not cast shadows.
* `shadowResolution`: The resolution of the shadows cast by the spotlight, defined as a single number representing the size of the shadow map in pixels (e.g., 1024, 2048, etc.). A higher value means that the shadows will be sharper and more detailed, but it may also impact performance.
* `addDebugObject`: A boolean value that indicates whether to add a debug object for the light in the scene or not. If `true`, a debug object will be added to the scene to visualize the position and orientation of the light. If `false`, no debug object will be added for the light.

### Area Light (`type = "arealight"`)

An area light is a light that illuminates the scene from a specific position and direction, emitting light from a rectangular or circular area. It has the following properties:

* `position`: The position of the area light in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `right`: The right vector of the area light, defined as an array of three numbers representing the x, y and z components of the right vector. This is used to define the orientation of the area light.
* `up`: The up vector of the area light, defined as an array of three numbers representing the x, y and z components of the up vector. This is used to define the orientation of the area light.
* `size`: The size of the area light, defined as an array of two numbers representing the width and height of the area light in units.
* `color`: The color of the area light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `shineColor`: The color of the specular highlights produced by the area light, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `intensity`: The intensity of the area light, defined as a single number representing the strength of the light (values between 0 and 1). A higher value means that the light will be brighter.
* `range`: The distance at which the area light's intensity will be reduced to zero, defined as a single number representing the distance in units. A higher value means that the light will affect objects that are farther away.
* `angle`: The angle of the area light, defined as a single number representing the angle in degrees that defines the spread of the light emitted by the area light. A smaller value means that the light will be more focused, while a larger value means that the light will be more spread out.
* `castsBothSides`: A boolean value that indicates whether the area light casts light on both sides or only on one side. If `true`, the area light will emit light on both sides of the area. If `false`, the area light will emit light only on one side of the area, defined by the right and up vectors.
* `castsShadows`: A boolean value that indicates whether the area light casts shadows or not. If `true`, the area light will cast shadows from objects in the scene. If `false`, the area light will not cast shadows.
* `shadowResolution`: The resolution of the shadows cast by the area light, defined as a single number representing the size of the shadow map in pixels (e.g., 1024, 2048, etc.). A higher value means that the shadows will be sharper and more detailed, but it may also impact performance.
* `addDebugObject`: A boolean value that indicates whether to add a debug object for the light in the scene or not. If `true`, a debug object will be added to the scene to visualize the position and orientation of the light. If `false`, no debug object will be added for the light.

## Camera

The camera is defined in the `camera` section of the scene format file, which is an object that defines the properties of the camera in the scene, such as its position, rotation, field of view, etc. The camera object has a `type` property that specifies the type of the camera (e.g., perspective, orthographic, etc.), and other properties that define the values for that camera.

* `position`: The position of the camera in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `target`: The target point that the camera is looking at, defined as an array of three numbers representing the x, y and z coordinates of the target point. This is used to define the orientation of the camera.
* `fov`: The field of view of the camera, defined as a single number representing the vertical field of view in degrees. A higher value means that the camera will have a wider field of view, while a lower value means that the camera will have a narrower field of view.
* `nearClip`: The near clipping plane of the camera, defined as a single number representing the distance from the camera to the near clipping plane in units. Objects that are closer to the camera than this distance will not be rendered.
* `farClip`: The far clipping plane of the camera, defined as a single number representing the distance from the camera to the far clipping plane in units. Objects that are farther from the camera
* `orthoSize`: The orthographic size of the camera, defined as a single number representing the size of the orthographic view in units. This is used only for orthographic cameras, and it defines the height of the orthographic view. A higher value means that the camera will show more of the scene, while a lower value means that the camera will show less of the scene.
* `movementSpeed`: The movement speed of the camera, defined as a single number representing the speed at which the camera moves in units per second. This is used for cameras that can be controlled by the user, such as a first-person camera or a free camera. A higher value means that the camera will move faster, while a lower value means that the camera will move slower.
* `mouseSensitivity`: The mouse sensitivity of the camera, defined as a single number representing the sensitivity of the camera to mouse movements. This is used for cameras that can be controlled by the user, such as a first-person camera or a free camera. A higher value means that the camera will be more sensitive to mouse movements, while a lower value means that the camera will be less sensitive to mouse movements.
* `controllerLookSensitivity`: The controller look sensitivity of the camera, defined as a single number representing the sensitivity of the camera to controller stick movements. This is used for cameras that can be controlled by the user, such as a first-person camera or a free camera. A higher value means that the camera will be more sensitive to controller stick movements, while a lower value means that the camera will be less sensitive to controller stick movements.
* `lookSmoothness`: The look smoothness of the camera, defined as a single number representing the smoothness of the camera's rotation when looking around. This is used for cameras that can be controlled by the user, such as a first-person camera or a free camera. A higher value means that the camera's rotation will be smoother and more gradual, while a lower value means that the camera's rotation will be more immediate and responsive.
* `orthographic`: A boolean value that indicates whether the camera is orthographic or perspective. If `true`, the camera will be orthographic, which means that it will render objects without perspective distortion. If `false`, the camera will be perspective, which means that it will render objects with perspective distortion, making objects that are farther away appear smaller.
* `focusDepth`: The focus depth of the camera, defined as a single number representing the distance from the camera to the point of focus in units. This is used for cameras that support depth of field effects, and it defines the distance at which objects will be in sharp focus. A higher value means that objects that are farther away will be in focus, while a lower value means that objects that are closer will be in focus.
* `focusRange`: The focus range of the camera, defined as a single number representing the range of distances from the focus point that will be in focus in units. This is used for cameras that support depth of field effects, and it defines the range of distances around the focus point that will be in sharp focus. A higher value means that a wider range of distances will be in focus, while a lower value means that a narrower range of distances will be in focus.
* `actions`: An array of input actions where:
  * The first item is for moving
  * The second item is for looking around
  * The third item is for moving up and down
* `automaticMoving`: A boolean value that indicates whether the camera should move automatically based on the input actions or not. If `true`, the camera will move automatically based on the input actions defined in the `actions` array. If `false`, the camera will not move automatically, and it will require manual control through scripts or other means. 

## Environment

The scene can define an `environment` object. This combines the scene `Environment` settings, the `Atmosphere` settings, and the switch for using the procedurally generated atmosphere skybox.

```json
"environment": {
  "fog": {
    "color": [180, 200, 255],
    "intensity": 0.015
  },
  "volumetricLighting": {
    "enabled": true,
    "density": 0.35,
    "weight": 0.02,
    "decay": 0.95,
    "exposure": 0.7
  },
  "lightBloom": {
    "radius": 0.01,
    "maxSamples": 6
  },
  "rimLight": {
    "intensity": 0.2,
    "color": [255, 244, 220]
  },
  "lookupTexture": "textures/lut.png",
  "automaticAmbient": true,
  "atmosphereSky": true,
  "atmosphere": {
    "enabled": true,
    "cycle": true,
    "timeOfDay": 17.5,
    "secondsPerHour": 180.0,
    "wind": [0.02, 0.0, 0.01],
    "sunColor": [255, 242, 214],
    "moonColor": [150, 150, 210],
    "sunSize": 1.0,
    "moonSize": 1.0,
    "sunTintStrength": 0.35,
    "moonTintStrength": 0.8,
    "starIntensity": 2.5,
    "globalLight": {
      "enabled": true,
      "castsShadows": true,
      "shadowResolution": 2048
    },
    "clouds": {
      "enabled": true,
      "frequency": 4,
      "divisions": 6,
      "position": [0, 100, 0],
      "size": [500, 80, 500],
      "scale": 1.5,
      "density": 0.45,
      "densityMultiplier": 1.5,
      "absorption": 1.1,
      "scattering": 0.85,
      "phase": 0.55,
      "clusterStrength": 0.5,
      "primaryStepCount": 12,
      "lightStepCount": 6,
      "lightStepMultiplier": 1.6,
      "minStepLength": 0.05,
      "wind": [0.03, 0.0, 0.02]
    },
    "weather": {
      "enabled": true,
      "condition": "rain",
      "intensity": 0.6,
      "wind": [0.1, -0.4, 0.05]
    }
  }
}
```

Top-level `environment` properties:

* `fog`: Configures scene fog.
  * `color`: Fog tint color.
  * `intensity`: Fog density.
* `volumetricLighting`: Configures deferred volumetric lighting.
  * `enabled`: Enables the effect.
  * `density`: Participating media density.
  * `weight`: Per-step contribution.
  * `decay`: Falloff per step.
  * `exposure`: Final intensity multiplier.
* `lightBloom`: Controls deferred bloom generation.
  * `radius`: Bloom blur radius.
  * `maxSamples`: Number of blur passes.
* `rimLight`: Controls rim lighting applied in supported shaders.
  * `intensity`: Rim light strength.
  * `color`: Rim light tint.
* `lookupTexture`: Optional LUT texture path or texture definition object.
* `automaticAmbient`: If `true`, ambient lighting is derived from the active skybox.
* `atmosphereSky`: If `true`, the scene uses the procedurally generated atmosphere sky instead of a user skybox.

The `environment.atmosphere` object maps to the engine `Atmosphere` configuration:

* `enabled`: Enables atmospheric rendering. This is optional if `atmosphereSky`, `globalLight`, `clouds`, or `weather` are used.
* `cycle`: Enables the day-night cycle.
* `timeOfDay`: Time in hours, from `0` to `24`.
* `secondsPerHour`: Simulation speed for the day-night cycle.
* `wind`: Global wind vector.
* `sunColor`: Daylight color.
* `moonColor`: Night light color.
* `sunSize`: Sun disk size.
* `moonSize`: Moon disk size.
* `sunTintStrength`: Strength of warm daylight tinting.
* `moonTintStrength`: Strength of moonlight tinting.
* `starIntensity`: Brightness of stars in the generated sky.
* `globalLight`: Enables the atmosphere-driven directional light. This can be a boolean or an object.
  * `enabled`: Enables the atmosphere global light.
  * `castsShadows`: Enables shadows for that generated light.
  * `shadowResolution`: Shadow map resolution used by the generated light.
* `clouds`: Enables and configures procedural volumetric clouds. This can be a boolean or an object.
  * `enabled`: Enables clouds.
  * `frequency`: Worley noise frequency.
  * `divisions`: Worley octave count.
  * `position`: Cloud volume center.
  * `size`: Cloud volume size.
  * `scale`: Sample scale.
  * `offset`: Noise offset.
  * `density`: Base density threshold.
  * `densityMultiplier`: Cluster density multiplier.
  * `absorption`: Light absorption strength.
  * `scattering`: Scattering strength.
  * `phase`: Phase function parameter.
  * `clusterStrength`: Fine detail strength.
  * `primaryStepCount`: Primary raymarch steps.
  * `lightStepCount`: Light sampling steps.
  * `lightStepMultiplier`: Distance multiplier between light steps.
  * `minStepLength`: Minimum raymarch step length.
  * `wind`: Cloud-specific wind vector.
* `weather`: Enables a static weather state. This can be a boolean or an object.
  * `enabled`: Enables weather particles.
  * `condition`: One of `clear`, `rain`, `snow`, or `storm`.
  * `intensity`: Weather intensity.
  * `wind`: Wind override used by that weather state.

If both a manual directional light and `environment.atmosphere.globalLight` are defined, the atmosphere global light becomes the active directional light for the scene.

## Input Actions

Input Actions are defined in a separate file that can be referenced by the camera or other objects in the scene. They are defined as an array of input action objects, where each input action object has a `name` property that specifies the name of the input action, and an `inputs` property that defines the inputs for that action. These files are structured as following:

* `name`: The name of the input action, which is a string that can be used to identify the input action.
* `triggerButtons`: An array of strings that specify the buttons that trigger the input action. These can be standard button names (e.g., "W", "A", "S", "D", "Space", etc.) or custom button names defined by the user.
* `triggerAxes`: An array of strings that specify the axes that trigger the input action. These axes are:
  * `"type": "mouse"`: for mouse movement
  * `"type": "controller"`: for controller stick movement but only for the controller id with field `id` and axis index for `index`
  * `"type": "custom"`: for keyboard input, where the `triggers` property defines the keys that trigger the input action (e.g., "W", "A", "S", "D", etc.)


## Terrain Generator Settings

These are defined in another type of file, and they are used to define the settings for the terrain generation algorithm. They can be referenced by the terrain objects in the scene format file, and they have the following properties:

* `name`: The name of the terrain generator, which is a string that can be used to identify the terrain generator.
* `algorithm`: The algorithm used for terrain generation, which can be `perlin_noise`, `simplex_noise`, `diamond_square`, etc. This defines the method used to generate the heightmap for the terrain.
* `settings`: An object that defines the settings for the terrain generation algorithm and can have different properties depending on the algorithm used. For example, for a noise-based algorithm, the settings can include properties such as `scale`, `octaves`, `persistence`, `lacunarity`, etc., which define the characteristics of the generated terrain. These settings will be used by the terrain generator to create the heightmap for the terrain based on the specified algorithm and settings.

## Render Target Settings

Render target settings are defined in the `targets` section of the scene format file, which is an array of render target objects. Each render target object has a `type` property that specifies the type of the render target (e.g., multisampled, post-processing, etc.), and other properties that define the values for that render target.

* `type`: The type of the render target, which can be `multisampled`, `scene`, etc. This defines the purpose and behavior of the render target in the rendering pipeline.
* `render`: A boolean value that indicates whether to render the scene to this render target or not. If `true`, the scene will be rendered to this render target. If `false`, the scene will not be rendered to this render target.
* `display`: A boolean value that indicates whether to display the contents of this render target on the screen or not. If `true`, the contents of this render target will be displayed on the screen. If `false`, the contents of this render target will not be displayed on the screen, but it can still be used for other purposes, such as post-processing effects or as a texture for other objects in the scene.
* `effects`: An array of post-processing effects that are applied to the contents of this render target before it is displayed on the screen. Each effect is defined as an object with a `type` property that specifies the type of the effect, and other properties that define the values for that effect. For example, an effect can be a bloom effect, a color grading effect, a depth of field effect, etc., and each effect will have its own specific properties that define how it modifies the rendered image.
* `name`: The name of the render target, which is a string that can be used to identify the render target. This is useful for referencing the render target in other parts of the scene format file, such as in post-processing effects or as a texture for other objects in the scene.

## Render Target Effects

Render target effects are defined as objects in the `effects` array of a render target, and they are used to apply post-processing effects to the rendered image before it is displayed on the screen. Each effect has a `type` property that specifies the type of the effect (e.g., bloom, color grading, depth of field, etc.), and other properties that define the values for that effect. For example, a bloom effect can have properties such as `threshold`, `intensity`, `radius`, etc., which define how the bloom effect is applied to the rendered image. These effects can be used to enhance the visual quality of the rendered image and create various visual effects in the scene.

### Inversion Effect (`type = "inversion"`)
An inversion effect is a post-processing effect that inverts the colors of the rendered image. 

### Grayscale Effect (`type = "grayscale"`)
A grayscale effect is a post-processing effect that converts the rendered image to grayscale, removing all color information and leaving only the intensity values.

### Sharpen Effect (`type = "sharpen"`)
A sharpen effect is a post-processing effect that enhances the edges and details of the rendered image, making it appear sharper and more defined. It can have properties such as `strength`, which defines the intensity of the sharpening effect.

### Blur Effect (`type = "blur"`)
A blur effect is a post-processing effect that blurs the rendered image, creating a softer and more out-of-focus appearance. It can have properties such as `magnitude`, which defines the strength of the blur effect.

### Edge Detection Effect (`type = "edge_detection"`)
An edge detection effect is a post-processing effect that detects and highlights the edges in the rendered image, creating a stylized and high-contrast appearance.

### Color Correction Effect (`type = "color_correction"`)
A color correction effect is a post-processing effect that modifies the colors of the rendered image based on various parameters such as brightness, contrast, saturation, etc. As properties for this effect, it can have:

* `exposure`: A single number that defines the exposure level of the color correction effect. A higher value means that the image will be brighter, while a lower value means that the image will be darker.
* `contrast`: A single number that defines the contrast level of the color correction effect. A higher value means that the contrast will be increased, making the dark areas darker and the bright areas brighter, while a lower value means that the contrast will be decreased, making the image appear more flat and less dynamic.
* `saturation`: A single number that defines the saturation level of the color correction effect. A higher value means that the colors will be more vibrant and intense, while a lower value means that the colors will be more muted and desaturated.
* `gamma`: A single number that defines the gamma level of the color correction effect. A higher value means that the image will be brighter and more washed out, while a lower value means that the image will be darker and more contrasty.
* `temperature`: A single number that defines the color temperature of the color correction effect in degrees Kelvin. A higher value means that the image will have a warmer color tone (more yellow/orange), while a lower value means that the image will have a cooler color tone (more blue).
* `tint`: A single number that defines the tint level of the color correction effect. A higher value means that the image will have a stronger tint in the specified color, while a lower value means that the image will have a weaker tint.

### Motion Blur Effect (`type = "motion_blur"`)
A motion blur effect is a post-processing effect that simulates the blurring of moving objects in the rendered image, creating a sense of motion and speed. It has properties such as:

* `size`: A single number that defines the size of the motion blur effect in pixels. A higher value means that the motion blur will be more pronounced and cover a larger area, while a lower value means that the motion blur will be less pronounced and cover a smaller area.
* `separation`: A single number that defines the separation of the motion blur effect in pixels. This determines how far apart the blurred images of the moving object will be, creating a trail effect. A higher value means that the blurred images will be more separated, while a lower value means that the blurred images will be closer together.

### Chromatic Aberration Effect (`type = "chromatic_aberration"`)
A chromatic aberration effect is a post-processing effect that simulates the distortion of colors at the edges of the rendered image, creating a colorful fringe effect. It has properties such as:
* `red`: A single number that defines the amount of chromatic aberration applied to the red channel. A higher value means that the red channel will be more distorted, while a lower value means that the red channel will be less distorted.
* `green`: A single number that defines the amount of chromatic aberration applied to the green channel. A higher value means that the green channel will be more distorted, while a lower value means that the green channel will be less distorted.
* `blue`: A single number that defines the amount of chromatic aberration applied to the blue channel. A higher value means that the blue channel will be more distorted, while a lower value means that the blue channel will be less distorted.
* `direction`: A set of two numbers that define the direction of the chromatic aberration effect in degrees. The first number represents the horizontal direction, where a positive value means that the effect will be applied to the right side of the image, and a negative value means that the effect will be applied to the left side of the image. The second number represents the vertical direction, where a positive value means that the effect will be applied to the bottom of the image, and a negative value means that the effect will be applied to the top of the image.

### Posterization Effect (`type = "posterization"`)
A posterization effect is a post-processing effect that reduces the number of colors in the rendered image, creating a stylized and cartoon-like appearance. It has properties such as:
* `levels`: A single number that defines the number of color levels in the posterization effect. A higher value means that the image will have more colors and a smoother appearance, while a lower value means that the image will have fewer colors and a more stylized appearance.

### Pixelation Effect (`type = "pixelation"`)
A pixelation effect is a post-processing effect that simulates the appearance of low-resolution images by enlarging the pixels in the rendered image, creating a blocky and pixelated appearance. It has properties such as:
* `pixelSize`: A single number that defines the size of the pixels in the pixelation effect in pixels. A higher value means that the pixels will be larger and the image will be more pixelated, while a lower value means that the pixels will be smaller and the image will be less pixelated.

### Dilation Effect (`type = "dilation"`)
A dilation effect is a post-processing effect that expands the bright areas of the rendered image, creating a glowing and blooming appearance. It has properties such as:
* `size`: A single number that defines the size of the dilation effect in pixels. A higher value means that the bright areas will be expanded more and the image will have a stronger glowing effect, while a lower value means that the bright areas will be expanded less and the image will have a weaker glowing effect.
* `separation`: A single number that defines the separation of the dilation effect in pixels. This determines how far apart the expanded bright areas will be, creating a blooming effect. A higher value means that the expanded bright areas will be more separated, while a lower value means that the expanded bright areas will be closer together.

### Film Grain Effect (`type = "film_grain"`)
A film grain effect is a post-processing effect that simulates the appearance of film grain in the rendered image, creating a vintage and textured appearance. It has properties such as:
* `amount`: A single number that defines the amount of film grain in the effect. A higher value means that the image will have more grain and a stronger vintage appearance, while a lower value means that the image will have less grain and a weaker vintage appearance.

## Biomes

Biomes are defined in separate files that can be referenced by the terrain objects in the scene format file. They are used to define different types of terrain and vegetation based on factors such as temperature, moisture, altitude, etc. Each biome file has the following properties:

* `name`: The name of the biome set which is a string that can be used to identify the biome set.
* `biomes`: An array of biome objects, where each biome object has the following properties:
  * `name`: The name of the biome, which is a string that can be used to identify the biome.
  * `texture`: The texture of the biome, which is a reference to a texture file (e.g., `.png`, `.jpg`, etc.) that defines the appearance of the biome.
  * `color`: The color of the biome, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255). This can be used to define the overall color tint of the biome.
  * `useTexture`: A boolean value that indicates whether to use the texture for the biome or not. If `true`, the biome will use the specified texture for its appearance. If `false`, the biome will use a solid color defined by the `color` property for its appearance.
  * `minHeight`: The minimum height at which the biome can appear, defined as a single number representing the height in units. This is used to define the altitude range for the biome.
  * `maxHeight`: The maximum height at which the biome can appear, defined as a single number representing the height in units. This is used to define the altitude range for the biome.
  * `minMoisture`: The minimum moisture level at which the biome can appear, defined as a single number representing the moisture level (values between 0 and 1). This is used to define the moisture range for the biome.
  * `maxMoisture`: The maximum moisture level at which the biome can appear, defined as a single number representing the moisture level (values between 0 and 1). This is used to define the moisture range for the biome.
  * `minTemperature`: The minimum temperature at which the biome can appear, defined as a single number representing the temperature in degrees Celsius. This is used to define the temperature range for the biome.
  * `maxTemperature`: The maximum temperature at which the biome can appear, defined as a single number representing the temperature in degrees Celsius. This is used to define the temperature range for the biome.

## Shaders

Custom shaders can be defined in separate files that can be referenced by the material files in the scene format file. They are used to define custom rendering effects and can have the following properties:

* `name`: The name of the shader, which is a string that can be used to identify the shader.
* `vertexShader`: The path to the vertex shader file (e.g., `.vert`, `.glsl`, etc.) that defines the vertex shader code for the shader.
* `fragmentShader`: The path to the fragment shader file (e.g., `.frag`,`.glsl`, etc.) that defines the fragment shader code for the shader.
* `uniforms`: An object that defines the uniforms for the shader, where each key is the name of the uniform and the value is an object that defines the type and value of the uniform. For example:
```json
"uniforms": {
    "time": {
        "type": "float",
        "value": 0.0
    },
    "color": {
        "type": "vec3",
        "value": [1.0, 0.0, 0.0]
    },
    "texture": {
        "type": "sampler2D",
        "value": "path/to/texture.png"
    }
}
```
* `platform`: A string that specifies the platform for which the shader is intended (e.g., "desktop", "mobile", etc.). This can be used to define different shaders for different platforms, allowing for optimization and compatibility with various devices.
