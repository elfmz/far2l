# Unit Testing Configuration for far2l

# Only build if TESTING is enabled
if(TESTING)
    # Create unit test executable
    add_executable(far2l_unit_tests
        test_tty_input_parser.cpp
        test_tty_backend.cpp
        # Core module tests
        test_winport.cpp
        test_file_operations.cpp
        test_string_utils.cpp
        test_plugin_api.cpp
        test_utils.cpp
        test_cregexp_hardening.cpp
        test_textparser_benchmarks.cpp
        
        # Main function
        main.cpp
        # Stub symbols for WinPort references that resolve in the far2l executable
        stubs.cpp
    )
    
    
    # Link with Google Test
    # Link with Google Test and WinPort
    target_link_libraries(far2l_unit_tests
        PRIVATE
        GoogleTest::gtest
        GoogleTest::gmock
        GoogleTest::gtest_main
        WinPort
    )
    
    # Include directories
    target_include_directories(far2l_unit_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/WinPort/src
        ${CMAKE_SOURCE_DIR}/WinPort/src/Backend
        ${CMAKE_SOURCE_DIR}/WinPort/src/Backend/TTY
        ${CMAKE_SOURCE_DIR}/WinPort
        ${CMAKE_SOURCE_DIR}/far2l
        ${CMAKE_SOURCE_DIR}/utils/include
    )

    target_compile_definitions(far2l_unit_tests
        PRIVATE
        FAR2L_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    # Compiler flags
    target_compile_options(far2l_unit_tests
        PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
    )
    
    # Add test target
    add_test(NAME far2l_unit_tests
        COMMAND far2l_unit_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    message(STATUS "Unit testing framework configured")
    
endif()
