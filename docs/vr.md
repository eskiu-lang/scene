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
