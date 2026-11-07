# Rendering, post-processing and the sky

How a frame is drawn, the HDR pipeline (`<scene/post>`) and the background (`<scene/sky>`).

API reference: [`<scene>`](api.md#scene), [`<scene/post>`](api.md#scenepost),
[`<scene/sky>`](api.md#scenesky).

## A frame

```eskiu
Result<GpuFrame, string> fr = surface.frame();
GpuFrame frame = fr.value;
scene.render(frame.view, surface.width, surface.height, surface.format)?;
surface.present(&frame);
```

`Scene_render(view, width, height, format)` records and submits:

1. The world matrices of every node, the camera, the lights.
2. One depth-only pass per shadow-casting light, into its layer of the shadow map array.
3. The scene pass, into an RGBA16Float image with a Depth32Float depth buffer, cleared to
   `Scene.background`: opaque and masked nodes sorted front to back (fewer shaded pixels),
   the skybox, then blended nodes back to front.
4. Bloom, when on: a bright pass into a half-size image and `bloom_passes` pairs of
   horizontal and vertical Gaussian blurs.
5. The final pass into `view`: the HDR image plus the bloom, tone mapped, sRGB-encoded when
   the format is not sRGB and `post.gamma` is on.

`Scene_render_in(&encoder, ...)` records the same passes into your encoder, so passes
recorded after it (an interface drawn with `GpuPassOptions{load: true}`) go over the
result in one submission. `Scene_render_to(&texture)` renders into a texture, to read it
back (`GpuTexture_read`) or sample it. The scene's internal images follow the target's
size and are remade when it changes.

## Tone mapping and gamma

`Scene.post` is a `ScenePost`. With `c = color * 2^exposure`:

| `tone_mapping` | Result |
|---|---|
| `SCENE_TONE_NONE` | `c` (an RGBA8 target clamps it) |
| `SCENE_TONE_REINHARD` | `c / (1 + c)` |
| `SCENE_TONE_EXPOSURE` | `1 - exp(-c)` |
| `SCENE_TONE_HABLE` | `hable(c) / hable(white_point)`, the Uncharted 2 filmic curve, `white_point` 11.2 by default |

Each works on red, green and blue separately. `scene_tone_map` and `scene_srgb` compute the
same on the CPU. `exposure` is in stops for every operator, so its zero value keeps the
brightness. With `gamma` off and an RGBA16Float or RGBA32Float target, the output is linear,
which is how the tests read exact values.

## Bloom

```eskiu
scene.post.bloom = true;
scene.post.bloom_threshold = 1.0;   // luminance where the glow starts
scene.post.bloom_strength = 1.0;
scene.post.bloom_passes = 4;        // more spreads wider
```

The bright pass keeps `color * max(luminance - threshold, 0) / luminance` at half size, the
blur is a 9-tap Gaussian (separable, with a step of one half-size texel), and the result is
added to the HDR image before tone mapping, times `bloom_strength`. Emissive materials and
lights brighter than 1 are what glow.

## Background, skybox and image-based lighting

`Scene.background` (linear RGBA) clears the image. A cube map drawn behind the scene
replaces it:

```eskiu
HdrBitmap hdr = hdr_load("sky.hdr")?;
GpuIbl ibl = gpu.ibl(&hdr, null)?;     // <gpu/ibl>
hdr.free();
scene.set_skybox(&ibl.environment);   // radiance by world direction
scene.set_ibl(&ibl);                  // PBR materials take their ambient light from it
scene.sky_intensity = 1.0;
```

`Scene_set_skybox` takes any cube texture sampled by world direction: a camera looking down
-Z sees its -Z face. `Scene_set_skybox_faces(&faces[0])` takes six square `Bitmap`s in the
order +X, -X, +Y, -Y, +Z, -Z, the usual skybox image sets (OpenGL's convention, where the
+Z image is the one in front of a camera looking down -Z), and the scene owns the cube it
makes. `scene_cube_faces` makes that cube on its own.

`Scene_set_ibl` lights `SCENE_PBR` materials with the environment's irradiance, its
prefiltered specular cube and the BRDF table, times `ibl.intensity`, in place of the
ambient lights (see [materials](materials.md#lighting-models)). The other models keep the
ambient lights. Both pointers must outlive their use; the scene does not release them.

## Formats

The scene renders into any color target: a surface frame (BGRA8Unorm or its sRGB twin), an
RGBA8 texture, RGBA16Float or RGBA32Float. A tone mapping pipeline is made per output format
the first time it is used.
