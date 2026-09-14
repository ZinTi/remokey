#!/usr/bin/env bash
# build.sh —— 一键配置、编译并安装
set -e

# 1. 配置：Release 构建，安装到 ./dist/，额外参数通过 $@ 传入（如工具链文件）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=dist "$@"

# 2. 编译并安装：多配置生成器需 --config，单配置忽略
cmake --build build --config Release --target install -j
