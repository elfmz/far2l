project(colorer)


set(LIBCOLORER_TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/Colorer-library/src")

include(src/Colorer-library/cmake/utils.cmake)

colorer_extract_version()
message(STATUS "Build Colorer: ${COLORER_VERSION}")

set(SOURCES
    src/pcolorer2/ChooseTypeMenu.h
    src/pcolorer2/ConsoleAnnotation.h
    src/pcolorer2/FarEditor.h
    src/pcolorer2/FarEditorSet.h
    src/pcolorer2/FarHrcSettings.h
    src/pcolorer2/pcolorer.h
    src/pcolorer2/registry_wide.h
    src/pcolorer2/resource.h
    src/pcolorer2/StringBuffer.h
    src/pcolorer2/tools.h
    src/pcolorer2/ChooseTypeMenu.cpp
    src/pcolorer2/FarEditor.cpp
    src/pcolorer2/FarEditorSet.cpp
    src/pcolorer2/FarHrcSettings.cpp
    src/pcolorer2/pcolorer.cpp
    src/pcolorer2/registry_wide.cpp
    src/pcolorer2/StringBuffer.cpp
    src/pcolorer2/tools.cpp
)

add_library(colorer MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

FIND_PACKAGE(Spdlog REQUIRED)
FIND_PACKAGE(XercesC REQUIRED)

# sometimes libstd requires linking to libfmt, sometimes not
# so, if libfmt there - lets link to it, if not - then dont care
FIND_PACKAGE(fmt)

# using fmt_LIBRARY directrly target_link_libraries fails build
# so have to copy it into intermediate var but only if fmt_FOUND
if (fmt_FOUND)
    set(fmt_LIBRARY_OPTIONAL "${fmt_LIBRARY}")
else()
    set(fmt_LIBRARY_OPTIONAL "")
endif()

target_include_directories(colorer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Colorer-library/src
    PUBLIC ${SPDLOG_INCLUDE_DIR}
    PUBLIC ${XERCESC_INCLUDE_DIR}
    )

target_link_libraries(colorer
    ${XERCESC_LIBRARIES}
    ${fmt_LIBRARY_OPTIONAL}
    utils
    colorer_lib
    )

set (SPDLOG_DEFINES
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
    message("Forcing SPDLOG_FMT_EXTERNAL cuz FMT installed and missing ${SPDLOG_INCLUDE_DIR}/spdlog/fmt/bundled")
  else()
    message("NOT forcing SPDLOG_FMT_EXTERNAL cuz FMT NOT installed")
  endif()
endif()

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
target_include_directories(colorer PRIVATE ../far2l)
target_include_directories(colorer PRIVATE ../far2l/Include)

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

