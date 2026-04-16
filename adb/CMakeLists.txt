# ADB Plugin for far2l

# Basic source files
set(SOURCES
    src/FARPlugin.cpp
    src/ADBPlugin.cpp
    src/ADBShell.cpp
    src/ADBDevice.cpp
    src/ADBDialogs.cpp
    src/ADBLog.cpp
)

# Create the plugin
add_library(adb MODULE ${SOURCES})

# Link with required libraries
target_link_libraries(adb utils far2l)

# Add compile definitions
target_compile_definitions(adb PRIVATE
    -DWINPORT_DIRECT
    -DUNICODE
    -D_UNICODE
    -DFAR_DONT_USE_INTERNALS
)

# Add include directories
target_include_directories(adb PRIVATE .)
target_include_directories(adb PRIVATE ../far2l/far2sdk)
target_include_directories(adb PRIVATE ../WinPort)

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(adb PRIVATE -Os -fomit-frame-pointer)
    if(APPLE)
        add_custom_command(TARGET adb POST_BUILD
            COMMAND strip -x "$<TARGET_FILE:adb>"
            COMMENT "Stripping debug symbols from adb plugin"
        )
    else()
        add_custom_command(TARGET adb POST_BUILD
            COMMAND strip --strip-unneeded "$<TARGET_FILE:adb>"
            COMMENT "Stripping debug symbols from adb plugin"
        )
    endif()
endif()

# Set output properties
set_target_properties(adb PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/adb/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide"
)

# Copy resource files
set(CURRENT_TARGET "adb")
file(GLOB_RECURSE RESOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)
setup_target_auxiliary_files(${CURRENT_TARGET}
    RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/configs
    "${INSTALL_DIR}/Plugins/${CURRENT_TARGET}/"
)
add_dependencies(${CURRENT_TARGET} copy_aux_files_for_${CURRENT_TARGET})