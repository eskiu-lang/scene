<p align="center">
  <img src="https://raw.githubusercontent.com/eskiu-lang/eskiu/main/assets/logo.png" alt="Eskiu, a kitten with a ball of yarn" width="140">
</p>

<h2 align="center">scene</h2>
<p align="center">A scene graph and material renderer for Eskiu, on gpu.</p>
<p align="center"><sub>Part of the <a href="https://github.com/eskiu-lang">Eskiu</a> project</sub></p>
<p align="center">
  <a href="https://eskiu-lang.org">eskiu-lang.org</a> &nbsp;&middot;&nbsp;
  <a href="docs/README.md">Documentation</a> &nbsp;&middot;&nbsp;
  <a href="examples/">Examples</a> &nbsp;&middot;&nbsp;
  <a href="https://github.com/eskiu-lang/eskiu">The language</a>
</p>

<p align="center">
  <a href="https://github.com/eskiu-lang/scene/actions/workflows/ci.yml"><img src="https://github.com/eskiu-lang/scene/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <img src="https://img.shields.io/badge/version-0.1.0-6448d4" alt="Version 0.1.0">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue" alt="License: MIT"></a>
</p>

---

`scene` draws 3D scenes with [gpu](https://github.com/eskiu-lang/gpu) (WebGPU through
wgpu-native, so Metal, Vulkan and DirectX 12). It covers the core of what ViroReact's
renderer does for an app: a tree of nodes, materials, lights, transparency and an HDR image
pipeline.

- **Scene graph.** Nodes with position, rotation and scale, parents and children,
  visibility, cameras (perspective and orthographic), procedural shapes and glTF or OBJ
  models.
- **Materials.** Constant, Lambert, Phong, Blinn and PBR (metallic-roughness) lighting,
  diffuse, specular, normal, emissive, metal-roughness and occlusion textures, opaque,
  masked and blended alpha with six blend modes, double-sided surfaces. One WGSL shader is
  composed and cached per variant.
- **Lights.** Ambient, directional, point (with start and end distances) and spot (inner
  and outer cones), up to 8 per draw, and directional and spot shadow maps with soft edges.
- **HDR.** Linear RGBA16Float rendering, transparent surfaces sorted back to front,
  Reinhard, exposure and Hable tone mapping, bloom, ViroReact's post effects, and sRGB
  output.
- **Sky.** A solid color, a cube map from `<gpu/ibl>` or six images, a 360 image, and
  image-based lighting for PBR materials.
- **Images and video.** Animated GIFs decoded in Eskiu, and video frames from
  eskiu-lang/video on surfaces or as a 360 sky.
- **Portals.** ViroPortalScene's worlds seen through windows, with their own skies, that the
  camera can walk into.
- **Shapes.** ViroPolygon's polygons with holes and ViroPolyline's camera-facing lines.
- **Particles.** ViroParticleEmitter's emitters: rates, bursts, spawn volumes, modifiers
  over time, distance or speed, explosions.
- **Text.** TrueType text from eskiu-lang/font in a box, wrapped, aligned, justified and
  clipped as ViroText lays it out.
- **Animation.** Position, scale, rotation, opacity and color animated in groups and
  chains with ViroReact's easing curves, node opacity down the tree, rendering order,
  billboards and frustum culling.
- **Input.** Hit tests from the screen or any controller, and click, drag, pinch, rotate,
  hover and fuse events on nodes, bubbling to the nearest handler as in ViroReact.

## A lit sphere

```eskiu
import <gpu>;
import <gpu/math>;
import <scene>;

Result<int, string> draw(*Gpu gpu, *GpuTexture target) {
    Scene scene = gpu.scene()?;
    defer scene.release();
    SceneGeometry ball = scene_sphere(1.0, 48, 24);
    int mesh = scene.add_geometry(&ball)?;
    ball.free();
    SceneMaterial m = scene_material(SCENE_PBR);
    m.diffuse = Vec4{0.9, 0.2, 0.1, 1.0};
    m.roughness = 0.4;
    scene.add_object(0, mesh, scene.add_material(m));
    scene.add_light(scene_directional(Vec3{-0.5, -1.0, -0.3}, Vec3{1.0, 1.0, 1.0}, 2.0));
    scene.add_light(scene_ambient(Vec3{0.4, 0.45, 0.5}, 0.3));
    int cam = scene.add_camera(0, scene_perspective(0.9, 0.1, 100.0));
    scene.node(cam).position = Vec3{0.0, 1.0, 4.0};
    scene.look_at(cam, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0});
    scene.set_camera(cam);
    scene.post.tone_mapping = SCENE_TONE_HABLE;
    return scene.render_to(target);   // or scene.render(frame.view, w, h, surface.format)
}
```

## Documentation

- [The scene graph](docs/scene-graph.md): nodes, cameras, meshes, glTF models
- [Materials](docs/materials.md): lighting models, textures, alpha and blending
- [Lights and shadows](docs/lights.md)
- [Rendering, post-processing and the sky](docs/rendering.md)
- [Examples, platforms, modules, tests and what is not there yet](docs/README.md)
- [API reference](docs/api.md): every public function and type

## Install

```bash
eskiuc add scene --git https://github.com/eskiu-lang/scene --rev v0.1.0
```

It needs Eskiu 0.15.1 or newer and brings gpu 0.6.0, which downloads wgpu-native on the first build.

## License

MIT. See [LICENSE](LICENSE).
