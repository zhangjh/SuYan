# src/core/storage/CMakeLists.txt
# 本地扩展存储模块 (SQLite)

# 查找 SQLite3
find_package(SQLite3 REQUIRED)

add_library(ime_storage STATIC
    local_storage.h
    sqlite_storage.h
    sqlite_storage.cpp
)

target_include_directories(ime_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ime_storage PUBLIC SQLite::SQLite3)

set_target_properties(ime_storage PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    LINKER_LANGUAGE CXX
)
