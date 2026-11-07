# Changelog

All notable changes to this package are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-11-07

### Added

- `<scene>`: `Scene` (`Gpu_scene`), a node tree with position, rotation, scale, visibility,
  reparenting and removal, `look_at`, perspective and orthographic cameras, meshes,
  textures, glTF and OBJ import (`Scene_add_gltf`, `SceneGltf_pose`), and rendering into a
  window frame or a texture (`Scene_render`, `Scene_render_in`, `Scene_render_to`).
- `<scene/geometry>`: `SceneGeometry` with normals and tangents, and the box, sphere, plane
  and quad shapes.
- `<scene/material>`: Constant, Lambert, Phong, Blinn and PBR lighting models with diffuse,
  specular, normal, emissive, metal-roughness and occlusion textures; opaque, masked and
  blended alpha; alpha, add, multiply, subtract, screen and premultiplied blending;
  double-sided materials; one composed WGSL shader and pipeline per variant.
- `<scene/light>`: ambient, directional, point and spot lights with attenuation start, end
  and falloff, spot cones, influence masks, up to 8 lights per draw; directional shadow
  maps in a 2D array with percentage-closer filtering.
- `<scene/post>`: rendering in RGBA16Float, Reinhard, exposure and Hable tone mapping,
  bloom, sRGB output.
- `<scene/sky>`: cube map backgrounds from `<gpu/ibl>` or six images, and image-based
  lighting for PBR materials.
