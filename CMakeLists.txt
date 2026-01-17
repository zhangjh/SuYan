cmake_minimum_required(VERSION 3.16)

project(CrossPlatformIME
    VERSION 1.0.0
    DESCRIPTION "Cross-platform Chinese-English Input Method based on RIME"
    LANGUAGES CXX C
)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 选项
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_WINDOWS_FRONTEND "Build Windows frontend (Weasel)" OFF)
option(BUILD_MACOS_FRONTEND "Build macOS frontend (Squirrel)" OFF)
option(ENABLE_CLOUD_SYNC "Enable cloud dictionary sync" ON)

# 平台检测
if(WIN32)
    set(PLATFORM_WINDOWS ON)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    set(PLATFORM_MACOS ON)
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX)
    set(PLATFORM_LINUX ON)
    add_definitions(-DPLATFORM_LINUX)
endif()

# 查找依赖
# SQLite3
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found, will use bundled version if available")
endif()

# librime (从 submodule 构建)
if(EXISTS "${CMAKE_SOURCE_DIR}/deps/librime/CMakeLists.txt")
    message(STATUS "Found librime submodule")
    set(LIBRIME_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/librime")
    # librime 将在后续任务中配置
else()
    message(WARNING "librime submodule not initialized. Run: git submodule update --init --recursive")
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/core
)

# 添加子目录
add_subdirectory(src)

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装配置
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/
    DESTINATION share/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.yaml"
)

# 打印配置摘要
message(STATUS "")
message(STATUS "=== CrossPlatformIME Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Cloud sync: ${ENABLE_CLOUD_SYNC}")
message(STATUS "======================================")
message(STATUS "")
