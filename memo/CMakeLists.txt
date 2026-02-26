project(memo)

set(SOURCES
    src/memo.cpp
)

add_library (${PROJECT_NAME} MODULE ${SOURCES})

target_link_libraries(${PROJECT_NAME} utils far2l)

target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE _UNICODE)

target_include_directories(${PROJECT_NAME} PRIVATE ../WinPort)
target_include_directories(${PROJECT_NAME} PRIVATE ../far2l/far2sdk)
target_include_directories(${PROJECT_NAME} PRIVATE ../far2l/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ../utils/include)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")
