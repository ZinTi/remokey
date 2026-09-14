# ============================================================================
#  remokey —— Windows 桌面应用程序
#  C++17 / MinGW-w64 / 静态链接
# ============================================================================
#
#  平台行为：
#    * Windows 主机  -> 使用本机 MinGW-w64 (g++) 直接编译 Windows 桌面程序
#    * Linux   主机  -> 自动切换到 x86_64-w64-mingw32-* 交叉工具链
#                       编译出 Windows PE 桌面程序（而非 ELF）
#
#  构建 / 安装：
#    ./build.sh                                # 自动选择平台
#  或手动：
#    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=dist
#    cmake --build build --config Release --target install -j
#
#  若想使用自定义工具链，可显式传入 -DCMAKE_TOOLCHAIN_FILE=xxx.cmake，
#  此时将跳过自动检测逻辑。
# ============================================================================

cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# Linux 主机：自动启用 MinGW-w64 交叉编译
# （必须在 project() 之前设置，CMake 才会正确识别交叉编译环境）
# ---------------------------------------------------------------------------
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux"
   AND NOT CMAKE_CROSSCOMPILING
   AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)

    message(STATUS "检测到 Linux 主机：自动配置 MinGW-w64 交叉编译（目标 Windows）")

    set(_MINGW_TRIPLE "x86_64-w64-mingw32")

    set(CMAKE_SYSTEM_NAME       Windows)
    set(CMAKE_SYSTEM_PROCESSOR  x86_64)

    set(CMAKE_C_COMPILER        "${_MINGW_TRIPLE}-gcc")
    set(CMAKE_CXX_COMPILER      "${_MINGW_TRIPLE}-g++")
    set(CMAKE_RC_COMPILER       "${_MINGW_TRIPLE}-windres")

    # 让 find_package / find_library / find_path 只在 MinGW 目录下搜索
    set(CMAKE_FIND_ROOT_PATH        "/usr/${_MINGW_TRIPLE}")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# ---------------------------------------------------------------------------
# 项目
# ---------------------------------------------------------------------------
project(remokey
    VERSION     0.1.0
    DESCRIPTION "Remote Desktop"
    LANGUAGES   C CXX
)

# ---------------------------------------------------------------------------
# 语言标准
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_C_STANDARD            99)
set(CMAKE_C_STANDARD_REQUIRED   ON)

# 单配置生成器（Makefile / Ninja）下默认 Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---------------------------------------------------------------------------
# 输出目录：build/<config>  例如 build/release、build/debug
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<LOWER_CASE:$<CONFIG>>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<LOWER_CASE:$<CONFIG>>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<LOWER_CASE:$<CONFIG>>")

# ---------------------------------------------------------------------------
# 收集源文件（src/）
# ---------------------------------------------------------------------------
file(GLOB_RECURSE REMOKEY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# 收集 QR Code 生成库源文件
list(APPEND REMOKEY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/qrcodegen/qrcodegen.cpp"
)

# 收集资源文件（.rc）
file(GLOB_RECURSE REMOKEY_RESOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/res/*.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.rc"
)

if(NOT REMOKEY_SOURCES)
    message(FATAL_ERROR "src/ 目录下未找到任何源文件。")
endif()

# ---------------------------------------------------------------------------
# 目标：Windows GUI 子系统程序（WIN32 -> -mwindows / /SUBSYSTEM:WINDOWS）
# ---------------------------------------------------------------------------

# 让 rc 编译器能在 res/ 目录下找到 ico
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/res")

add_executable(remokey WIN32 
    ${REMOKEY_SOURCES}
    ${REMOKEY_RESOURCES}
)

# 头文件搜索路径（include/ + src/ + res/），仅本目标使用
# 注意：必须在 add_executable 之后才能对目标设置属性
target_include_directories(remokey
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/core"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/ui"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/utils"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/utils"
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/qrcodegen"
)

# Windows 平台宏定义
target_compile_definitions(remokey
    PRIVATE
        $<$<PLATFORM_ID:Windows>:UNICODE>
        $<$<PLATFORM_ID:Windows>:_UNICODE>
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

# ---------------------------------------------------------------------------
# thirdparty/libuv：已编译好的静态库
# ---------------------------------------------------------------------------
set(LIBUV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libuv")

if(NOT EXISTS "${LIBUV_ROOT}/include/uv.h")
    message(FATAL_ERROR "libuv 头文件不存在： ${LIBUV_ROOT}/include/uv.h")
endif()

# 交叉编译时 CMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY 会找不到项目内的库
# 临时切 BOTH 再恢复
set(_OLD_FIND_ROOT_PATH_MODE_LIBRARY ${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)

# 优先选静态库 uv_a / libuv.a，避免误用 libuv.dll.a 导致运行时依赖 DLL
find_library(LIBUV_LIBRARY
    NAMES uv_a libuv.a uv libuv
    HINTS "${LIBUV_ROOT}/lib"
    REQUIRED
)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ${_OLD_FIND_ROOT_PATH_MODE_LIBRARY})

# ---------------------------------------------------------------------------
# thirdparty/zlib：uWebSockets permessage-deflate 依赖
# ---------------------------------------------------------------------------
set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlib")

if(NOT EXISTS "${ZLIB_ROOT}/zlib.h")
    message(FATAL_ERROR "zlib 源码不存在： ${ZLIB_ROOT}/zlib.h")
endif()

# 只编静态库，不编测试与示例
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING  OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC   ON  CACHE BOOL "" FORCE)

# 让 zlib 自己的 CMake 能在交叉环境下正确找到东西
set(_OLD_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

add_subdirectory("${ZLIB_ROOT}" "${CMAKE_BINARY_DIR}/zlib" EXCLUDE_FROM_ALL)

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${_OLD_FIND_ROOT_PATH_MODE_PACKAGE})

# 不同版本的 zlib 生成的目标名可能是 zlibstatic 或 zlib
if(TARGET zlibstatic)
    set(ZLIB_TARGET zlibstatic)
elseif(TARGET zlib)
    set(ZLIB_TARGET zlib)
else()
    message(FATAL_ERROR "未能在 zlib 中找到可用的静态库目标")
endif()

message(STATUS "  zlib       : ${ZLIB_TARGET}")

# ---------------------------------------------------------------------------
# thirdparty/uWebSockets/uSockets：C 源文件
# ---------------------------------------------------------------------------
set(UWEBSOCKETS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/uWebSockets")

if(NOT EXISTS "${UWEBSOCKETS_ROOT}/uSockets/src")
    message(FATAL_ERROR "uSockets 源码不存在： ${UWEBSOCKETS_ROOT}/uSockets/src")
endif()

file(GLOB_RECURSE USOCKETS_SOURCES CONFIGURE_DEPENDS
    "${UWEBSOCKETS_ROOT}/uSockets/src/*.c"
)

add_library(uSockets STATIC ${USOCKETS_SOURCES})

set_target_properties(uSockets PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# GCC 16 兼容：incompatible-pointer-types 在 C 里也是 error
target_compile_options(uSockets PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wno-error=incompatible-pointer-types>
    $<$<C_COMPILER_ID:GNU>:-Wno-discarded-qualifiers>
    $<$<C_COMPILER_ID:GNU>:-Wno-unused-variable>
)

target_include_directories(uSockets
    PUBLIC
        "${UWEBSOCKETS_ROOT}/uSockets/src"
        "${UWEBSOCKETS_ROOT}/src"
        "${LIBUV_ROOT}/include"
)

target_compile_definitions(uSockets
    PUBLIC
        LIBUS_NO_SSL
        LIBUS_USE_LIBUV
)

# zlib 作为 PUBLIC 依赖，include 路径会传播给 uSockets / remokey
target_link_libraries(uSockets
    PUBLIC
        ${ZLIB_TARGET}
)

# uWebSockets 头文件（header-only）
target_include_directories(remokey
    PRIVATE
        "${UWEBSOCKETS_ROOT}/src"
        "${UWEBSOCKETS_ROOT}/uSockets/src"
        "${LIBUV_ROOT}/include"
)

# ---------------------------------------------------------------------------
# 链接
# ---------------------------------------------------------------------------
target_link_libraries(remokey
    PRIVATE
        uSockets
        ${ZLIB_TARGET}
        ${LIBUV_LIBRARY}
        ws2_32
        iphlpapi
        psapi
        userenv
        user32
        advapi32
        gdi32
        gdiplus
        dbghelp
        dwmapi
)

# 警告选项
target_compile_options(remokey
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-finput-charset=UTF-8>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fexec-charset=UTF-8>
)

# ---------------------------------------------------------------------------
# 静态编译：静态链接 libgcc / libstdc++ / winpthread，无运行时 DLL 依赖
# （无论 Linux 交叉编译还是 Windows 本机 MinGW，MINGW 变量都为真）
# ---------------------------------------------------------------------------
if(MINGW)
    target_link_options(remokey
        PRIVATE
            -municode
            -static
            -static-libgcc
            -static-libstdc++
    )
endif()

# 若使用 std::thread / std::mutex，取消下面两行注释
# find_package(Threads REQUIRED)
# target_link_libraries(remokey PRIVATE Threads::Threads)

# ---------------------------------------------------------------------------
# 安装：可执行文件和前端文件
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS remokey
    RUNTIME DESTINATION "."
    COMPONENT Runtime
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/www"
    DESTINATION "."                  # 装到 dist/www/
    FILES_MATCHING PATTERN "*"
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/remokey_config.ini"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/kill_remokey.bat"
    DESTINATION "."                  # 装到 dist 根目录
)

# ---------------------------------------------------------------------------
# 配置摘要
# ---------------------------------------------------------------------------
message(STATUS "remokey  ${PROJECT_VERSION}")
message(STATUS "  主机       : ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "  目标       : ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  交叉编译   : ${CMAKE_CROSSCOMPILING}")
message(STATUS "  构建类型   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  编译器     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  编译器路径 : ${CMAKE_CXX_COMPILER}")
message(STATUS "  输出目录   : ${CMAKE_BINARY_DIR}/<config>")
message(STATUS "  安装前缀   : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  libuv      : ${LIBUV_LIBRARY}")
message(STATUS "  uWebSockets: ${UWEBSOCKETS_ROOT}")
message(STATUS "  zlib       : ${ZLIB_TARGET}")
