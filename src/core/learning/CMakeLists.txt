# src/core/learning/CMakeLists.txt
# 自动学词模块

add_library(ime_learning STATIC
    auto_learner_impl.cpp
)

target_include_directories(ime_learning PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ime_learning PUBLIC
    ime_storage
)

# 设置 C++ 标准
target_compile_features(ime_learning PUBLIC cxx_std_17)
