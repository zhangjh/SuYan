# tests/CMakeLists.txt
# 测试配置

# 查找或获取 Google Test
include(FetchContent)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# RapidCheck (属性测试框架)
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG master
)

# 配置 Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest rapidcheck)

# 启用测试
enable_testing()
include(GoogleTest)

# 添加测试子目录
add_subdirectory(unit)
add_subdirectory(property)
