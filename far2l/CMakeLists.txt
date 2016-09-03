
project(far2l)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory (bootstrap)

add_subdirectory (UCD)

add_definitions(-DUNICODE)
include_directories(Include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
PluginA.cpp
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
vtansi.cpp
vtshell.cpp
xlat.cpp
)

add_executable (far2l ${SOURCES})
add_dependencies(far2l bootstrap)

target_link_libraries(far2l UCD WinPort dl)
