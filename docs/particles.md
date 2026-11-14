# Particles

`<scene/particles>` emits particles from a node as ViroReact's `ViroParticleEmitter` does:
textured quads that face the camera, spawned over time, over distance and in bursts,
changing color, alpha, scale and rotation as they age, travel or slow down.

```c
SceneParticleEmitter sparks = scene_particles(&scene, torch)?;
defer sparks.free(&scene);
sparks.image(&scene, scene.load_texture("spark.png", true)?, 0.05, 0.05);
sparks.rate_per_second = Vec2{40.0, 60.0};
sparks.lifetime = Vec2{0.6, 1.0};
sparks.velocity.min = Vec3{-0.2, 1.0, -0.2};
sparks.velocity.max = Vec3{0.2, 1.5, 0.2};
sparks.acceleration = scene_particle_constant(Vec3{0.0, -1.0, 0.0});
sparks.alpha.over(SCENE_PARTICLE_TIME, Vec3{0.0, 0.0, 0.0}, 0.4, 1.0);   // fade out
...each frame, after moving nodes and before rendering:
sparks.update(&scene, now)?;
```

`scene_particles(&scene, node)` makes an emitter at `node` with ViroReact's defaults and adds
a node, at the root, that draws its particles; `SceneParticleEmitter_update` advances it to
a time in seconds and rebuilds that node's quads (written into the same buffers each frame).
ViroReact's properties are in milliseconds; here they are in seconds.

## Emission

| Field | Default | Meaning |
|---|---|---|
| `duration`, `loop`, `delay` | 2, true, 0 | one emission cycle, whether the next one follows, the wait before the first |
| `run` | true | emitting; turned off, the particles alive keep going |
| `paused` | false | freezes the particles and the emitter's clock |
| `fixed_to_emitter` | true | particles move with the node after spawning; false leaves them where they were born |
| `max_particles` | 500 | no spawn that would pass it |
| `lifetime` | 2, 2 | seconds, uniform between the two |
| `rate_per_second` | 10, 10 | a rate drawn between the two each second, spawned evenly |
| `rate_per_meter` | 0, 0 | particles per world unit the node moves, drawn again each unit |
| `bursts` | none | `SceneParticleBurst`: a count once the cycle's time (or distance) passes `start`, again every `interval`, `cycles` times |
| `volume`, `volume_size`, `volume_point`, `on_surface` | point | `SCENE_SPAWN_POINT` at `volume_point`, `_BOX` (width, height, length) or `_SPHERE` (radii, or one radius), inside or on the surface |
| `width`, `height` | 1, 1 | the quad before scale |

Spawning follows ViroReact's timing: the per-second rate is recomputed every second and
spawns nothing in the frame that recomputes it; the first frame of a cycle spawns
nothing.

## Modifiers

`color`, `alpha` (in `x`), `scale`, `rotation` (radians, about X, then Y, then Z),
`velocity` and `acceleration` are `SceneParticleModifier`s: a particle starts with a value
uniform between `min` and `max`, then the intervals added with `over(factor, target, start,
end)` move it to each target while the factor goes from `start` to `end`. The factor is the
particle's age in seconds (`SCENE_PARTICLE_TIME`), the distance it has travelled
(`SCENE_PARTICLE_DISTANCE`) or its speed (`SCENE_PARTICLE_VELOCITY`). Between intervals
the value holds at the last target; past the last, at its target.
`scene_particle_constant(v)` is a modifier fixed at `v`.

Defaults: white, alpha 1, scale 1, no rotation, a velocity between (-2, 6, 0) and
(2, 6, 0), no acceleration.

A particle moves by `velocity * t + acceleration * t² / 2` from where it spawned. As in
ViroReact, its scale and rotation apply to that offset from the emitter too, not only to
its quad. `explosion_impulse` (above 0) replaces the initial velocity by an impulse away
from `explosion_center`, weaker with the square of the distance; `explosion_decel` (above
0) brings it to a stop over that many seconds.

## Drawing

The particles are one mesh of quads turned to face the camera with ViroReact's billboard
rotation, colored by each particle's color and alpha. The material is unlit
(`SCENE_CONSTANT`), double-sided, does not write depth and adds its color
(`SCENE_BLEND_ADD`, ViroReact's default); set another blend mode or texture through
`Scene_material(emitter.material)`, or `SceneParticleEmitter_image`. The drawing node
ignores hit tests and casts no shadow.
