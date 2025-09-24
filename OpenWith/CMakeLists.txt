project(OpenWith)

set(SOURCES
src/OpenWith.cpp
src/AppProvider.cpp
src/DummyAppProvider.cpp
src/XDGBasedAppProvider.cpp
)

if(APPLE)
    list(APPEND SOURCES src/MacOSAppProvider.mm)
endif()

add_library (${PROJECT_NAME} MODULE ${SOURCES})
target_link_libraries(${PROJECT_NAME} utils far2l)

# Frameworks for macOS
if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        "-framework Cocoa"
        "-framework CoreServices"
        "-framework UniformTypeIdentifiers"
    )
    target_compile_options(${PROJECT_NAME} PRIVATE "-fobjc-arc")
endif()

target_compile_definitions(OpenWith PRIVATE)

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
