project(far2l_ttyx)

set(SOURCES
    TTYX.cpp
)

add_executable (far2l_ttyx ${SOURCES})

target_include_directories(far2l_ttyx PRIVATE ../..)
target_include_directories(far2l_ttyx PRIVATE ../../..)
target_include_directories(far2l_ttyx PRIVATE ../../../..)
target_include_directories(far2l_ttyx PRIVATE ../../../../../utils/include)

find_package(X11 REQUIRED)

link_directories(${X11_LIBRARIES})

if (NOT DEFINED TTYXI)
    if(X11_Xi_FOUND)
        set(TTYXI "YES")
        message(STATUS "Building with TTY backend Xi support due to Xi library found at ${X11_Xi_LIB}")
    else()
        message(STATUS "Building without TTY backend Xi support due to Xi library not found")
    endif()
#   message(STATUS "Building without TTY backend XI support, configure with -DTTYXI=YES if you want it")

elseif(NOT TTYXI)
    message(STATUS "Building without TTY backend Xi support due to TTYXI=${TTYXI}")
elseif(NOT X11_Xi_FOUND)
    message(FATAL_ERROR "${ColorRed}TTYXI requested but Xi lib not found, please install it or configure with -DTTYXI=NO.${ColorNormal}")
endif()

if (TTYXI)
    include_directories(${X11_INCLUDE_DIR} ${X11_Xi_INCLUDE_PATH})
    target_link_libraries(far2l_ttyx utils ${X11_LIBRARIES} ${X11_Xi_LIB})
    target_compile_options(far2l_ttyx PUBLIC -DTTYXI)

else()
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(far2l_ttyx utils ${X11_LIBRARIES})
endif()

set_target_properties(far2l_ttyx PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}"
    PREFIX ""
    SUFFIX ".broker")


