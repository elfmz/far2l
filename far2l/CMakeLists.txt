
project(far2l)

add_subdirectory (bootstrap)

set(SOURCES
src/cache.cpp
src/clipboard.cpp
src/cmdline.cpp
src/copy.cpp
src/ctrlobj.cpp
src/datetime.cpp
src/delete.cpp
src/dialog.cpp
src/dirinfo.cpp
src/dizlist.cpp
src/FarDlgBuilder.cpp
src/dlgedit.cpp
src/DlgGuid.cpp
src/edit.cpp
src/editor.cpp
src/execute.cpp
src/farqueue.cpp
src/farwinapi.cpp
src/fileattr.cpp
src/fileedit.cpp
src/filefilter.cpp
src/filefilterparams.cpp
src/filelist.cpp
src/FilesSuggestor.cpp
src/fileowner.cpp
src/filepanels.cpp
src/filestr.cpp
src/filetype.cpp
src/fileview.cpp
src/findfile.cpp
src/flmodes.cpp
src/flink.cpp
src/flplugin.cpp
src/flshow.cpp
src/flupdate.cpp
src/fnparce.cpp
src/foldtree.cpp
src/frame.cpp
src/global.cpp
src/help.cpp
src/hilight.cpp
src/history.cpp
src/hmenu.cpp
src/Mounts.cpp
src/infolist.cpp
src/keybar.cpp
src/language.cpp
src/main.cpp
src/manager.cpp
src/menubar.cpp
src/message.cpp
src/mkdir.cpp
src/modal.cpp
src/namelist.cpp
src/options.cpp
src/panel.cpp
src/plist.cpp
src/poscache.cpp
src/qview.cpp
src/rdrwdsk.cpp
src/RefreshFrameManager.cpp
src/scantree.cpp
src/setattr.cpp
src/setcolor.cpp
src/stddlg.cpp
src/synchro.cpp
src/syslog.cpp
src/TPreRedrawFunc.cpp
src/treelist.cpp
src/usermenu.cpp
src/viewer.cpp
src/vmenu.cpp
src/execute_oscmd.cpp
src/InterThreadCall.cpp
src/SafeMMap.cpp
src/ViewerPrinter.cpp

src/console/AnsiEsc.cpp
src/console/keyboard.cpp
src/console/console.cpp
src/console/constitle.cpp
src/console/interf.cpp
src/console/grabber.cpp
src/console/lockscrn.cpp
src/console/palette.cpp
src/console/savescr.cpp
src/console/scrbuf.cpp
src/console/scrobj.cpp
src/console/scrsaver.cpp

src/macro/chgmmode.cpp
src/macro/macro.cpp
src/macro/syntax.cpp
src/macro/tvar.cpp

src/filemasks/CFileMask.cpp
src/filemasks/FileMasksProcessor.cpp
src/filemasks/FileMasksWithExclude.cpp

src/locale/codepage.cpp
src/locale/DetectCodepage.cpp
src/locale/xlat.cpp
src/locale/locale.cpp

src/bookmarks/Bookmarks.cpp
src/bookmarks/BookmarksMenu.cpp
src/bookmarks/BookmarksLegacy.cpp

src/cfg/config.cpp
src/cfg/ConfigRW.cpp
src/cfg/HotkeyLetterDialog.cpp

src/plugs/plugapi.cpp
src/plugs/plugins.cpp
src/plugs/PluginW.cpp
src/plugs/PluginA.cpp
src/plugs/plclass.cpp

src/vt/vtansi.cpp
src/vt/vtshell.cpp
src/vt/vtshell_translation.cpp
src/vt/vtshell_compose.cpp
src/vt/vtshell_leader.cpp
src/vt/vtlog.cpp
src/vt/vtcompletor.cpp
src/vt/VTFar2lExtensios.cpp

src/aux/DList.cpp
src/aux/format.cpp
src/aux/udlist.cpp
src/aux/cvtname.cpp
src/aux/cddrv.cpp
src/aux/chgprior.cpp
src/aux/MountInfo.cpp
src/aux/FARString.cpp
src/aux/dirmix.cpp
src/aux/drivemix.cpp
src/aux/farrtl.cpp
src/aux/mix.cpp
src/aux/panelmix.cpp
src/aux/pathmix.cpp
src/aux/processname.cpp
src/aux/RegExp.cpp
src/aux/strmix.cpp


)

# Set target names for the executables
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IDENTIFIER})
    set(MACOSX_BUNDLE_ICON_FILE "DE/Far.icns")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})
    add_executable(far2l MACOSX_BUNDLE ${SOURCES} "DE/Far.icns")
    set_target_properties(far2l PROPERTIES
        OUTPUT_NAME ${APP_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${APP_DIR})
    set_source_files_properties("DE/Far.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
else()
    add_executable(far2l ${SOURCES})
    set_target_properties(far2l PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}")
endif()

if (NOT DEFINED USEUCD)
	set(USEUCD "YES")
endif()

if (NOT ${USEUCD})
	message(STATUS "Building without libuchardet due to USEUCD=${USEUCD}")
else()
	message(STATUS "Building with libuchardet")
	find_package(Uchardet REQUIRED)
	target_compile_definitions(far2l PRIVATE -DUSEUCD)
endif()

target_compile_definitions(far2l PRIVATE -DUNICODE)

target_include_directories(far2l PRIVATE
	.
	src
	src/aux
	src/bookmarks
	src/cfg
	src/console
	src/filemasks
	src/locale
	src/macro
	src/plugs
	src/vt
	${CMAKE_CURRENT_BINARY_DIR}
	${UCHARDET_INCLUDE_DIRS}
	../WinPort)

add_dependencies(far2l bootstrap WinPort)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET (WINPORT -force_load WinPort wineguts utils)
else()
    SET (WINPORT -Wl,--whole-archive WinPort -Wl,--no-whole-archive)
endif()

set_target_properties(far2l
    PROPERTIES
    ENABLE_EXPORTS TRUE)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_libraries(far2l
	PRIVATE ${WINPORT}
	PRIVATE c
	PRIVATE ${UCHARDET_LIBRARIES})
else()
    target_link_libraries(far2l
	PRIVATE ${WINPORT}
	PRIVATE dl
	PRIVATE ${UCHARDET_LIBRARIES})
endif()

add_custom_command(TARGET far2l POST_BUILD
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2l_askpass
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2l_sudoapp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#TODO
else()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DE/far2l.desktop" "${CMAKE_CURRENT_BINARY_DIR}/DE/far2l.desktop")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DE/icons" DESTINATION "share" USE_SOURCE_PERMISSIONS COMPONENT desktop FILES_MATCHING PATTERN "*")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DE/far2l.desktop" DESTINATION "share/applications" COMPONENT desktop)
endif()
