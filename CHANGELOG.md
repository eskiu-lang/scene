# Changelog

All notable changes to this package are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- `<scene/input>`: `Scene_screen_ray`, `Scene_hit_test` and `Scene_hit_test_all` (mesh
  bounds tested in each node's own space, so rotated, scaled and nested nodes are exact),
  and `SceneInput`, which turns touches (`SceneInput_touch`) or any controller's pointer,
  button, gestures and gaze into click, drag, pinch, rotate, hover and fuse events on
  nodes. Events bubble to the nearest ancestor with a handler; thresholds, drag types and
  hover hysteresis follow ViroReact. `docs/input.md`.
- `SceneNode.ignore_events` keeps a node out of hit tests.
- `Scene_camera_matrices` and `Scene_camera_is_ortho`: the view and projection the camera
  renders with.

## [0.1.0] - 2026-11-07

### Added

- glTF in the scene as in `<gpu/model>`: skinning (joint matrices per skinned primitive,
  recomputed by `SceneGltf_pose`), morph targets (`Scene_set_morph_targets`,
  `Scene_set_weights`), the second texture coordinate set, `KHR_texture_transform` per
  texture (`SceneMaterial.uv`, `SceneUv`), `KHR_node_visibility`, and the material factors
  and texture transforms that `KHR_animation_pointer` channels animate. Shadows follow
  skinning and morphs. `Scene_set_joints`, `Scene_set_weights` and `Scene_set_morph_targets`
  work on any mesh; a vertex is now 26 floats (second uv, joints, weights).

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
