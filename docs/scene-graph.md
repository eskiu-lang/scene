# The scene graph

Nodes, cameras, meshes and glTF models in `<scene>` and `<scene/geometry>`. The short
example is in the [README](../README.md).

API reference: [`<scene>`](api.md#scene), [`<scene/geometry>`](api.md#scenegeometry).

## Ids

A `Scene` keeps its objects in lists and hands out ids:

| Object | Made by | Ids | 0 means |
|---|---|---|---|
| Node | `add_node`, `add_object`, `add_camera`, `add_gltf` | from 1 | the root node |
| Mesh | `add_geometry` | from 1 | no mesh (a node that draws nothing) |
| Material | `add_material` | from 1 | the default material (white Blinn) |
| Texture | `add_texture`, `image_texture`, `load_texture` | from 1 | no texture |
| Light | `add_light` | from 0 | the first light |

`scene.node(id)`, `scene.material(id)` and `scene.light(id)` return pointers for changing
an object in place. A pointer is good until the next object of its kind is added (the list
may move), so take it again after adding. Changes show at the next render.

## Nodes

```eskiu
int arm = scene.add_node(0, "arm");                 // under the root
int hand = scene.add_object(arm, mesh, material);   // a node that draws a mesh
scene.node(arm).rotation = quat_axis_angle(Vec3{0.0, 0.0, 1.0}, 0.5);
scene.node(hand).position = Vec3{1.0, 0.0, 0.0};    // in the arm's space
scene.node(hand).scale = Vec3{0.5, 0.5, 0.5};
```

A node's local matrix is `translation * rotation * scale`, and its world matrix is its
parent's world matrix times its local matrix. `Scene_update` computes every world matrix
into `SceneNode.world` (the renderer calls it first); `Scene_world(id)` computes one now by
walking up its parents.

| Field | Default | Meaning |
|---|---|---|
| `position`, `rotation`, `scale` | origin, identity, 1 | the local transform |
| `visible` | true | false hides the node and its subtree |
| `mesh`, `material` | 0, 0 | what the node draws |
| `camera` | -1 | the camera it holds |
| `casts_shadows`, `receives_shadows` | true, true | its part in shadows |
| `light_mask` | 1 | the lights that reach it (a light reaches it when `light.influence & light_mask` is not 0) |
| `name` | | a `String`; `Scene_find(name)` returns the first live node with it |

`Scene_set_parent(node, parent)` moves a subtree (it refuses to make a cycle), and
`Scene_remove(node)` removes a subtree; removed ids are not reused, and their `alive` field
is false. `Scene_look_at(node, target, up)` turns a node so its -Z axis points at a world
point, whatever its parent's rotation; cameras, spot lights and directional lights on nodes
are aimed with it.

## Cameras

A camera sits at its node's origin and looks down the node's -Z axis with +Y up.

```eskiu
int cam = scene.add_camera(0, scene_perspective(0.9, 0.1, 100.0));   // fov in radians, near, far
scene.node(cam).position = Vec3{0.0, 2.0, 6.0};
scene.look_at(cam, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0});
scene.set_camera(cam);
```

`scene_orthographic(height, near, far)` sees `height` world units from the bottom of the
image to the top. `SceneCamera.aspect` 0 takes the target's width over height. With no
camera set, the scene renders from the origin down -Z with a perspective of 1 radian.

## Meshes and shapes

A `SceneGeometry` is a mesh on the CPU: an interleaved vertex array (position, normal,
texture coordinate, tangent, color: `SCENE_VERTEX_FLOATS` floats per vertex) and uint32
indices, counterclockwise front faces. `Scene_add_geometry` uploads it and returns the
mesh id; the geometry can be freed afterwards.

| Shape | Size | Layout |
|---|---|---|
| `scene_box(w, h, d)` | centered on the origin | 24 vertices, each face mapped to the whole texture |
| `scene_sphere(r, segments, rings)` | centered on the origin | u around the Y axis, v from the top (0) to the bottom (1) |
| `scene_plane(w, d)` | XZ plane, facing +Y | a ground |
| `scene_quad(w, h)` | XY plane, facing +Z | an image or a billboard |

For your own meshes, `scene_geometry(vertices, indices)` allocates one (white, normals +Y);
fill it with `set` (position, normal, uv) and the indices, then call `normals()` (averaged
face normals), `tangents()` (from the texture coordinates, for normal maps), `color()` and
`bounds()`.

## glTF and OBJ models

```eskiu
Gltf g = model_load("helmet.glb")?;        // <gpu/model> reads glTF and OBJ
defer g.free();
SceneGltf helmet = scene.add_gltf(&g, 0)?;
defer helmet.free();
scene.node(helmet.root).position = Vec3{0.0, 1.0, 0.0};
```

`Scene_add_gltf` adds the model's images as textures (sRGB for base color and emission,
linear for the others), its materials as `SCENE_PBR` materials (`SCENE_CONSTANT` for
`KHR_materials_unlit`), its triangle primitives as meshes, and its node tree under a new
node, `SceneGltf.root`. Each primitive becomes a child node of its glTF node, so a node with
three primitives has three children. `SceneGltf.nodes[i]` is the scene node of glTF node
`i`.

To play an animation, keep the `Gltf` and copy its pose each frame:

```eskiu
g.animate(0, seconds);
helmet.pose(&scene, &g);
```

Skins and morph targets are not applied (the mesh is drawn in its bind pose); use
`<gpu/model>` for those for now. Only the first texture coordinate set is read, and the
glTF samplers are replaced by the scene's sampler (linear, mipmapped, repeating).
