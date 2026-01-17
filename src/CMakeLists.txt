# src/CMakeLists.txt
# 源代码构建配置

# 公共库
add_subdirectory(common)

# 核心扩展模块
add_subdirectory(core)

# 平台适配层 (根据平台选择性构建)
if(WIN32)
    # Windows 平台：总是构建集成库
    add_subdirectory(platform/windows)
elseif(APPLE)
    # macOS 平台：总是构建集成库
    add_subdirectory(platform/macos)
endif()
