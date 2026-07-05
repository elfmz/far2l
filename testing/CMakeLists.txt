# Testing Configuration for far2l
# This file configures the unit, integration, and performance testing frameworks

# Only build tests if TESTING is enabled
# GoogleTest is fetched here (not in the unit/ integration/ benchmarks/
# subdirectories) so a single FetchContent_MakeAvailable call serves all
# test targets.  The include MUST stay inside if(TESTING): when TESTING is
# OFF (e.g. the _ci/Dockerfile.* CI builds) the FetchContent git clone must
# not run — those images may lack git and have no network, which previously
# aborted cmake configuration with "could not find git for clone of
# googletest-populate".
if(TESTING)

    include(../cmake/FindGoogleTest.cmake)

    # Unit Testing Configuration
    add_subdirectory(unit)

    # Integration Testing Configuration
    add_subdirectory(integration)

    # Performance Benchmarking Configuration
    add_subdirectory(benchmarks)

endif()