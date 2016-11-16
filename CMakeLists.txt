
cmake_minimum_required (VERSION 3.2.2)
project (FAR)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

#if(NOT CMAKE_INSTALL_PREFIX)
   set(CMAKE_INSTALL_PREFIX "/usr")
#endif()



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fPIC -Wno-unused-function -Wno-c++11-narrowing -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fPIC -Wno-unused-function -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/install)
set(TOOLS_DIR ${CMAKE_BINARY_DIR}/tools)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(CMAKE_VERSION VERSION_GREATER "3.3")
  cmake_policy(SET CMP0063 NEW)
endif()


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
add_subdirectory (SimpleIndent)

install(PROGRAMS "${INSTALL_DIR}/far2l" DESTINATION "bin" )

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "lib/far2l" USE_SOURCE_PERMISSIONS FILES_MATCHING 
	PATTERN "colorer/base" EXCLUDE 
	PATTERN "plug/*.far-plug-*")

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "share/far2l" USE_SOURCE_PERMISSIONS FILES_MATCHING 
	PATTERN "far2l" EXCLUDE 
	PATTERN "far2l_askpass" EXCLUDE 
	PATTERN "far2l_sudoapp" EXCLUDE 
	PATTERN "*.far-plug-*" EXCLUDE 
	PATTERN "*")

install(CODE "
      execute_process(COMMAND ln -s ${CMAKE_INSTALL_PREFIX}/bin/far2l ${CMAKE_INSTALL_PREFIX}/lib/far2l/far2l_askpass)
      execute_process(COMMAND ln -s ${CMAKE_INSTALL_PREFIX}/bin/far2l ${CMAKE_INSTALL_PREFIX}/lib/far2l/far2l_sudoapp)
      "
    )
