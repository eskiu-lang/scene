# scene documentation

Guides and reference for scene. The [README](../README.md) has the overview.

<!-- pages -->
- [The scene graph](scene-graph.md): nodes, cameras, meshes, glTF models
- [Materials](materials.md): lighting models, textures, alpha and blending
- [Lights and shadows](lights.md)
- [Rendering, post-processing and the sky](rendering.md)
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
| `<scene/input>` | screen rays, hit tests, `SceneInput` and node events |

## Tests

Each test renders small images and reads them back:

| Test | What it checks |
|---|---|
| `tests/graph.esk` | world matrices through parents, `look_at` under a rotated and scaled parent, reparenting, removal, the shapes' winding, normals and bounds, matrix decomposition, material keys, the CPU falloff and tone curves |
| `tests/lighting.esk` | each lighting model on a sphere against its formula at ten pixels (linear output, within 0.006 + 1%) |
| `tests/attenuation.esk` | point light falloff (exponent 2 and 1) and the spot cone along a profile, the influence mask, eight lights per draw |
| `tests/transparency.esk` | back-to-front order of two transparent quads whatever order they were added, an opaque quad between them, each blend mode against its formula, a texture with a masked quadrant |
| `tests/post.esk` | each tone mapping operator at three exposures against `scene_tone_map`, sRGB encoding by the shader and by an sRGB target, bloom spreading and fading, its strength and threshold |
| `tests/sky.esk` | background color, skybox faces by view direction, sky intensity, six-image orientation, image-based lighting from a constant environment against its expected value |
| `tests/shadows.esk` | a box's shadow on the ground where geometry puts it, opacity, nodes that do not cast or receive, two shadowed lights |
| `tests/gltf.esk` | a glTF model's node tree, transforms, unlit material and pose updates |
| `tests/input.esk` | screen rays against the camera's field of view, hits on rotated, scaled and nested nodes, hidden and `ignore_events` nodes, bubbling to a handler, click states, fixed-distance and plane drags, pinch, rotation, hover hysteresis, fuse |

## Not yet

These ViroReact renderer features are planned for later versions:

- Portals (a window into another scene, through the stencil buffer).
- Particle emitters.
- Polylines and lines with a width.
- 3D text, on [eskiu-font](https://github.com/eskiu-lang/font).
- Post effects beyond bloom (color grading, vignette, depth of field, SSAO).
- Shader modifiers (user WGSL inserted into the generated shaders).
- Shadows from spot and point lights, and cascaded shadow maps for large scenes.
- glTF samplers (the scene's sampler is used: linear, mipmapped, repeating).
- Node opacity, render order overrides, instanced drawing, frustum culling.

## Contributing

Bug reports are welcome; please include the `eskiuc --version` output, the GPU and, for a
rendering bug, a small program that shows it. The tests run with `eskiuc test`.
`python3 -I tools/gen_api.py` regenerates `docs/api.md` from the source comments, and CI
fails when a public declaration has no comment or the file is stale.
