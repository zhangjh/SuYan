# SuYan TSF DLL - CMake Configuration
# Pure Win32 TSF Text Service (no Qt/RIME dependencies)

if(NOT WIN32)
    return()
endif()

# Determine output name based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TSF_DLL_NAME "SuYan")
    set(TSF_DLL_ARCH "x64")
else()
    set(TSF_DLL_NAME "SuYan32")
    set(TSF_DLL_ARCH "x86")
endif()

# Icon resource path
set(TSF_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/icons/app-icon.ico")

# Generate resource file with icon (ID 100, same as Weasel's IDI_WEASEL)
set(TSF_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/tsf_dll_generated.rc")
file(WRITE "${TSF_RC_FILE}"
"// Auto-generated resource file
#include <windows.h>

// Icon resource (ID 100)
100 ICON \"${TSF_ICON_FILE}\"

// Version info
VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_DLL
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904b0\"
        BEGIN
            VALUE \"CompanyName\",      \"SuYan\\\\0\"
            VALUE \"FileDescription\",  \"SuYan Input Method TSF Module\\\\0\"
            VALUE \"FileVersion\",      \"1.0.0.0\\\\0\"
            VALUE \"InternalName\",     \"${TSF_DLL_NAME}\\\\0\"
            VALUE \"LegalCopyright\",   \"Copyright (C) 2024\\\\0\"
            VALUE \"OriginalFilename\", \"${TSF_DLL_NAME}.dll\\\\0\"
            VALUE \"ProductName\",      \"SuYan Input Method\\\\0\"
            VALUE \"ProductVersion\",   \"1.0.0.0\\\\0\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x0409, 1200
    END
END
")

# Source files
set(TSF_DLL_SOURCES
    dll_main.cpp
    tsf_text_service.cpp
    edit_session.cpp
    ipc_client.cpp
    logger.cpp
    langbar_button.cpp
    ${TSF_RC_FILE}
)

set(TSF_DLL_HEADERS
    tsf_text_service.h
    edit_session.h
    ipc_client.h
    logger.h
    langbar_button.h
)

# Create the DLL
add_library(${TSF_DLL_NAME} SHARED ${TSF_DLL_SOURCES} ${TSF_DLL_HEADERS})

# Include directories
target_include_directories(${TSF_DLL_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/shared
)

# Link libraries
target_link_libraries(${TSF_DLL_NAME} PRIVATE
    ole32
    oleaut32
    advapi32
    shell32
    shlwapi
    user32
)

# Generate exports.def file in build directory
set(TSF_DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/exports.def")
file(WRITE "${TSF_DEF_FILE}"
"LIBRARY ${TSF_DLL_NAME}
EXPORTS
    DllMain PRIVATE
    DllGetClassObject PRIVATE
    DllCanUnloadNow PRIVATE
    DllRegisterServer PRIVATE
    DllUnregisterServer PRIVATE
")

# Compiler options
if(MSVC)
    target_compile_options(${TSF_DLL_NAME} PRIVATE
        /utf-8
        /W4
        /WX-
        /MP
    )
    
    set_target_properties(${TSF_DLL_NAME} PROPERTIES
        LINK_FLAGS "/DEF:\"${TSF_DEF_FILE}\""
    )
endif()

# Output directory
set_target_properties(${TSF_DLL_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
)

# Status output
message(STATUS "TSF DLL Configuration:")
message(STATUS "  Name: ${TSF_DLL_NAME}.dll")
message(STATUS "  Architecture: ${TSF_DLL_ARCH}")
