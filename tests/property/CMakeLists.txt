# tests/property/CMakeLists.txt
# 属性测试 (Property-Based Testing)

# 属性测试可执行文件
add_executable(property_tests
    main.cpp
    sample_property_test.cpp
    dictionary_property_test.cpp
)

target_link_libraries(property_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    rapidcheck
    ime_storage
    ime_dictionary
)

target_include_directories(property_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${rapidcheck_SOURCE_DIR}/extras/gtest/include
)

# 注册测试
gtest_discover_tests(property_tests)
