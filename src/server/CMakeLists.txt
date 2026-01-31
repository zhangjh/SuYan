# SuYan Server - CMake 配置
# Windows 后台服务进程，运行 RIME 引擎和 Qt 候选窗口

if(NOT WIN32)
    return()
endif()

set(SERVER_SOURCES
    main.cpp
    server_app.cpp
    logger.cpp
)

set(SERVER_HEADERS
    server_app.h
    logger.h
)

add_executable(SuYanServer WIN32
    ${SERVER_SOURCES}
    ${SERVER_HEADERS}
)

target_include_directories(SuYanServer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/shared
)

target_link_libraries(SuYanServer PRIVATE
    suyan_core
    suyan_ui
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

set_target_properties(SuYanServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    AUTOMOC ON
    WIN32_EXECUTABLE TRUE
)

if(MSVC)
    target_compile_options(SuYanServer PRIVATE /utf-8)
    set_target_properties(SuYanServer PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()
