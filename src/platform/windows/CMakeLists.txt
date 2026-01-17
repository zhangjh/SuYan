# src/platform/windows/CMakeLists.txt
# Windows 平台适配层 (基于 Weasel)

# 仅在 Windows 平台构建
if(NOT WIN32)
    message(STATUS "Skipping Windows platform code on non-Windows system")
    return()
endif()

# 定义平台宏
add_definitions(-DPLATFORM_WINDOWS)

# 源文件
set(WEASEL_INTEGRATION_SOURCES
    weasel_integration.cpp
    weasel_handler_ext.cpp
    weasel_tray_ext.cpp
)

set(WEASEL_INTEGRATION_HEADERS
    weasel_integration.h
    weasel_handler_ext.h
    weasel_tray_ext.h
)

# 创建静态库
add_library(ime_weasel_integration STATIC
    ${WEASEL_INTEGRATION_SOURCES}
    ${WEASEL_INTEGRATION_HEADERS}
)

# 包含目录
target_include_directories(ime_weasel_integration PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)

# 链接依赖
target_link_libraries(ime_weasel_integration PUBLIC
    ime_storage
    ime_frequency
    ime_input
    ime_learning
)

# Windows 特定链接
target_link_libraries(ime_weasel_integration PRIVATE
    Shlwapi
    Shell32
)

# 安装 RIME 配置文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rime_config/
    DESTINATION share/rime
    FILES_MATCHING 
    PATTERN "*.yaml"
)

# 导出头文件
install(FILES ${WEASEL_INTEGRATION_HEADERS}
    DESTINATION include/ime/platform/windows
)
