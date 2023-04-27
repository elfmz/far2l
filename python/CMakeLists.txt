
project(python)

set(SOURCES
src/python.cpp
${CMAKE_CURRENT_BINARY_DIR}/incpy/far2lcffi.py
)

if (DEFINED VIRTUAL_PYTHON)
    message(STATUS "VIRTUAL env interpreter=${VIRTUAL_PYTHON}")
    message(STATUS "VIRTUAL env interpreter version=${VIRTUAL_PYTHON_VERSION}")
    # for finding adequate python package
    set(Python3_EXECUTABLE ${VIRTUAL_PYTHON})
endif()

find_package(Python3 COMPONENTS Interpreter Development)        # find Python interpreter and library
message(STATUS "Python3_VERSION = ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
message(STATUS "Python3_EXECUTABLE = ${Python3_EXECUTABLE}")
message(STATUS "Python3_STDLIB     = ${Python3_STDLIB}")
message(STATUS "Python3_STDARCH    = ${Python3_STDARCH}")
message(STATUS "Python3_SITELIB    = ${Python3_SITELIB}")
message(STATUS "Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARY_DIRS = ${Python3_LIBRARY_DIRS}")
message(STATUS "Python3_LIBRARIES = ${Python3_LIBRARIES}")
message(STATUS "Python3_RUNTIME_LIBRARY_DIRS = ${Python3_RUNTIME_LIBRARY_DIRS}")

add_library (python MODULE ${SOURCES})

target_link_libraries(python utils far2l ${Python3_LIBRARIES})

target_compile_definitions(python PRIVATE
    -DWINPORT_DIRECT
    -DUNICODE
    -DFAR_USE_INTERNALS
    -DPROCPLUGINMACROFUNC
    -DPYTHON_LIBRARY="${Python3_LIBRARIES}"
    -DPYTHON_VERSION="${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}"
)
if (DEFINED VIRTUAL_PYTHON)
    target_compile_definitions(python PRIVATE
        -DVIRTUAL_PYTHON="${VIRTUAL_PYTHON}"
    )
    set(Python_EXECUTABLE ${VIRTUAL_PYTHON})
else()
    set(VIRTUAL_PYTHON_VERSION 0)
    set(Python_EXECUTABLE ${Python3_EXECUTABLE})
endif()


target_include_directories(python PRIVATE .)
target_include_directories(python PRIVATE ../WinPort)
target_include_directories(python PRIVATE ../far2l/far2sdk)
target_include_directories(python PRIVATE ${Python3_INCLUDE_DIRS} )

set_target_properties(python PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/python/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide"
)

add_custom_command(TARGET python POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/far2lcffi.py
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/configs" "${INSTALL_DIR}/Plugins/python"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/incpy/far2lcffi.py" "${INSTALL_DIR}/Plugins/python/plug/far2l"
)
if (NOT DEFINED VIRTUAL_PYTHON)
    add_custom_command(TARGET python POST_BUILD
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/far2lcffi.py
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/python" "${INSTALL_DIR}/Plugins/python/plug/python"
    )
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/incpy/far2lcffi.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/pythongen.py
    DEPENDS ../far2l/far2sdk/farplug-wide.h
    DEPENDS ../far2l/far2sdk/farcolor.h
    DEPENDS ../far2l/far2sdk/farkeys.h
    DEPENDS ../WinPort/windows.h
    DEPENDS ../WinPort/WinCompat.h
    DEPENDS ../WinPort/WinPort.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/prebuild.sh "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" "${VIRTUAL_PYTHON_VERSION}" "${Python_EXECUTABLE}" "${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS}"
)
