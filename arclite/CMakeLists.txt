
project(arclite)

set(SOURCES
    archive.cpp
	attr.cpp
	cmdline.cpp
	common.cpp
    headers.cpp
	delete.cpp
    extract.cpp
	farutils.cpp
    plugin.cpp
	msearch.cpp
	open.cpp
	options.cpp
	Patch7zCP.cpp
	pathutils.cpp
	rsrc.cpp
	sfx.cpp
	SimpleXML.cpp
	strutils.cpp
	sysutils.cpp
	ui.cpp
	update.cpp
	test.cpp
    MyWindows.cpp
)

#set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
#set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

add_library (arclite MODULE ${SOURCES})

target_compile_definitions(arclite PRIVATE -DNDEBUG)

target_link_libraries(arclite utils far2l)

target_include_directories(arclite PRIVATE 
    ../WinPort
    ../far2l/far2sdk 
    ../far2l/src/base
)

target_precompile_headers(arclite PRIVATE headers.hpp)
set_target_properties(arclite
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/arclite/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

add_custom_command(TARGET arclite POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/arclite")
