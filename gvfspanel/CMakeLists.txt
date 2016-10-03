
project(gvfspanel)

set(SOURCES
    src/PluginMain.cpp
    src/Plugin.cpp
    src/gvfsdlg.cpp
    src/MountOptions.cpp
    src/GvfsService.cpp
    src/KeyBarTitlesHelper.cpp
)

find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-2.4)

add_library (gvfspanel MODULE ${SOURCES})

set_property(TARGET gvfspanel PROPERTY CXX_STANDARD 14)
set_property(TARGET gvfspanel PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(gvfspanel utils ${GTKMM_LIBRARIES})

target_compile_definitions(gvfspanel PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(gvfspanel PRIVATE ./inc)
target_include_directories(gvfspanel PRIVATE ../far2l)
target_include_directories(gvfspanel PRIVATE ../far2l/Include)
target_include_directories(gvfspanel PRIVATE ../WinPort)
target_include_directories(gvfspanel PRIVATE ${GTKMM_INCLUDE_DIRS})

set_target_properties(gvfspanel
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/gvfspanel/bin"
    PREFIX ""
    SUFFIX ".far-plug-wide")

add_custom_command(TARGET gvfspanel POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/gvfspanel"
)
