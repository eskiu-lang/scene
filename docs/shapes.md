# Polygons and polylines

`<scene/shapes>` has ViroReact's two shapes made from points.

## Polygons

```c
List<Vec2> outline;    // the points, in either winding
List<List<Vec2>> holes;
SceneGeometry g = scene_polygon(&outline, &holes, Vec4{0.0, 0.0, 1.0, 1.0});
int mesh = scene.add_geometry(&g)?;
g.free();
```

`scene_polygon(path, holes, uv)` is `ViroPolygon`: the flat shape inside `path`, in the XY
plane facing +Z, minus each hole (`holes` may be null). Repeated points are dropped, as
ViroReact does. The texture coordinates span the shape's bounds: (`uv.x`, `uv.y`) at the top
left (smallest x, largest y) and (`uv.z`, `uv.w`) at the bottom right, ViroReact's
`uvCoordinates`. The shape is triangulated by ear clipping, holes joined to the outline by
bridges; ViroReact uses a constrained Delaunay triangulation, which gives other triangles
over the same area, so the drawn pixels and texture coordinates match.

## Polylines

```c
ScenePolyline line = scene_polyline(&scene, parent, 0.02)?;   // 2 cm wide
line.add_path(&points);
line.append(Vec3{0.3, 0.1, 0.0});
...each frame: line.update(&scene)?;
```

A `ScenePolyline` is `ViroPolyline`: each path a ribbon `thickness` world units wide that
turns to face the camera, with a round cap at each end and, with `SCENE_JOIN_ROUND` (the
default), a round joint at each inner point (`SCENE_JOIN_NONE` leaves the corners open).
Points are in the node's space; the thickness stays in world units whatever the node's
scale. `ScenePolyline_update` rebuilds the ribbons toward the scene's camera, so call it when
the camera, the node or the paths change. The material is unlit white and double-sided.
