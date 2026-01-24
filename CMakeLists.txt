# SuYan Input Method - CMake 配置
cmake_minimum_required(VERSION 3.20)

project(SuYan
    VERSION 1.0.0
    DESCRIPTION "Cross-platform Chinese Input Method"
    LANGUAGES CXX C OBJCXX
)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS 部署目标
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")

# Qt 配置
# Homebrew Qt 路径（Apple Silicon）
set(Qt6_DIR "/opt/homebrew/opt/qt/lib/cmake/Qt6" CACHE PATH "Qt6 CMake directory")
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

# Qt MOC/UIC/RCC 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# librime 配置
set(LIBRIME_DIR "${CMAKE_SOURCE_DIR}/deps/librime")
set(LIBRIME_INCLUDE_DIR "${LIBRIME_DIR}/include")
set(LIBRIME_LIB_DIR "${LIBRIME_DIR}/lib")

# 创建 librime imported target
add_library(librime SHARED IMPORTED)
set_target_properties(librime PROPERTIES
    IMPORTED_LOCATION "${LIBRIME_LIB_DIR}/librime.dylib"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBRIME_INCLUDE_DIR}"
)

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)
