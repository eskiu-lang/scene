# Materials

`<scene/material>`: lighting models, textures, alpha and blending.

API reference: [`<scene/material>`](api.md#scenematerial).

```eskiu
SceneMaterial m = scene_material(SCENE_PBR);
m.diffuse = Vec4{0.9, 0.2, 0.1, 1.0};       // linear RGBA
m.diffuse_texture = scene.load_texture("albedo.png", true)?;     // sRGB
m.normal_texture = scene.load_texture("normal.png", false)?;     // linear
m.roughness = 0.4;
int id = scene.add_material(m);
```

Start from `scene_material(lighting)`, which fills in the defaults, and set what differs.
Colors are linear (not sRGB) and may go above 1: a surface brighter than 1 is what bloom
picks up. Texture fields hold texture ids (0 is none); color textures (diffuse, specular,
emissive) belong in an sRGB format and data textures (normal, metal-roughness, occlusion)
in a linear one, as `Scene_load_texture`'s `srgb` argument chooses.

## Lighting models

Below, `base` is `diffuse * diffuse_texture * vertex color`, `n` the surface normal (after
the normal map), `v` the direction to the eye, and for each light `l` the direction to it,
`radiance` its `color * intensity` times its attenuation, cone and shadow, and `ambient`
the sum of the scene's ambient lights.

| Model | Color |
|---|---|
| `SCENE_CONSTANT` | `base` (no lighting) |
| `SCENE_LAMBERT` | `ambient * base + sum(radiance * max(n.l, 0)) * base * diffuse_intensity` |
| `SCENE_PHONG` | Lambert plus `sum(radiance * max(v.reflect(-l, n), 0)^shininess) * specular` |
| `SCENE_BLINN` | Lambert plus `sum(radiance * max(n.normalize(v + l), 0)^shininess) * specular` |
| `SCENE_PBR` | glTF's metallic-roughness model, below |

`specular` is the material's `specular` color times `specular_texture`. A highlight is only
added where `n.l > 0`. Every model then adds `emissive * emissive_texture`.

`SCENE_PBR` uses `metal = metalness * metal_rough_texture.b` and
`rough = roughness * metal_rough_texture.g` (clamped to 0.04 at least), `F0 = mix(0.04,
base, metal)`, GGX for the distribution, height-correlated Smith visibility and
Fresnel-Schlick:

```
direct  = sum((kd * base / pi + F * D * Vis) * radiance * n.l * pi),  kd = (1 - F) * (1 - metal)
ambient = ambient * base * ao                       without an environment
ambient = (irradiance(n) * base * (1 - metal) * (1 - Fr)
           + specular(reflect(-v, n), rough) * (F0 * brdf.r + brdf.g)) * ao * ibl.intensity
                                                    with Scene_set_ibl
```

The `pi` factor on punctual lights makes a white light of intensity 1 straight onto a white
surface give 1 in every lit model (`kd` is just below 1 for a dielectric). `ao` is
`1 + occlusion_strength * (occlusion_texture.r - 1)`.

The normal map is a tangent-space map (`normal_scale` scales its x and y). Meshes with
tangents (the shapes, glTF files with `TANGENT` or texture coordinates) use them; others get
a frame from screen-space derivatives.

## Alpha and blending

`alpha_mode` is `SCENE_ALPHA_OPAQUE` (alpha ignored), `SCENE_ALPHA_MASK` (fragments with
`base.a < alpha_cutoff` are discarded; they cast shadows as if opaque) or
`SCENE_ALPHA_BLEND`. A blended material is drawn after every opaque one, sorted back to
front by the view depth of its node's bounds, tests depth without writing it, and casts no
shadow. Its `blend_mode`, with `s` the surface color, `a` its alpha and `d` the color behind:

| Mode | Result |
|---|---|
| `SCENE_BLEND_ALPHA` | `s * a + d * (1 - a)` |
| `SCENE_BLEND_ADD` | `d + s * a` |
| `SCENE_BLEND_MULTIPLY` | `d * mix(1, s, a)` |
| `SCENE_BLEND_SUBTRACT` | `d - s * a` |
| `SCENE_BLEND_SCREEN` | `d + s * a * (1 - d)` (`s` clamped to 0..1) |
| `SCENE_BLEND_PREMULTIPLIED` | `s + d * (1 - a)`, for colors already multiplied by alpha |

Blending happens in the linear HDR image, before tone mapping. `double_sided` turns off
back-face culling and lights back faces with the flipped normal.

## Shader variants

Each material compiles to a variant: lighting model, alpha mode, blend mode (for blended
materials) and sidedness, packed by `scene_material_key`. The renderer composes one WGSL
shader per variant (`scene_material_shader`) the first time it is drawn and keeps its
pipeline; every material with the same key shares it, and textures and factors are
uniforms and bindings, not variants. `Scene_pipeline_count` says how many were made.
