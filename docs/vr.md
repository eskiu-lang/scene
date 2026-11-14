# VR: eyes, Cardboard, reticle and controllers

`<scene/vr>` draws a scene for a headset and drives its input the way ViroReact's
`ViroVRSceneNavigator` and `ViroController` do. The scene's camera node is the head: move
and turn it with the headset's pose each frame.

## Eyes

`Scene_render_eye(target, eye)` draws one eye. A `SceneEye` is the eye's pose in the head
(for a plain stereo pair, half the interpupillary distance to each side) and its
projection, which is what a headset runtime hands over for each eye:

```c
SceneEye left = scene_eye(-SCENE_IPD / 2.0, fov, aspect, 0.01, 100.0);
SceneEye right = scene_eye(SCENE_IPD / 2.0, fov, aspect, 0.01, 100.0);
scene.render_eye(&left_image, left)?;
scene.render_eye(&right_image, right)?;
```

`scene_fov_projection(left, right, down, up, near, far)` makes the projection from the four
half angles of an eye's field of view, left and down negative, as OpenXR's `XrFovf` gives
them; asymmetric fields of view are common on headsets. The camera node is not moved: the
eye's offset goes into the projection, so nodes under the camera (a head-locked reticle, a
HUD) are seen with each eye's parallax, as ViroReact draws its reticle. `SCENE_IPD` is
ViroReact's 64 mm.

## Cardboard

`SceneCardboard` draws both eyes of a phone in a Cardboard viewer, side by side, through
the lenses' distortion. Its fields are the interpupillary distance, the vertical field of
view each eye sees on the screen, the lens's radial distortion (`k1`, `k2`: a point at
tangent radius `r` is seen at `r (1 + k1 r^2 + k2 r^4)`, Cardboard's polynomial) and the
clipping distances; `scene_cardboard()` gives Google Cardboard (2015)'s values. Each eye is
drawn wide enough that the distortion of the screen's corners stays inside its image.

```c
SceneCardboard vr = scene_cardboard();
defer vr.release();
// each frame, after moving the camera node to the head's pose
vr.render(&scene, &frame_texture)?;
```

## Reticle

`SceneReticle` is ViroReact's reticle: a cyan ring three pixels wide at the distance of what
the pointer hits. While a node that takes fuse events counts down, the ring gives way to a
faint ring three times as wide (grown in 0.25 s) with a white arc that fills as the fuse
time runs out; when it completes, a burst grows and fades in 0.4 s. A click pulses the ring
three times thicker and back in 0.4 s. A head-locked reticle (Cardboard's) lives under the
camera, straight ahead at the hit's depth, and keeps its depth when the pointer moves to the
background, so it does not jump; otherwise it sits on the hit, facing the camera. The
reticle's lines skip the depth test and draw after everything else.

## Controllers

A `SceneController` turns a controller into `SceneInput` events (see
[Hit testing and events](input.md)) and moves its reticle:

- `SCENE_CONTROLLER_CARDBOARD`: the gaze, the camera's forward ray. Its button is a tap on
  the screen. Its reticle is head-locked.
- `SCENE_CONTROLLER_POINTER`: a tracked controller (Daydream, Quest): it points along the
  -Z of the pose given with `set_pose`. Its reticle sits on the hit.

```c
SceneInput input = scene_input();
SceneController controller = scene_controller(&scene, SCENE_CONTROLLER_CARDBOARD);
controller.width = eye_width;       // the view's pixels, for the reticle's size
controller.height = eye_height;
// each frame
controller.update(&scene, &input, seconds)?;
// on a screen tap, or a controller's trigger
controller.button(&scene, &input, true);
controller.button(&scene, &input, false);
```

`update` moves the pointer (hover and drag) and the gaze (fuse) along the controller's ray,
then places the reticle and runs its animations. `show_reticle(false)` hides it
(ViroController's `reticleVisibility`). The controller's 3D model (`controllerVisibility`)
is the app's to draw at the pose.

## Depth for the compositor

Headset compositors reproject a late frame by its depth. `SceneDepthWriter` copies the depth
of the last frame drawn into a depth texture as large as that frame, such as the one a
visionOS drawable hands over (wrapped with `Gpu_texture_from_metal`):

```c
SceneDepthWriter w = scene_depth_writer();
defer w.release();
scene.render_eye(&color, eye)?;
w.write(&scene, &color, &depth, true)?;   // true: reverse-Z, 1 at near, as visionOS wants
```

The reversed value is `1 - d`, which is the same projection with reverse-Z for the same near
and far planes.

## Headsets: visionOS and Quest

`examples/xr` draws one scene (`xr_scene.esk`: three turning boxes over a lit floor) on two
headsets, each through the system's own frame loop. Pointing at a box and clicking stops or
restarts its turn: the scene's `SceneController` (`SCENE_CONTROLLER_POINTER`) takes the ray of
a Quest controller's aim pose and its trigger, or of a visionOS pinch (`xr_scene_pointer`). The scene exports a few C functions
(`xr_scene_open`, `xr_scene_step`, `xr_scene_render`, `xr_scene_wait`); for each view the host
hands over the system's color image, the view's world pose and its field-of-view tangents,
and the scene wraps the image as a `GpuTexture` and draws with `Scene_render_eye`. Both hosts
link wgpu-native from eskiu-lang/wgpu-native's eskiu branch (`WGPU`), which adds the calls that
wrap a system image, and an eskiu-gpu that has them (`GPU_DIR`, until a gpu release does).

### visionOS

`examples/xr/visionos` is a full immersive space on Apple Vision Pro. A small SwiftUI window
opens the space at launch and closes; its `CompositorLayer` runs the frame loop of `host.m`,
which takes each frame's drawables from CompositorServices and the device's pose at the
frame's presentation time from ARKit's world tracking. A pinch arrives through the layer's
`onSpatialEvent` on the main thread, as a selection ray held down until the pinch ends; the
render thread hands the queued pinches to the scene before each frame. The view's pose is the device anchor
times the view's transform; the color texture is wrapped with `Gpu_texture_from_metal`. After
the views are drawn, the loop waits for the GPU and presents the drawable.

```sh
examples/xr/visionos/build.sh       # build/EskiuXR.app for the simulator (SDK=xros: a device)
xcrun simctl install booted examples/xr/visionos/build/EskiuXR.app
xcrun simctl launch booted dev.eskiu.scene.xr
```

Each view's depth goes into the drawable's depth texture (`SceneDepthWriter`, reversed, with
the drawable's depth range set to the scene's planes; the compositor wants a near plane of
at least 0.1 m), so the system can reproject the frames. In the simulator the head sits at
the world origin, which is the floor on a device, so the example lifts it to standing
height there.

### Quest

`examples/xr/quest` is a NativeActivity for Meta Quest (`host.c`). It opens the GPU on
Vulkan, binds an OpenXR session to wgpu's Vulkan device (`XR_KHR_vulkan_enable`, with the
objects from `Gpu_vulkan_handles`), makes a swapchain per eye in `R8G8B8A8_SRGB`, and each
frame wraps the acquired swapchain image with `Gpu_texture_from_vulkan` and draws the eye at
the pose and field of view `xrLocateViews` gives in the stage space (local space when there
is no stage). With `XR_KHR_composition_layer_depth` it also makes a `D32_SFLOAT` depth
swapchain per eye, writes each eye's depth into it (`SceneDepthWriter`, forward) and chains
it to the projection view, for the runtime's reprojection. Input is an action set with each
hand's aim pose and trigger, bound for the Touch controllers, the simple controller and, with
`XR_EXT_hand_interaction`, the hands' pinch; the hand whose trigger was pulled last points.

```sh
OPENXR_DIR=path/to/openxr_loader_for_android examples/xr/quest/build.sh   # build/EskiuXR.apk
adb install -r examples/xr/quest/build/EskiuXR.apk
```

`OPENXR_DIR` is Khronos' `openxr_loader_for_android` AAR, extracted. It runs on a Quest 3
(`quest/quest3.png`, the compositor's view of both eyes). OpenXR wants the Vulkan device to
carry the extensions `xrGetVulkanDeviceExtensionsKHR` lists, and wgpu makes the device, so the
host hands that list to `wgpuSetVulkanDeviceExtensions` before the scene opens the GPU (without
them `xrCreateSession` fails with `XR_ERROR_INITIALIZATION_FAILED`). Each hand shows its
controller: a small body and a laser along the aim pose to what it points at, with the
scene's reticle there; hands without controllers point and pinch through
`XR_EXT_hand_interaction` (the manifest asks for hand tracking).
