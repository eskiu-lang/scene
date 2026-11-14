#!/bin/sh
# Builds the Quest example into build/EskiuXR.apk (arm64, debug-signed): ../xr_scene.esk and
# host.c over the OpenXR loader (OPENXR_DIR: the include/ and jni/ of Khronos'
# openxr_loader_for_android AAR) and wgpu-native from eskiu-lang/wgpu-native's eskiu branch
# (WGPU, its eskiu/build_android.sh).
#
#     examples/xr/quest/build.sh
#     adb install -r examples/xr/quest/build/EskiuXR.apk
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
GPU_DIR=${GPU_DIR:-$root/../eskiu-gpu}
WGPU=${WGPU:-$root/../wgpu-native-eskiu}
WGPU_LIB=${WGPU_LIB:-$WGPU/eskiu/out/aarch64-linux-android/libwgpu_native.a}
ESKIUC=${ESKIUC:-eskiuc}
SDK=${ANDROID_HOME:-$HOME/Library/Android/sdk}
NDK=${ANDROID_NDK_HOME:-$(ls -d "$SDK/ndk/"* | sort -V | tail -1)}
BT=$(ls -d "$SDK/build-tools/"* | sort -V | tail -1)
PLATFORM=$(ls -d "$SDK/platforms/android-"* | sort -V | tail -1)
OPENXR_DIR=${OPENXR_DIR:?set OPENXR_DIR to an extracted openxr_loader_for_android AAR}
tc=$(ls -d "$NDK/toolchains/llvm/prebuilt/"*/bin | head -1)
cc="$tc/aarch64-linux-android29-clang"
out=$here/build
pkg=$out/pkg
rm -rf "$pkg" "$out/apk"
mkdir -p "$pkg" "$out/apk/lib/arm64-v8a"
cp -R "$root/src" "$pkg/src"
cp "$here/../xr_scene.esk" "$pkg/xr_scene.esk"
sed -e "s|^gpu = .*|gpu = { path = \"$GPU_DIR\" }|" -e "s|^main = .*|main = \"xr_scene.esk\"|" \
    "$root/eskiu.toml" > "$pkg/eskiu.toml"
(cd "$pkg" && "$ESKIUC" xr_scene.esk --target aarch64-linux-android29 -c -o "$out/xr_scene.o")
glue=$NDK/sources/android/native_app_glue
inc=$(find "$OPENXR_DIR" -path "*include/openxr/openxr.h" | head -1)
inc=${inc%/openxr/openxr.h}
loader=$(find "$OPENXR_DIR" -path "*arm64-v8a/libopenxr_loader.so" | head -1)
"$cc" -O2 -fPIC -I"$glue" -I"$inc" -c "$here/host.c" -o "$out/host.o"
"$cc" -O2 -fPIC -I"$glue" -c "$glue/android_native_app_glue.c" -o "$out/glue.o"
"$cc" -shared -o "$out/apk/lib/arm64-v8a/libeskiuxr.so" "$out/host.o" "$out/glue.o" "$out/xr_scene.o" \
    "$WGPU_LIB" "$loader" -landroid -llog -lm -ldl -lmediandk \
    -Wl,-u,ANativeActivity_onCreate -Wl,-z,max-page-size=16384
cp "$loader" "$out/apk/lib/arm64-v8a/"
# the showcase's files go in the APK's assets
"$here/../assets.sh" "$out/assets" >/dev/null
"$BT/aapt2" link -o "$out/unsigned.apk" --manifest "$here/AndroidManifest.xml" -I "$PLATFORM/android.jar" -A "$out/assets"
(cd "$out/apk" && zip -qr "$out/unsigned.apk" lib)
"$BT/zipalign" -f -p 16 "$out/unsigned.apk" "$out/aligned.apk"
ks=$out/debug.keystore
[ -f "$ks" ] || keytool -genkeypair -keystore "$ks" -storepass android -keypass android -alias debug \
    -keyalg RSA -validity 10000 -dname "CN=Eskiu debug" >/dev/null 2>&1
"$BT/apksigner" sign --ks "$ks" --ks-pass pass:android --out "$out/EskiuXR.apk" "$out/aligned.apk"
echo "$out/EskiuXR.apk"
