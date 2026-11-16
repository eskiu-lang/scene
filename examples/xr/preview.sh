#!/bin/sh
# Renders each scene of the showcase on this Mac (xr_scene_preview: a standing viewer looking
# ahead) into OUT/NN-name.png, to check the scenes off the headset. Needs the assets
# (assets.sh), wgpu-native's eskiu branch built for macOS (WGPU) and an eskiu-gpu with the
# headset calls (GPU_DIR).
#
#     examples/xr/preview.sh OUT [seconds]
set -eu
out=${1:?usage: preview.sh OUT [seconds]}
secs=${2:-2}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
GPU_DIR=${GPU_DIR:-$root/../gpu}
WGPU=${WGPU:-$root/../wgpu-native}
ESKIUC=${ESKIUC:-eskiuc}
mkdir -p "$out"
out=$(cd "$out" && pwd)
work=$out/.build
rm -rf "$work/pkg"
mkdir -p "$work/pkg"
"$here/assets.sh" "$work/assets" >/dev/null
cp -R "$root/src" "$work/pkg/src"
cp "$here"/xr_scene*.esk "$work/pkg/"
sed -e "s|^gpu = .*|gpu = { path = \"$GPU_DIR\" }|" -e "s|^main = .*|main = \"xr_scene.esk\"|" \
    "$root/eskiu.toml" > "$work/pkg/eskiu.toml"
(cd "$work/pkg" && "$ESKIUC" xr_scene.esk -c -o "$work/xr_scene.o")
cat > "$work/main.c" <<'C'
#include <stdio.h>
#include <stdlib.h>
void xr_scene_set_assets(const char *dir);
int xr_scene_preview(int i, const char *path, unsigned w, unsigned h, float seconds);
int main(int argc, char **argv) {
    xr_scene_set_assets(argv[1]);
    for (int i = 0; i < 12; i++) {
        char path[1024];
        snprintf(path, sizeof path, "%s/%02d.ppm", argv[2], i);
        if (xr_scene_preview(i, path, 960, 720, (float)atof(argv[3])) != 0) { fprintf(stderr, "scene %d failed\n", i); return 1; }
    }
    return 0;
}
C
clang -O1 "$work/main.c" "$work/xr_scene.o" "$WGPU/target/release/libwgpu_native.a" -framework Metal \
    -framework QuartzCore -framework Foundation -framework CoreGraphics -framework IOKit -lc++ -o "$work/preview"
"$work/preview" "$work/assets" "$work" "$secs"
names="basics materials lights images video models text particles physics drag animation portal"
i=0
for n in $names; do
    f=$(printf "%02d" $i)
    sips -s format png "$work/$f.ppm" --out "$out/$f-$n.png" >/dev/null
    i=$((i + 1))
done
echo "$out"
