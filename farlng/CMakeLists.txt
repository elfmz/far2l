
project(farlng)

set(SOURCES
  src/farlng.cpp
  src/lng.common.cpp
  src/lng.convertor.cpp
  src/lng.generator.cpp
  src/lng.inserter.cpp
)

set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_C_VISIBILITY_PRESET default)

add_executable (farlng ${SOURCES})

target_link_libraries(farlng utils)
