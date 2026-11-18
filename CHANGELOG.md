# Changelog

All notable changes to this package are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- `SceneNode.base`, a transform between a node's parent and its own position, rotation and scale (`Scene_set_base`, `Scene_clear_base`; `scene_node_frame` is the space the node's transform is in). Drags and physics place the node through it. eskiu-packages/arscene puts an AR anchor's correction there.

### Fixed

- A tap on a node that moved during the press (an animation, physics, the camera) is a click: a touch that lifts close to where it went down, soon, clicks the node it pressed, at the press's point, instead of only when the release still hits it (`SceneInput_release_tap`, `scene_is_tap`; `SceneInput_touch` uses them). A mouse or a controller still clicks only when it releases on the node.

## [0.2.1] - 2026-11-16

### Changed

- The package moved to the eskiu-packages organization; the old URL redirects. Depends on gpu 0.7.1.

## [0.2.0]

### Added

- `SCENE_PBR_VIRO`: virocore's physically based lighting (physical omni and spot lights,
  Schlick-GGX, ambient without metal), matched pixel for pixel by eskiu-lang/viro's golden check.
- ViroReact's output pipeline in `ScenePost`: `SCENE_TONE_HABLE_LUMINANCE` (HableLuminanceOnly),
  `gamma22` (a 1/2.2 power) and `backdrop` (a clear color that skips tone mapping and gamma).
- `<scene/vr>`: `Scene_render_eye` (an eye's pose in the head and its projection, folded into
  the projection so the camera node stays the head), `scene_fov_projection` from OpenXR-style
  angles, `SceneCardboard` (both eyes side by side through Cardboard's lens polynomial),
  `SceneReticle` (ViroReact's reticle: size, colors, fuse arc, burst and click pulse) and
  `SceneController` (Cardboard gaze and tap, or a pointer's pose) driving `SceneInput`.
  `docs/vr.md`, `tests/vr.esk`.
- Triangle mesh bodies: `scene_physics_triangles` and `scene_physics_geometry` for static and
  kinematic bodies (the real world's mesh and depth in AR), with sphere and box contacts and
  ray tests.
- Instancing: `Scene_set_instances` draws a node's mesh at many transforms in one draw call,
  with shadows, shader modifiers and culling over every instance.
- Semantic masks (ViroReact's): `scene_semantic_mask`, `scene_semantic_label`, the
  `SCENE_SEMANTIC_*` modes, and `Scene.semantic_texture`, `semantic_confidence_texture` and
  `semantic_transform`; modifier bodies see `viewport_size`.
- `<scene/input>`: `Scene_screen_ray`, `Scene_hit_test` and `Scene_hit_test_all` (mesh
  bounds tested in each node's own space, so rotated, scaled and nested nodes are exact),
  and `SceneInput`, which turns touches (`SceneInput_touch`) or any controller's pointer,
  button, gestures and gaze into click, drag, pinch, rotate, hover and fuse events on
  nodes. Events bubble to the nearest ancestor with a handler; thresholds, drag types and
  hover hysteresis follow ViroReact. `docs/input.md`.
- `SceneNode.ignore_events` keeps a node out of hit tests.
- `SceneNode.opacity` fades a node and its subtree: it multiplies down the tree, a node
  below 1 is drawn blended (whatever its material's alpha mode), one at 0 is not drawn.
- `SceneNode.rendering_order` sorts draws ahead of everything else, and
  `SceneMaterial.skip_depth_test` / `skip_depth_write` (ViroReact's `readsFromDepthBuffer`
  and `writesToDepthBuffer`) put a node on top; `Scene_hit_test` returns the hit drawn on
  top under the same rules, as ViroReact does.
- `SceneNode.billboard` (`SCENE_BILLBOARD_ALL`, `_X`, `_Y`, `_Z`) turns a node to face the
  camera with ViroReact's billboard rotation.
- Frustum culling: nodes wholly outside the camera's view are not drawn (they still cast
  shadows).
- `<scene/gif>`: a GIF decoder in Eskiu (LZW, color tables, interlacing, transparency,
  disposal, loop count) and `SceneAnimatedImage`, ViroAnimatedImage's playback into a texture.
  `Scene_borrow_texture` shows a texture the caller keeps, such as a video frame from
  `<gpu/yuv>` (ViroReact's video materials). `docs/images.md`.
- Spot light shadows, as in ViroReact: a perspective shadow map as wide as the outer
  cone, `SceneLight.shadow_near` and `shadow_far`.
- `SceneOrbit` (ViroOrbitCamera): a camera circling a target, turned by a drag and
  zoomed by a pinch.
- Post effects: `ScenePost.effects`, ViroReact's postProcessEffects (grayscale, sepia,
  sincity, barrel and pincushion distortion, thermal vision, crosshatch, pixelated,
  toonify, inverted, swirl and zoom distortion over `ScenePost.window` driven by
  `ScenePost.time`) with its formulas, chained in order inside the tone mapping pass;
  `scene_effect_named`.
- Portals: `SceneNode.portal`, `portal_entrance`, `passable`, `portal_sky`, `portal_ibl` and
  `Scene.active_portal` (ViroPortalScene and ViroPortal): a portal's world drawn only
  through its window with the stencil buffer, its own sky and lighting environment, walking in through a passable
  window and seeing out, portals inside portals (`SCENE_PORTAL_DEPTH` levels, ViroReact's
  portal tree). The scene's depth buffer now has a stencil
  (`SCENE_DEPTH_FORMAT`, Depth24PlusStencil8). `docs/portals.md`.
- `<scene/shapes>`: `scene_polygon` (ViroPolygon: holes, uv over the bounds) and
  `ScenePolyline` (ViroPolyline: camera-facing ribbons with round ends and joins).
  `docs/shapes.md`.
- `<scene/particles>`: `SceneParticleEmitter`, ViroParticleEmitter's model and timing:
  rates per second and per meter, bursts, point, box and sphere spawn volumes, color,
  alpha, scale, rotation, velocity and acceleration modifiers over time, distance or
  speed, explosions, camera-facing quads with additive blending. `docs/particles.md`.
- `Scene_set_geometry` writes in place when the new data fits the mesh's buffers.
- `<scene/text>`: `Scene_add_text` and `SceneText_set` draw TrueType text (eskiu-packages/font,
  now a dependency) from a glyph atlas, laid out as ViroText: points at 0.01 world units, a
  box, word, character or no wrapping, justification by ViroReact's Knuth-Plass formatter,
  alignment, clipping, a line cap,
  `scene_text_size`; ViroText's outer strokes (`SCENE_TEXT_STROKE_OUTLINE` and
  `_DROP_SHADOW`, `stroke_width`, `stroke_color`) and extrusion (`extrusion`: front, back and
  side meshes with their own materials). `docs/text.md`.
- 360 skies: `Scene_load_sky_360` and `Scene_set_sky_360` draw an equirectangular image
  behind the scene (ViroReact's `Viro360Image`), and `Scene.sky_rotation` turns any sky.
- Texture sampling per material: `SceneMaterial.wrap_s`, `wrap_t` (repeat, clamp, mirror),
  `filter` and `mip_filter` (linear or nearest), ViroReact's wrap and filter properties; a
  glTF material takes its base color texture's sampler.
- `Scene_set_geometry` and `Scene_set_texture` replace a mesh's or a texture's contents in
  place.
- Shader modifiers (ViroReact's `shaderModifiers`): `SceneShaderModifier` at the geometry,
  vertex, surface, lighting and fragment entry points with ViroReact's names (`_geometry`,
  `_transforms`, `_vertex`, `_surface`, `_light`, `_lightingContribution`, `_output_color`),
  in GLSL (translated by `<scene/glsl>`) or WGSL; `Scene_add_shader`,
  `SceneMaterial.shader`, per-material uniforms (`Scene_set_uniform_float` and the others,
  `Scene_set_uniform_texture`) and varyings; `scene_depth_texture` (the previous frame's
  depth), `camera_texture` and `camera_image_transform` (`Scene.camera_texture`,
  `Scene.camera_image_transform`); geometry modifiers apply to the shadow maps.
  `docs/modifiers.md`.
- `SceneMaterial.no_color_write` (ViroReact's `colorWriteMask` None): depth-only occluders.
- `SceneMaterial.chroma_key` and `chroma_key_color` (ViroReact's `chromaKeyFilteringColor`).
- AR occlusion: `Scene.occlusion_depth` and `occlusion_transform` hide virtual content
  behind real surfaces from the AR session's depth (ViroReact's DepthBased occlusion).
- AR rendering: `SceneMaterial.shadow_catcher` (ViroReact's `arShadowReceiver`), a surface
  that shows only the shadows it receives; `Scene.camera_background` shows the camera picture behind the scene as it
  is (the scene over it as premultiplied color), and `scene_projection` /
  `SCENE_CUSTOM` give a camera the session's projection matrix.
- `<scene/physics>`: `ScenePhysicsWorld`, ViroReact's physics bodies: static, kinematic and
  dynamic bodies with box, sphere, automatic and compound shapes, gravity, friction,
  restitution, forces, torques, impulses, constant velocities and sleeping, stepped in fixed
  1/60 s steps (at most 10 a frame); collision events sampled every 160 ms on new contacts,
  with the other body's tag, point and normal; ray and shape tests that notify the bodies.
- `<scene/animation>`: `SceneAnimations` animates position, scale, Euler rotation,
  opacity and material color in groups (duration, delay, easing) and serial or parallel
  chains, with ViroReact's easing curves, relative values and property strings, loops,
  pause, resume and stop. `docs/animation.md`.
- `Scene_camera_matrices` and `Scene_camera_is_ortho`: the view and projection the camera
  renders with.
- `Scene.msaa`: 4x multisampling of the scene's pass, its depth resolved for what reads it
  after the frame.
- Selective bloom: `SceneMaterial.bloom` and `bloom_threshold` bloom only the marked
  materials, each from its own threshold; `ScenePost.bloom_marked_only` (ViroReact's
  behavior) blooms nothing when none of them is in view, instead of the whole image.
- Headsets: `SceneDepthWriter` writes the scene's depth into a compositor's depth texture
  (reverse-Z or not) for reprojection, and `SceneIdWriter` the ids of the nodes under the
  gaze into visionOS's tracking areas texture. The `examples/xr` hosts run on Meta Quest
  (OpenXR, controllers, hands, passthrough) and Apple Vision Pro (CompositorServices).
- `SceneInput_listen`: one listener for every event of the kinds asked, besides the node
  handlers.
- `SCENE_EFFECT_FXAA` among the post effects.

### Changed

- Depends on gpu 0.7.0.
- The larger modules keep their parts in private `src/_module/` directories. No API change.

### Fixed

- A press that moves past the drag threshold is a drag, not a click as well: releasing a
  dragged body no longer pushes it as a click would.
- Explicit physics shapes (Box, Sphere) keep the size given, as in ViroReact, instead of
  being scaled again by the node's scale; shapes made from the node's meshes still take it.
- With no backdrop and no camera picture, the post pass keeps the scene's alpha, so a
  headset's passthrough shows behind the scene.

## [0.1.0]

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
