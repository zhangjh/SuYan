# src/common/CMakeLists.txt
# 跨平台公共代码

add_library(ime_common STATIC
    common.cpp
)

target_include_directories(ime_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置为接口库直到有实际源文件
set_target_properties(ime_common PROPERTIES LINKER_LANGUAGE CXX)
