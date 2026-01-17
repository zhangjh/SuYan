# tests/unit/CMakeLists.txt
# 单元测试

# 单元测试源文件
set(UNIT_TEST_SOURCES
    main.cpp
    sample_test.cpp
    storage_test.cpp
    frequency_test.cpp
    candidate_merger_test.cpp
    input_session_test.cpp
    input_state_test.cpp
    dictionary_manager_test.cpp
    auto_learner_test.cpp
)

# Windows 平台添加 Weasel 集成测试
if(WIN32)
    list(APPEND UNIT_TEST_SOURCES weasel_integration_test.cpp)
endif()

# 单元测试可执行文件
add_executable(unit_tests ${UNIT_TEST_SOURCES})

target_link_libraries(unit_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    ime_storage
    ime_frequency
    ime_dictionary
    ime_learning
    ime_input
)

# Windows 平台链接 Weasel 集成库
if(WIN32)
    target_link_libraries(unit_tests PRIVATE ime_weasel_integration)
endif()

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 注册测试
gtest_discover_tests(unit_tests)
