set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

cmake_minimum_required (VERSION 3.0.2)

project (FAR)

set(APP_NAME "far2l")
set(APP_IDENTIFIER "com.far2l")
set(CPACK_PACKAGE_NAME "far2l")
set(VERSION_MAJOR 2)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

string(ASCII 27 Esc)
set(ColorNormal "${Esc}[m")
set(ColorRed "${Esc}[1;31m")


if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
   if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CMAKE_INSTALL_PREFIX "/usr/local")
   elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
      set(CMAKE_INSTALL_PREFIX "/usr/local")
   else()
      set(CMAKE_INSTALL_PREFIX "/usr")
   endif()
endif()

##############################

if (NOT DEFINED USEWX)
	set(USEWX "YES")
endif()

if (NOT DEFINED EACP)
	set(EACP "YES")
endif()

if (NOT DEFINED LEGACY)
	# TODO: default it to NO  somewhere in 2022, after all obsolete settings
	# registry-upgrade-to-ini stuff will be safe to remove
	set(LEGACY "YES")
elseif(FARFTP)
	set(LEGACY "YES")
	message(STATUS "Force-enable LEGACY due to enabled FARFTP")
endif()

if (NOT ${EACP})
	message(STATUS "${ColorRed}Building without East Asian codepages support due to EACP=${EACP}${ColorNormal}")
endif()

if (NOT ${USEWX})
	message(STATUS "Building without wxWidgets backend due to USEWX=${USEWX}")
	set(USEWX_INCS "")
	set(USEWX_LIBS "")
	set(USEWX_CXX_FLAGS "")
	set(USEWX_USEFILE "")
else()
	message(STATUS "Building with wxWidgets backend")
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
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		if (EXISTS "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wx-config")
			# MacPorts
			set(wxWidgets_CONFIG_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wx-config")
			set(wxWidgets_wxrc_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wxrc")
			message(STATUS "Using known location of wx-config 3.1")
		else()
			message(STATUS "No wx-config in expected locations")
		endif()
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		find_package(wxWidgets REQUIRED net core base adv)
	else()
		find_package(wxWidgets REQUIRED net core base)
	endif()

	include(${wxWidgets_USE_FILE})

	set(USEWX_USEFILE ${wxWidgets_USE_FILE})
	set(USEWX_INCS ${wxWidgets_INCLUDE_DIRS})
	set(USEWX_LIBS ${wxWidgets_LIBRARIES})
	set(USEWX_CXX_FLAGS ${wxWidgets_CXX_FLAGS})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    find_program(M4_PROG NAMES gm4)
else()
    find_program(M4_PROG NAMES m4)
endif()
if(NOT M4_PROG)
    message(FATAL_ERROR "${ColorRed}gawk not found - please install it${ColorNormal}")
endif ()
set(M4 ${M4_PROG} -P "-DARCH=${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" "-DMAJOR=${VERSION_MAJOR}" "-DMINOR=${VERSION_MINOR}" "-DPATCH=${VERSION_PATCH}")

# set(RM rm) actually not needed: use $(CMAKE_COMMAND) -E remove
find_program (GAWK NAMES gawk)
if(NOT GAWK)
  message(FATAL_ERROR "${ColorRed}gawk not found - please install it${ColorNormal}")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wno-unused-function -Wno-c++11-narrowing -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fPIC -Wno-unused-function -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
set(TOOLS_DIR ${CMAKE_BINARY_DIR}/tools)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(APP_DIR ${CMAKE_BINARY_DIR}/install)
	set(INSTALL_DIR ${APP_DIR}/${APP_NAME}.app/Contents/MacOS)
	set(EXECUTABLE_NAME ${APP_NAME})
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -Wl,--gc-sections")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wl,--gc-sections")
	set(INSTALL_DIR ${CMAKE_BINARY_DIR}/install)
	set(EXECUTABLE_NAME far2l)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600")
  message(STATUS "Running under cygwin, wow!")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if (LEGACY)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWINPORT_REGISTRY")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWINPORT_REGISTRY")
	message(STATUS "Enabling registry support due to enabled LEGACY")
else()
	message(STATUS "LEGACY is not enabled, your old registry settings will be forgotten, if any")
endif()

if(CMAKE_VERSION VERSION_GREATER "3.3")
  cmake_policy(SET CMP0063 NEW)
  cmake_policy(SET CMP0057 NEW)
endif()

include_directories(utils/include)

add_subdirectory (farlng)
add_subdirectory (WinPort)
add_subdirectory (utils)
add_subdirectory (far2l)
add_dependencies (far2l farlng)

set_target_properties(farlng
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TOOLS_DIR}"
)

##############################
# plugins directives

if (NOT DEFINED ALIGN OR ALIGN)
	message(STATUS "ALIGN plugin enabled")
	add_subdirectory (align)
else()
	message(STATUS "${ColorRed}ALIGN plugin disabled due to ALIGN=${ALIGN}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/align/plug/align.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED AUTOWRAP OR AUTOWRAP)
	message(STATUS "AUTOWRAP plugin enabled")
	add_subdirectory (autowrap)
else()
	message(STATUS "${ColorRed}AUTOWRAP plugin disabled due to AUTOWRAP=${AUTOWRAP}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/autowrap/plug/autowrap.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED CALC OR CALC)
	message(STATUS "CALC plugin enabled")
	add_subdirectory (calc)
else()
	message(STATUS "${ColorRed}CALC plugin disabled due to CALC=${CALC}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/calc/plug/calc.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED COLORER OR COLORER)
	message(STATUS "COLORER plugin enabled")
	add_subdirectory (colorer)
else()
	message(STATUS "${ColorRed}COLORER plugin disabled due to COLORER=${COLORER}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/colorer/plug/colorer.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED COMPARE OR COMPARE)
	message(STATUS "COMPARE plugin enabled")
	add_subdirectory (compare)
else()
	message(STATUS "${ColorRed}COMPARE plugin disabled due to COMPARE=${COMPARE}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/compare/plug/compare.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED DRAWLINE OR DRAWLINE)
	message(STATUS "DRAWLINE plugin enabled")
	add_subdirectory (drawline)
else()
	message(STATUS "${ColorRed}DRAWLINE plugin disabled due to DRAWLINE=${DRAWLINE}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/drawline/plug/drawline.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED EDITCASE OR EDITCASE)
	message(STATUS "EDITCASE plugin enabled")
	add_subdirectory (editcase)
else()
	message(STATUS "${ColorRed}EDITCASE plugin disabled due to EDITCASE=${EDITCASE}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/editcase/plug/editcase.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED EDITORCOMP OR EDITORCOMP)
	message(STATUS "EDITORCOMP plugin enabled")
	add_subdirectory (editorcomp)
else()
	message(STATUS "${ColorRed}EDITORCOMP plugin disabled due to EDITORCOMP=${EDITORCOMP}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/editorcomp/plug/editorcomp.far-plug-wide)
		" COMPONENT system)
endif()

if (DEFINED FARFTP AND FARFTP)
	message(STATUS "FARFTP plugin enabled (obsolete)")
	add_subdirectory (FARStdlib)
	add_subdirectory (farftp)
else()
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/farftp/plug/farftp.far-plug-mb)
		" COMPONENT system)
endif()

if (NOT DEFINED FILECASE OR FILECASE)
	message(STATUS "FILECASE plugin enabled")
	add_subdirectory (filecase)
else()
	message(STATUS "${ColorRed}FILECASE plugin disabled due to FILECASE=${FILECASE}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/filecase/plug/filecase.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED INCSRCH OR INCSRCH)
	message(STATUS "INCSRCH plugin enabled")
	add_subdirectory (incsrch)
else()
	message(STATUS "${ColorRed}Building without <INCSRCH> plugin due to INCSRCH=${INCSRCH}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/incsrch/plug/incsrch.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED INSIDE OR INSIDE)
	message(STATUS "INSIDE plugin enabled")
	add_subdirectory (inside)
else()
	message(STATUS "${ColorRed}INSIDE plugin disabled due to INSIDE=${INSIDE}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/inside/plug/inside.far-plug-mb)
		" COMPONENT system)
endif()

if (NOT DEFINED MULTIARC OR MULTIARC)
	message(STATUS "MULTIARC plugin enabled")
	# workaround for brew's/macports' libarchive
	if(IS_DIRECTORY "/usr/local/opt/libarchive/include")
		set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
		set(LibArchive_LIBRARY "/usr/local/opt/libarchive/lib/libarchive.dylib")
	elseif(EXISTS "/opt/local/lib/libarchive.dylib")
		set(LibArchive_LIBRARY "/opt/local/lib/libarchive.dylib")
	endif()

	find_package(LibArchive)
	if(NOT LibArchive_FOUND)
		message(WARNING "${ColorRed}libarchive not found, multiarc will have weaker archives support. Its recommended to install libarchive-dev and reconfigure far2l.${ColorNormal}")
	endif()

	find_package(PCRE)
	if(NOT PCRE_FOUND)
		message(WARNING "${ColorRed}libpcre not found, multiarc will have no custom archives support. Install libpcre and reconfigure far2l if you need that functionality.${ColorNormal}")
	endif()

	add_subdirectory (multiarc)
else()
	message(STATUS "${ColorRed}MULTIARC plugin disabled due to MULTIARC=${MULTIARC}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/multiarc/plug/multiarc.far-plug-mb)
		" COMPONENT system)
endif()

if (NOT DEFINED NETROCKS OR NETROCKS)
	message(STATUS "NETROCKS plugin enabled")
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		# workaround for brew/macports vs system openssl conflicts
		if(IS_DIRECTORY "/usr/local/opt/openssl@1.1/include" AND IS_DIRECTORY "/usr/local/opt/openssl@1.1/lib")
			set(OPENSSL_LIBRARIES "/usr/local/opt/openssl@1.1/lib/libcrypto.dylib;/usr/local/opt/openssl@1.1/lib/libssl.dylib")
			set(OPENSSL_INCLUDE_DIRS "/usr/local/opt/openssl@1.1/include")
		elseif(EXISTS "/opt/local/lib/libssl.dylib" AND IS_DIRECTORY "/opt/local/include/openssl")
			set(OPENSSL_LIBRARIES "/opt/local//lib/libcrypto.dylib;/opt/local/lib/libssl.dylib")
			set(OPENSSL_INCLUDE_DIRS "/opt/local/include")
		endif()
	endif()

	find_package(OpenSSL)
	if(OPENSSL_FOUND)
		message(STATUS "OpenSSL found -> enjoy FTPS support in NetRocks")
	else()
		message(WARNING "${ColorRed}OpenSSL not found, NetRocks will not have FTPS protocol support. Install libssl-dev if you want FTPS protocol available in NetRocks.${ColorNormal}")
	endif(OPENSSL_FOUND)

	find_package(LibSSH 0.5.0)
	if(LIBSSH_FOUND)
		message(STATUS "libssh found -> enjoy SFTP support in NetRocks")
	else()
		message(WARNING "${ColorRed}libssh not found, NetRocks will not have SFTP protocol support. Install libssh-dev if you want SFTP protocol available in NetRocks.${ColorNormal}")
	endif(LIBSSH_FOUND)

	find_package(Libsmbclient)
	if(LIBSMBCLIENT_FOUND)
		message(STATUS "libsmbclient found -> enjoy SMB support in NetRocks")
	else()
		message(WARNING "${ColorRed}libsmbclient not found, NetRocks will not have SMB protocol support. Install libsmbclient-dev if you want SMB protocol available in NetRocks.${ColorNormal}")
	endif(LIBSMBCLIENT_FOUND)

	find_package(LibNfs)
	if(LIBNFS_FOUND)
		message(STATUS "libnfs found -> enjoy NFS support in NetRocks")
	else()
		message(WARNING "${ColorRed}libnfs not found, NetRocks will not have NFS protocol support. Install libnfs-dev if you want NFS protocol available in NetRocks.${ColorNormal}")
	endif(LIBNFS_FOUND)

	find_package(LibNEON)
	if(LIBNEON_FOUND)
		message(STATUS "libneon found -> enjoy WebDav support in NetRocks")
	else()
		message(WARNING "${ColorRed}libneon not found, NetRocks will not have WebDav protocol support. Install libneon*-dev if you want WebDav protocol available in NetRocks.${ColorNormal}")
	endif(LIBNEON_FOUND)

	add_subdirectory (NetRocks)
else()
	message(STATUS "${ColorRed}NETROCKS plugin disabled due to NETROCKS=${NETROCKS}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/NetRocks/plug/NetRocks.far-plug-wide)
		" COMPONENT system)
endif()

if (DEFINED PYTHON AND PYTHON)
	find_package(PythonLibs 3.0)

	if(PYTHONLIBS_FOUND)
		find_program (VIRTUALENV NAMES virtualenv)
		find_program (PYTHON3 NAMES python3)
		if(VIRTUALENV AND PYTHON3)
			message(STATUS "PYTHON plugin enabled")
			add_subdirectory (python)
		else()
			message(WARNING "${ColorRed}PYTHON plugin disabled due to virtualenv or python3 not found${ColorNormal}")
		endif ()
	else()
		message(WARNING "${ColorRed}PYTHON plugin disabled due to Python libraries not found${ColorNormal}")
	endif()
else()
	message(STATUS "${ColorRed}PYTHON plugin disabled, use -DPYTHON=yes if you need it${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/python/plug/python.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED SIMPLEINDENT OR SIMPLEINDENT)
	message(STATUS "SIMPLEINDENT plugin enabled")
	add_subdirectory (SimpleIndent)
else()
	message(STATUS "${ColorRed}SIMPLEINDENT plugin disabled due to SIMPLEINDENT=${SIMPLEINDENT}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/SimpleIndent/plug/SimpleIndent.far-plug-wide)
		" COMPONENT system)
endif()

if (NOT DEFINED TMPPANEL OR TMPPANEL)
	message(STATUS "TMPPANEL plugin enabled")
	add_subdirectory (tmppanel)
else()
	message(STATUS "${ColorRed}TMPPANEL plugin disabled due to TMPPANEL=${TMPPANEL}${ColorNormal}")
	install(CODE "
		execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/tmppanel/plug/tmppanel.far-plug-wide)
		" COMPONENT system)
endif()

##############################
# common install directives

add_subdirectory (packaging)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(FIXUP_SCRIPT packaging/osx/FixupBundle.cmake)
	configure_file(${FIXUP_SCRIPT} ${PROJECT_BINARY_DIR}/${FIXUP_SCRIPT} @ONLY)
	install(DIRECTORY ${APP_DIR}/${APP_NAME}.app DESTINATION . USE_SOURCE_PERMISSIONS COMPONENT app EXCLUDE_FROM_ALL)
	install(SCRIPT ${PROJECT_BINARY_DIR}/${FIXUP_SCRIPT} COMPONENT app EXCLUDE_FROM_ALL)
endif()

install(PROGRAMS "${INSTALL_DIR}/${EXECUTABLE_NAME}" DESTINATION "bin" RENAME far2l COMPONENT base)

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "lib/far2l" USE_SOURCE_PERMISSIONS COMPONENT base FILES_MATCHING
	PATTERN "colorer/base" EXCLUDE
	PATTERN "plug/*.far-plug-*"
	PATTERN "plug/*.broker")

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "share/far2l" USE_SOURCE_PERMISSIONS COMPONENT base FILES_MATCHING
	PATTERN "${EXECUTABLE_NAME}" EXCLUDE
	PATTERN "far2l_*" EXCLUDE
	PATTERN "*.far-plug-*" EXCLUDE
	PATTERN "*.broker" EXCLUDE
	PATTERN "*")

# setup some symlinks and remove deprecated stuff from previous installation
install(CODE "
	execute_process(COMMAND ln -sf ../../bin/far2l ${CMAKE_INSTALL_PREFIX}/lib/far2l/far2l_askpass)
	execute_process(COMMAND ln -sf ../../bin/far2l ${CMAKE_INSTALL_PREFIX}/lib/far2l/far2l_sudoapp)
	execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2l/Plugins/objinfo/plug/objinfo.far-plug-mb)
	" COMPONENT system)
