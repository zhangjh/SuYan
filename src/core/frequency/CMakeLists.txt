# src/core/frequency/CMakeLists.txt
# 用户词频管理模块

add_library(ime_frequency STATIC
    frequency_manager.h
    frequency_manager_impl.h
    frequency_manager_impl.cpp
)

target_include_directories(ime_frequency PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ime_frequency PUBLIC
    ime_storage
)

set_target_properties(ime_frequency PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    LINKER_LANGUAGE CXX
)
