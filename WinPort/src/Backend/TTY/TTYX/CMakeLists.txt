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
include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})
target_link_libraries(far2l_ttyx utils ${X11_LIBRARIES})

set_target_properties(far2l_ttyx PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}"
	PREFIX ""
	SUFFIX ".broker")


