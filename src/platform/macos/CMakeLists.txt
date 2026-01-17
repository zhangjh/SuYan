# src/platform/macos/CMakeLists.txt
# macOS 平台适配层 (基于 Squirrel)

if(NOT PLATFORM_MACOS)
    message(STATUS "Skipping macOS platform code (not on macOS)")
    return()
endif()

# 设置源文件
set(MACOS_SOURCES
    squirrel_integration.cpp
)

set(MACOS_HEADERS
    squirrel_integration.h
)

# 创建静态库
add_library(ime_platform_macos STATIC
    ${MACOS_SOURCES}
    ${MACOS_HEADERS}
)

# 设置包含目录
target_include_directories(ime_platform_macos
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖
target_link_libraries(ime_platform_macos
    PUBLIC
        ime_core_storage
        ime_core_frequency
        ime_core_input
        ime_core_learning
)

# 设置编译定义
target_compile_definitions(ime_platform_macos
    PUBLIC
        PLATFORM_MACOS
)

# macOS 特定编译选项
target_compile_options(ime_platform_macos
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

# 安装 RIME 配置文件
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rime_config/
    DESTINATION share/rime
    FILES_MATCHING
    PATTERN "*.yaml"
)

# 安装 Swift 文件（供 Squirrel 项目使用）
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/ImeIntegration.swift
    DESTINATION share/squirrel
)

# 导出头文件供 Swift Bridging Header 使用
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/squirrel_integration.h
    DESTINATION include/ime
)
