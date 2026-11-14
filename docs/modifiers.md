# Shader modifiers

A shader modifier inserts code into a material's shaders at one of ViroReact's entry points,
with the same names ViroReact gives the values there. Modifiers are written in GLSL, as
ViroReact's `shaderModifiers`, and translated to WGSL, or written in WGSL directly.

```eskiu
List<SceneShaderModifier> mods;
mods.push(scene_modifier(SCENE_MODIFIER_GEOMETRY, "uniform float time;",
    "_geometry.position += _geometry.normal * sin(time * 4.0 + _geometry.position.y * 8.0) * 0.02;"));
mods.push(scene_modifier(SCENE_MODIFIER_SURFACE, "uniform highp vec3 tint;",
    "_surface.diffuse_color.rgb *= tint;"));
SceneMaterial m = scene_material(SCENE_BLINN);
m.shader = scene.add_shader(&mods)?;
int wobbly = scene.add_material(m);
...each frame:
scene.set_uniform_float(wobbly, "time", (float)now);
scene.set_uniform_vec3(wobbly, "tint", Vec3{1.0, 0.6, 0.2});
```

`Scene_add_shader` gathers the modifiers into a program and returns its id for
`SceneMaterial.shader`; materials with the same program share its pipelines. Each material
keeps its own uniform values (ViroReact's `materialUniforms`), set at any time with
`Scene_set_uniform_float`, `_vec2`, `_vec3`, `_vec4`, `_mat4`, `Scene_set_uniform` (floats,
matrices by column) and `Scene_set_uniform_texture` (ViroReact's `updateShaderUniform`).

## Entry points

| Entry point | Runs | Reads and writes |
|---|---|---|
| `SCENE_MODIFIER_GEOMETRY` | in the vertex stage, before the transforms | `_geometry` (`position`, `normal`, `texcoord`, `tangent`, `bone_weights`, `bone_indices`, in the node's space, after morphing and skinning), `_transforms` (`model_matrix`, `view_matrix`, `projection_matrix`) |
| `SCENE_MODIFIER_VERTEX` | after them | `_vertex.position` (clip space) |
| `SCENE_MODIFIER_SURFACE` | in the fragment stage, before lighting | `_surface` (`diffuse_color`, `diffuse_texcoord`, `diffuse_intensity`, `shininess`, `specular_color`, `specular_texcoord`, `roughness`, `roughness_intensity`, `metalness`, `metalness_intensity`, `ao`, `alpha`, `normal`, `position`, `view`) |
| `SCENE_MODIFIER_LIGHTING` | once per light, after the material's lighting model | `_lightingContribution` (`ambient`, `diffuse`, `specular`, `visibility`), from `_light` (`type`, `attenuation_start_distance`, `attenuation_end_distance`, `attenuation_falloff_exp`, `position`, `direction`, `color`, `intensity`, `spot_inner_angle`, `spot_outer_angle`) and `_surface` |
| `SCENE_MODIFIER_FRAGMENT` | last | `_output_color` |

As in ViroReact, the surface's `diffuse_color` already holds the diffuse texture and the
vertex color, the lighting model fills `_lightingContribution` for each light before the
lighting modifiers change it, and the contributions are summed as `ambient` plus `diffuse`
and `specular` times `visibility` (the light's shadow). `_light.type` is 1 for a directional
light, 2 for a point light and 3 for a spot. A modifier body also sees `camera_position`,
`v_texcoord`, `v_surface_position` and `gl_FragCoord` in the fragment stage, and
`camera_position` in the vertex stage.

## Uniforms, textures and varyings

A modifier's uniforms are declared in its `uniforms` text: `uniform float time;`,
`uniform highp vec3 tint;`, `uniform mat4 m;`, `uniform sampler2D noise;` (one per line or
`;`-separated, the precision and `uniform` optional). In a WGSL modifier they are
`time: f32;`, `tint: vec3<f32>;`, `noise: texture_2d<f32>;`. The types are ViroReact's:
float, vec2, vec3, vec4, mat3, mat4 and 2D textures. A body reads a uniform by its name and
samples a texture with `texture(noise, uv)` (WGSL: `textureSample(noise, mod_samp, uv)`).

`varyings` declares values the vertex stage passes to the fragment stage (`highp float
lift`), written in a geometry or vertex body and read in a surface or fragment body:

```eskiu
SceneShaderModifier g = scene_modifier(SCENE_MODIFIER_GEOMETRY, "", "lift = _geometry.position.y;");
g.varyings = "highp float lift";
SceneShaderModifier s = scene_modifier(SCENE_MODIFIER_SURFACE, "", "_surface.diffuse_color.rgb *= lift;");
s.varyings = "highp float lift";
```

## GLSL in WGSL

`<scene/glsl>` translates the GLSL ES 3.0 statements a modifier holds: declarations
(`const` and precisions dropped), assignments, `if`, `else`, `for`, `while`, `break`,
`continue`, `discard` and `return`, with GLSL's types, constructors and built-ins. Where the
two languages differ it writes what GLSL means:

- a scalar meeting a vector in `clamp`, `min`, `max`, `step`, `smoothstep`, `pow`, `mod` or
  `%` is splatted to the vector;
- `mod(a, b)` is GLSL's floored modulo, `a - b * floor(a / b)`;
- `c ? a : b` is `select(b, a, c)`;
- an assignment to a swizzle of several components (`v.rgb *= k`) is made one component at a
  time, the value evaluated once;
- the `stpq` swizzle letters are `xyzw`;
- `texture`, `texture2D` and `textureLod` sample through the modifier's sampler;
  `atan(y, x)`, `inversesqrt`, `dFdx` and `dFdy` get their WGSL names.

A body the translator cannot read makes `Scene_add_shader` fail with the place it stopped;
WGSL it produces that the GPU rejects fails when a material with the program is first drawn,
with the GPU's message.

## Differences from ViroReact

- Geometry and vertex modifiers do not move shadows: the shadow pass draws the meshes as they
  are.
- The constant lighting model has no lights, so its lighting modifiers do not run.
- Varyings are not visible in lighting bodies.
- `gl_FragCoord` counts rows from the top of the target.
- ViroReact's `requiresSceneDepth` and `requiresCameraTexture` (the previous frame's depth
  and the AR camera image as samplers) are not provided; image post-processing modifiers are
  the effects of `ScenePost` (see [rendering](rendering.md)).
