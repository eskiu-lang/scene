# Portals

A portal is a world of its own inside the scene, seen only through a window, as
ViroReact's `ViroPortalScene` and `ViroPortal` are: a door into another room, a picture
frame showing another place, a window onto a 360 image.

```c
int room = scene.add_node(0, "room");
scene.node(room).portal = true;            // ViroPortalScene
scene.node(room).passable = true;          // the camera can walk in
int door = scene.add_object(room, window_mesh, clear_material);
scene.node(door).portal_entrance = true;   // ViroPortal: its meshes are the window
scene.add_object(room, sofa_mesh, sofa_material);   // inside the room
scene.node(room).portal_sky = scene.add_texture(panorama)?;
scene.node(room).portal_sky_pano = true;
```

A node with `portal` set makes its subtree a world: its contents are drawn only where its
window shows, never around it. The window is every mesh of the portal's children marked
`portal_entrance` (and of their descendants), drawn in the world around the portal. A
window mesh is drawn like any other, so give its see-through part a material with alpha 0
(blended) and keep the frame around it opaque; the frame is drawn again over the portal's
world, so it stays in front of what is behind the window.

Each frame the scene draws:

1. The world the camera is in, with the windows of its portals, and its sky.
2. For each world next to it: its window into the stencil buffer where it is visible (an
   object in front of the window still covers it), raising the stencil one level there; the
   depth cleared there; then that world's sky and contents only there, its window again on
   top, and its own neighbors one level deeper.

`portal_sky` is a texture id from `Scene_add_texture`: a cube map, or a 360 image with
`portal_sky_pano`. A portal without a sky shows, where its contents leave room, what was
drawn there before (in AR, the camera image).

## Lighting environment

`portal_ibl` lights the PBR materials of a portal's world with a `GpuIbl` from `<gpu/ibl>`,
as ViroReact's lighting environment per portal; `Scene_set_ibl` is the outer world's. Each
world uses only its own: a portal without one lights its PBR materials with the ambient
lights, whatever the scene's environment.

```eskiu
GpuIbl room_light = gpu.ibl(&room_hdr, &opts)?;
scene.node(room).portal_ibl = &room_light;
```

The `GpuIbl` is not owned by the scene and must outlive its use.

## Walking through

When `passable` is set and the camera moves through the bounds of a window between two
frames, it enters the portal: `Scene.active_portal` becomes the portal node, the portal's
world is drawn around the camera, and the world it came from is seen through the window
(the same window, from the other side). Moving back through it returns to the outer world
(`active_portal` 0). Set `active_portal` to put the camera in a portal directly. A portal
that is not passable is only looked into.

Portals nest, and the scene walks them as ViroReact walks its portal tree: from the world
the camera is in, each neighboring world (a portal of it, or the world around it when it is
a portal) is drawn through its window, and that world's own neighbors through their windows
inside it, `SCENE_PORTAL_DEPTH` (4) levels deep. A world seen through two windows shows only
where both are open. Walking through a passable window moves the camera one level, in or
out. Lights reach every world.
