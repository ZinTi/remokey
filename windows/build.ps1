#!/usr/bin/env pwsh
# build.ps1 —— 一键配置、编译并安装（固定使用 MinGW）
$ErrorActionPreference = "Stop"

# 1. 配置：Release 构建，安装到 .\dist\，强制使用 MinGW 工具链
cmake -B build -G "MinGW Makefiles" `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_INSTALL_PREFIX=dist `
      -DCMAKE_C_COMPILER=gcc `
      -DCMAKE_CXX_COMPILER=g++ `
      @args

# 2. 编译并安装：多配置生成器需 --config，单配置忽略
cmake --build build --config Release --target install -j
