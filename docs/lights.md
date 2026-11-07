# Lights and shadows

`<scene/light>`: ambient, directional, point and spot lights, and directional shadow maps.

API reference: [`<scene/light>`](api.md#scenelight).

```eskiu
SceneLight sun = scene_directional(Vec3{-0.4, -1.0, -0.3}, Vec3{1.0, 0.95, 0.9}, 2.0);
sun.casts_shadows = true;
scene.add_light(sun);
scene.add_light(scene_ambient(Vec3{0.3, 0.35, 0.4}, 0.5));
int lamp = scene.add_light(scene_point(Vec3{0.0, 2.0, 0.0}, Vec3{1.0, 0.6, 0.3}, 3.0, 0.5, 6.0));
SceneLight spot = scene_spot(Vec3{0.0, 3.0, 0.0}, Vec3{0.0, -1.0, 0.0}, Vec3{1.0, 1.0, 1.0}, 4.0,
    0.3, 0.5, 1.0, 10.0);   // inner and outer angles from the axis in radians, then start and end
spot.node = lantern;        // position and direction in that node's space
scene.add_light(spot);
```

## Strength

A light's strength is `color * intensity`. A white light of intensity 1 straight onto a
white Lambert (or PBR dielectric, nearly) surface lights it to 1. ViroReact gives
intensities in lumens, 1000 for the same brightness: divide them by 1000.

## Attenuation and cones

Point and spot lights are at full strength up to `attenuation_start`, none from
`attenuation_end`, and in between

```
attenuation = 1 - t^(1 / falloff),  t = (distance - start) / (end - start)
```

with `falloff` 2 by default (1 is linear). A spot light also fades from its axis:

```
cone = clamp((cos(angle) - cos(outer_angle)) / (cos(inner_angle) - cos(outer_angle)), 0, 1)
```

where `angle` is between the axis and the direction to the lit point. `inner_angle` and
`outer_angle` are measured from the axis (half the cone's opening). `SceneLight_attenuation`
and `SceneLight_cone` compute both on the CPU.

## Which lights reach a node

Ambient lights are summed and light every node. Of the other lights (at most
`SCENE_MAX_LIGHTS`, 64, enabled at once), each drawn node gets up to
`SCENE_LIGHTS_PER_DRAW`, 8: those whose `influence` shares a bit with the node's
`light_mask`, directional lights first, then point and spot lights by distance from the
node's bounding sphere, skipping those whose `attenuation_end` does not reach it. Lights on
a node move with it (`SceneLight.node`); with `node` -1 their position and direction are in
world space.

## Shadows

Directional lights with `casts_shadows` (up to `SCENE_MAX_SHADOWS`, 4) each get a layer of
a Depth32Float shadow map array of `Scene.shadow_size` texels square (2048 by default). The
light sees an orthographic box: the cube around every visible node's bounds, or the cube of
half-size `shadow_radius` around `shadow_center` when that is set (better for large or
changing scenes). Opaque and masked nodes with `casts_shadows` are drawn into it; blended
ones are not.

The shading moves each point `shadow_normal_bias` texels along its normal (1.5), subtracts
`shadow_bias` from its depth in the light's box (0.002), and averages a 3 x 3 grid of
comparisons, each itself a bilinear 2 x 2 comparison, so the edge is soft over about two
texels. `shadow_opacity` (1) blends between no shadow and a black one; ambient light is not
shadowed. Nodes with `receives_shadows` false ignore shadows. Points outside the light's box
are lit.

Point and spot lights do not cast shadows yet.
