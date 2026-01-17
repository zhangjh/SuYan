# src/core/dictionary/CMakeLists.txt
# 词库管理模块

add_library(ime_dictionary STATIC
    dictionary_manager_impl.cpp
)

target_include_directories(ime_dictionary PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ime_dictionary PUBLIC
    ime_storage
)

# 设置 C++ 标准
target_compile_features(ime_dictionary PUBLIC cxx_std_17)
