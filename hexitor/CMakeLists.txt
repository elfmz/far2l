
project(hexitor)

set(SOURCES
src/editor.cpp
src/file.cpp
src/find_dlg.cpp
src/goto_dlg.cpp
src/hex_ctl.cpp
src/history.cpp
src/keybar_ctl.cpp
src/plugin.cpp
src/progress.cpp
src/settings.cpp
src/statusbar_ctl.cpp
src/fardialog.cpp
src/farapi.cpp
)

add_library (hexitor MODULE ${SOURCES})

target_link_libraries(hexitor utils far2l)

target_compile_definitions(hexitor PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_USE_INTERNALS)

target_include_directories(hexitor PRIVATE .)
target_include_directories(hexitor PRIVATE ../far2l/far2sdk)
target_include_directories(hexitor PRIVATE ../WinPort)

set_target_properties(hexitor
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/hexitor/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

# update resource files
set(CURRENT_TARGET "hexitor")
file(GLOB_RECURSE I18N_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/i18n
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/*"
)
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/i18nindex.h"
    DEPENDS I18N_FILES
    COMMAND echo python ${CMAKE_CURRENT_SOURCE_DIR}/i18n2lng.py
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/i18n2lng.py
)

# copy resource files
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
