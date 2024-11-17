project(colorer)


set(LIBCOLORER_TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Colorer-library/src")

include(src/Colorer-library/cmake/utils.cmake)

colorer_extract_version()
message(STATUS "Build Colorer: ${COLORER_VERSION}")

set(SOURCES
    src/pcolorer2/ChooseTypeMenu.h
    src/pcolorer2/FarEditor.h
    src/pcolorer2/FarEditorSet.h
    src/pcolorer2/FarHrcSettings.h
    src/pcolorer2/pcolorer.h
    src/pcolorer2/tools.h
    src/pcolorer2/ChooseTypeMenu.cpp
    src/pcolorer2/FarEditor.cpp
    src/pcolorer2/FarEditorSet.cpp
    src/pcolorer2/FarHrcSettings.cpp
    src/pcolorer2/pcolorer.cpp
    src/pcolorer2/tools.cpp
    src/pcolorer2/CerrLogger.cpp
    src/pcolorer2/CerrLogger.h
)

# set options for colorer-library
set(COLORER_USE_ICU_STRINGS OFF)
set(COLORER_USE_LIBXML ON)
set(COLORER_BUILD_HARD_WARNINGS OFF)
set(COLORER_BUILD_OLD_COMPILERS ON)

add_library(colorer MODULE ${SOURCES})

FIND_PACKAGE(LibXml2 REQUIRED)

target_include_directories(colorer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Colorer-library/src
    )

target_link_libraries(colorer
    utils
    colorer_lib
    far2l
    )

target_compile_definitions(colorer PUBLIC
    _UNICODE
    UNICODE
    __unix__
)


target_include_directories(colorer PRIVATE ${LIBCOLORER_TREE}/src)
target_include_directories(colorer PRIVATE src/pcolorer2)
target_include_directories(colorer PRIVATE src)
target_include_directories(colorer PRIVATE ../WinPort)
target_include_directories(colorer PRIVATE ../far2l/far2sdk)

set_target_properties(colorer
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/colorer/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

add_custom_command(TARGET colorer POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/colorer"
)

add_subdirectory("${LIBCOLORER_TREE}" "${CMAKE_BINARY_DIR}/Colorer-library/src")

set_target_properties(colorer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES
)
