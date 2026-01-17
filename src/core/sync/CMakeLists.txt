# src/core/sync/CMakeLists.txt
# 云端同步模块

add_library(ime_sync STATIC
    sync.cpp
)

target_include_directories(ime_sync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ime_sync PUBLIC
    ime_storage
)

set_target_properties(ime_sync PROPERTIES LINKER_LANGUAGE CXX)
