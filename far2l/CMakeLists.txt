
project(far2l)

add_subdirectory (bootstrap)

set(SOURCES
cache.cpp
cddrv.cpp
CFileMask.cpp
chgmmode.cpp
chgprior.cpp
clipboard.cpp
cmdline.cpp
codepage.cpp
config.cpp
console.cpp
constitle.cpp
copy.cpp
ctrlobj.cpp
cvtname.cpp
datetime.cpp
delete.cpp
dialog.cpp
dirinfo.cpp
dirmix.cpp
dizlist.cpp
DlgBuilder.cpp
dlgedit.cpp
DlgGuid.cpp
DList.cpp
drivemix.cpp
edit.cpp
editor.cpp
execute.cpp
farqueue.cpp
farrtl.cpp
farwinapi.cpp
ffolders.cpp
fileattr.cpp
fileedit.cpp
filefilter.cpp
filefilterparams.cpp
filelist.cpp
FileMasksProcessor.cpp
FileMasksWithExclude.cpp
fileowner.cpp
filepanels.cpp
filestr.cpp
filetype.cpp
fileview.cpp
findfile.cpp
flmodes.cpp
flink.cpp
flplugin.cpp
flshow.cpp
flupdate.cpp
fnparce.cpp
foldtree.cpp
format.cpp
frame.cpp
global.cpp
grabber.cpp
headers.cpp
help.cpp
hilight.cpp
history.cpp
hmenu.cpp
hotplug.cpp
infolist.cpp
interf.cpp
keybar.cpp
keyboard.cpp
language.cpp
local.cpp
lockscrn.cpp
macro.cpp
main.cpp
manager.cpp
menubar.cpp
message.cpp
mix.cpp
mkdir.cpp
modal.cpp
namelist.cpp
options.cpp
palette.cpp
panel.cpp
panelmix.cpp
pathmix.cpp
plist.cpp
plognmn.cpp
plugapi.cpp
plugins.cpp
PluginW.cpp
poscache.cpp
processname.cpp
qview.cpp
rdrwdsk.cpp
RefreshFrameManager.cpp
RegExp.cpp
registry.cpp
savefpos.cpp
savescr.cpp
scantree.cpp
scrbuf.cpp
scrobj.cpp
scrsaver.cpp
setattr.cpp
setcolor.cpp
stddlg.cpp
strmix.cpp
synchro.cpp
syntax.cpp
syslog.cpp
TPreRedrawFunc.cpp
treelist.cpp
tvar.cpp
udlist.cpp
UnicodeString.cpp
usermenu.cpp
viewer.cpp
vmenu.cpp
xlat.cpp
vtshell.cpp
vtansi.cpp
PluginA.cpp
execute_oscmd.cpp
vtshell_translation.cpp
VTFar2lExtensios.cpp
vtlog.cpp
vtcompletor.cpp
InterThreadCall.cpp
UCD/CharDistribution.cpp
UCD/JpCntx.cpp
UCD/LangBulgarianModel.cpp
UCD/LangCyrillicModel.cpp
UCD/LangGreekModel.cpp
UCD/LangHebrewModel.cpp
UCD/LangHungarianModel.cpp
UCD/LangThaiModel.cpp
UCD/nsBig5Prober.cpp
UCD/nsCharSetProber.cpp
UCD/nsEscCharsetProber.cpp
UCD/nsEscSM.cpp
UCD/nsEUCJPProber.cpp
UCD/nsEUCKRProber.cpp
UCD/nsEUCTWProber.cpp
UCD/nsGB2312Prober.cpp
UCD/nsHebrewProber.cpp
UCD/nsLatin1Prober.cpp
UCD/nsMBCSGroupProber.cpp
UCD/nsMBCSSM.cpp
UCD/nsSBCharSetProber.cpp
UCD/nsSBCSGroupProber.cpp
UCD/nsSJISProber.cpp
UCD/nsUniversalDetector.cpp
UCD/nsUTF8Prober.cpp
UCD/prmem.c
)

# Set target names for the executables
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IDENTIFIER})
    set(MACOSX_BUNDLE_ICON_FILE "Far.icns")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})
    add_executable(far2l MACOSX_BUNDLE ${SOURCES} Far.icns)
    set_target_properties(far2l PROPERTIES
        OUTPUT_NAME ${APP_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${APP_DIR})
    set_source_files_properties(Far.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
else()
    add_executable(far2l ${SOURCES})
    set_target_properties(far2l PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}")
endif()

target_compile_definitions(far2l PRIVATE -DUNICODE)
target_include_directories(far2l PRIVATE Include)
target_include_directories(far2l PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_dependencies(far2l bootstrap WinPort)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(wxWidgets REQUIRED net core base)
    SET (WINPORT "-force_load \"${CMAKE_BINARY_DIR}/WinPort/libWinPort.a\" -framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL" wineguts utils ${wxWidgets_LIBRARIES} ${wxWidgets_CXX_FLAGS})
else()
    SET (WINPORT -Wl,--whole-archive WinPort -Wl,--no-whole-archive)
endif()

set_source_files_properties(
    main.cpp PROPERTIES OBJECT_DEPENDS
    ${CMAKE_BINARY_DIR}/WinPort/libWinPort.a
)

set_target_properties(far2l
    PROPERTIES
    ENABLE_EXPORTS TRUE)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_libraries(far2l
	PRIVATE ${WINPORT}
	PRIVATE c)
else()
    target_link_libraries(far2l
	PRIVATE ${WINPORT}
	PRIVATE dl)
endif()

add_custom_command(TARGET far2l POST_BUILD
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2l_askpass
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2l_sudoapp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#TODO
else()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DE/far2l.desktop" "${CMAKE_CURRENT_BINARY_DIR}/DE/far2l.desktop")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DE/icons" DESTINATION "${CMAKE_INSTALL_PREFIX}/share" USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN "*")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DE/far2l.desktop" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
endif()
