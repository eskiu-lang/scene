#!/bin/sh
# Gathers the showcase's files into DEST: what xr_scene.esk shows (xr_scene_set_assets names
# the directory) and the video the hosts play. They come from ReactVision's showcase app
# (SHOWCASE, its assets/ directory) and eskiu-lang/viro (VIRO, Roboto from its web build),
# so they are not part of this repository.
#
#     examples/xr/assets.sh DEST
set -eu
dest=${1:?usage: assets.sh DEST}
here=$(cd "$(dirname "$0")" && pwd)
SHOWCASE=${SHOWCASE:-$HOME/Documents/ViroWorkspace/showcase/assets}
VIRO=${VIRO:-$here/../../../../eskiu-projects/viro}
mkdir -p "$dest"
cp "$VIRO/web/fonts/Roboto.ttf" "$dest/Roboto.ttf"
cp "$SHOWCASE/360/360_guadalupe.jpg" "$dest/photo.jpg"
cp "$SHOWCASE/0001.jpg" "$dest/normal.jpg"
cp "$SHOWCASE/images/pikachu.png" "$dest/pikachu.png"
cp "$SHOWCASE/images/particle_fire.png" "$dest/fire.png"
cp "$SHOWCASE/particles/circle_05.png" "$dest/spark.png"
cp "$SHOWCASE/360/360_westlake.jpg" "$dest/sky360.jpg"
cp "$SHOWCASE/video.mp4" "$dest/video.mp4"
cp "$SHOWCASE/models/khronos/Fox.glb" "$dest/Fox.glb"
cp "$SHOWCASE/models/RobotExpressive.glb" "$dest/RobotExpressive.glb"
cp "$SHOWCASE/object_bday_cake/object_bday_cake_pbr.vrx" "$dest/cake.vrx"
cp "$SHOWCASE"/object_bday_cake/*.png "$dest/"
cp "$SHOWCASE/portals/archway/portal_archway.vrx" "$dest/portal_archway.vrx"
cp "$SHOWCASE"/portals/archway/*.png "$dest/"
echo "$dest"
