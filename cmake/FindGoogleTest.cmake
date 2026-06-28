# FindGoogleTest.cmake - Google Test CMake module for far2l
# Provides Google Test integration for unit testing

include(FetchContent)

# Google Test configuration
set(GOOGLETEST_VERSION "1.14.0")

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v${GOOGLETEST_VERSION}
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

# Make Google Test available
FetchContent_MakeAvailable(googletest)

# Create an interface target for Google Test
if(TARGET gtest_main AND TARGET gmock_main)
    add_library(GoogleTest::gtest ALIAS gtest)
    add_library(GoogleTest::gtest_main ALIAS gtest_main)
    add_library(GoogleTest::gmock ALIAS gmock)
    add_library(GoogleTest::gmock_main ALIAS gmock_main)
    
    message(STATUS "Google Test ${GOOGLETEST_VERSION} configured successfully")
else()
    message(WARNING "Google Test targets not available")
endif()