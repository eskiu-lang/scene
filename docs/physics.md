# Physics

`<scene/physics>` gives nodes rigid bodies as ViroReact's `physicsBody` does: static,
kinematic or dynamic bodies with box, sphere and compound shapes, gravity, friction and
restitution, forces, torques, impulses and velocities, collision events, and ray and shape
tests.

```c
ScenePhysicsWorld world = scene_physics();
defer world.free();
world.add(&scene, ground, SCENE_BODY_STATIC, 0.0, scene_physics_box(Vec3{10.0, 0.1, 10.0}));
int ball = world.add(&scene, node, SCENE_BODY_DYNAMIC, 1.0, scene_physics_sphere(0.2));
world.body(ball).restitution = 0.6;
world.body(ball).tag = "ball";
world.on_collision(ball, void(*ScenePhysicsCollision c) {
    printf("hit %s\n", c.other_tag);
});
...each frame, after moving nodes and before rendering:
world.update(&scene, now);
```

`ScenePhysicsWorld_update` advances the world to a time in seconds in fixed steps of 1/60 s,
at most 10 a frame (ViroReact's), then writes each dynamic body's position and rotation to
its node. Static and kinematic bodies read their node's transform at each step.

## Bodies

`ScenePhysicsWorld_add(&scene, node, type, mass, shape)` returns the body's id;
`ScenePhysicsWorld_body(id)` gives its fields.

| Type | Mass | Moves |
|---|---|---|
| `SCENE_BODY_STATIC` | 0 | never |
| `SCENE_BODY_KINEMATIC` | 0 | with its node, pushing dynamic bodies |
| `SCENE_BODY_DYNAMIC` | above 0 | under the simulation |

As in ViroReact, a static or kinematic body's mass is 0 and a dynamic body needs one above 0
(a wrong mass becomes 0 or 1).

| Field | Default | Meaning |
|---|---|---|
| `tag` | `""` | the name other bodies see in their collision events (ViroReact's `viroTag`) |
| `friction` | 0.5 | combined with the other body's as the square root of the product |
| `restitution` | 0 | bounciness, combined as the product (against a static or kinematic body, the larger) |
| `use_gravity` | true | |
| `enabled` | true | false takes the body out of the simulation and the queries |
| `linear_velocity`, `angular_velocity` | 0 | read or set directly |
| `sleeping` | | a body nearly still for 2 s sleeps until something touches it or a force, impulse or velocity is given |

The node's scale applies to the shape when the body is added. After moving a dynamic body's
node by hand, `ScenePhysicsWorld_refresh(&scene, id)` takes the new transform.
`ScenePhysicsWorld_remove(id)` takes a body out for good.

## Shapes

| Shape | ViroReact |
|---|---|
| `scene_physics_box(Vec3{w, h, l})` | `Box` with width, height and length |
| `scene_physics_sphere(r)` | `Sphere` |
| `scene_physics_auto(&scene, node)` | no shape given: a box around the node's mesh, in its scale |
| `scene_physics_auto_compound(&scene, node)` | `Compound` without parts: a box per mesh in the node and its descendants |
| `scene_physics_compound()` + `_add_box(size, offset)`, `_add_sphere(r, offset)` | `Compound` with authored parts, weighted by volume for the center of mass |

ViroReact's automatic shape is a sphere for sphere geometry; here it is always a box.

## Forces and velocities

| Function | Effect |
|---|---|
| `apply_force(id, force, position)` | a force at an offset from the center, at every step until `clear_forces` |
| `apply_torque(id, torque)` | a torque at every step until `clear_forces` |
| `apply_impulse(id, impulse, position)` | a change of momentum, once |
| `apply_torque_impulse(id, impulse)` | a change of angular momentum, once |
| `set_velocity(id, v, constant)` | the velocity, once, or kept at every step when `constant` |

## Collisions

`ScenePhysicsWorld_on_collision(id, handler)` calls the handler with a
`ScenePhysicsCollision`: the body, the other body's tag, the contact point and the normal
pointing from the other body into this one. As in ViroReact, contacts are sampled every 160
ms and only a contact that was not there at the previous sample is reported, so a ball
resting on the floor reports the floor once.

## Queries

`ScenePhysicsWorld_ray_test(from, to, closest, tag)` finds the bodies the segment crosses
and tells each (only the nearest when `closest`) through its collision handler, with `tag`
as the other tag and the point and normal where the ray entered.
`ScenePhysicsWorld_shape_test(from, to, shape, tag)` places a shape at `from`, or sweeps it
to `to`, and tells the bodies it touches (when swept, those at the first place it touches
any). Both return whether they hit something, as ViroReact's `findCollisionsWithRayAsync`
and `findCollisionsWithShapeAsync`.

## Differences from ViroReact

ViroReact runs Bullet. This module is its own solver: contact manifolds from separating axes
and face clipping (exact for spheres), sequential impulses with friction and restitution,
and sleeping. Bodies come to rest, stack and bounce as they do there, but a simulation is
not step-for-step identical, and triangle mesh shapes are not supported.
