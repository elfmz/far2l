
project(tmppanel)

set(SOURCES
src/TmpCfg.cpp 
src/TmpClass.cpp 
src/TmpMix.cpp 
src/TmpPanel.cpp
)

add_library (tmppanel MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(tmppanel utils far2l)

target_compile_definitions(tmppanel PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(tmppanel PRIVATE .)
target_include_directories(tmppanel PRIVATE ../far2l)
target_include_directories(tmppanel PRIVATE ../far2l/Include)
target_include_directories(tmppanel PRIVATE ../WinPort)

set_target_properties(tmppanel
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/tmppanel/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")

add_custom_command(TARGET tmppanel POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/tmppanel"
)
