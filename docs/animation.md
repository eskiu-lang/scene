# Property animations

`<scene/animation>` animates a node's position, scale, rotation and opacity, and the
color of its material, with the model of ViroReact's `ViroAnimations`: groups with a
duration, a delay and an easing curve, chained one after another or run together.

## Groups

```c
SceneAnimations anims = scene_animations();
defer anims.free();
int up = anims.group(0.5, SCENE_EASE_IN_OUT);      // 0.5 seconds
anims.set(up, SCENE_ANIM_POSITION_Y, SCENE_ANIM_ADD, 1.0);
anims.set(up, SCENE_ANIM_OPACITY, SCENE_ANIM_ASSIGN, 0.5);
anims.set_delay(up, 0.2);
```

A group changes each property it sets from the value the node has when the group starts
to a new one: `SCENE_ANIM_ASSIGN` sets the value, `SCENE_ANIM_ADD` adds to the start,
`SCENE_ANIM_MULTIPLY` multiplies it. Properties: `SCENE_ANIM_POSITION_X/Y/Z`,
`SCENE_ANIM_SCALE_X/Y/Z`, `SCENE_ANIM_ROTATE_X/Y/Z` (Euler angles in radians, applied
about X, then Y, then Z as in ViroReact) and `SCENE_ANIM_OPACITY` (`SceneNode.opacity`).

`SceneAnimations_set_color` animates the diffuse color of the node's material. A material
is shared by every node that uses it, so give the node its own material to color one node.
`scene_argb_color` reads ViroReact's ARGB int. `SceneAnimations_set_material` gives the
node another material when the group starts, as ViroReact's `material` property does.

`SceneAnimations_property(anim, name, value)` takes a property as a ViroReact animation
writes it: `positionX`, `scaleY`, `rotateZ` (in degrees), `opacity`, with a number or
`"+=90"`, `"-=1"`, `"*=2"`, `"/=2"`.

## Easing

| Constant | ViroReact | Curve |
|---|---|---|
| `SCENE_EASE_LINEAR` | `Linear` | t |
| `SCENE_EASE_IN` | `EaseIn` | 2t² up to half time, then t |
| `SCENE_EASE_OUT` | `EaseOut` | t up to half time, then slows to a stop |
| `SCENE_EASE_IN_OUT` | `EaseInEaseOut` | both |
| `SCENE_EASE_BOUNCE` | `Bounce` | overshoots to 1.11 at half time and settles to 1 |
| `SCENE_EASE_POWER_DECEL` | `PowerDecel` | 1 - (1 - t)² |
| `SCENE_EASE_STEP` | | the start value until the end |

These are ViroReact's formulas. `scene_easing_named` maps ViroReact's names; any other
name, `PowerAccel` among them, is linear, as ViroReact treats it.

## Chains

`SceneAnimations_chain(anims, count, parallel)` runs groups and other chains one after
another, or all at once. A serial chain's next animation starts when the one before it
ends, from the values it left. ViroReact's array form maps directly: its outer array is a
parallel chain of serial chains.

```c
int[2] steps = {up, spin};
int hop = anims.chain(&steps[0], 2, false);
```

## Playing

```c
int play = anims.play(&scene, node, hop, true, now);   // loop
...each frame, before rendering:
anims.update(&scene, now);
```

`SceneAnimations_update` moves every playing animation to the given time. A frame that
skips past several groups still runs each of them in order, so a chain ends where it would
have at any frame rate. A loop starts again from the values the last pass left, so
relative values keep adding up, as in ViroReact. `SceneAnimations_play_then` takes a
callback for the end of a playback that does not loop. `SceneAnimations_pause` and
`SceneAnimations_resume` hold and continue one; `SceneAnimations_stop` ends it, with
`jump_to_end` setting every unfinished group to its end values first. A playback whose
node is removed ends.

Times are seconds on any clock that only goes forward, usually the frame time.
