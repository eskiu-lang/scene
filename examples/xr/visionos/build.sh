#!/bin/sh
# Builds the visionOS example for the simulator (or a device with SDK=xros) into
# build/EskiuXR.app. It needs wgpu-native built from eskiu-packages/wgpu-native's eskiu branch
# (WGPU_LIB, its eskiu/build_apple.sh) and an eskiu-gpu with Gpu_texture_from_metal (GPU_DIR,
# until a gpu release has it).
#
#     examples/xr/visionos/build.sh
#     xcrun simctl boot "Apple Vision Pro"; xcrun simctl install booted build/EskiuXR.app
#     xcrun simctl launch booted dev.eskiu.scene.xr
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
SDK=${SDK:-xrsimulator}
GPU_DIR=${GPU_DIR:-$root/../gpu}
WGPU=${WGPU:-$root/../wgpu-native}
ESKIUC=${ESKIUC:-eskiuc}
case $SDK in
    xrsimulator) triple=arm64-apple-xros26.0-simulator; rust=aarch64-apple-visionos-sim ;;
    *) triple=arm64-apple-xros26.0; rust=aarch64-apple-visionos ;;
esac
WGPU_LIB=${WGPU_LIB:-$WGPU/eskiu/out/$rust/libwgpu_native.a}
out=$here/build
pkg=$out/pkg
rm -rf "$pkg"
mkdir -p "$pkg" "$out/EskiuXR.app"
# scene with gpu taken from GPU_DIR
cp -R "$root/src" "$pkg/src"
cp "$here"/../xr_scene*.esk "$pkg/"
sed -e "s|^gpu = .*|gpu = { path = \"$GPU_DIR\" }|" -e "s|^main = .*|main = \"xr_scene.esk\"|" \
    "$root/eskiu.toml" > "$pkg/eskiu.toml"
(cd "$pkg" && "$ESKIUC" xr_scene.esk --target "$triple" -c -o "$out/xr_scene.o")
# the showcase's files (the Xcode project copies build/assets into the app too)
"$here/../assets.sh" "$out/assets" >/dev/null
sdk=$(xcrun --sdk "$SDK" --show-sdk-path)
xcrun -sdk "$SDK" clang -target "$triple" -fobjc-arc -O2 -c "$here/host.m" -o "$out/host.o"
xcrun -sdk "$SDK" swiftc -target "$triple" -sdk "$sdk" -O -parse-as-library \
    -import-objc-header "$here/bridge.h" "$here/App.swift" "$out/host.o" "$out/xr_scene.o" "$WGPU_LIB" \
    -framework Metal -framework QuartzCore -framework CompositorServices -framework ARKit \
    -framework Foundation -framework AVFoundation -framework CoreMedia -framework CoreVideo -lc++ -o "$out/EskiuXR.app/EskiuXR"
rm -rf "$out/EskiuXR.app/assets"
cp -R "$out/assets" "$out/EskiuXR.app/assets"
# the platform the bundle is for: XRSimulator or XROS
plat=XRSimulator
[ "$SDK" = xros ] && plat=XROS
sed "s|<string>XRSimulator</string>|<string>$plat</string>|" "$here/Info.plist" > "$out/EskiuXR.app/Info.plist"
codesign -f -s - "$out/EskiuXR.app" >/dev/null 2>&1
echo "$out/EskiuXR.app"
