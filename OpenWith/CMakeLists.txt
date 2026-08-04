project(OpenWith)

set(SOURCES
    src/OpenWith.cpp
    src/AppProvider.cpp
)

if(APPLE)
    list(APPEND SOURCES
        src/LaunchServicesBridge.mm
        src/MacOSAppProvider.cpp
    )
else()
    list(APPEND SOURCES
        src/XDGBasedAppProvider.cpp
        src/ExtensionMimeMap.cpp
    )
endif()

add_library (${PROJECT_NAME} MODULE ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE utils far2l)

# Frameworks for macOS
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework CoreServices"
    )

    find_library(UTI_FRAMEWORK UniformTypeIdentifiers)
    if(UTI_FRAMEWORK)
        target_link_libraries(${PROJECT_NAME} PRIVATE "-weak_framework UniformTypeIdentifiers")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set_source_files_properties(src/LaunchServicesBridge.mm
            PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    endif()
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE -DFAR_DONT_USE_INTERNALS)

target_include_directories(${PROJECT_NAME} PRIVATE .)
target_include_directories(${PROJECT_NAME} PRIVATE ../far2l/far2sdk)
target_include_directories(${PROJECT_NAME} PRIVATE ../WinPort)
target_include_directories(${PROJECT_NAME} PRIVATE ../utils/include)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")

set(CURRENT_TARGET "OpenWith")

file(GLOB_RECURSE RESOURCE_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)

setup_target_auxiliary_files(${CURRENT_TARGET}
        RESOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${INSTALL_DIR}/Plugins/${CURRENT_TARGET}/plug"
)

add_dependencies(${CURRENT_TARGET} copy_aux_files_for_${CURRENT_TARGET})
