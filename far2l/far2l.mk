##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=far2l
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=06/09/16
CodeLitePath           :="/home/user/.codelite"
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=../Build/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)UNICODE 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="far2l.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  -Wl,--whole-archive -lWinPort -Wl,--no-whole-archive -lglib-2.0  -ldl  $(shell wx-config --debug=yes --libs --unicode=yes) -export-dynamic
IncludePath            :=  $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)../WinPort/Debug 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O2 -std=c++11 -Wall $(shell wx-config --cxxflags --unicode=yes --debug=yes)  -Wno-delete-non-virtual-dtor -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 $(shell wx-config --cxxflags --unicode=yes --debug=yes)  -Wno-delete-non-virtual-dtor -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/cache.cpp$(ObjectSuffix) $(IntermediateDirectory)/cddrv.cpp$(ObjectSuffix) $(IntermediateDirectory)/CFileMask.cpp$(ObjectSuffix) $(IntermediateDirectory)/chgmmode.cpp$(ObjectSuffix) $(IntermediateDirectory)/chgprior.cpp$(ObjectSuffix) $(IntermediateDirectory)/clipboard.cpp$(ObjectSuffix) $(IntermediateDirectory)/cmdline.cpp$(ObjectSuffix) $(IntermediateDirectory)/codepage.cpp$(ObjectSuffix) $(IntermediateDirectory)/config.cpp$(ObjectSuffix) $(IntermediateDirectory)/console.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/constitle.cpp$(ObjectSuffix) $(IntermediateDirectory)/copy.cpp$(ObjectSuffix) $(IntermediateDirectory)/ctrlobj.cpp$(ObjectSuffix) $(IntermediateDirectory)/cvtname.cpp$(ObjectSuffix) $(IntermediateDirectory)/datetime.cpp$(ObjectSuffix) $(IntermediateDirectory)/delete.cpp$(ObjectSuffix) $(IntermediateDirectory)/dialog.cpp$(ObjectSuffix) $(IntermediateDirectory)/dirinfo.cpp$(ObjectSuffix) $(IntermediateDirectory)/dirmix.cpp$(ObjectSuffix) $(IntermediateDirectory)/dizlist.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/DlgBuilder.cpp$(ObjectSuffix) $(IntermediateDirectory)/dlgedit.cpp$(ObjectSuffix) $(IntermediateDirectory)/DlgGuid.cpp$(ObjectSuffix) $(IntermediateDirectory)/DList.cpp$(ObjectSuffix) $(IntermediateDirectory)/drivemix.cpp$(ObjectSuffix) $(IntermediateDirectory)/edit.cpp$(ObjectSuffix) $(IntermediateDirectory)/editor.cpp$(ObjectSuffix) $(IntermediateDirectory)/execute.cpp$(ObjectSuffix) $(IntermediateDirectory)/farqueue.cpp$(ObjectSuffix) $(IntermediateDirectory)/farrtl.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/farwinapi.cpp$(ObjectSuffix) $(IntermediateDirectory)/ffolders.cpp$(ObjectSuffix) $(IntermediateDirectory)/fileattr.cpp$(ObjectSuffix) $(IntermediateDirectory)/fileedit.cpp$(ObjectSuffix) $(IntermediateDirectory)/filefilter.cpp$(ObjectSuffix) $(IntermediateDirectory)/filefilterparams.cpp$(ObjectSuffix) $(IntermediateDirectory)/filelist.cpp$(ObjectSuffix) $(IntermediateDirectory)/FileMasksProcessor.cpp$(ObjectSuffix) $(IntermediateDirectory)/FileMasksWithExclude.cpp$(ObjectSuffix) $(IntermediateDirectory)/fileowner.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/filepanels.cpp$(ObjectSuffix) $(IntermediateDirectory)/filestr.cpp$(ObjectSuffix) $(IntermediateDirectory)/filetype.cpp$(ObjectSuffix) $(IntermediateDirectory)/fileview.cpp$(ObjectSuffix) $(IntermediateDirectory)/findfile.cpp$(ObjectSuffix) $(IntermediateDirectory)/flmodes.cpp$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/flplugin.cpp$(ObjectSuffix) $(IntermediateDirectory)/flshow.cpp$(ObjectSuffix) $(IntermediateDirectory)/flupdate.cpp$(ObjectSuffix) $(IntermediateDirectory)/fnparce.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/foldtree.cpp$(ObjectSuffix) $(IntermediateDirectory)/format.cpp$(ObjectSuffix) $(IntermediateDirectory)/frame.cpp$(ObjectSuffix) $(IntermediateDirectory)/global.cpp$(ObjectSuffix) $(IntermediateDirectory)/grabber.cpp$(ObjectSuffix) $(IntermediateDirectory)/headers.cpp$(ObjectSuffix) $(IntermediateDirectory)/help.cpp$(ObjectSuffix) $(IntermediateDirectory)/hilight.cpp$(ObjectSuffix) $(IntermediateDirectory)/history.cpp$(ObjectSuffix) $(IntermediateDirectory)/hmenu.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/hotplug.cpp$(ObjectSuffix) $(IntermediateDirectory)/infolist.cpp$(ObjectSuffix) $(IntermediateDirectory)/interf.cpp$(ObjectSuffix) $(IntermediateDirectory)/keybar.cpp$(ObjectSuffix) $(IntermediateDirectory)/keyboard.cpp$(ObjectSuffix) $(IntermediateDirectory)/language.cpp$(ObjectSuffix) $(IntermediateDirectory)/local.cpp$(ObjectSuffix) $(IntermediateDirectory)/lockscrn.cpp$(ObjectSuffix) $(IntermediateDirectory)/macro.cpp$(ObjectSuffix) $(IntermediateDirectory)/main.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/manager.cpp$(ObjectSuffix) $(IntermediateDirectory)/menubar.cpp$(ObjectSuffix) $(IntermediateDirectory)/message.cpp$(ObjectSuffix) $(IntermediateDirectory)/mix.cpp$(ObjectSuffix) $(IntermediateDirectory)/mkdir.cpp$(ObjectSuffix) $(IntermediateDirectory)/modal.cpp$(ObjectSuffix) $(IntermediateDirectory)/namelist.cpp$(ObjectSuffix) $(IntermediateDirectory)/options.cpp$(ObjectSuffix) $(IntermediateDirectory)/palette.cpp$(ObjectSuffix) $(IntermediateDirectory)/panel.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/panelmix.cpp$(ObjectSuffix) $(IntermediateDirectory)/pathmix.cpp$(ObjectSuffix) $(IntermediateDirectory)/plist.cpp$(ObjectSuffix) $(IntermediateDirectory)/plognmn.cpp$(ObjectSuffix) $(IntermediateDirectory)/plugapi.cpp$(ObjectSuffix) $(IntermediateDirectory)/plugins.cpp$(ObjectSuffix) $(IntermediateDirectory)/PluginW.cpp$(ObjectSuffix) $(IntermediateDirectory)/poscache.cpp$(ObjectSuffix) $(IntermediateDirectory)/processname.cpp$(ObjectSuffix) $(IntermediateDirectory)/qview.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/rdrwdsk.cpp$(ObjectSuffix) $(IntermediateDirectory)/RefreshFrameManager.cpp$(ObjectSuffix) $(IntermediateDirectory)/RegExp.cpp$(ObjectSuffix) 

Objects2=$(IntermediateDirectory)/registry.cpp$(ObjectSuffix) $(IntermediateDirectory)/savefpos.cpp$(ObjectSuffix) $(IntermediateDirectory)/savescr.cpp$(ObjectSuffix) $(IntermediateDirectory)/scantree.cpp$(ObjectSuffix) $(IntermediateDirectory)/scrbuf.cpp$(ObjectSuffix) $(IntermediateDirectory)/scrobj.cpp$(ObjectSuffix) $(IntermediateDirectory)/scrsaver.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/setattr.cpp$(ObjectSuffix) $(IntermediateDirectory)/setcolor.cpp$(ObjectSuffix) $(IntermediateDirectory)/stddlg.cpp$(ObjectSuffix) $(IntermediateDirectory)/strmix.cpp$(ObjectSuffix) $(IntermediateDirectory)/synchro.cpp$(ObjectSuffix) $(IntermediateDirectory)/syntax.cpp$(ObjectSuffix) $(IntermediateDirectory)/syslog.cpp$(ObjectSuffix) $(IntermediateDirectory)/TPreRedrawFunc.cpp$(ObjectSuffix) $(IntermediateDirectory)/treelist.cpp$(ObjectSuffix) $(IntermediateDirectory)/tvar.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix) $(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix) $(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix) $(IntermediateDirectory)/viewer.cpp$(ObjectSuffix) $(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix) $(IntermediateDirectory)/xlat.cpp$(ObjectSuffix) $(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix) $(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix) $(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix) $(IntermediateDirectory)/execute_oscmd.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/vtshell_translation.cpp$(ObjectSuffix) $(IntermediateDirectory)/vtlog.cpp$(ObjectSuffix) $(IntermediateDirectory)/flink.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix) 

Objects3=$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) $(Objects2) $(Objects3) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	@echo $(Objects2) >> $(ObjectsFileList)
	@echo $(Objects3) >> $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

PostBuild:
	@echo Executing Post Build commands ...
	
	@echo Done

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:
	@echo Executing Pre Build commands ...
	make -f scripts.mk
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/cache.cpp$(ObjectSuffix): cache.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cache.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cache.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cache.cpp$(PreprocessSuffix): cache.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cache.cpp$(PreprocessSuffix) "cache.cpp"

$(IntermediateDirectory)/cddrv.cpp$(ObjectSuffix): cddrv.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cddrv.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cddrv.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cddrv.cpp$(PreprocessSuffix): cddrv.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cddrv.cpp$(PreprocessSuffix) "cddrv.cpp"

$(IntermediateDirectory)/CFileMask.cpp$(ObjectSuffix): CFileMask.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./CFileMask.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/CFileMask.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/CFileMask.cpp$(PreprocessSuffix): CFileMask.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/CFileMask.cpp$(PreprocessSuffix) "CFileMask.cpp"

$(IntermediateDirectory)/chgmmode.cpp$(ObjectSuffix): chgmmode.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./chgmmode.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/chgmmode.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/chgmmode.cpp$(PreprocessSuffix): chgmmode.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/chgmmode.cpp$(PreprocessSuffix) "chgmmode.cpp"

$(IntermediateDirectory)/chgprior.cpp$(ObjectSuffix): chgprior.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./chgprior.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/chgprior.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/chgprior.cpp$(PreprocessSuffix): chgprior.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/chgprior.cpp$(PreprocessSuffix) "chgprior.cpp"

$(IntermediateDirectory)/clipboard.cpp$(ObjectSuffix): clipboard.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./clipboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/clipboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/clipboard.cpp$(PreprocessSuffix): clipboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/clipboard.cpp$(PreprocessSuffix) "clipboard.cpp"

$(IntermediateDirectory)/cmdline.cpp$(ObjectSuffix): cmdline.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cmdline.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cmdline.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cmdline.cpp$(PreprocessSuffix): cmdline.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cmdline.cpp$(PreprocessSuffix) "cmdline.cpp"

$(IntermediateDirectory)/codepage.cpp$(ObjectSuffix): codepage.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./codepage.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepage.cpp$(PreprocessSuffix): codepage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepage.cpp$(PreprocessSuffix) "codepage.cpp"

$(IntermediateDirectory)/config.cpp$(ObjectSuffix): config.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./config.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/config.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/config.cpp$(PreprocessSuffix): config.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/config.cpp$(PreprocessSuffix) "config.cpp"

$(IntermediateDirectory)/console.cpp$(ObjectSuffix): console.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./console.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/console.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/console.cpp$(PreprocessSuffix): console.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/console.cpp$(PreprocessSuffix) "console.cpp"

$(IntermediateDirectory)/constitle.cpp$(ObjectSuffix): constitle.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./constitle.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/constitle.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/constitle.cpp$(PreprocessSuffix): constitle.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/constitle.cpp$(PreprocessSuffix) "constitle.cpp"

$(IntermediateDirectory)/copy.cpp$(ObjectSuffix): copy.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./copy.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/copy.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/copy.cpp$(PreprocessSuffix): copy.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/copy.cpp$(PreprocessSuffix) "copy.cpp"

$(IntermediateDirectory)/ctrlobj.cpp$(ObjectSuffix): ctrlobj.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./ctrlobj.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ctrlobj.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ctrlobj.cpp$(PreprocessSuffix): ctrlobj.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ctrlobj.cpp$(PreprocessSuffix) "ctrlobj.cpp"

$(IntermediateDirectory)/cvtname.cpp$(ObjectSuffix): cvtname.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cvtname.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cvtname.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cvtname.cpp$(PreprocessSuffix): cvtname.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cvtname.cpp$(PreprocessSuffix) "cvtname.cpp"

$(IntermediateDirectory)/datetime.cpp$(ObjectSuffix): datetime.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./datetime.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/datetime.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/datetime.cpp$(PreprocessSuffix): datetime.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/datetime.cpp$(PreprocessSuffix) "datetime.cpp"

$(IntermediateDirectory)/delete.cpp$(ObjectSuffix): delete.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./delete.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/delete.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/delete.cpp$(PreprocessSuffix): delete.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/delete.cpp$(PreprocessSuffix) "delete.cpp"

$(IntermediateDirectory)/dialog.cpp$(ObjectSuffix): dialog.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dialog.cpp$(PreprocessSuffix): dialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dialog.cpp$(PreprocessSuffix) "dialog.cpp"

$(IntermediateDirectory)/dirinfo.cpp$(ObjectSuffix): dirinfo.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dirinfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dirinfo.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dirinfo.cpp$(PreprocessSuffix): dirinfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dirinfo.cpp$(PreprocessSuffix) "dirinfo.cpp"

$(IntermediateDirectory)/dirmix.cpp$(ObjectSuffix): dirmix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dirmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dirmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dirmix.cpp$(PreprocessSuffix): dirmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dirmix.cpp$(PreprocessSuffix) "dirmix.cpp"

$(IntermediateDirectory)/dizlist.cpp$(ObjectSuffix): dizlist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dizlist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dizlist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dizlist.cpp$(PreprocessSuffix): dizlist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dizlist.cpp$(PreprocessSuffix) "dizlist.cpp"

$(IntermediateDirectory)/DlgBuilder.cpp$(ObjectSuffix): DlgBuilder.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./DlgBuilder.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DlgBuilder.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DlgBuilder.cpp$(PreprocessSuffix): DlgBuilder.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DlgBuilder.cpp$(PreprocessSuffix) "DlgBuilder.cpp"

$(IntermediateDirectory)/dlgedit.cpp$(ObjectSuffix): dlgedit.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dlgedit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dlgedit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dlgedit.cpp$(PreprocessSuffix): dlgedit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dlgedit.cpp$(PreprocessSuffix) "dlgedit.cpp"

$(IntermediateDirectory)/DlgGuid.cpp$(ObjectSuffix): DlgGuid.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./DlgGuid.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DlgGuid.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DlgGuid.cpp$(PreprocessSuffix): DlgGuid.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DlgGuid.cpp$(PreprocessSuffix) "DlgGuid.cpp"

$(IntermediateDirectory)/DList.cpp$(ObjectSuffix): DList.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./DList.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DList.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DList.cpp$(PreprocessSuffix): DList.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DList.cpp$(PreprocessSuffix) "DList.cpp"

$(IntermediateDirectory)/drivemix.cpp$(ObjectSuffix): drivemix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./drivemix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/drivemix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/drivemix.cpp$(PreprocessSuffix): drivemix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/drivemix.cpp$(PreprocessSuffix) "drivemix.cpp"

$(IntermediateDirectory)/edit.cpp$(ObjectSuffix): edit.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./edit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/edit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/edit.cpp$(PreprocessSuffix): edit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/edit.cpp$(PreprocessSuffix) "edit.cpp"

$(IntermediateDirectory)/editor.cpp$(ObjectSuffix): editor.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./editor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/editor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/editor.cpp$(PreprocessSuffix): editor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/editor.cpp$(PreprocessSuffix) "editor.cpp"

$(IntermediateDirectory)/execute.cpp$(ObjectSuffix): execute.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./execute.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/execute.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/execute.cpp$(PreprocessSuffix): execute.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/execute.cpp$(PreprocessSuffix) "execute.cpp"

$(IntermediateDirectory)/farqueue.cpp$(ObjectSuffix): farqueue.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./farqueue.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/farqueue.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/farqueue.cpp$(PreprocessSuffix): farqueue.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/farqueue.cpp$(PreprocessSuffix) "farqueue.cpp"

$(IntermediateDirectory)/farrtl.cpp$(ObjectSuffix): farrtl.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./farrtl.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/farrtl.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/farrtl.cpp$(PreprocessSuffix): farrtl.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/farrtl.cpp$(PreprocessSuffix) "farrtl.cpp"

$(IntermediateDirectory)/farwinapi.cpp$(ObjectSuffix): farwinapi.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./farwinapi.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/farwinapi.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/farwinapi.cpp$(PreprocessSuffix): farwinapi.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/farwinapi.cpp$(PreprocessSuffix) "farwinapi.cpp"

$(IntermediateDirectory)/ffolders.cpp$(ObjectSuffix): ffolders.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./ffolders.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ffolders.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ffolders.cpp$(PreprocessSuffix): ffolders.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ffolders.cpp$(PreprocessSuffix) "ffolders.cpp"

$(IntermediateDirectory)/fileattr.cpp$(ObjectSuffix): fileattr.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileattr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileattr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileattr.cpp$(PreprocessSuffix): fileattr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileattr.cpp$(PreprocessSuffix) "fileattr.cpp"

$(IntermediateDirectory)/fileedit.cpp$(ObjectSuffix): fileedit.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileedit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileedit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileedit.cpp$(PreprocessSuffix): fileedit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileedit.cpp$(PreprocessSuffix) "fileedit.cpp"

$(IntermediateDirectory)/filefilter.cpp$(ObjectSuffix): filefilter.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filefilter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filefilter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filefilter.cpp$(PreprocessSuffix): filefilter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filefilter.cpp$(PreprocessSuffix) "filefilter.cpp"

$(IntermediateDirectory)/filefilterparams.cpp$(ObjectSuffix): filefilterparams.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filefilterparams.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filefilterparams.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filefilterparams.cpp$(PreprocessSuffix): filefilterparams.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filefilterparams.cpp$(PreprocessSuffix) "filefilterparams.cpp"

$(IntermediateDirectory)/filelist.cpp$(ObjectSuffix): filelist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filelist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filelist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filelist.cpp$(PreprocessSuffix): filelist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filelist.cpp$(PreprocessSuffix) "filelist.cpp"

$(IntermediateDirectory)/FileMasksProcessor.cpp$(ObjectSuffix): FileMasksProcessor.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./FileMasksProcessor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FileMasksProcessor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FileMasksProcessor.cpp$(PreprocessSuffix): FileMasksProcessor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FileMasksProcessor.cpp$(PreprocessSuffix) "FileMasksProcessor.cpp"

$(IntermediateDirectory)/FileMasksWithExclude.cpp$(ObjectSuffix): FileMasksWithExclude.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./FileMasksWithExclude.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FileMasksWithExclude.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FileMasksWithExclude.cpp$(PreprocessSuffix): FileMasksWithExclude.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FileMasksWithExclude.cpp$(PreprocessSuffix) "FileMasksWithExclude.cpp"

$(IntermediateDirectory)/fileowner.cpp$(ObjectSuffix): fileowner.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileowner.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileowner.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileowner.cpp$(PreprocessSuffix): fileowner.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileowner.cpp$(PreprocessSuffix) "fileowner.cpp"

$(IntermediateDirectory)/filepanels.cpp$(ObjectSuffix): filepanels.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filepanels.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filepanels.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filepanels.cpp$(PreprocessSuffix): filepanels.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filepanels.cpp$(PreprocessSuffix) "filepanels.cpp"

$(IntermediateDirectory)/filestr.cpp$(ObjectSuffix): filestr.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filestr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filestr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filestr.cpp$(PreprocessSuffix): filestr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filestr.cpp$(PreprocessSuffix) "filestr.cpp"

$(IntermediateDirectory)/filetype.cpp$(ObjectSuffix): filetype.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filetype.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filetype.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filetype.cpp$(PreprocessSuffix): filetype.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filetype.cpp$(PreprocessSuffix) "filetype.cpp"

$(IntermediateDirectory)/fileview.cpp$(ObjectSuffix): fileview.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileview.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileview.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileview.cpp$(PreprocessSuffix): fileview.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileview.cpp$(PreprocessSuffix) "fileview.cpp"

$(IntermediateDirectory)/findfile.cpp$(ObjectSuffix): findfile.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./findfile.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/findfile.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/findfile.cpp$(PreprocessSuffix): findfile.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/findfile.cpp$(PreprocessSuffix) "findfile.cpp"

$(IntermediateDirectory)/flmodes.cpp$(ObjectSuffix): flmodes.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flmodes.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flmodes.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flmodes.cpp$(PreprocessSuffix): flmodes.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flmodes.cpp$(PreprocessSuffix) "flmodes.cpp"

$(IntermediateDirectory)/flplugin.cpp$(ObjectSuffix): flplugin.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flplugin.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flplugin.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flplugin.cpp$(PreprocessSuffix): flplugin.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flplugin.cpp$(PreprocessSuffix) "flplugin.cpp"

$(IntermediateDirectory)/flshow.cpp$(ObjectSuffix): flshow.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flshow.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flshow.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flshow.cpp$(PreprocessSuffix): flshow.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flshow.cpp$(PreprocessSuffix) "flshow.cpp"

$(IntermediateDirectory)/flupdate.cpp$(ObjectSuffix): flupdate.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flupdate.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flupdate.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flupdate.cpp$(PreprocessSuffix): flupdate.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flupdate.cpp$(PreprocessSuffix) "flupdate.cpp"

$(IntermediateDirectory)/fnparce.cpp$(ObjectSuffix): fnparce.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fnparce.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fnparce.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fnparce.cpp$(PreprocessSuffix): fnparce.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fnparce.cpp$(PreprocessSuffix) "fnparce.cpp"

$(IntermediateDirectory)/foldtree.cpp$(ObjectSuffix): foldtree.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./foldtree.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/foldtree.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/foldtree.cpp$(PreprocessSuffix): foldtree.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/foldtree.cpp$(PreprocessSuffix) "foldtree.cpp"

$(IntermediateDirectory)/format.cpp$(ObjectSuffix): format.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./format.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/format.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/format.cpp$(PreprocessSuffix): format.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/format.cpp$(PreprocessSuffix) "format.cpp"

$(IntermediateDirectory)/frame.cpp$(ObjectSuffix): frame.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./frame.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/frame.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/frame.cpp$(PreprocessSuffix): frame.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/frame.cpp$(PreprocessSuffix) "frame.cpp"

$(IntermediateDirectory)/global.cpp$(ObjectSuffix): global.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./global.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/global.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/global.cpp$(PreprocessSuffix): global.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/global.cpp$(PreprocessSuffix) "global.cpp"

$(IntermediateDirectory)/grabber.cpp$(ObjectSuffix): grabber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./grabber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/grabber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/grabber.cpp$(PreprocessSuffix): grabber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/grabber.cpp$(PreprocessSuffix) "grabber.cpp"

$(IntermediateDirectory)/headers.cpp$(ObjectSuffix): headers.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./headers.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/headers.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/headers.cpp$(PreprocessSuffix): headers.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/headers.cpp$(PreprocessSuffix) "headers.cpp"

$(IntermediateDirectory)/help.cpp$(ObjectSuffix): help.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./help.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/help.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/help.cpp$(PreprocessSuffix): help.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/help.cpp$(PreprocessSuffix) "help.cpp"

$(IntermediateDirectory)/hilight.cpp$(ObjectSuffix): hilight.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./hilight.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/hilight.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hilight.cpp$(PreprocessSuffix): hilight.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hilight.cpp$(PreprocessSuffix) "hilight.cpp"

$(IntermediateDirectory)/history.cpp$(ObjectSuffix): history.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./history.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/history.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/history.cpp$(PreprocessSuffix): history.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/history.cpp$(PreprocessSuffix) "history.cpp"

$(IntermediateDirectory)/hmenu.cpp$(ObjectSuffix): hmenu.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./hmenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/hmenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hmenu.cpp$(PreprocessSuffix): hmenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hmenu.cpp$(PreprocessSuffix) "hmenu.cpp"

$(IntermediateDirectory)/hotplug.cpp$(ObjectSuffix): hotplug.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./hotplug.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/hotplug.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hotplug.cpp$(PreprocessSuffix): hotplug.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hotplug.cpp$(PreprocessSuffix) "hotplug.cpp"

$(IntermediateDirectory)/infolist.cpp$(ObjectSuffix): infolist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./infolist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/infolist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/infolist.cpp$(PreprocessSuffix): infolist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/infolist.cpp$(PreprocessSuffix) "infolist.cpp"

$(IntermediateDirectory)/interf.cpp$(ObjectSuffix): interf.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./interf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/interf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/interf.cpp$(PreprocessSuffix): interf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/interf.cpp$(PreprocessSuffix) "interf.cpp"

$(IntermediateDirectory)/keybar.cpp$(ObjectSuffix): keybar.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./keybar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/keybar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/keybar.cpp$(PreprocessSuffix): keybar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/keybar.cpp$(PreprocessSuffix) "keybar.cpp"

$(IntermediateDirectory)/keyboard.cpp$(ObjectSuffix): keyboard.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./keyboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/keyboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/keyboard.cpp$(PreprocessSuffix): keyboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/keyboard.cpp$(PreprocessSuffix) "keyboard.cpp"

$(IntermediateDirectory)/language.cpp$(ObjectSuffix): language.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./language.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/language.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/language.cpp$(PreprocessSuffix): language.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/language.cpp$(PreprocessSuffix) "language.cpp"

$(IntermediateDirectory)/local.cpp$(ObjectSuffix): local.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./local.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/local.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/local.cpp$(PreprocessSuffix): local.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/local.cpp$(PreprocessSuffix) "local.cpp"

$(IntermediateDirectory)/lockscrn.cpp$(ObjectSuffix): lockscrn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lockscrn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/lockscrn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/lockscrn.cpp$(PreprocessSuffix): lockscrn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/lockscrn.cpp$(PreprocessSuffix) "lockscrn.cpp"

$(IntermediateDirectory)/macro.cpp$(ObjectSuffix): macro.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./macro.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/macro.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/macro.cpp$(PreprocessSuffix): macro.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/macro.cpp$(PreprocessSuffix) "macro.cpp"

$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) "main.cpp"

$(IntermediateDirectory)/manager.cpp$(ObjectSuffix): manager.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./manager.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/manager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/manager.cpp$(PreprocessSuffix): manager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/manager.cpp$(PreprocessSuffix) "manager.cpp"

$(IntermediateDirectory)/menubar.cpp$(ObjectSuffix): menubar.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./menubar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/menubar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/menubar.cpp$(PreprocessSuffix): menubar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/menubar.cpp$(PreprocessSuffix) "menubar.cpp"

$(IntermediateDirectory)/message.cpp$(ObjectSuffix): message.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./message.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/message.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/message.cpp$(PreprocessSuffix): message.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/message.cpp$(PreprocessSuffix) "message.cpp"

$(IntermediateDirectory)/mix.cpp$(ObjectSuffix): mix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./mix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mix.cpp$(PreprocessSuffix): mix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mix.cpp$(PreprocessSuffix) "mix.cpp"

$(IntermediateDirectory)/mkdir.cpp$(ObjectSuffix): mkdir.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./mkdir.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mkdir.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mkdir.cpp$(PreprocessSuffix): mkdir.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mkdir.cpp$(PreprocessSuffix) "mkdir.cpp"

$(IntermediateDirectory)/modal.cpp$(ObjectSuffix): modal.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./modal.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/modal.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/modal.cpp$(PreprocessSuffix): modal.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/modal.cpp$(PreprocessSuffix) "modal.cpp"

$(IntermediateDirectory)/namelist.cpp$(ObjectSuffix): namelist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./namelist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/namelist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/namelist.cpp$(PreprocessSuffix): namelist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/namelist.cpp$(PreprocessSuffix) "namelist.cpp"

$(IntermediateDirectory)/options.cpp$(ObjectSuffix): options.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./options.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/options.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/options.cpp$(PreprocessSuffix): options.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/options.cpp$(PreprocessSuffix) "options.cpp"

$(IntermediateDirectory)/palette.cpp$(ObjectSuffix): palette.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./palette.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/palette.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/palette.cpp$(PreprocessSuffix): palette.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/palette.cpp$(PreprocessSuffix) "palette.cpp"

$(IntermediateDirectory)/panel.cpp$(ObjectSuffix): panel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./panel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/panel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/panel.cpp$(PreprocessSuffix): panel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/panel.cpp$(PreprocessSuffix) "panel.cpp"

$(IntermediateDirectory)/panelmix.cpp$(ObjectSuffix): panelmix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./panelmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/panelmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/panelmix.cpp$(PreprocessSuffix): panelmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/panelmix.cpp$(PreprocessSuffix) "panelmix.cpp"

$(IntermediateDirectory)/pathmix.cpp$(ObjectSuffix): pathmix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./pathmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pathmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pathmix.cpp$(PreprocessSuffix): pathmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pathmix.cpp$(PreprocessSuffix) "pathmix.cpp"

$(IntermediateDirectory)/plist.cpp$(ObjectSuffix): plist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plist.cpp$(PreprocessSuffix): plist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plist.cpp$(PreprocessSuffix) "plist.cpp"

$(IntermediateDirectory)/plognmn.cpp$(ObjectSuffix): plognmn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plognmn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plognmn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plognmn.cpp$(PreprocessSuffix): plognmn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plognmn.cpp$(PreprocessSuffix) "plognmn.cpp"

$(IntermediateDirectory)/plugapi.cpp$(ObjectSuffix): plugapi.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plugapi.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plugapi.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plugapi.cpp$(PreprocessSuffix): plugapi.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plugapi.cpp$(PreprocessSuffix) "plugapi.cpp"

$(IntermediateDirectory)/plugins.cpp$(ObjectSuffix): plugins.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plugins.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plugins.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plugins.cpp$(PreprocessSuffix): plugins.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plugins.cpp$(PreprocessSuffix) "plugins.cpp"

$(IntermediateDirectory)/PluginW.cpp$(ObjectSuffix): PluginW.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./PluginW.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/PluginW.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/PluginW.cpp$(PreprocessSuffix): PluginW.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/PluginW.cpp$(PreprocessSuffix) "PluginW.cpp"

$(IntermediateDirectory)/poscache.cpp$(ObjectSuffix): poscache.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./poscache.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/poscache.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/poscache.cpp$(PreprocessSuffix): poscache.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/poscache.cpp$(PreprocessSuffix) "poscache.cpp"

$(IntermediateDirectory)/processname.cpp$(ObjectSuffix): processname.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./processname.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/processname.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/processname.cpp$(PreprocessSuffix): processname.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/processname.cpp$(PreprocessSuffix) "processname.cpp"

$(IntermediateDirectory)/qview.cpp$(ObjectSuffix): qview.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./qview.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/qview.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/qview.cpp$(PreprocessSuffix): qview.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/qview.cpp$(PreprocessSuffix) "qview.cpp"

$(IntermediateDirectory)/rdrwdsk.cpp$(ObjectSuffix): rdrwdsk.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./rdrwdsk.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/rdrwdsk.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/rdrwdsk.cpp$(PreprocessSuffix): rdrwdsk.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/rdrwdsk.cpp$(PreprocessSuffix) "rdrwdsk.cpp"

$(IntermediateDirectory)/RefreshFrameManager.cpp$(ObjectSuffix): RefreshFrameManager.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./RefreshFrameManager.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/RefreshFrameManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/RefreshFrameManager.cpp$(PreprocessSuffix): RefreshFrameManager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/RefreshFrameManager.cpp$(PreprocessSuffix) "RefreshFrameManager.cpp"

$(IntermediateDirectory)/RegExp.cpp$(ObjectSuffix): RegExp.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./RegExp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/RegExp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/RegExp.cpp$(PreprocessSuffix): RegExp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/RegExp.cpp$(PreprocessSuffix) "RegExp.cpp"

$(IntermediateDirectory)/registry.cpp$(ObjectSuffix): registry.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./registry.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/registry.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/registry.cpp$(PreprocessSuffix): registry.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/registry.cpp$(PreprocessSuffix) "registry.cpp"

$(IntermediateDirectory)/savefpos.cpp$(ObjectSuffix): savefpos.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./savefpos.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/savefpos.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/savefpos.cpp$(PreprocessSuffix): savefpos.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/savefpos.cpp$(PreprocessSuffix) "savefpos.cpp"

$(IntermediateDirectory)/savescr.cpp$(ObjectSuffix): savescr.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./savescr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/savescr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/savescr.cpp$(PreprocessSuffix): savescr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/savescr.cpp$(PreprocessSuffix) "savescr.cpp"

$(IntermediateDirectory)/scantree.cpp$(ObjectSuffix): scantree.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scantree.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scantree.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scantree.cpp$(PreprocessSuffix): scantree.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scantree.cpp$(PreprocessSuffix) "scantree.cpp"

$(IntermediateDirectory)/scrbuf.cpp$(ObjectSuffix): scrbuf.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scrbuf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scrbuf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scrbuf.cpp$(PreprocessSuffix): scrbuf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scrbuf.cpp$(PreprocessSuffix) "scrbuf.cpp"

$(IntermediateDirectory)/scrobj.cpp$(ObjectSuffix): scrobj.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scrobj.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scrobj.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scrobj.cpp$(PreprocessSuffix): scrobj.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scrobj.cpp$(PreprocessSuffix) "scrobj.cpp"

$(IntermediateDirectory)/scrsaver.cpp$(ObjectSuffix): scrsaver.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scrsaver.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scrsaver.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scrsaver.cpp$(PreprocessSuffix): scrsaver.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scrsaver.cpp$(PreprocessSuffix) "scrsaver.cpp"

$(IntermediateDirectory)/setattr.cpp$(ObjectSuffix): setattr.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./setattr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/setattr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/setattr.cpp$(PreprocessSuffix): setattr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/setattr.cpp$(PreprocessSuffix) "setattr.cpp"

$(IntermediateDirectory)/setcolor.cpp$(ObjectSuffix): setcolor.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./setcolor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/setcolor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/setcolor.cpp$(PreprocessSuffix): setcolor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/setcolor.cpp$(PreprocessSuffix) "setcolor.cpp"

$(IntermediateDirectory)/stddlg.cpp$(ObjectSuffix): stddlg.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./stddlg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/stddlg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/stddlg.cpp$(PreprocessSuffix): stddlg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/stddlg.cpp$(PreprocessSuffix) "stddlg.cpp"

$(IntermediateDirectory)/strmix.cpp$(ObjectSuffix): strmix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./strmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/strmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/strmix.cpp$(PreprocessSuffix): strmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/strmix.cpp$(PreprocessSuffix) "strmix.cpp"

$(IntermediateDirectory)/synchro.cpp$(ObjectSuffix): synchro.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./synchro.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/synchro.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/synchro.cpp$(PreprocessSuffix): synchro.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/synchro.cpp$(PreprocessSuffix) "synchro.cpp"

$(IntermediateDirectory)/syntax.cpp$(ObjectSuffix): syntax.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./syntax.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/syntax.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/syntax.cpp$(PreprocessSuffix): syntax.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/syntax.cpp$(PreprocessSuffix) "syntax.cpp"

$(IntermediateDirectory)/syslog.cpp$(ObjectSuffix): syslog.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./syslog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/syslog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/syslog.cpp$(PreprocessSuffix): syslog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/syslog.cpp$(PreprocessSuffix) "syslog.cpp"

$(IntermediateDirectory)/TPreRedrawFunc.cpp$(ObjectSuffix): TPreRedrawFunc.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./TPreRedrawFunc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/TPreRedrawFunc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/TPreRedrawFunc.cpp$(PreprocessSuffix): TPreRedrawFunc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/TPreRedrawFunc.cpp$(PreprocessSuffix) "TPreRedrawFunc.cpp"

$(IntermediateDirectory)/treelist.cpp$(ObjectSuffix): treelist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./treelist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/treelist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/treelist.cpp$(PreprocessSuffix): treelist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/treelist.cpp$(PreprocessSuffix) "treelist.cpp"

$(IntermediateDirectory)/tvar.cpp$(ObjectSuffix): tvar.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./tvar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/tvar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/tvar.cpp$(PreprocessSuffix): tvar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/tvar.cpp$(PreprocessSuffix) "tvar.cpp"

$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix): udlist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./udlist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/udlist.cpp$(PreprocessSuffix): udlist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/udlist.cpp$(PreprocessSuffix) "udlist.cpp"

$(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix): UnicodeString.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UnicodeString.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UnicodeString.cpp$(PreprocessSuffix): UnicodeString.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UnicodeString.cpp$(PreprocessSuffix) "UnicodeString.cpp"

$(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix): usermenu.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./usermenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/usermenu.cpp$(PreprocessSuffix): usermenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/usermenu.cpp$(PreprocessSuffix) "usermenu.cpp"

$(IntermediateDirectory)/viewer.cpp$(ObjectSuffix): viewer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./viewer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/viewer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/viewer.cpp$(PreprocessSuffix): viewer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/viewer.cpp$(PreprocessSuffix) "viewer.cpp"

$(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix): vmenu.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vmenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vmenu.cpp$(PreprocessSuffix): vmenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vmenu.cpp$(PreprocessSuffix) "vmenu.cpp"

$(IntermediateDirectory)/xlat.cpp$(ObjectSuffix): xlat.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./xlat.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/xlat.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/xlat.cpp$(PreprocessSuffix): xlat.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/xlat.cpp$(PreprocessSuffix) "xlat.cpp"

$(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix): vtshell.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vtshell.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vtshell.cpp$(PreprocessSuffix): vtshell.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vtshell.cpp$(PreprocessSuffix) "vtshell.cpp"

$(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix): vtansi.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vtansi.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vtansi.cpp$(PreprocessSuffix): vtansi.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vtansi.cpp$(PreprocessSuffix) "vtansi.cpp"

$(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix): PluginA.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./PluginA.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/PluginA.cpp$(PreprocessSuffix): PluginA.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/PluginA.cpp$(PreprocessSuffix) "PluginA.cpp"

$(IntermediateDirectory)/execute_oscmd.cpp$(ObjectSuffix): execute_oscmd.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./execute_oscmd.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/execute_oscmd.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/execute_oscmd.cpp$(PreprocessSuffix): execute_oscmd.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/execute_oscmd.cpp$(PreprocessSuffix) "execute_oscmd.cpp"

$(IntermediateDirectory)/vtshell_translation.cpp$(ObjectSuffix): vtshell_translation.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vtshell_translation.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vtshell_translation.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vtshell_translation.cpp$(PreprocessSuffix): vtshell_translation.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vtshell_translation.cpp$(PreprocessSuffix) "vtshell_translation.cpp"

$(IntermediateDirectory)/vtlog.cpp$(ObjectSuffix): vtlog.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vtlog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vtlog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vtlog.cpp$(PreprocessSuffix): vtlog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vtlog.cpp$(PreprocessSuffix) "vtlog.cpp"

$(IntermediateDirectory)/flink.cpp$(ObjectSuffix): flink.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flink.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flink.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flink.cpp$(PreprocessSuffix): flink.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flink.cpp$(PreprocessSuffix) "flink.cpp"

$(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix): UCD/CharDistribution.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/CharDistribution.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_CharDistribution.cpp$(PreprocessSuffix): UCD/CharDistribution.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_CharDistribution.cpp$(PreprocessSuffix) "UCD/CharDistribution.cpp"

$(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix): UCD/JpCntx.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/JpCntx.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_JpCntx.cpp$(PreprocessSuffix): UCD/JpCntx.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_JpCntx.cpp$(PreprocessSuffix) "UCD/JpCntx.cpp"

$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix): UCD/LangBulgarianModel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangBulgarianModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(PreprocessSuffix): UCD/LangBulgarianModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(PreprocessSuffix) "UCD/LangBulgarianModel.cpp"

$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix): UCD/LangCyrillicModel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangCyrillicModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(PreprocessSuffix): UCD/LangCyrillicModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(PreprocessSuffix) "UCD/LangCyrillicModel.cpp"

$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix): UCD/LangGreekModel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangGreekModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(PreprocessSuffix): UCD/LangGreekModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangGreekModel.cpp$(PreprocessSuffix) "UCD/LangGreekModel.cpp"

$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix): UCD/LangHebrewModel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangHebrewModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(PreprocessSuffix): UCD/LangHebrewModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(PreprocessSuffix) "UCD/LangHebrewModel.cpp"

$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix): UCD/LangHungarianModel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangHungarianModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(PreprocessSuffix): UCD/LangHungarianModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(PreprocessSuffix) "UCD/LangHungarianModel.cpp"

$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix): UCD/LangThaiModel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangThaiModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(PreprocessSuffix): UCD/LangThaiModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangThaiModel.cpp$(PreprocessSuffix) "UCD/LangThaiModel.cpp"

$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix): UCD/nsBig5Prober.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsBig5Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(PreprocessSuffix): UCD/nsBig5Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(PreprocessSuffix) "UCD/nsBig5Prober.cpp"

$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix): UCD/nsCharSetProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsCharSetProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(PreprocessSuffix): UCD/nsCharSetProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(PreprocessSuffix) "UCD/nsCharSetProber.cpp"

$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix): UCD/nsEscCharsetProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEscCharsetProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(PreprocessSuffix): UCD/nsEscCharsetProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(PreprocessSuffix) "UCD/nsEscCharsetProber.cpp"

$(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix): UCD/nsEscSM.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEscSM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEscSM.cpp$(PreprocessSuffix): UCD/nsEscSM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEscSM.cpp$(PreprocessSuffix) "UCD/nsEscSM.cpp"

$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix): UCD/nsEUCJPProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEUCJPProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(PreprocessSuffix): UCD/nsEUCJPProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(PreprocessSuffix) "UCD/nsEUCJPProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix): UCD/nsEUCKRProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEUCKRProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(PreprocessSuffix): UCD/nsEUCKRProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(PreprocessSuffix) "UCD/nsEUCKRProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix): UCD/nsEUCTWProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEUCTWProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(PreprocessSuffix): UCD/nsEUCTWProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(PreprocessSuffix) "UCD/nsEUCTWProber.cpp"

$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix): UCD/nsGB2312Prober.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsGB2312Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(PreprocessSuffix): UCD/nsGB2312Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(PreprocessSuffix) "UCD/nsGB2312Prober.cpp"

$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix): UCD/nsHebrewProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsHebrewProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(PreprocessSuffix): UCD/nsHebrewProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(PreprocessSuffix) "UCD/nsHebrewProber.cpp"

$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix): UCD/nsLatin1Prober.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsLatin1Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(PreprocessSuffix): UCD/nsLatin1Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(PreprocessSuffix) "UCD/nsLatin1Prober.cpp"

$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix): UCD/nsMBCSGroupProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsMBCSGroupProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(PreprocessSuffix): UCD/nsMBCSGroupProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(PreprocessSuffix) "UCD/nsMBCSGroupProber.cpp"

$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix): UCD/nsMBCSSM.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsMBCSSM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(PreprocessSuffix): UCD/nsMBCSSM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(PreprocessSuffix) "UCD/nsMBCSSM.cpp"

$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix): UCD/nsSBCharSetProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsSBCharSetProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(PreprocessSuffix): UCD/nsSBCharSetProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(PreprocessSuffix) "UCD/nsSBCharSetProber.cpp"

$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix): UCD/nsSBCSGroupProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsSBCSGroupProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(PreprocessSuffix): UCD/nsSBCSGroupProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(PreprocessSuffix) "UCD/nsSBCSGroupProber.cpp"

$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix): UCD/nsSJISProber.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsSJISProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(PreprocessSuffix): UCD/nsSJISProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsSJISProber.cpp$(PreprocessSuffix) "UCD/nsSJISProber.cpp"

$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix): UCD/nsUniversalDetector.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsUniversalDetector.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(PreprocessSuffix): UCD/nsUniversalDetector.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(PreprocessSuffix) "UCD/nsUniversalDetector.cpp"

$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix): UCD/nsUTF8Prober.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsUTF8Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(PreprocessSuffix): UCD/nsUTF8Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(PreprocessSuffix) "UCD/nsUTF8Prober.cpp"

$(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix): UCD/prmem.c 
	$(CC) $(SourceSwitch) "./UCD/prmem.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_prmem.c$(PreprocessSuffix): UCD/prmem.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_prmem.c$(PreprocessSuffix) "UCD/prmem.c"

##
## Clean
##
clean:
	$(RM) -r ./Debug/


