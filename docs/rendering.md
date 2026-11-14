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
   `Scene.background`: by `SceneNode.rendering_order` first (lower first), then opaque and
   masked nodes front to back (fewer shaded pixels), the skybox, then blended and faded
   nodes back to front. A node whose bounds are wholly outside the camera's view is not
   drawn in this pass (it still casts shadows). Then each [portal](portals.md) seen
   from there: its window into the stencil, and its world drawn only where the stencil
   says. The depth buffer is `SCENE_DEPTH_FORMAT` (Depth24PlusStencil8).
4. Bloom, when on: a bright pass into a half-size image and `bloom_passes` pairs of
   horizontal and vertical Gaussian blurs.
5. The final pass into `view`: the HDR image plus the bloom, tone mapped, sRGB-encoded when
   the format is not sRGB and `post.gamma` is on.

`Scene_render_in(&encoder, ...)` records the same passes into your encoder, so passes
recorded after it (an interface drawn with `GpuPassOptions{load: true}`) go over the
result in one submission. `Scene_render_to(&texture)` renders into a texture, to read it
back (`GpuTexture_read`) or sample it. The scene's internal images follow the target's
size and are remade when it changes.

## Instancing

`Scene_set_instances(node, transforms, count)` draws a node's mesh once per transform, each
under the node's own, in one draw call (ViroReact's instancing): a forest, a crowd, a field of
markers for the cost of one object. The transforms are copied; an empty list draws the mesh
once again. The instances cast shadows, take shader modifiers (`_transforms.model_matrix` is
the instance's) and are culled together, by a sphere around all of them. They should be rigid
with a uniform scale, since their normals turn with them; hit tests see the node's own mesh.

## Tone mapping and gamma

`Scene.post` is a `ScenePost`. With `c = color * 2^exposure`:

| `tone_mapping` | Result |
|---|---|
| `SCENE_TONE_NONE` | `c` (an RGBA8 target clamps it) |
| `SCENE_TONE_REINHARD` | `c / (1 + c)` |
| `SCENE_TONE_EXPOSURE` | `1 - exp(-c)` |
| `SCENE_TONE_HABLE` | `hable(c) / hable(white_point)`, the Uncharted 2 filmic curve, `white_point` 11.2 by default |
| `SCENE_TONE_HABLE_LUMINANCE` | ViroReact's HableLuminanceOnly: the curve on the luminance, the color scaled by the same factor (hues keep their saturation), clamped to 1 |

Each but the last works on red, green and blue separately. `scene_tone_map` and `scene_srgb` compute the
same on the CPU. `exposure` is in stops for every operator, so its zero value keeps the
brightness. With `gamma` off and an RGBA16Float or RGBA32Float target, the output is linear,
which is how the tests read exact values.

`gamma22` encodes with a power of 1/2.2 instead of the sRGB curve, as ViroReact's software
gamma pass does. `backdrop`, when its alpha is above 0, is a color shown as it is where the
scene leaves the picture uncovered (give the scene a `background` of alpha 0): ViroReact's
clear color, which skips its tone mapping and gamma.

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

## Effects

`scene.post.effects` lists up to eight post-process effects, applied in order to the image
as it is displayed (tone mapped, and sRGB-encoded when `gamma` is on), with ViroReact's
`postProcessEffects` formulas:

| Constant | ViroReact | What it does |
|---|---|---|
| `SCENE_EFFECT_GRAYSCALE` | `grayscale` | luminance (0.2126, 0.7152, 0.0722) |
| `SCENE_EFFECT_SEPIA` | `sepia` | the sepia matrix at strength 0.9 |
| `SCENE_EFFECT_SINCITY` | `sincity` | gray, keeping strongly red parts |
| `SCENE_EFFECT_BARREL` | `baralleldistortion` | the image bulges from the center (radius to the power 1.75) |
| `SCENE_EFFECT_PINCUSHION` | `pincushiondistortion` | the image pinches toward the center (power 0.5) |
| `SCENE_EFFECT_THERMAL` | `thermalvision` | brightness mapped blue, yellow, red |
| `SCENE_EFFECT_CROSSHATCH` | `crosshatch` | black diagonal lines on white, denser in darker parts |
| `SCENE_EFFECT_PIXELATED` | `pixelated` | blocks of 2% of the image |
| `SCENE_EFFECT_TOONIFY` | `toonify` | black where the brightness changes sharply |
| `SCENE_EFFECT_INVERTED` | `inverted` | one minus the color |
| `SCENE_EFFECT_SWIRL` | `swirldistortion` | a swirl in a circle, turning back and forth with `post.time` |
| `SCENE_EFFECT_ZOOM` | `zoomindistortion` | a circle magnified twice |

```eskiu
scene.post.effects[0] = SCENE_EFFECT_SEPIA;
scene.post.effects[1] = scene_effect_named("pixelated");
```

`scene_effect_named` maps ViroReact's names. The effects are composed into the tone
mapping pass, one shader per list, so they add no render pass.

The swirl and the zoom work in a circle centered in `post.window` (left, bottom, right, top,
from 0 to 1 with the origin at the bottom left; the whole image when all are 0), its radius
half the window's smaller side, as ViroReact's window mask. The swirl's angle is
`sin(2 * post.time)`: set `post.time` to the time in seconds each frame, as ViroReact reads
its clock.

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

A 360 image works the same way (ViroReact's `Viro360Image`): `Scene_load_sky_360(path)`
loads an equirectangular PNG or JPEG the scene keeps, and `Scene_set_sky_360(&texture)`
takes any 2D texture, an HDR one included. Its center is straight ahead down -Z, its left
and right edges meet behind, and its top row is straight up. `Scene.sky_rotation` turns
any sky, cube or 360, in the world (ViroReact's `rotation` on `Viro360Image` and
`ViroSkyBox`).

`Scene_set_ibl` lights `SCENE_PBR` materials with the environment's irradiance, its
prefiltered specular cube and the BRDF table, times `ibl.intensity`, in place of the
ambient lights (see [materials](materials.md#lighting-models)). The other models keep the
ambient lights. Both pointers must outlive their use; the scene does not release them.

## AR: the camera picture

An AR app draws the camera picture behind the scene: `Scene.camera_background` is a texture
id holding the picture already fitted to the view (eskiu-lang/ar's frame drawn with
`<gpu/yuv>` into a texture of the view's size, then `Scene_borrow_texture`), with
`Scene.background` at alpha 0. The final pass puts the scene over it as premultiplied color,
after tone mapping and effects, so the picture stays as the camera shot it: opaque surfaces
cover it, blended ones mix with it and additive ones (a hologram, sparks) glow on it.

The camera then takes the session's matrices: its node's transform is the device's pose and
`scene_projection(matrix, near, far)` gives it the session's projection (the intrinsics and
the crop the picture was fitted with).

```eskiu
scene.camera_background = scene.borrow_texture(scene.camera_background, &camera_picture);
scene.background = Vec4{0.0, 0.0, 0.0, 0.0};
*scene.camera(cam) = scene_projection(*(*Mat4)&frame.projection(w, h, 0.02, 100.0), 0.02, 100.0);
```

## AR: occlusion by the real world

`Scene.occlusion_depth` (a texture id) hides virtual content behind real surfaces, as
ViroReact's `occlusionMode` `DepthBased` does. It holds the depth of the real world in meters
in its red channel (`R32Float` or `R16Float`, 0 where unknown), the AR session's depth image
uploaded as it comes, and `occlusion_transform` maps the view's coordinates (x and y from 0
to 1 from the top left) to the image's: eskiu-lang/ar's `ArFrame_display_transform`. Before
the scene is drawn, a full-screen pass writes that depth, converted with the camera's
projection, into the depth buffer, so a virtual surface farther than the real one at a pixel
fails the depth test there and the camera picture shows instead. Nothing in the materials
changes; a texel of 0 occludes nothing.

## Formats

The scene renders into any color target: a surface frame (BGRA8Unorm or its sRGB twin), an
RGBA8 texture, RGBA16Float or RGBA32Float. A tone mapping pipeline is made per output format
the first time it is used.
