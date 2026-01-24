# SuYan Tests - CMake 配置

# Qt 环境验证测试
add_executable(qt_env_test qt_env_test.cpp)
target_link_libraries(qt_env_test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# librime API 验证测试
add_executable(librime_test librime_test.cpp)
target_include_directories(librime_test PRIVATE ${LIBRIME_INCLUDE_DIR})
target_link_libraries(librime_test PRIVATE librime)

# 设置 RPATH 以便运行时找到 librime
set_target_properties(librime_test PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
)

# ========== 核心层单元测试 ==========

# RimeWrapper 单元测试
add_executable(rime_wrapper_test core/rime_wrapper_test.cpp)
target_link_libraries(rime_wrapper_test PRIVATE suyan_core)
set_target_properties(rime_wrapper_test PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
)

# InputEngine 单元测试
add_executable(input_engine_test core/input_engine_test.cpp)
target_link_libraries(input_engine_test PRIVATE suyan_core)
set_target_properties(input_engine_test PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
)

# ConfigManager 单元测试
add_executable(config_manager_test core/config_manager_test.cpp)
target_link_libraries(config_manager_test PRIVATE suyan_core Qt6::Test)
set_target_properties(config_manager_test PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
)

# FrequencyManager 单元测试
add_executable(frequency_manager_test core/frequency_manager_test.cpp)
target_link_libraries(frequency_manager_test PRIVATE suyan_core Qt6::Test)
set_target_properties(frequency_manager_test PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
)

# ========== UI 层测试 ==========

# UI 测试程序
add_executable(ui_test_app ui/ui_test_app.cpp)
target_link_libraries(ui_test_app PRIVATE 
    suyan_ui 
    suyan_core 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
)
set_target_properties(ui_test_app PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
    AUTOMOC ON
)

# Qt/IMK 集成测试
add_executable(qt_imk_integration_test ui/qt_imk_integration_test.mm)
target_link_libraries(qt_imk_integration_test PRIVATE 
    suyan_ui 
    suyan_core 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
)
set_target_properties(qt_imk_integration_test PROPERTIES
    BUILD_RPATH "${LIBRIME_LIB_DIR}"
    INSTALL_RPATH "${LIBRIME_LIB_DIR}"
)

# macOS 特定：链接 Cocoa 框架
if(APPLE)
    target_link_libraries(qt_imk_integration_test PRIVATE
        "-framework Cocoa"
        "-framework AppKit"
    )
endif()
