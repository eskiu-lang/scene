# Hit testing and events

`<scene/input>` finds the node under a pointer and turns presses, moves and two-finger
gestures into events on nodes, with the rules of ViroReact's `onClick`, `onDrag`,
`onPinch`, `onRotate`, `onHover` and `onFuse`.

## Rays and hit tests

`Scene_screen_ray(x, y, width, height)` is the ray from the scene's camera through a pixel
of a `width` x `height` image, counted from the top left, the units of a `GpuTouch` and
of the render target. It uses the same projection as the render, so the ray through a
pixel passes through what that pixel shows. For a perspective camera it starts at the
camera, for an orthographic one on the near plane.

```c
SceneRay r = scene.screen_ray(t.x, t.y, width, height);
SceneHit hit;
if (scene.hit_test(r, &hit)) printf("node %d at %.2f\n", hit.node, hit.distance);
```

A node is hit when the ray crosses the bounds of its mesh, carried through the node's world
matrix: the test runs in the node's own space, so it is exact for a rotated, scaled or
nested node. Nodes without a mesh, hidden nodes (and the subtrees of hidden nodes) and
nodes with `ignore_events` set are not hit; rays pass through them.
`Scene_hit_test` gives the hit drawn on top, as ViroReact does: hits are taken in draw order
(`rendering_order`, then farthest first), and one behind a nearer node whose material
writes depth is hidden unless its own material skips the depth test. Without rendering
orders or depth settings that is the nearest hit. When the ray hits nothing, it returns
false and the hit is the background: node 0, `background` true, the point `SCENE_BACKGROUND_DISTANCE`
(8) units along the ray. `Scene_hit_test_all` lists every hit, nearest first.

Bounds are those of the mesh as uploaded. A skinned or morphed mesh is tested against its
bind pose.

## Handlers

A `SceneInput` holds one handler per node and the state of the pointer and gestures:

```c
SceneInput input = scene_input();
defer input.free();
input.on(model.root, SCENE_ON_CLICK | SCENE_ON_DRAG, void(*SceneEvent e) {
    if (e.kind == SCENE_EVENT_CLICK && e.state == SCENE_CLICKED) select(e.node);
});
```

The mask says which events the handler takes (`SCENE_ON_CLICK`, `SCENE_ON_DRAG`,
`SCENE_ON_PINCH`, `SCENE_ON_ROTATE`, `SCENE_ON_HOVER`, `SCENE_ON_FUSE`). An event goes to
the node hit, or, when that node has no handler for that event, to its nearest ancestor
that has one. A handler on a glTF model's root node therefore takes the events of all its
meshes. In the event, `node` is the handler's node and `target` the node the ray hit.
`SceneInput_off` removes a handler.

## Feeding it

`SceneInput_touch(scene, t, width, height)` takes each `GpuTouch` from `<gpu/ui>` (or any
source of touches in view pixels):

- One finger points, presses where it lands, drags, and releases.
- A second finger ends that press without a click, ends a drag, and starts a pinch and a
  rotation, which end when either finger lifts.

Another controller (a mouse, a headset's controller, a hand) calls the parts directly:
`SceneInput_pointer(scene, ray, time)` when it moves, `SceneInput_press` and
`SceneInput_release` for its button, `SceneInput_cancel` when the system takes the
gesture, `SceneInput_pinch` and `SceneInput_rotate` for gestures it recognizes, and
`SceneInput_gaze(scene, ray, time)` once a frame for fuse. Times are seconds on any clock
that only goes forward.

## Events

| Event | `state` | Fields |
|---|---|---|
| `SCENE_EVENT_CLICK` | `SCENE_CLICK_DOWN` on the press, `SCENE_CLICK_UP` on the release (sent to the node that took the press), then `SCENE_CLICKED` when the release is still on that node | `point`, `has_point` (false for an up that left the node) |
| `SCENE_EVENT_DRAG` | `SCENE_GESTURE_START` on the press, `SCENE_GESTURE_MOVE` as the node moves, `SCENE_GESTURE_END` on the release | `position`, the node's world position |
| `SCENE_EVENT_PINCH` | start, move, end | `scale`, the fingers' distance over the starting one |
| `SCENE_EVENT_ROTATE` | start, move, end | `rotation`, radians turned since the start |
| `SCENE_EVENT_HOVER` | 1 on entering, 0 on leaving | `point` on entering |
| `SCENE_EVENT_FUSE` | 0 | `fuse`, the share of the fuse time left: from 1 down to 0 (fused, sent once), -1 when the gaze leaves |

Moves smaller than ViroReact's thresholds are not reported: `SCENE_DRAG_THRESHOLD` (0.01
world units), `SCENE_PINCH_THRESHOLD` (0.02) and `SCENE_ROTATE_THRESHOLD` (0.01 radians).
A hover change is reported only once it holds for `SCENE_HOVER_HYSTERESIS` (75 ms), so a
ray on a node's edge does not flicker between enter and leave; the first hover is reported
at once. While a node is dragged its hit is not recomputed, so the release after a drag
still counts as a click on it, as in ViroReact.

## Dragging

The drag moves the node that takes `SCENE_ON_DRAG` (the hit node or its ancestor) by
setting its position; the rotation stays. `SceneInput_set_drag(node, type, plane_point,
plane_normal, max_distance)` chooses how it follows the pointer:

- `SCENE_DRAG_FIXED_DISTANCE` (the default): the point that was grabbed stays at the same
  distance from the pointer's origin.
- `SCENE_DRAG_FIXED_DISTANCE_ORIGIN`: the node's origin stays at its distance from the
  pointer's origin.
- `SCENE_DRAG_FIXED_TO_PLANE`: the node moves on the plane through `plane_point` with
  normal `plane_normal`, where the ray meets it within `max_distance` of the pointer's
  origin. A node more than 0.01 off the plane snaps onto it at the first move.

## Fuse

A node that takes `SCENE_ON_FUSE` counts down while `SceneInput_gaze` stays on it, over
`SceneInput_set_fuse_time` seconds (2 by default). Call `SceneInput_gaze` once a frame
with the ray through the image's center, or a headset's forward ray.
