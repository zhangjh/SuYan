# src/core/input/CMakeLists.txt
# 输入核心模块

set(INPUT_SOURCES
    rime_config.cpp
    candidate_merger.cpp
    input_session.cpp
    input_state.cpp
)

set(INPUT_HEADERS
    rime_config.h
    candidate_merger.h
    input_session.h
    input_state.h
)

add_library(ime_input STATIC
    ${INPUT_SOURCES}
    ${INPUT_HEADERS}
)

target_include_directories(ime_input PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# 链接依赖
target_link_libraries(ime_input PUBLIC
    ime_storage
    ime_frequency
)

# 如果有 librime，链接它
if(TARGET rime)
    target_link_libraries(ime_input PUBLIC rime)
endif()
