#!/usr/bin/env bash
# build-all.sh 构建 Windows 服务端以及 Android 移动端
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$ROOT_DIR/remokey_v0.1.0-alpha_release_all"

# 1. 配置 Windows 端
cmake -S "$ROOT_DIR/windows/" -B "$ROOT_DIR/build/windows/" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$OUT_DIR/windows" "$@"

# 2. 编译并安装
cmake --build "$ROOT_DIR/build/windows/" --config Release --target install -j

# 3. 构建安卓端
( cd "$ROOT_DIR/android" && ./gradlew assembleRelease )

# 4. 复制 APK 到发布目录
mkdir -p "$OUT_DIR/android"

# 未签名包（可能不存在，失败不中断）
cp "$ROOT_DIR/android/app/build/outputs/apk/release/app-release-unsigned.apk" \
   "$OUT_DIR/android/RemokeyPad_release_unsigned.apk" || true

# 已签名包
cp "$ROOT_DIR/android/app/build/outputs/apk/release/app-release.apk" \
   "$OUT_DIR/android/RemokeyPad_release.apk" || true

echo "构建完成，产物位于：$OUT_DIR"
