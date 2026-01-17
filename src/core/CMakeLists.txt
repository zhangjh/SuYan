# src/core/CMakeLists.txt
# 核心扩展模块

add_subdirectory(storage)
add_subdirectory(frequency)
add_subdirectory(dictionary)
add_subdirectory(learning)
add_subdirectory(input)

if(ENABLE_CLOUD_SYNC)
    add_subdirectory(sync)
endif()
