# Animated images and video

## Animated GIFs

`<scene/gif>` decodes GIFs in Eskiu and plays them into a texture, ViroReact's
`ViroAnimatedImage`.

```c
Gif g = gif_load("loading.gif")?;
SceneAnimatedImage spin = scene_animated_image(&scene, g)?;   // takes the Gif over
SceneMaterial m = scene_material(SCENE_CONSTANT);
m.diffuse_texture = spin.texture;
m.alpha_mode = SCENE_ALPHA_BLEND;
...each frame: spin.update(&scene, now)?;
```

`gif_decode` reads GIF87a and GIF89a: LZW data, global and local color tables, interlaced
images, transparency, the three disposal methods (keep, clear the image's area, restore what
was before it) and the NETSCAPE loop count (`Gif.loops`). Each `GifFrame` is the whole canvas
in RGBA, already composited, with its delay in seconds; delays of 0 and 1 hundredths show
for 0.1 s, as browsers do.

A `SceneAnimatedImage` owns an sRGB texture the size of the canvas and uploads a frame only
when the time crosses into it. `loop` (true) plays it again from the start; turned off it
stops on the last frame and `update` returns true. `paused` holds the frame showing when it
was set.

## Video and other changing pictures

`Scene_borrow_texture(id, &texture)` shows a texture that stays the caller's: the scene never
releases it, and every material with that texture id shows it. This is how a video plays on
a surface (ViroReact's video materials and `ViroVideo`): eskiu-lang/video hands decoded
frames to Eskiu, `<gpu/yuv>` turns each into an RGBA texture, and the scene shows it.

```c
GpuYuv yuv = gpu.yuv(WGPUTextureFormat_RGBA8Unorm)?;
int screen_tex = 0;
use_video_frames(player, VideoFrameOptions{max_width: 1280, max_height: 720}, void(*VideoFrame f) {
    yuv.upload((*GpuYuvImage)f)?;
    *GpuTexture rgba = yuv.to_rgba(true)?;
    screen_tex = scene.borrow_texture(screen_tex, rgba);   // the same id while the size holds
});
...the material: m.diffuse_texture = screen_tex;
```

Call `Scene_borrow_texture` again with the id whenever the texture is made anew (another
size gives another handle); writing new pixels into the same texture needs nothing more.

A 360 video (`Viro360Video`) is the same texture shown as the sky:
`Scene_set_sky_360(rgba)`, which takes the caller's texture too.
