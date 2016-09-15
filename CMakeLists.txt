
cmake_minimum_required (VERSION 3.2.2)
project (FAR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -fPIC -Wno-unused-function -Wno-c++11-narrowing") #  -fsanitize=address
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fPIC -Wno-unused-function") #  -fsanitize=address
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/install)
set(TOOLS_DIR ${CMAKE_BINARY_DIR}/tools)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

include_directories(utils/include)

add_subdirectory (farlng)
add_subdirectory (WinPort)
add_subdirectory (utils)
add_subdirectory (far2l)
add_dependencies (far2l farlng)

set_target_properties(far2l
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}"
)
set_target_properties(farlng
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TOOLS_DIR}"
)

add_subdirectory (farftp)
add_subdirectory (colorer)
add_subdirectory (multiarc)
add_subdirectory (tmppanel)
