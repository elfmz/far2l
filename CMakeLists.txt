
cmake_minimum_required (VERSION 3.2.2)
project (FAR)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

#if(NOT CMAKE_INSTALL_PREFIX)
   set(CMAKE_INSTALL_PREFIX "/opt")
#endif()



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fPIC -Wno-unused-function -Wno-c++11-narrowing") #  -fsanitize=address
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fPIC -Wno-unused-function") #  -fsanitize=address
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


install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "far2l" USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN "*")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#TODO
else()
    INSTALL(FILES ${INSTALL_DIR}/far2l.desktop DESTINATION "/usr/share/applications")
    INSTALL(CODE "
      execute_process(COMMAND ln -s ${CMAKE_INSTALL_PREFIX}/far2l/far2l /bin/far2l)
      "
    )

#    find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
#   INSTALL(CODE "
#      execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor /opt/far2l/far2l.desktop)
#      "
#    )
endif()

