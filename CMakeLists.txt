set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

cmake_minimum_required (VERSION 3.0.2)

project (FAR)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

string(ASCII 27 Esc)
set(ColourReset "${Esc}[m")

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
   if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CMAKE_INSTALL_PREFIX "/usr/local")
   elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
      set(CMAKE_INSTALL_PREFIX "/usr/local")
   else()
      set(CMAKE_INSTALL_PREFIX "/usr")
   endif()
endif()

if (NOT DEFINED USEWX)
	set(USEWX "YES")
endif()

if (NOT DEFINED PYTHON)
	set(PYTHON "NO")
endif()

if (NOT DEFINED FARFTP)
	set(FARFTP "NO")
endif()

if (NOT ${USEWX})
	set(USEWX_INCS "")
	set(USEWX_LIBS "")
	set(USEWX_CXX_FLAGS "")
	set(USEWX_USEFILE "")
else()
	if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		if (DEFINED wxWidgets_CONFIG_EXECUTABLE)
			message(STATUS "Using pre-defined wx config: ${wxWidgets_CONFIG_EXECUTABLE}")

		elseif(EXISTS "/usr/local/bin/wxgtk3u-3.1-config")
			set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.1-config")
			set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.1")
			message(STATUS "Using known location of wx-config 3.1")

		elseif (EXISTS "/usr/local/bin/wxgtk3u-3.0-config")
			set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.0-config")
			set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.0")
			message(STATUS "Using known location of wx-config 3.0")

		else()
			message(STATUS "No wxgtk3u-3.*-config in expected locations")
		endif()
endif()

	find_package(wxWidgets REQUIRED net core base)
	include(${wxWidgets_USE_FILE})

	set(USEWX_USEFILE ${wxWidgets_USE_FILE})
	set(USEWX_INCS ${wxWidgets_INCLUDE_DIRS})
	set(USEWX_LIBS ${wxWidgets_LIBRARIES})
	set(USEWX_CXX_FLAGS ${wxWidgets_CXX_FLAGS})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
# workaround for brew vs system openssl conflicts
	if(IS_DIRECTORY "/usr/local/opt/openssl@1.1/include" AND IS_DIRECTORY "/usr/local/opt/openssl@1.1/lib")
		set(OPENSSL_LIBRARIES "/usr/local/opt/openssl@1.1/lib/libcrypto.dylib;/usr/local/opt/openssl@1.1/lib/libssl.dylib")
		set(OPENSSL_INCLUDE_DIRS "/usr/local/opt/openssl@1.1/include")
	endif()
endif()

find_package(OpenSSL)
if (OPENSSL_FOUND)
	message(STATUS "OpenSSL found -> enjoy FTPS support in NetRocks")
else()
	message(WARNING "${Esc}[1;31mOpenSSL not found, NetRocks will not have FTPS protocol support. Install libssl-dev if you want FTPS protocol available in NetRocks.${Esc}[39;22m")
endif (OPENSSL_FOUND)

find_package(LibSSH 0.5.0)
if(LIBSSH_FOUND)
	message(STATUS "libssh found -> enjoy SFTP support in NetRocks")
else()
	message(WARNING "${Esc}[1;31mlibssh not found, NetRocks will not have SFTP protocol support. Install libssh-dev if you want SFTP protocol available in NetRocks.${Esc}[39;22m")
endif(LIBSSH_FOUND)

find_package(Libsmbclient)
if (LIBSMBCLIENT_FOUND)
	message(STATUS "libsmbclient found -> enjoy SMB support in NetRocks")
else()
	message(WARNING "${Esc}[1;31mlibsmbclient not found, NetRocks will not have SMB protocol support. Install libsmbclient-dev if you want SMB protocol available in NetRocks.${Esc}[39;22m")
endif (LIBSMBCLIENT_FOUND)

find_package(LibNfs)
if (LIBNFS_FOUND)
	message(STATUS "libnfs found -> enjoy NFS support in NetRocks")
else()
	message(WARNING "${Esc}[1;31mlibnfs not found, NetRocks will not have NFS protocol support. Install libnfs-dev if you want NFS protocol available in NetRocks.${Esc}[39;22m")
endif (LIBNFS_FOUND)

find_package(LibNEON)
if (LIBNEON_FOUND)
	message(STATUS "libneon found -> enjoy WebDav support in NetRocks")
else()
	message(WARNING "${Esc}[1;31mlibneon not found, NetRocks will not have WebDav protocol support. Install libneon*-dev if you want WebDav protocol available in NetRocks.${Esc}[39;22m")
endif (LIBNEON_FOUND)

find_package(PCRE)
if(NOT PCRE_FOUND)
	message(WARNING "${Esc}[1;31mlibpcre not found, multiarc will have no custom archives support. Install libpcre and reconfigure far2l if you need that functionality.${Esc}[39;22m")
endif()

# workaround for brew's libarchive
if(IS_DIRECTORY "/usr/local/opt/libarchive/include")
	set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
	set(LibArchive_LIBRARY "/usr/local/opt/libarchive/lib/libarchive.a")
else()
	find_package(LibArchive)
	if(NOT LibArchive_FOUND)
		message(WARNING "${Esc}[1;31mlibarchive not found, multiarc will have weaker archives support. Its recommended to install libarchive-dev and reconfigure far2l.${Esc}[39;22m")
	endif()
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wno-unused-function -Wno-c++11-narrowing -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fPIC -Wno-unused-function -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/install)
set(TOOLS_DIR ${CMAKE_BINARY_DIR}/tools)

if (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600")
  message(STATUS "Running under cygwin, wow!")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(CMAKE_VERSION VERSION_GREATER "3.3")
  cmake_policy(SET CMP0063 NEW)
endif()

include_directories(utils/include)

add_subdirectory (farlng)
add_subdirectory (WinPort)
add_subdirectory (utils)
add_subdirectory (FARStdlib)
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

if(FARFTP)
	add_subdirectory (farftp)
endif ()

add_subdirectory (colorer)
add_subdirectory (multiarc)
add_subdirectory (tmppanel)
add_subdirectory (SimpleIndent)
add_subdirectory (editcase)
add_subdirectory (compare)
add_subdirectory (filecase)
add_subdirectory (align)
add_subdirectory (autowrap)
add_subdirectory (drawline)
add_subdirectory (editorcomp)
add_subdirectory (inside)
add_subdirectory (incsrch)
add_subdirectory (NetRocks)

if(PYTHON)
	find_package(PythonLibs 3.0)

	if(PYTHONLIBS_FOUND)
		find_program (VIRTUALENV NAMES virtualenv)
		if(VIRTUALENV)
			message(STATUS "Python plugin enabled")
			add_subdirectory (python)
		else()
			message(WARNING "${Esc}[1;31mvirtualenv not found, python plugin will not be built${Esc}[39;22m")
		endif ()
	else()
		message(WARNING "${Esc}[1;31mPython libraries not found, python plugin will not be built${Esc}[39;22m")
	endif ()
else()
	message(WARNING "${Esc}[1;31mPython plugin not enabled by default, use -DPYTHON=yes if you really want it${Esc}[39;22m")
endif ()

install(PROGRAMS "${INSTALL_DIR}/far2l" DESTINATION "bin" )

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "lib/far2l" USE_SOURCE_PERMISSIONS FILES_MATCHING 
    PATTERN "colorer/base" EXCLUDE 
    PATTERN "plug/*.far-plug-*"
    PATTERN "plug/*.broker")

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "share/far2l" USE_SOURCE_PERMISSIONS FILES_MATCHING 
    PATTERN "far2l" EXCLUDE 
    PATTERN "far2l_askpass" EXCLUDE 
    PATTERN "far2l_sudoapp" EXCLUDE 
    PATTERN "*.far-plug-*" EXCLUDE 
    PATTERN "*.broker" EXCLUDE 
    PATTERN "*")

# setup some symlinks and remove deprecated stuff from previous installation
install(CODE "
      execute_process(COMMAND ln -s ../../bin/far2l ${CMAKE_INSTALL_PREFIX}/lib/far2l/far2l_askpass)
      execute_process(COMMAND ln -s ../../bin/far2l ${CMAKE_INSTALL_PREFIX}/lib/far2l/far2l_sudoapp)
      execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/objinfo/plug/objinfo.far-plug-mb)
      "
    )

if (NOT FARFTP)
install(CODE "
      execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/farftp/plug/farftp.far-plug-mb && echo Removed existing farftp plugin)
      "
    )
endif()


if (NOT PYTHON)
install(CODE "
      execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/python/plug/python.far-plug-wide && echo Removed existing python plugin)
      "
    )
endif()
