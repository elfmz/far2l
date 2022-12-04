project(inside)

set(SOURCES
src/inside.cpp
src/Globals.cpp
src/Commands.cpp
src/ItemList.cpp
src/PluginImpl.cpp
src/Storage.cpp

src/plain/PluginImplPlain.cpp

)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(SOURCES
		${SOURCES}
		src/elf/Dumper.cpp
		src/elf/PluginImplELF.cpp
	)
endif()




add_library (inside MODULE ${SOURCES})

target_link_libraries(inside utils far2l)

target_include_directories(inside PRIVATE src)
target_include_directories(inside PRIVATE src/elf)
target_include_directories(inside PRIVATE src/plain)
target_include_directories(inside PRIVATE ../WinPort)
target_include_directories(inside PRIVATE ../far2l/far2sdk)

target_compile_definitions(inside PRIVATE)

set_target_properties(inside
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/inside/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET inside POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/inside")
