# scene documentation

Guides and reference for scene. The [README](../README.md) has the overview.

<!-- pages -->
- [The scene graph](scene-graph.md): nodes, cameras, meshes, glTF models
- [Materials](materials.md): lighting models, textures, alpha and blending
- [Lights and shadows](lights.md)
- [Rendering, post-processing and the sky](rendering.md)
- [Animated images and video](images.md): GIFs, video frames on surfaces and as a 360 sky
- [Portals](portals.md): worlds seen through windows, walking through them
- [Polygons and polylines](shapes.md)
- [Particles](particles.md): emitters, spawn volumes, bursts, modifiers
- [Text](text.md): TrueType text laid out as ViroText
- [Physics](physics.md): rigid bodies, forces, collisions, ray and shape tests
- [Property animations](animation.md): groups, easing, chains, loops
- [Hit testing and events](input.md): rays, hit tests, click, drag, pinch, rotate, hover and fuse
- [API reference](api.md): every public function and type
<!-- /pages -->

## Examples

[`examples/scene.esk`](../examples/scene.esk) opens a window with a ground, five spheres
(one per lighting model, the unlit one emissive), a transparent box, a sun with shadows, a
point light and a spot light, under a sky made in code that also lights the PBR sphere
through `<gpu/ibl>`, with Hable tone mapping and bloom. Drag to orbit, scroll to zoom, B
toggles bloom and T cycles the tone mapping operators.

```bash
eskiuc run examples/scene.esk
eskiuc run examples/scene.esk -- --frames 120     # stop after 120 frames
```

## Platforms

scene is pure Eskiu over `<gpu>`, so it runs where gpu renders: Metal on macOS and iOS,
Vulkan on Linux and Android, DirectX 12 on Windows. The tests read rendered pixels back and
run in CI on macOS (Metal), Linux (lavapipe, software Vulkan) and Windows (WARP, software
DirectX 12). The example needs `<gpu/window>`, which links SDL3 (`brew install sdl3` on
macOS; on Linux and Windows, an SDL3 build from your package manager or libsdl.org).

## Modules

| Module | Contents |
|---|---|
| `<scene>` | `Scene`, nodes, cameras, meshes, textures, glTF import, rendering; imports the others |
| `<scene/geometry>` | `SceneGeometry` and the box, sphere, plane and quad shapes |
| `<scene/material>` | `SceneMaterial`, lighting models, alpha and blend modes, shader variants |
| `<scene/light>` | `SceneLight`, its constructors and falloff curves |
| `<scene/post>` | `ScenePost`, tone mapping operators, CPU versions of the curves |
| `<scene/sky>` | cube maps from six images |
| `<scene/gif>` | `gif_decode`, `gif_load`, `SceneAnimatedImage` |
| `<scene/shapes>` | `scene_polygon` (holes, uv bounds), `ScenePolyline` (camera-facing ribbons) |
| `<scene/particles>` | `SceneParticleEmitter`, modifiers, bursts, spawn volumes |
| `<scene/text>` | `SceneText`, `SceneTextStyle`, layout, glyph atlases, outlines, drop shadows and extrusion over eskiu-lang/font |
| `<scene/animation>` | `SceneAnimations`: groups, easing curves, chains, playback |
| `<scene/physics>` | `ScenePhysicsWorld`, bodies, shapes, forces, collision events, ray and shape tests |
| `<scene/input>` | screen rays, hit tests, `SceneInput` and node events, `SceneOrbit` |

## Tests

Each test renders small images and reads them back:

| Test | What it checks |
|---|---|
| `tests/gif.esk` | GIF frames with disposal, transparency, interlacing and loop count against a file made by a standard encoder; playback timing, looping, pausing, the last frame; a borrowed texture shown and rebound |
| `tests/graph.esk` | world matrices through parents, `look_at` under a rotated and scaled parent, reparenting, removal, the shapes' winding, normals and bounds, matrix decomposition, material keys, the CPU falloff and tone curves |
| `tests/lighting.esk` | each lighting model on a sphere against its formula at ten pixels (linear output, within 0.006 + 1%) |
| `tests/attenuation.esk` | point light falloff (exponent 2 and 1) and the spot cone along a profile, the influence mask, eight lights per draw |
| `tests/effects.esk` | each color effect against ViroReact's formula, a chain's order, crosshatch lines at their pixels, toonify's edge, barrel and pincushion moving an edge, the zoom in a window, the swirl over time |
| `tests/portals.esk` | a portal's world only through its window, an outer object covering the window, the portal's sky, walking in and looking back out, a portal that is not passable, a portal inside a portal, each world lit only by its own environment |
| `tests/shapes.esk` | polygon area with holes and concave outlines, winding, texture coordinates, repeated points; polyline width, round ends and joins; both drawn |
| `tests/particles.esk` | modifier intervals, the rate per second, the cap, bursts, a cycle without loop, motion, fixed and free particles, spawn volumes, an explosion, additive drawing |
| `tests/text.esk` | sizes at 0.01 world units a point, word, character and no wrapping, justification, clipping and the line cap, alignment, rebuilding, the glyphs drawn inside their bounds, an outline around a glyph and behind it, a drop shadow's placement, extruded faces at their depths covering the glyph with its holes |
| `tests/transparency.esk` | back-to-front order of two transparent quads whatever order they were added, an opaque quad between them, each blend mode against its formula, node opacity on opaque and blended materials and down the tree, a texture with a masked quadrant |
| `tests/post.esk` | each tone mapping operator at three exposures against `scene_tone_map`, sRGB encoding by the shader and by an sRGB target, bloom spreading and fading, its strength and threshold |
| `tests/sky.esk` | background color, skybox faces by view direction, sky intensity, six-image orientation, image-based lighting from a constant environment against its expected value, a 360 image by direction and turned by `sky_rotation` |
| `tests/shadows.esk` | a box's shadow on the ground where geometry puts it, opacity, nodes that do not cast or receive, two shadowed lights, a spot light's shadow over its whole cone |
| `tests/gltf.esk` | a glTF model's node tree, transforms, unlit material and pose updates |
| `tests/animation.esk` | the easing curves against ViroReact's formulas, Euler order, groups with delays, serial and parallel chains, a skipped frame, ViroReact's property strings, relative values across loops, opacity, color, pause, resume, stop |
| `tests/physics.esk` | free fall against ViroReact's gravity and fixed steps, the 10-step cap, a ball resting on a floor with one collision event, a bounce, a stack of boxes settling flat and sleeping, forces, constant velocity, a torque impulse, a kinematic body pushing, closest and all ray hits, shape tests at a point and swept, shapes from scaled meshes and children |
| `tests/input.esk` | screen rays against the camera's field of view, hits on rotated, scaled and nested nodes, hidden and `ignore_events` nodes, bubbling to a handler, click states, fixed-distance and plane drags, pinch, rotation, hover hysteresis, fuse |

## Not yet

These ViroReact renderer features are planned for later versions:

- Knuth-Plass line breaks for justified text.
- Post effects beyond ViroReact's (color grading, vignette, depth of field, SSAO).
- Shader modifiers (user WGSL inserted into the generated shaders).
- Cascaded shadow maps for large scenes, and shadows from point lights (ViroReact has neither).
- Instanced drawing.

## Contributing

Bug reports are welcome; please include the `eskiuc --version` output, the GPU and, for a
rendering bug, a small program that shows it. The tests run with `eskiuc test`.
`python3 -I tools/gen_api.py` regenerates `docs/api.md` from the source comments, and CI
fails when a public declaration has no comment or the file is stale.
