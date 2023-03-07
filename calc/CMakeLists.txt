project(calc)

set(SOURCES
    src/plugcalc/api.cpp
    src/plugcalc/api.h
    src/plugcalc/api-far2.cpp
    src/plugcalc/calc.cpp
    src/plugcalc/calc.h
    src/plugcalc/calc.rc
    src/plugcalc/config.cpp
    src/plugcalc/config.h
    src/plugcalc/messages.h
    src/plugcalc/newparse.cpp
    src/plugcalc/newparse.h
    src/plugcalc/sarg.cpp
    src/plugcalc/sarg.h
    src/plugcalc/syntax.cpp
    src/plugcalc/syntax.h
    src/plugcalc/version.h
    src/shared/mathexpression/MathExpressionBase.h
    src/shared/sgml/sgml.cpp
    src/shared/trex/trex.c
    src/shared/trex/trex.h
    src/shared/ttmath/ttmath.h
    src/shared/ttmath/ttmathbig.h
    src/shared/ttmath/ttmathint.h
    src/shared/ttmath/ttmathobjects.h
    src/shared/ttmath/ttmathparser.h
    src/shared/ttmath/ttmathtypes.h
    src/shared/ttmath/ttmathuint.h
    src/shared/ttmath/ttmathuint_noasm.h
)

#====================================================
# build
#====================================================

add_library (${PROJECT_NAME} MODULE ${SOURCES})

target_link_libraries(${PROJECT_NAME} utils far2l)

target_compile_definitions(${PROJECT_NAME} PUBLIC TTMATH_NOASM UNICODE _UNICODE)

target_include_directories(${PROJECT_NAME} PRIVATE src/shared)
target_include_directories(${PROJECT_NAME} PRIVATE ../WinPort)
target_include_directories(${PROJECT_NAME} PRIVATE ../far2l/far2sdk)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/plug")

