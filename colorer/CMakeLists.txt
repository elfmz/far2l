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
)

# set options for colorer-library
set(COLORER_USE_ICU_STRINGS OFF)
set(COLORER_USE_LIBXML ON)
set(COLORER_BUILD_HARD_WARNINGS OFF)
set(COLORER_BUILD_OLD_COMPILERS ON)

if (USESPDLOG)
    FIND_PACKAGE(Spdlog REQUIRED)
    set (SPDLOG_DEFINES
        USESPDLOG
        SPDLOG_DISABLE_TID_CACHING
        SPDLOG_NO_THREAD_ID
        SPDLOG_NO_TLS
    )

    if (NOT EXISTS "${SPDLOG_INCLUDE_DIR}/spdlog/fmt/bundled")
        if (fmt_FOUND)
            set (SPDLOG_DEFINES
                ${SPDLOG_DEFINES}
                SPDLOG_FMT_EXTERNAL=ON
            )
            message("SPDLOG: using with SPDLOG_FMT_EXTERNAL cuz FMT installed but ${SPDLOG_INCLUDE_DIR}/spdlog/fmt/bundled is missing")
        else()
            message("SPDLOG: using without forced SPDLOG_FMT_EXTERNAL cuz FMT NOT installed")
        endif()
    else()
        message("SPDLOG: using without forced SPDLOG_FMT_EXTERNAL cuz ${SPDLOG_INCLUDE_DIR}/spdlog/fmt/bundled exists")
    endif()
    set(COLORER_USE_DUMMY_LOGGER OFF)
else()
    set(COLORER_USE_DUMMY_LOGGER ON)
endif()


add_library(colorer MODULE ${SOURCES})

FIND_PACKAGE(LibXml2 REQUIRED)

# sometimes libstd requires linking to libfmt, sometimes not
# so, if libfmt there - lets link to it, if not - then dont care
FIND_PACKAGE(fmt)

# using fmt_LIBRARY directrly target_link_libraries fails build
# so have to copy it into intermediate var but only if fmt_FOUND
if (fmt_FOUND)
    set(fmt_LIBRARY_OPTIONAL "${fmt_LIBRARY}")
    # add libfmt package into far2l dependencies if needed
    IF(PACKAGE_DEPENDENCIES)
        SET(PACKAGE_DEPENDENCIES "${PACKAGE_DEPENDENCIES}, libfmt${fmt_VERSION_MAJOR}" PARENT_SCOPE)
    ELSE(PACKAGE_DEPENDENCIES)
        SET(PACKAGE_DEPENDENCIES "libfmt${fmt_VERSION_MAJOR}" PARENT_SCOPE)
    ENDIF(PACKAGE_DEPENDENCIES)
else()
    set(fmt_LIBRARY_OPTIONAL "")
endif()

target_include_directories(colorer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Colorer-library/src
    )

if(USESPDLOG)
    target_include_directories(colorer PUBLIC ${SPDLOG_INCLUDE_DIR})
endif()

target_link_libraries(colorer
    ${fmt_LIBRARY_OPTIONAL}
    utils
    colorer_lib
    far2l
    )

target_compile_definitions(colorer PUBLIC
    _UNICODE
    UNICODE
    __unix__
    ${SPDLOG_DEFINES}
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
    CXX_EXTENSIONS NO
)