
project(python)

set(SOURCES
src/python.cpp 
)

find_package(PythonLibs 3.0)

add_library (python MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(python utils ${PYTHON_LIBRARIES})

target_compile_definitions(python PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS -DPYTHON_LIBRARY="${PYTHON_LIBRARY}")

target_include_directories(python PRIVATE .)
target_include_directories(python PRIVATE ../far2l)
target_include_directories(python PRIVATE ../far2l/Include)
target_include_directories(python PRIVATE ../WinPort)
target_include_directories(python PRIVATE ${PYTHON_INCLUDE_DIRS} )

set_target_properties(python PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/python/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide"
)


add_custom_command(TARGET python POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/python"
)

add_custom_command(TARGET python PRE_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND virtualenv -p ${PYTHON3} --system-site-packages ${CMAKE_CURRENT_SOURCE_DIR}/configs/plug/python
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/configs/plug/python/bin/python -m pip install cffi
)
