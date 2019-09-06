project(inside)

set(SOURCES
src/inside.cpp
src/Globals.cpp
src/Commands.cpp
src/PluginImpl.cpp

src/plain/PluginImplPlain.cpp

)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(SOURCES
		${SOURCES}
		src/elf/Dumper.cpp
		src/elf/Storage.cpp
		src/elf/PluginImplELF.cpp
	)
endif()




add_library (inside MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(inside utils FARStdlib far2l)

target_include_directories(inside PRIVATE src)
target_include_directories(inside PRIVATE src/elf)
target_include_directories(inside PRIVATE src/plain)
target_include_directories(inside PRIVATE ../FARStdlib/include)
target_include_directories(inside PRIVATE ../WinPort)
target_include_directories(inside PRIVATE ../far2l)
target_include_directories(inside PRIVATE ../far2l/Include)

target_compile_definitions(inside PRIVATE)

set_target_properties(inside
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/inside/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET inside POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/inside")
