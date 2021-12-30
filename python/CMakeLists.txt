
project(python)

set(SOURCES
src/python.cpp
_pyfar.cpp
)

find_package(PythonLibs 3.0)

add_library (python MODULE ${SOURCES})

target_link_libraries(python utils far2l ${PYTHON_LIBRARIES})

target_compile_definitions(python PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_USE_INTERNALS -DPROCPLUGINMACROFUNC -DPYTHON_LIBRARY="${PYTHON_LIBRARY}")

target_include_directories(python PRIVATE .)
target_include_directories(python PRIVATE ../WinPort)
target_include_directories(python PRIVATE ../far2l)
target_include_directories(python PRIVATE ../far2l/fpsdk/)
target_include_directories(python PRIVATE ${PYTHON_INCLUDE_DIRS} )

set_target_properties(python PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/python/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide"
)

execute_process(
    COMMAND ${PYTHON3} -m venv --system-site-packages ${CMAKE_CURRENT_SOURCE_DIR}/configs/plug/python
)
execute_process(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/configs/plug/python/bin/python -m pip install cffi debugpy
)

add_custom_command(TARGET python POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/python"
)

add_custom_command(OUTPUT _pyfar.cpp
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/configs/plug/python/bin/python ${CMAKE_CURRENT_SOURCE_DIR}/src/pythongen.py ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
