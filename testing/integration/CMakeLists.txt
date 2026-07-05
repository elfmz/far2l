# Integration Testing Configuration for far2l

# Only build if TESTING is enabled
if(TESTING)

    # TODO: uncomment when vt-partial-diff branch merges TTYCaps::synchronized_updates
    # add_executable(far2l_integration_tests
    #     test_tty_caps_decrqm.cpp
    # )
    #
    # target_link_libraries(far2l_integration_tests
    #     PRIVATE
    #     GoogleTest::gtest
    #     GoogleTest::gmock
    #     GoogleTest::gtest_main
    #     WinPort
    # )
    #
    # target_include_directories(far2l_integration_tests
    #     PRIVATE
    #     ${CMAKE_SOURCE_DIR}
    #     ${CMAKE_SOURCE_DIR}/WinPort/src
    #     ${CMAKE_SOURCE_DIR}/WinPort/src/Backend
    #     ${CMAKE_SOURCE_DIR}/WinPort/src/Backend/TTY
    #     ${CMAKE_SOURCE_DIR}/WinPort
    #     ${CMAKE_SOURCE_DIR}/far2l
    #     ${CMAKE_SOURCE_DIR}/utils/include
    # )
    #
    # target_compile_options(far2l_integration_tests
    #     PRIVATE
    #     -Wall
    #     -Wextra
    #     -Werror
    #     -Wno-unused-parameter
    # )
    #
    # add_test(NAME far2l_integration_tests
    #     COMMAND far2l_integration_tests
    #     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    # )
    #
    # message(STATUS "Integration testing framework configured")

endif()
