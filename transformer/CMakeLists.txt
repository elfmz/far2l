project(transformer)

set(SOURCES
    src/Config.cpp
    src/Transformer.cpp
)

add_library(${PROJECT_NAME} MODULE ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE utils far2l)
if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    -DWINPORT_DIRECT
    -DUNICODE
    -DFAR_DONT_USE_INTERNALS
)

target_include_directories(${PROJECT_NAME} PRIVATE
    .
    ../far2l/far2sdk
    ../WinPort
    ../utils/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide"
)

file(GLOB_RECURSE RESOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)
setup_target_auxiliary_files(${PROJECT_NAME}
    RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/configs
    "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/"
)
add_dependencies(${PROJECT_NAME} copy_aux_files_for_${PROJECT_NAME})

if(TESTING)
    enable_testing()
    add_executable(transformer_config_test
        tests/ConfigTest.cpp
        src/Config.cpp
    )
    target_link_libraries(transformer_config_test PRIVATE utils)
    if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        target_link_libraries(transformer_config_test PRIVATE stdc++fs)
    endif()
    target_include_directories(transformer_config_test PRIVATE
        src
        ../utils/include
    )
    add_test(NAME transformer_config_test
        COMMAND transformer_config_test "${CMAKE_CURRENT_SOURCE_DIR}/configs/plug/formats")
endif()
