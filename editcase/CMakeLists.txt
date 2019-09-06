
project(editcase)

set(SOURCES
src/EditCase.cpp 
)

add_library (editcase MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(editcase utils far2l)

target_compile_definitions(editcase PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(editcase PRIVATE .)
target_include_directories(editcase PRIVATE ../far2l)
target_include_directories(editcase PRIVATE ../far2l/Include)
target_include_directories(editcase PRIVATE ../WinPort)

set_target_properties(editcase 
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/editcase/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")


add_custom_command(TARGET editcase POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/editcase"
)
