#!/usr/bin/env bash
# build-all.sh 构建 Windows 服务端以及 Android 移动端
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$ROOT_DIR/remokey_v0.1.0-alpha2_release_all"

# ---------------------------------------------------------------
# 1. Windows 端
# ---------------------------------------------------------------
cmake -S "$ROOT_DIR/windows/" -B "$ROOT_DIR/build/windows/" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$OUT_DIR/windows" "$@"

cmake --build "$ROOT_DIR/build/windows/" --config Release --target install -j

# ---------------------------------------------------------------
# 2. Android 端
# ---------------------------------------------------------------
( cd "$ROOT_DIR/android" && ./gradlew :app:assembleRelease )

# ---------------------------------------------------------------
# 3. 复制 APK（已签名 / 未签名 是互斥的，按存在性复制）
# ---------------------------------------------------------------
mkdir -p "$OUT_DIR/android"

APK_DIR="$ROOT_DIR/android/app/build/outputs/apk/release"
SIGNED_APK="$APK_DIR/app-release.apk"
UNSIGNED_APK="$APK_DIR/app-release-unsigned.apk"

copied=0

if [[ -f "$SIGNED_APK" ]]; then
    cp -f "$SIGNED_APK" "$OUT_DIR/android/RemokeyPad_release.apk"
    echo "已复制已签名 APK: app-release.apk"
    copied=$((copied + 1))
fi

if [[ -f "$UNSIGNED_APK" ]]; then
    cp -f "$UNSIGNED_APK" "$OUT_DIR/android/RemokeyPad_release_unsigned.apk"
    echo "已复制未签名 APK: app-release-unsigned.apk"
    copied=$((copied + 1))
fi

# 兜底：若两种标准名都没有，取目录里最新的 .apk（适配自定义命名/Flavor）
if [[ $copied -eq 0 ]]; then
    fallback="$(find "$APK_DIR" -maxdepth 1 -name '*.apk' -type f 2>/dev/null | head -n1)"
    if [[ -n "$fallback" ]]; then
        cp -f "$fallback" "$OUT_DIR/android/$(basename "$fallback")"
        echo "已复制 APK: $(basename "$fallback")"
        copied=$((copied + 1))
    fi
fi

if [[ $copied -eq 0 ]]; then
    echo "错误: 未在 $APK_DIR 下找到任何 APK 产物" >&2
    exit 1
fi

echo "构建完成，产物位于：$OUT_DIR"
