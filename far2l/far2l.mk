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
Date                   :=15/08/16
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
LinkOptions            :=  $(shell wx-config --debug=yes --libs --unicode=yes) -export-dynamic
IncludePath            :=  $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)WinPort $(LibrarySwitch)dl $(LibrarySwitch)glib-2.0 
ArLibs                 :=  "WinPort" "dl" "glib-2.0" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)../WinPort/Debug 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O2 -std=c++11 -Wall $(shell wx-config --cxxflags --unicode=yes --debug=yes) -Wno-delete-non-virtual-dtor -mcmodel=medium -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 $(shell wx-config --cxxflags --unicode=yes --debug=yes) -Wno-delete-non-virtual-dtor -mcmodel=medium -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
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
	$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix) $(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix) $(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix) $(IntermediateDirectory)/viewer.cpp$(ObjectSuffix) $(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix) $(IntermediateDirectory)/xlat.cpp$(ObjectSuffix) $(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix) $(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix) $(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix) 

Objects3=$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix) $(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) $(Objects2) $(Objects3) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d "../.build-debug/WinPort" "../.build-debug/farlng" $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	@echo $(Objects2) >> $(ObjectsFileList)
	@echo $(Objects3) >> $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

"../.build-debug/WinPort":
	@$(MakeDirCommand) "../.build-debug"
	@echo stam > "../.build-debug/WinPort"


"../.build-debug/farlng":
	@$(MakeDirCommand) "../.build-debug"
	@echo stam > "../.build-debug/farlng"




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
$(IntermediateDirectory)/cache.cpp$(ObjectSuffix): cache.cpp $(IntermediateDirectory)/cache.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cache.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cache.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cache.cpp$(DependSuffix): cache.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/cache.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/cache.cpp$(DependSuffix) -MM "cache.cpp"

$(IntermediateDirectory)/cache.cpp$(PreprocessSuffix): cache.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cache.cpp$(PreprocessSuffix) "cache.cpp"

$(IntermediateDirectory)/cddrv.cpp$(ObjectSuffix): cddrv.cpp $(IntermediateDirectory)/cddrv.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cddrv.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cddrv.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cddrv.cpp$(DependSuffix): cddrv.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/cddrv.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/cddrv.cpp$(DependSuffix) -MM "cddrv.cpp"

$(IntermediateDirectory)/cddrv.cpp$(PreprocessSuffix): cddrv.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cddrv.cpp$(PreprocessSuffix) "cddrv.cpp"

$(IntermediateDirectory)/CFileMask.cpp$(ObjectSuffix): CFileMask.cpp $(IntermediateDirectory)/CFileMask.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./CFileMask.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/CFileMask.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/CFileMask.cpp$(DependSuffix): CFileMask.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/CFileMask.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/CFileMask.cpp$(DependSuffix) -MM "CFileMask.cpp"

$(IntermediateDirectory)/CFileMask.cpp$(PreprocessSuffix): CFileMask.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/CFileMask.cpp$(PreprocessSuffix) "CFileMask.cpp"

$(IntermediateDirectory)/chgmmode.cpp$(ObjectSuffix): chgmmode.cpp $(IntermediateDirectory)/chgmmode.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./chgmmode.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/chgmmode.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/chgmmode.cpp$(DependSuffix): chgmmode.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/chgmmode.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/chgmmode.cpp$(DependSuffix) -MM "chgmmode.cpp"

$(IntermediateDirectory)/chgmmode.cpp$(PreprocessSuffix): chgmmode.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/chgmmode.cpp$(PreprocessSuffix) "chgmmode.cpp"

$(IntermediateDirectory)/chgprior.cpp$(ObjectSuffix): chgprior.cpp $(IntermediateDirectory)/chgprior.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./chgprior.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/chgprior.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/chgprior.cpp$(DependSuffix): chgprior.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/chgprior.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/chgprior.cpp$(DependSuffix) -MM "chgprior.cpp"

$(IntermediateDirectory)/chgprior.cpp$(PreprocessSuffix): chgprior.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/chgprior.cpp$(PreprocessSuffix) "chgprior.cpp"

$(IntermediateDirectory)/clipboard.cpp$(ObjectSuffix): clipboard.cpp $(IntermediateDirectory)/clipboard.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./clipboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/clipboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/clipboard.cpp$(DependSuffix): clipboard.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/clipboard.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/clipboard.cpp$(DependSuffix) -MM "clipboard.cpp"

$(IntermediateDirectory)/clipboard.cpp$(PreprocessSuffix): clipboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/clipboard.cpp$(PreprocessSuffix) "clipboard.cpp"

$(IntermediateDirectory)/cmdline.cpp$(ObjectSuffix): cmdline.cpp $(IntermediateDirectory)/cmdline.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cmdline.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cmdline.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cmdline.cpp$(DependSuffix): cmdline.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/cmdline.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/cmdline.cpp$(DependSuffix) -MM "cmdline.cpp"

$(IntermediateDirectory)/cmdline.cpp$(PreprocessSuffix): cmdline.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cmdline.cpp$(PreprocessSuffix) "cmdline.cpp"

$(IntermediateDirectory)/codepage.cpp$(ObjectSuffix): codepage.cpp $(IntermediateDirectory)/codepage.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./codepage.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepage.cpp$(DependSuffix): codepage.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepage.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/codepage.cpp$(DependSuffix) -MM "codepage.cpp"

$(IntermediateDirectory)/codepage.cpp$(PreprocessSuffix): codepage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepage.cpp$(PreprocessSuffix) "codepage.cpp"

$(IntermediateDirectory)/config.cpp$(ObjectSuffix): config.cpp $(IntermediateDirectory)/config.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./config.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/config.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/config.cpp$(DependSuffix): config.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/config.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/config.cpp$(DependSuffix) -MM "config.cpp"

$(IntermediateDirectory)/config.cpp$(PreprocessSuffix): config.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/config.cpp$(PreprocessSuffix) "config.cpp"

$(IntermediateDirectory)/console.cpp$(ObjectSuffix): console.cpp $(IntermediateDirectory)/console.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./console.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/console.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/console.cpp$(DependSuffix): console.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/console.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/console.cpp$(DependSuffix) -MM "console.cpp"

$(IntermediateDirectory)/console.cpp$(PreprocessSuffix): console.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/console.cpp$(PreprocessSuffix) "console.cpp"

$(IntermediateDirectory)/constitle.cpp$(ObjectSuffix): constitle.cpp $(IntermediateDirectory)/constitle.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./constitle.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/constitle.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/constitle.cpp$(DependSuffix): constitle.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/constitle.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/constitle.cpp$(DependSuffix) -MM "constitle.cpp"

$(IntermediateDirectory)/constitle.cpp$(PreprocessSuffix): constitle.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/constitle.cpp$(PreprocessSuffix) "constitle.cpp"

$(IntermediateDirectory)/copy.cpp$(ObjectSuffix): copy.cpp $(IntermediateDirectory)/copy.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./copy.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/copy.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/copy.cpp$(DependSuffix): copy.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/copy.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/copy.cpp$(DependSuffix) -MM "copy.cpp"

$(IntermediateDirectory)/copy.cpp$(PreprocessSuffix): copy.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/copy.cpp$(PreprocessSuffix) "copy.cpp"

$(IntermediateDirectory)/ctrlobj.cpp$(ObjectSuffix): ctrlobj.cpp $(IntermediateDirectory)/ctrlobj.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./ctrlobj.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ctrlobj.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ctrlobj.cpp$(DependSuffix): ctrlobj.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ctrlobj.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ctrlobj.cpp$(DependSuffix) -MM "ctrlobj.cpp"

$(IntermediateDirectory)/ctrlobj.cpp$(PreprocessSuffix): ctrlobj.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ctrlobj.cpp$(PreprocessSuffix) "ctrlobj.cpp"

$(IntermediateDirectory)/cvtname.cpp$(ObjectSuffix): cvtname.cpp $(IntermediateDirectory)/cvtname.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./cvtname.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cvtname.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cvtname.cpp$(DependSuffix): cvtname.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/cvtname.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/cvtname.cpp$(DependSuffix) -MM "cvtname.cpp"

$(IntermediateDirectory)/cvtname.cpp$(PreprocessSuffix): cvtname.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cvtname.cpp$(PreprocessSuffix) "cvtname.cpp"

$(IntermediateDirectory)/datetime.cpp$(ObjectSuffix): datetime.cpp $(IntermediateDirectory)/datetime.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./datetime.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/datetime.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/datetime.cpp$(DependSuffix): datetime.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/datetime.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/datetime.cpp$(DependSuffix) -MM "datetime.cpp"

$(IntermediateDirectory)/datetime.cpp$(PreprocessSuffix): datetime.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/datetime.cpp$(PreprocessSuffix) "datetime.cpp"

$(IntermediateDirectory)/delete.cpp$(ObjectSuffix): delete.cpp $(IntermediateDirectory)/delete.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./delete.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/delete.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/delete.cpp$(DependSuffix): delete.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/delete.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/delete.cpp$(DependSuffix) -MM "delete.cpp"

$(IntermediateDirectory)/delete.cpp$(PreprocessSuffix): delete.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/delete.cpp$(PreprocessSuffix) "delete.cpp"

$(IntermediateDirectory)/dialog.cpp$(ObjectSuffix): dialog.cpp $(IntermediateDirectory)/dialog.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dialog.cpp$(DependSuffix): dialog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dialog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dialog.cpp$(DependSuffix) -MM "dialog.cpp"

$(IntermediateDirectory)/dialog.cpp$(PreprocessSuffix): dialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dialog.cpp$(PreprocessSuffix) "dialog.cpp"

$(IntermediateDirectory)/dirinfo.cpp$(ObjectSuffix): dirinfo.cpp $(IntermediateDirectory)/dirinfo.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dirinfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dirinfo.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dirinfo.cpp$(DependSuffix): dirinfo.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dirinfo.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dirinfo.cpp$(DependSuffix) -MM "dirinfo.cpp"

$(IntermediateDirectory)/dirinfo.cpp$(PreprocessSuffix): dirinfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dirinfo.cpp$(PreprocessSuffix) "dirinfo.cpp"

$(IntermediateDirectory)/dirmix.cpp$(ObjectSuffix): dirmix.cpp $(IntermediateDirectory)/dirmix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dirmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dirmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dirmix.cpp$(DependSuffix): dirmix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dirmix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dirmix.cpp$(DependSuffix) -MM "dirmix.cpp"

$(IntermediateDirectory)/dirmix.cpp$(PreprocessSuffix): dirmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dirmix.cpp$(PreprocessSuffix) "dirmix.cpp"

$(IntermediateDirectory)/dizlist.cpp$(ObjectSuffix): dizlist.cpp $(IntermediateDirectory)/dizlist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dizlist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dizlist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dizlist.cpp$(DependSuffix): dizlist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dizlist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dizlist.cpp$(DependSuffix) -MM "dizlist.cpp"

$(IntermediateDirectory)/dizlist.cpp$(PreprocessSuffix): dizlist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dizlist.cpp$(PreprocessSuffix) "dizlist.cpp"

$(IntermediateDirectory)/DlgBuilder.cpp$(ObjectSuffix): DlgBuilder.cpp $(IntermediateDirectory)/DlgBuilder.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./DlgBuilder.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DlgBuilder.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DlgBuilder.cpp$(DependSuffix): DlgBuilder.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DlgBuilder.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DlgBuilder.cpp$(DependSuffix) -MM "DlgBuilder.cpp"

$(IntermediateDirectory)/DlgBuilder.cpp$(PreprocessSuffix): DlgBuilder.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DlgBuilder.cpp$(PreprocessSuffix) "DlgBuilder.cpp"

$(IntermediateDirectory)/dlgedit.cpp$(ObjectSuffix): dlgedit.cpp $(IntermediateDirectory)/dlgedit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./dlgedit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dlgedit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dlgedit.cpp$(DependSuffix): dlgedit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dlgedit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dlgedit.cpp$(DependSuffix) -MM "dlgedit.cpp"

$(IntermediateDirectory)/dlgedit.cpp$(PreprocessSuffix): dlgedit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dlgedit.cpp$(PreprocessSuffix) "dlgedit.cpp"

$(IntermediateDirectory)/DlgGuid.cpp$(ObjectSuffix): DlgGuid.cpp $(IntermediateDirectory)/DlgGuid.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./DlgGuid.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DlgGuid.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DlgGuid.cpp$(DependSuffix): DlgGuid.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DlgGuid.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DlgGuid.cpp$(DependSuffix) -MM "DlgGuid.cpp"

$(IntermediateDirectory)/DlgGuid.cpp$(PreprocessSuffix): DlgGuid.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DlgGuid.cpp$(PreprocessSuffix) "DlgGuid.cpp"

$(IntermediateDirectory)/DList.cpp$(ObjectSuffix): DList.cpp $(IntermediateDirectory)/DList.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./DList.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DList.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DList.cpp$(DependSuffix): DList.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DList.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DList.cpp$(DependSuffix) -MM "DList.cpp"

$(IntermediateDirectory)/DList.cpp$(PreprocessSuffix): DList.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DList.cpp$(PreprocessSuffix) "DList.cpp"

$(IntermediateDirectory)/drivemix.cpp$(ObjectSuffix): drivemix.cpp $(IntermediateDirectory)/drivemix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./drivemix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/drivemix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/drivemix.cpp$(DependSuffix): drivemix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/drivemix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/drivemix.cpp$(DependSuffix) -MM "drivemix.cpp"

$(IntermediateDirectory)/drivemix.cpp$(PreprocessSuffix): drivemix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/drivemix.cpp$(PreprocessSuffix) "drivemix.cpp"

$(IntermediateDirectory)/edit.cpp$(ObjectSuffix): edit.cpp $(IntermediateDirectory)/edit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./edit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/edit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/edit.cpp$(DependSuffix): edit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/edit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/edit.cpp$(DependSuffix) -MM "edit.cpp"

$(IntermediateDirectory)/edit.cpp$(PreprocessSuffix): edit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/edit.cpp$(PreprocessSuffix) "edit.cpp"

$(IntermediateDirectory)/editor.cpp$(ObjectSuffix): editor.cpp $(IntermediateDirectory)/editor.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./editor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/editor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/editor.cpp$(DependSuffix): editor.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/editor.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/editor.cpp$(DependSuffix) -MM "editor.cpp"

$(IntermediateDirectory)/editor.cpp$(PreprocessSuffix): editor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/editor.cpp$(PreprocessSuffix) "editor.cpp"

$(IntermediateDirectory)/execute.cpp$(ObjectSuffix): execute.cpp $(IntermediateDirectory)/execute.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./execute.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/execute.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/execute.cpp$(DependSuffix): execute.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/execute.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/execute.cpp$(DependSuffix) -MM "execute.cpp"

$(IntermediateDirectory)/execute.cpp$(PreprocessSuffix): execute.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/execute.cpp$(PreprocessSuffix) "execute.cpp"

$(IntermediateDirectory)/farqueue.cpp$(ObjectSuffix): farqueue.cpp $(IntermediateDirectory)/farqueue.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./farqueue.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/farqueue.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/farqueue.cpp$(DependSuffix): farqueue.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/farqueue.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/farqueue.cpp$(DependSuffix) -MM "farqueue.cpp"

$(IntermediateDirectory)/farqueue.cpp$(PreprocessSuffix): farqueue.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/farqueue.cpp$(PreprocessSuffix) "farqueue.cpp"

$(IntermediateDirectory)/farrtl.cpp$(ObjectSuffix): farrtl.cpp $(IntermediateDirectory)/farrtl.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./farrtl.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/farrtl.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/farrtl.cpp$(DependSuffix): farrtl.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/farrtl.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/farrtl.cpp$(DependSuffix) -MM "farrtl.cpp"

$(IntermediateDirectory)/farrtl.cpp$(PreprocessSuffix): farrtl.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/farrtl.cpp$(PreprocessSuffix) "farrtl.cpp"

$(IntermediateDirectory)/farwinapi.cpp$(ObjectSuffix): farwinapi.cpp $(IntermediateDirectory)/farwinapi.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./farwinapi.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/farwinapi.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/farwinapi.cpp$(DependSuffix): farwinapi.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/farwinapi.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/farwinapi.cpp$(DependSuffix) -MM "farwinapi.cpp"

$(IntermediateDirectory)/farwinapi.cpp$(PreprocessSuffix): farwinapi.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/farwinapi.cpp$(PreprocessSuffix) "farwinapi.cpp"

$(IntermediateDirectory)/ffolders.cpp$(ObjectSuffix): ffolders.cpp $(IntermediateDirectory)/ffolders.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./ffolders.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ffolders.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ffolders.cpp$(DependSuffix): ffolders.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ffolders.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ffolders.cpp$(DependSuffix) -MM "ffolders.cpp"

$(IntermediateDirectory)/ffolders.cpp$(PreprocessSuffix): ffolders.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ffolders.cpp$(PreprocessSuffix) "ffolders.cpp"

$(IntermediateDirectory)/fileattr.cpp$(ObjectSuffix): fileattr.cpp $(IntermediateDirectory)/fileattr.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileattr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileattr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileattr.cpp$(DependSuffix): fileattr.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fileattr.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/fileattr.cpp$(DependSuffix) -MM "fileattr.cpp"

$(IntermediateDirectory)/fileattr.cpp$(PreprocessSuffix): fileattr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileattr.cpp$(PreprocessSuffix) "fileattr.cpp"

$(IntermediateDirectory)/fileedit.cpp$(ObjectSuffix): fileedit.cpp $(IntermediateDirectory)/fileedit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileedit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileedit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileedit.cpp$(DependSuffix): fileedit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fileedit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/fileedit.cpp$(DependSuffix) -MM "fileedit.cpp"

$(IntermediateDirectory)/fileedit.cpp$(PreprocessSuffix): fileedit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileedit.cpp$(PreprocessSuffix) "fileedit.cpp"

$(IntermediateDirectory)/filefilter.cpp$(ObjectSuffix): filefilter.cpp $(IntermediateDirectory)/filefilter.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filefilter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filefilter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filefilter.cpp$(DependSuffix): filefilter.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/filefilter.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/filefilter.cpp$(DependSuffix) -MM "filefilter.cpp"

$(IntermediateDirectory)/filefilter.cpp$(PreprocessSuffix): filefilter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filefilter.cpp$(PreprocessSuffix) "filefilter.cpp"

$(IntermediateDirectory)/filefilterparams.cpp$(ObjectSuffix): filefilterparams.cpp $(IntermediateDirectory)/filefilterparams.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filefilterparams.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filefilterparams.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filefilterparams.cpp$(DependSuffix): filefilterparams.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/filefilterparams.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/filefilterparams.cpp$(DependSuffix) -MM "filefilterparams.cpp"

$(IntermediateDirectory)/filefilterparams.cpp$(PreprocessSuffix): filefilterparams.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filefilterparams.cpp$(PreprocessSuffix) "filefilterparams.cpp"

$(IntermediateDirectory)/filelist.cpp$(ObjectSuffix): filelist.cpp $(IntermediateDirectory)/filelist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filelist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filelist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filelist.cpp$(DependSuffix): filelist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/filelist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/filelist.cpp$(DependSuffix) -MM "filelist.cpp"

$(IntermediateDirectory)/filelist.cpp$(PreprocessSuffix): filelist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filelist.cpp$(PreprocessSuffix) "filelist.cpp"

$(IntermediateDirectory)/FileMasksProcessor.cpp$(ObjectSuffix): FileMasksProcessor.cpp $(IntermediateDirectory)/FileMasksProcessor.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./FileMasksProcessor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FileMasksProcessor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FileMasksProcessor.cpp$(DependSuffix): FileMasksProcessor.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FileMasksProcessor.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FileMasksProcessor.cpp$(DependSuffix) -MM "FileMasksProcessor.cpp"

$(IntermediateDirectory)/FileMasksProcessor.cpp$(PreprocessSuffix): FileMasksProcessor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FileMasksProcessor.cpp$(PreprocessSuffix) "FileMasksProcessor.cpp"

$(IntermediateDirectory)/FileMasksWithExclude.cpp$(ObjectSuffix): FileMasksWithExclude.cpp $(IntermediateDirectory)/FileMasksWithExclude.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./FileMasksWithExclude.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FileMasksWithExclude.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FileMasksWithExclude.cpp$(DependSuffix): FileMasksWithExclude.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FileMasksWithExclude.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FileMasksWithExclude.cpp$(DependSuffix) -MM "FileMasksWithExclude.cpp"

$(IntermediateDirectory)/FileMasksWithExclude.cpp$(PreprocessSuffix): FileMasksWithExclude.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FileMasksWithExclude.cpp$(PreprocessSuffix) "FileMasksWithExclude.cpp"

$(IntermediateDirectory)/fileowner.cpp$(ObjectSuffix): fileowner.cpp $(IntermediateDirectory)/fileowner.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileowner.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileowner.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileowner.cpp$(DependSuffix): fileowner.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fileowner.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/fileowner.cpp$(DependSuffix) -MM "fileowner.cpp"

$(IntermediateDirectory)/fileowner.cpp$(PreprocessSuffix): fileowner.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileowner.cpp$(PreprocessSuffix) "fileowner.cpp"

$(IntermediateDirectory)/filepanels.cpp$(ObjectSuffix): filepanels.cpp $(IntermediateDirectory)/filepanels.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filepanels.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filepanels.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filepanels.cpp$(DependSuffix): filepanels.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/filepanels.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/filepanels.cpp$(DependSuffix) -MM "filepanels.cpp"

$(IntermediateDirectory)/filepanels.cpp$(PreprocessSuffix): filepanels.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filepanels.cpp$(PreprocessSuffix) "filepanels.cpp"

$(IntermediateDirectory)/filestr.cpp$(ObjectSuffix): filestr.cpp $(IntermediateDirectory)/filestr.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filestr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filestr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filestr.cpp$(DependSuffix): filestr.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/filestr.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/filestr.cpp$(DependSuffix) -MM "filestr.cpp"

$(IntermediateDirectory)/filestr.cpp$(PreprocessSuffix): filestr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filestr.cpp$(PreprocessSuffix) "filestr.cpp"

$(IntermediateDirectory)/filetype.cpp$(ObjectSuffix): filetype.cpp $(IntermediateDirectory)/filetype.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./filetype.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/filetype.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/filetype.cpp$(DependSuffix): filetype.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/filetype.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/filetype.cpp$(DependSuffix) -MM "filetype.cpp"

$(IntermediateDirectory)/filetype.cpp$(PreprocessSuffix): filetype.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/filetype.cpp$(PreprocessSuffix) "filetype.cpp"

$(IntermediateDirectory)/fileview.cpp$(ObjectSuffix): fileview.cpp $(IntermediateDirectory)/fileview.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fileview.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fileview.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fileview.cpp$(DependSuffix): fileview.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fileview.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/fileview.cpp$(DependSuffix) -MM "fileview.cpp"

$(IntermediateDirectory)/fileview.cpp$(PreprocessSuffix): fileview.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fileview.cpp$(PreprocessSuffix) "fileview.cpp"

$(IntermediateDirectory)/findfile.cpp$(ObjectSuffix): findfile.cpp $(IntermediateDirectory)/findfile.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./findfile.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/findfile.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/findfile.cpp$(DependSuffix): findfile.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/findfile.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/findfile.cpp$(DependSuffix) -MM "findfile.cpp"

$(IntermediateDirectory)/findfile.cpp$(PreprocessSuffix): findfile.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/findfile.cpp$(PreprocessSuffix) "findfile.cpp"

$(IntermediateDirectory)/flmodes.cpp$(ObjectSuffix): flmodes.cpp $(IntermediateDirectory)/flmodes.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flmodes.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flmodes.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flmodes.cpp$(DependSuffix): flmodes.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/flmodes.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/flmodes.cpp$(DependSuffix) -MM "flmodes.cpp"

$(IntermediateDirectory)/flmodes.cpp$(PreprocessSuffix): flmodes.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flmodes.cpp$(PreprocessSuffix) "flmodes.cpp"

$(IntermediateDirectory)/flplugin.cpp$(ObjectSuffix): flplugin.cpp $(IntermediateDirectory)/flplugin.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flplugin.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flplugin.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flplugin.cpp$(DependSuffix): flplugin.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/flplugin.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/flplugin.cpp$(DependSuffix) -MM "flplugin.cpp"

$(IntermediateDirectory)/flplugin.cpp$(PreprocessSuffix): flplugin.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flplugin.cpp$(PreprocessSuffix) "flplugin.cpp"

$(IntermediateDirectory)/flshow.cpp$(ObjectSuffix): flshow.cpp $(IntermediateDirectory)/flshow.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flshow.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flshow.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flshow.cpp$(DependSuffix): flshow.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/flshow.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/flshow.cpp$(DependSuffix) -MM "flshow.cpp"

$(IntermediateDirectory)/flshow.cpp$(PreprocessSuffix): flshow.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flshow.cpp$(PreprocessSuffix) "flshow.cpp"

$(IntermediateDirectory)/flupdate.cpp$(ObjectSuffix): flupdate.cpp $(IntermediateDirectory)/flupdate.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./flupdate.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/flupdate.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/flupdate.cpp$(DependSuffix): flupdate.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/flupdate.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/flupdate.cpp$(DependSuffix) -MM "flupdate.cpp"

$(IntermediateDirectory)/flupdate.cpp$(PreprocessSuffix): flupdate.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/flupdate.cpp$(PreprocessSuffix) "flupdate.cpp"

$(IntermediateDirectory)/fnparce.cpp$(ObjectSuffix): fnparce.cpp $(IntermediateDirectory)/fnparce.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./fnparce.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fnparce.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fnparce.cpp$(DependSuffix): fnparce.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fnparce.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/fnparce.cpp$(DependSuffix) -MM "fnparce.cpp"

$(IntermediateDirectory)/fnparce.cpp$(PreprocessSuffix): fnparce.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fnparce.cpp$(PreprocessSuffix) "fnparce.cpp"

$(IntermediateDirectory)/foldtree.cpp$(ObjectSuffix): foldtree.cpp $(IntermediateDirectory)/foldtree.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./foldtree.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/foldtree.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/foldtree.cpp$(DependSuffix): foldtree.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/foldtree.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/foldtree.cpp$(DependSuffix) -MM "foldtree.cpp"

$(IntermediateDirectory)/foldtree.cpp$(PreprocessSuffix): foldtree.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/foldtree.cpp$(PreprocessSuffix) "foldtree.cpp"

$(IntermediateDirectory)/format.cpp$(ObjectSuffix): format.cpp $(IntermediateDirectory)/format.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./format.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/format.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/format.cpp$(DependSuffix): format.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/format.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/format.cpp$(DependSuffix) -MM "format.cpp"

$(IntermediateDirectory)/format.cpp$(PreprocessSuffix): format.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/format.cpp$(PreprocessSuffix) "format.cpp"

$(IntermediateDirectory)/frame.cpp$(ObjectSuffix): frame.cpp $(IntermediateDirectory)/frame.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./frame.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/frame.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/frame.cpp$(DependSuffix): frame.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/frame.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/frame.cpp$(DependSuffix) -MM "frame.cpp"

$(IntermediateDirectory)/frame.cpp$(PreprocessSuffix): frame.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/frame.cpp$(PreprocessSuffix) "frame.cpp"

$(IntermediateDirectory)/global.cpp$(ObjectSuffix): global.cpp $(IntermediateDirectory)/global.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./global.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/global.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/global.cpp$(DependSuffix): global.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/global.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/global.cpp$(DependSuffix) -MM "global.cpp"

$(IntermediateDirectory)/global.cpp$(PreprocessSuffix): global.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/global.cpp$(PreprocessSuffix) "global.cpp"

$(IntermediateDirectory)/grabber.cpp$(ObjectSuffix): grabber.cpp $(IntermediateDirectory)/grabber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./grabber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/grabber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/grabber.cpp$(DependSuffix): grabber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/grabber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/grabber.cpp$(DependSuffix) -MM "grabber.cpp"

$(IntermediateDirectory)/grabber.cpp$(PreprocessSuffix): grabber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/grabber.cpp$(PreprocessSuffix) "grabber.cpp"

$(IntermediateDirectory)/headers.cpp$(ObjectSuffix): headers.cpp $(IntermediateDirectory)/headers.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./headers.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/headers.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/headers.cpp$(DependSuffix): headers.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/headers.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/headers.cpp$(DependSuffix) -MM "headers.cpp"

$(IntermediateDirectory)/headers.cpp$(PreprocessSuffix): headers.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/headers.cpp$(PreprocessSuffix) "headers.cpp"

$(IntermediateDirectory)/help.cpp$(ObjectSuffix): help.cpp $(IntermediateDirectory)/help.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./help.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/help.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/help.cpp$(DependSuffix): help.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/help.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/help.cpp$(DependSuffix) -MM "help.cpp"

$(IntermediateDirectory)/help.cpp$(PreprocessSuffix): help.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/help.cpp$(PreprocessSuffix) "help.cpp"

$(IntermediateDirectory)/hilight.cpp$(ObjectSuffix): hilight.cpp $(IntermediateDirectory)/hilight.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./hilight.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/hilight.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hilight.cpp$(DependSuffix): hilight.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/hilight.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/hilight.cpp$(DependSuffix) -MM "hilight.cpp"

$(IntermediateDirectory)/hilight.cpp$(PreprocessSuffix): hilight.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hilight.cpp$(PreprocessSuffix) "hilight.cpp"

$(IntermediateDirectory)/history.cpp$(ObjectSuffix): history.cpp $(IntermediateDirectory)/history.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./history.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/history.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/history.cpp$(DependSuffix): history.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/history.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/history.cpp$(DependSuffix) -MM "history.cpp"

$(IntermediateDirectory)/history.cpp$(PreprocessSuffix): history.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/history.cpp$(PreprocessSuffix) "history.cpp"

$(IntermediateDirectory)/hmenu.cpp$(ObjectSuffix): hmenu.cpp $(IntermediateDirectory)/hmenu.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./hmenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/hmenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hmenu.cpp$(DependSuffix): hmenu.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/hmenu.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/hmenu.cpp$(DependSuffix) -MM "hmenu.cpp"

$(IntermediateDirectory)/hmenu.cpp$(PreprocessSuffix): hmenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hmenu.cpp$(PreprocessSuffix) "hmenu.cpp"

$(IntermediateDirectory)/hotplug.cpp$(ObjectSuffix): hotplug.cpp $(IntermediateDirectory)/hotplug.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./hotplug.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/hotplug.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hotplug.cpp$(DependSuffix): hotplug.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/hotplug.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/hotplug.cpp$(DependSuffix) -MM "hotplug.cpp"

$(IntermediateDirectory)/hotplug.cpp$(PreprocessSuffix): hotplug.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hotplug.cpp$(PreprocessSuffix) "hotplug.cpp"

$(IntermediateDirectory)/infolist.cpp$(ObjectSuffix): infolist.cpp $(IntermediateDirectory)/infolist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./infolist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/infolist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/infolist.cpp$(DependSuffix): infolist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/infolist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/infolist.cpp$(DependSuffix) -MM "infolist.cpp"

$(IntermediateDirectory)/infolist.cpp$(PreprocessSuffix): infolist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/infolist.cpp$(PreprocessSuffix) "infolist.cpp"

$(IntermediateDirectory)/interf.cpp$(ObjectSuffix): interf.cpp $(IntermediateDirectory)/interf.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./interf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/interf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/interf.cpp$(DependSuffix): interf.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/interf.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/interf.cpp$(DependSuffix) -MM "interf.cpp"

$(IntermediateDirectory)/interf.cpp$(PreprocessSuffix): interf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/interf.cpp$(PreprocessSuffix) "interf.cpp"

$(IntermediateDirectory)/keybar.cpp$(ObjectSuffix): keybar.cpp $(IntermediateDirectory)/keybar.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./keybar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/keybar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/keybar.cpp$(DependSuffix): keybar.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/keybar.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/keybar.cpp$(DependSuffix) -MM "keybar.cpp"

$(IntermediateDirectory)/keybar.cpp$(PreprocessSuffix): keybar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/keybar.cpp$(PreprocessSuffix) "keybar.cpp"

$(IntermediateDirectory)/keyboard.cpp$(ObjectSuffix): keyboard.cpp $(IntermediateDirectory)/keyboard.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./keyboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/keyboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/keyboard.cpp$(DependSuffix): keyboard.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/keyboard.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/keyboard.cpp$(DependSuffix) -MM "keyboard.cpp"

$(IntermediateDirectory)/keyboard.cpp$(PreprocessSuffix): keyboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/keyboard.cpp$(PreprocessSuffix) "keyboard.cpp"

$(IntermediateDirectory)/language.cpp$(ObjectSuffix): language.cpp $(IntermediateDirectory)/language.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./language.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/language.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/language.cpp$(DependSuffix): language.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/language.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/language.cpp$(DependSuffix) -MM "language.cpp"

$(IntermediateDirectory)/language.cpp$(PreprocessSuffix): language.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/language.cpp$(PreprocessSuffix) "language.cpp"

$(IntermediateDirectory)/local.cpp$(ObjectSuffix): local.cpp $(IntermediateDirectory)/local.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./local.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/local.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/local.cpp$(DependSuffix): local.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/local.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/local.cpp$(DependSuffix) -MM "local.cpp"

$(IntermediateDirectory)/local.cpp$(PreprocessSuffix): local.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/local.cpp$(PreprocessSuffix) "local.cpp"

$(IntermediateDirectory)/lockscrn.cpp$(ObjectSuffix): lockscrn.cpp $(IntermediateDirectory)/lockscrn.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lockscrn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/lockscrn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/lockscrn.cpp$(DependSuffix): lockscrn.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/lockscrn.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/lockscrn.cpp$(DependSuffix) -MM "lockscrn.cpp"

$(IntermediateDirectory)/lockscrn.cpp$(PreprocessSuffix): lockscrn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/lockscrn.cpp$(PreprocessSuffix) "lockscrn.cpp"

$(IntermediateDirectory)/macro.cpp$(ObjectSuffix): macro.cpp $(IntermediateDirectory)/macro.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./macro.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/macro.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/macro.cpp$(DependSuffix): macro.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/macro.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/macro.cpp$(DependSuffix) -MM "macro.cpp"

$(IntermediateDirectory)/macro.cpp$(PreprocessSuffix): macro.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/macro.cpp$(PreprocessSuffix) "macro.cpp"

$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM "main.cpp"

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) "main.cpp"

$(IntermediateDirectory)/manager.cpp$(ObjectSuffix): manager.cpp $(IntermediateDirectory)/manager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./manager.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/manager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/manager.cpp$(DependSuffix): manager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/manager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/manager.cpp$(DependSuffix) -MM "manager.cpp"

$(IntermediateDirectory)/manager.cpp$(PreprocessSuffix): manager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/manager.cpp$(PreprocessSuffix) "manager.cpp"

$(IntermediateDirectory)/menubar.cpp$(ObjectSuffix): menubar.cpp $(IntermediateDirectory)/menubar.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./menubar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/menubar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/menubar.cpp$(DependSuffix): menubar.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/menubar.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/menubar.cpp$(DependSuffix) -MM "menubar.cpp"

$(IntermediateDirectory)/menubar.cpp$(PreprocessSuffix): menubar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/menubar.cpp$(PreprocessSuffix) "menubar.cpp"

$(IntermediateDirectory)/message.cpp$(ObjectSuffix): message.cpp $(IntermediateDirectory)/message.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./message.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/message.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/message.cpp$(DependSuffix): message.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/message.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/message.cpp$(DependSuffix) -MM "message.cpp"

$(IntermediateDirectory)/message.cpp$(PreprocessSuffix): message.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/message.cpp$(PreprocessSuffix) "message.cpp"

$(IntermediateDirectory)/mix.cpp$(ObjectSuffix): mix.cpp $(IntermediateDirectory)/mix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./mix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mix.cpp$(DependSuffix): mix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/mix.cpp$(DependSuffix) -MM "mix.cpp"

$(IntermediateDirectory)/mix.cpp$(PreprocessSuffix): mix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mix.cpp$(PreprocessSuffix) "mix.cpp"

$(IntermediateDirectory)/mkdir.cpp$(ObjectSuffix): mkdir.cpp $(IntermediateDirectory)/mkdir.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./mkdir.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mkdir.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mkdir.cpp$(DependSuffix): mkdir.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mkdir.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/mkdir.cpp$(DependSuffix) -MM "mkdir.cpp"

$(IntermediateDirectory)/mkdir.cpp$(PreprocessSuffix): mkdir.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mkdir.cpp$(PreprocessSuffix) "mkdir.cpp"

$(IntermediateDirectory)/modal.cpp$(ObjectSuffix): modal.cpp $(IntermediateDirectory)/modal.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./modal.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/modal.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/modal.cpp$(DependSuffix): modal.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/modal.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/modal.cpp$(DependSuffix) -MM "modal.cpp"

$(IntermediateDirectory)/modal.cpp$(PreprocessSuffix): modal.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/modal.cpp$(PreprocessSuffix) "modal.cpp"

$(IntermediateDirectory)/namelist.cpp$(ObjectSuffix): namelist.cpp $(IntermediateDirectory)/namelist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./namelist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/namelist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/namelist.cpp$(DependSuffix): namelist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/namelist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/namelist.cpp$(DependSuffix) -MM "namelist.cpp"

$(IntermediateDirectory)/namelist.cpp$(PreprocessSuffix): namelist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/namelist.cpp$(PreprocessSuffix) "namelist.cpp"

$(IntermediateDirectory)/options.cpp$(ObjectSuffix): options.cpp $(IntermediateDirectory)/options.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./options.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/options.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/options.cpp$(DependSuffix): options.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/options.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/options.cpp$(DependSuffix) -MM "options.cpp"

$(IntermediateDirectory)/options.cpp$(PreprocessSuffix): options.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/options.cpp$(PreprocessSuffix) "options.cpp"

$(IntermediateDirectory)/palette.cpp$(ObjectSuffix): palette.cpp $(IntermediateDirectory)/palette.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./palette.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/palette.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/palette.cpp$(DependSuffix): palette.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/palette.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/palette.cpp$(DependSuffix) -MM "palette.cpp"

$(IntermediateDirectory)/palette.cpp$(PreprocessSuffix): palette.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/palette.cpp$(PreprocessSuffix) "palette.cpp"

$(IntermediateDirectory)/panel.cpp$(ObjectSuffix): panel.cpp $(IntermediateDirectory)/panel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./panel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/panel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/panel.cpp$(DependSuffix): panel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/panel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/panel.cpp$(DependSuffix) -MM "panel.cpp"

$(IntermediateDirectory)/panel.cpp$(PreprocessSuffix): panel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/panel.cpp$(PreprocessSuffix) "panel.cpp"

$(IntermediateDirectory)/panelmix.cpp$(ObjectSuffix): panelmix.cpp $(IntermediateDirectory)/panelmix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./panelmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/panelmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/panelmix.cpp$(DependSuffix): panelmix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/panelmix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/panelmix.cpp$(DependSuffix) -MM "panelmix.cpp"

$(IntermediateDirectory)/panelmix.cpp$(PreprocessSuffix): panelmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/panelmix.cpp$(PreprocessSuffix) "panelmix.cpp"

$(IntermediateDirectory)/pathmix.cpp$(ObjectSuffix): pathmix.cpp $(IntermediateDirectory)/pathmix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./pathmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pathmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pathmix.cpp$(DependSuffix): pathmix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/pathmix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/pathmix.cpp$(DependSuffix) -MM "pathmix.cpp"

$(IntermediateDirectory)/pathmix.cpp$(PreprocessSuffix): pathmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pathmix.cpp$(PreprocessSuffix) "pathmix.cpp"

$(IntermediateDirectory)/plist.cpp$(ObjectSuffix): plist.cpp $(IntermediateDirectory)/plist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plist.cpp$(DependSuffix): plist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/plist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/plist.cpp$(DependSuffix) -MM "plist.cpp"

$(IntermediateDirectory)/plist.cpp$(PreprocessSuffix): plist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plist.cpp$(PreprocessSuffix) "plist.cpp"

$(IntermediateDirectory)/plognmn.cpp$(ObjectSuffix): plognmn.cpp $(IntermediateDirectory)/plognmn.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plognmn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plognmn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plognmn.cpp$(DependSuffix): plognmn.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/plognmn.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/plognmn.cpp$(DependSuffix) -MM "plognmn.cpp"

$(IntermediateDirectory)/plognmn.cpp$(PreprocessSuffix): plognmn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plognmn.cpp$(PreprocessSuffix) "plognmn.cpp"

$(IntermediateDirectory)/plugapi.cpp$(ObjectSuffix): plugapi.cpp $(IntermediateDirectory)/plugapi.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plugapi.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plugapi.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plugapi.cpp$(DependSuffix): plugapi.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/plugapi.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/plugapi.cpp$(DependSuffix) -MM "plugapi.cpp"

$(IntermediateDirectory)/plugapi.cpp$(PreprocessSuffix): plugapi.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plugapi.cpp$(PreprocessSuffix) "plugapi.cpp"

$(IntermediateDirectory)/plugins.cpp$(ObjectSuffix): plugins.cpp $(IntermediateDirectory)/plugins.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./plugins.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/plugins.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/plugins.cpp$(DependSuffix): plugins.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/plugins.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/plugins.cpp$(DependSuffix) -MM "plugins.cpp"

$(IntermediateDirectory)/plugins.cpp$(PreprocessSuffix): plugins.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/plugins.cpp$(PreprocessSuffix) "plugins.cpp"

$(IntermediateDirectory)/PluginW.cpp$(ObjectSuffix): PluginW.cpp $(IntermediateDirectory)/PluginW.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./PluginW.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/PluginW.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/PluginW.cpp$(DependSuffix): PluginW.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/PluginW.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/PluginW.cpp$(DependSuffix) -MM "PluginW.cpp"

$(IntermediateDirectory)/PluginW.cpp$(PreprocessSuffix): PluginW.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/PluginW.cpp$(PreprocessSuffix) "PluginW.cpp"

$(IntermediateDirectory)/poscache.cpp$(ObjectSuffix): poscache.cpp $(IntermediateDirectory)/poscache.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./poscache.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/poscache.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/poscache.cpp$(DependSuffix): poscache.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/poscache.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/poscache.cpp$(DependSuffix) -MM "poscache.cpp"

$(IntermediateDirectory)/poscache.cpp$(PreprocessSuffix): poscache.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/poscache.cpp$(PreprocessSuffix) "poscache.cpp"

$(IntermediateDirectory)/processname.cpp$(ObjectSuffix): processname.cpp $(IntermediateDirectory)/processname.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./processname.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/processname.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/processname.cpp$(DependSuffix): processname.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/processname.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/processname.cpp$(DependSuffix) -MM "processname.cpp"

$(IntermediateDirectory)/processname.cpp$(PreprocessSuffix): processname.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/processname.cpp$(PreprocessSuffix) "processname.cpp"

$(IntermediateDirectory)/qview.cpp$(ObjectSuffix): qview.cpp $(IntermediateDirectory)/qview.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./qview.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/qview.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/qview.cpp$(DependSuffix): qview.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/qview.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/qview.cpp$(DependSuffix) -MM "qview.cpp"

$(IntermediateDirectory)/qview.cpp$(PreprocessSuffix): qview.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/qview.cpp$(PreprocessSuffix) "qview.cpp"

$(IntermediateDirectory)/rdrwdsk.cpp$(ObjectSuffix): rdrwdsk.cpp $(IntermediateDirectory)/rdrwdsk.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./rdrwdsk.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/rdrwdsk.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/rdrwdsk.cpp$(DependSuffix): rdrwdsk.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/rdrwdsk.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/rdrwdsk.cpp$(DependSuffix) -MM "rdrwdsk.cpp"

$(IntermediateDirectory)/rdrwdsk.cpp$(PreprocessSuffix): rdrwdsk.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/rdrwdsk.cpp$(PreprocessSuffix) "rdrwdsk.cpp"

$(IntermediateDirectory)/RefreshFrameManager.cpp$(ObjectSuffix): RefreshFrameManager.cpp $(IntermediateDirectory)/RefreshFrameManager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./RefreshFrameManager.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/RefreshFrameManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/RefreshFrameManager.cpp$(DependSuffix): RefreshFrameManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/RefreshFrameManager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/RefreshFrameManager.cpp$(DependSuffix) -MM "RefreshFrameManager.cpp"

$(IntermediateDirectory)/RefreshFrameManager.cpp$(PreprocessSuffix): RefreshFrameManager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/RefreshFrameManager.cpp$(PreprocessSuffix) "RefreshFrameManager.cpp"

$(IntermediateDirectory)/RegExp.cpp$(ObjectSuffix): RegExp.cpp $(IntermediateDirectory)/RegExp.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./RegExp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/RegExp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/RegExp.cpp$(DependSuffix): RegExp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/RegExp.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/RegExp.cpp$(DependSuffix) -MM "RegExp.cpp"

$(IntermediateDirectory)/RegExp.cpp$(PreprocessSuffix): RegExp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/RegExp.cpp$(PreprocessSuffix) "RegExp.cpp"

$(IntermediateDirectory)/registry.cpp$(ObjectSuffix): registry.cpp $(IntermediateDirectory)/registry.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./registry.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/registry.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/registry.cpp$(DependSuffix): registry.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/registry.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/registry.cpp$(DependSuffix) -MM "registry.cpp"

$(IntermediateDirectory)/registry.cpp$(PreprocessSuffix): registry.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/registry.cpp$(PreprocessSuffix) "registry.cpp"

$(IntermediateDirectory)/savefpos.cpp$(ObjectSuffix): savefpos.cpp $(IntermediateDirectory)/savefpos.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./savefpos.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/savefpos.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/savefpos.cpp$(DependSuffix): savefpos.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/savefpos.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/savefpos.cpp$(DependSuffix) -MM "savefpos.cpp"

$(IntermediateDirectory)/savefpos.cpp$(PreprocessSuffix): savefpos.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/savefpos.cpp$(PreprocessSuffix) "savefpos.cpp"

$(IntermediateDirectory)/savescr.cpp$(ObjectSuffix): savescr.cpp $(IntermediateDirectory)/savescr.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./savescr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/savescr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/savescr.cpp$(DependSuffix): savescr.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/savescr.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/savescr.cpp$(DependSuffix) -MM "savescr.cpp"

$(IntermediateDirectory)/savescr.cpp$(PreprocessSuffix): savescr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/savescr.cpp$(PreprocessSuffix) "savescr.cpp"

$(IntermediateDirectory)/scantree.cpp$(ObjectSuffix): scantree.cpp $(IntermediateDirectory)/scantree.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scantree.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scantree.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scantree.cpp$(DependSuffix): scantree.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/scantree.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/scantree.cpp$(DependSuffix) -MM "scantree.cpp"

$(IntermediateDirectory)/scantree.cpp$(PreprocessSuffix): scantree.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scantree.cpp$(PreprocessSuffix) "scantree.cpp"

$(IntermediateDirectory)/scrbuf.cpp$(ObjectSuffix): scrbuf.cpp $(IntermediateDirectory)/scrbuf.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scrbuf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scrbuf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scrbuf.cpp$(DependSuffix): scrbuf.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/scrbuf.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/scrbuf.cpp$(DependSuffix) -MM "scrbuf.cpp"

$(IntermediateDirectory)/scrbuf.cpp$(PreprocessSuffix): scrbuf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scrbuf.cpp$(PreprocessSuffix) "scrbuf.cpp"

$(IntermediateDirectory)/scrobj.cpp$(ObjectSuffix): scrobj.cpp $(IntermediateDirectory)/scrobj.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scrobj.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scrobj.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scrobj.cpp$(DependSuffix): scrobj.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/scrobj.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/scrobj.cpp$(DependSuffix) -MM "scrobj.cpp"

$(IntermediateDirectory)/scrobj.cpp$(PreprocessSuffix): scrobj.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scrobj.cpp$(PreprocessSuffix) "scrobj.cpp"

$(IntermediateDirectory)/scrsaver.cpp$(ObjectSuffix): scrsaver.cpp $(IntermediateDirectory)/scrsaver.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./scrsaver.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/scrsaver.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/scrsaver.cpp$(DependSuffix): scrsaver.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/scrsaver.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/scrsaver.cpp$(DependSuffix) -MM "scrsaver.cpp"

$(IntermediateDirectory)/scrsaver.cpp$(PreprocessSuffix): scrsaver.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/scrsaver.cpp$(PreprocessSuffix) "scrsaver.cpp"

$(IntermediateDirectory)/setattr.cpp$(ObjectSuffix): setattr.cpp $(IntermediateDirectory)/setattr.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./setattr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/setattr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/setattr.cpp$(DependSuffix): setattr.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/setattr.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/setattr.cpp$(DependSuffix) -MM "setattr.cpp"

$(IntermediateDirectory)/setattr.cpp$(PreprocessSuffix): setattr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/setattr.cpp$(PreprocessSuffix) "setattr.cpp"

$(IntermediateDirectory)/setcolor.cpp$(ObjectSuffix): setcolor.cpp $(IntermediateDirectory)/setcolor.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./setcolor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/setcolor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/setcolor.cpp$(DependSuffix): setcolor.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/setcolor.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/setcolor.cpp$(DependSuffix) -MM "setcolor.cpp"

$(IntermediateDirectory)/setcolor.cpp$(PreprocessSuffix): setcolor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/setcolor.cpp$(PreprocessSuffix) "setcolor.cpp"

$(IntermediateDirectory)/stddlg.cpp$(ObjectSuffix): stddlg.cpp $(IntermediateDirectory)/stddlg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./stddlg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/stddlg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/stddlg.cpp$(DependSuffix): stddlg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/stddlg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/stddlg.cpp$(DependSuffix) -MM "stddlg.cpp"

$(IntermediateDirectory)/stddlg.cpp$(PreprocessSuffix): stddlg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/stddlg.cpp$(PreprocessSuffix) "stddlg.cpp"

$(IntermediateDirectory)/strmix.cpp$(ObjectSuffix): strmix.cpp $(IntermediateDirectory)/strmix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./strmix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/strmix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/strmix.cpp$(DependSuffix): strmix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/strmix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/strmix.cpp$(DependSuffix) -MM "strmix.cpp"

$(IntermediateDirectory)/strmix.cpp$(PreprocessSuffix): strmix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/strmix.cpp$(PreprocessSuffix) "strmix.cpp"

$(IntermediateDirectory)/synchro.cpp$(ObjectSuffix): synchro.cpp $(IntermediateDirectory)/synchro.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./synchro.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/synchro.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/synchro.cpp$(DependSuffix): synchro.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/synchro.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/synchro.cpp$(DependSuffix) -MM "synchro.cpp"

$(IntermediateDirectory)/synchro.cpp$(PreprocessSuffix): synchro.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/synchro.cpp$(PreprocessSuffix) "synchro.cpp"

$(IntermediateDirectory)/syntax.cpp$(ObjectSuffix): syntax.cpp $(IntermediateDirectory)/syntax.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./syntax.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/syntax.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/syntax.cpp$(DependSuffix): syntax.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/syntax.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/syntax.cpp$(DependSuffix) -MM "syntax.cpp"

$(IntermediateDirectory)/syntax.cpp$(PreprocessSuffix): syntax.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/syntax.cpp$(PreprocessSuffix) "syntax.cpp"

$(IntermediateDirectory)/syslog.cpp$(ObjectSuffix): syslog.cpp $(IntermediateDirectory)/syslog.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./syslog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/syslog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/syslog.cpp$(DependSuffix): syslog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/syslog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/syslog.cpp$(DependSuffix) -MM "syslog.cpp"

$(IntermediateDirectory)/syslog.cpp$(PreprocessSuffix): syslog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/syslog.cpp$(PreprocessSuffix) "syslog.cpp"

$(IntermediateDirectory)/TPreRedrawFunc.cpp$(ObjectSuffix): TPreRedrawFunc.cpp $(IntermediateDirectory)/TPreRedrawFunc.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./TPreRedrawFunc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/TPreRedrawFunc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/TPreRedrawFunc.cpp$(DependSuffix): TPreRedrawFunc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/TPreRedrawFunc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/TPreRedrawFunc.cpp$(DependSuffix) -MM "TPreRedrawFunc.cpp"

$(IntermediateDirectory)/TPreRedrawFunc.cpp$(PreprocessSuffix): TPreRedrawFunc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/TPreRedrawFunc.cpp$(PreprocessSuffix) "TPreRedrawFunc.cpp"

$(IntermediateDirectory)/treelist.cpp$(ObjectSuffix): treelist.cpp $(IntermediateDirectory)/treelist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./treelist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/treelist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/treelist.cpp$(DependSuffix): treelist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/treelist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/treelist.cpp$(DependSuffix) -MM "treelist.cpp"

$(IntermediateDirectory)/treelist.cpp$(PreprocessSuffix): treelist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/treelist.cpp$(PreprocessSuffix) "treelist.cpp"

$(IntermediateDirectory)/tvar.cpp$(ObjectSuffix): tvar.cpp $(IntermediateDirectory)/tvar.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./tvar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/tvar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/tvar.cpp$(DependSuffix): tvar.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/tvar.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/tvar.cpp$(DependSuffix) -MM "tvar.cpp"

$(IntermediateDirectory)/tvar.cpp$(PreprocessSuffix): tvar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/tvar.cpp$(PreprocessSuffix) "tvar.cpp"

$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix): udlist.cpp $(IntermediateDirectory)/udlist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./udlist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/udlist.cpp$(DependSuffix): udlist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/udlist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/udlist.cpp$(DependSuffix) -MM "udlist.cpp"

$(IntermediateDirectory)/udlist.cpp$(PreprocessSuffix): udlist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/udlist.cpp$(PreprocessSuffix) "udlist.cpp"

$(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix): UnicodeString.cpp $(IntermediateDirectory)/UnicodeString.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UnicodeString.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UnicodeString.cpp$(DependSuffix): UnicodeString.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UnicodeString.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UnicodeString.cpp$(DependSuffix) -MM "UnicodeString.cpp"

$(IntermediateDirectory)/UnicodeString.cpp$(PreprocessSuffix): UnicodeString.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UnicodeString.cpp$(PreprocessSuffix) "UnicodeString.cpp"

$(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix): usermenu.cpp $(IntermediateDirectory)/usermenu.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./usermenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/usermenu.cpp$(DependSuffix): usermenu.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/usermenu.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/usermenu.cpp$(DependSuffix) -MM "usermenu.cpp"

$(IntermediateDirectory)/usermenu.cpp$(PreprocessSuffix): usermenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/usermenu.cpp$(PreprocessSuffix) "usermenu.cpp"

$(IntermediateDirectory)/viewer.cpp$(ObjectSuffix): viewer.cpp $(IntermediateDirectory)/viewer.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./viewer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/viewer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/viewer.cpp$(DependSuffix): viewer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/viewer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/viewer.cpp$(DependSuffix) -MM "viewer.cpp"

$(IntermediateDirectory)/viewer.cpp$(PreprocessSuffix): viewer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/viewer.cpp$(PreprocessSuffix) "viewer.cpp"

$(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix): vmenu.cpp $(IntermediateDirectory)/vmenu.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vmenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vmenu.cpp$(DependSuffix): vmenu.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/vmenu.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/vmenu.cpp$(DependSuffix) -MM "vmenu.cpp"

$(IntermediateDirectory)/vmenu.cpp$(PreprocessSuffix): vmenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vmenu.cpp$(PreprocessSuffix) "vmenu.cpp"

$(IntermediateDirectory)/xlat.cpp$(ObjectSuffix): xlat.cpp $(IntermediateDirectory)/xlat.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./xlat.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/xlat.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/xlat.cpp$(DependSuffix): xlat.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/xlat.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/xlat.cpp$(DependSuffix) -MM "xlat.cpp"

$(IntermediateDirectory)/xlat.cpp$(PreprocessSuffix): xlat.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/xlat.cpp$(PreprocessSuffix) "xlat.cpp"

$(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix): vtshell.cpp $(IntermediateDirectory)/vtshell.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vtshell.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vtshell.cpp$(DependSuffix): vtshell.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/vtshell.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/vtshell.cpp$(DependSuffix) -MM "vtshell.cpp"

$(IntermediateDirectory)/vtshell.cpp$(PreprocessSuffix): vtshell.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vtshell.cpp$(PreprocessSuffix) "vtshell.cpp"

$(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix): vtansi.cpp $(IntermediateDirectory)/vtansi.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./vtansi.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/vtansi.cpp$(DependSuffix): vtansi.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/vtansi.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/vtansi.cpp$(DependSuffix) -MM "vtansi.cpp"

$(IntermediateDirectory)/vtansi.cpp$(PreprocessSuffix): vtansi.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/vtansi.cpp$(PreprocessSuffix) "vtansi.cpp"

$(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix): PluginA.cpp $(IntermediateDirectory)/PluginA.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./PluginA.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/PluginA.cpp$(DependSuffix): PluginA.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/PluginA.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/PluginA.cpp$(DependSuffix) -MM "PluginA.cpp"

$(IntermediateDirectory)/PluginA.cpp$(PreprocessSuffix): PluginA.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/PluginA.cpp$(PreprocessSuffix) "PluginA.cpp"

$(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix): UCD/CharDistribution.cpp $(IntermediateDirectory)/UCD_CharDistribution.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/CharDistribution.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_CharDistribution.cpp$(DependSuffix): UCD/CharDistribution.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_CharDistribution.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_CharDistribution.cpp$(DependSuffix) -MM "UCD/CharDistribution.cpp"

$(IntermediateDirectory)/UCD_CharDistribution.cpp$(PreprocessSuffix): UCD/CharDistribution.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_CharDistribution.cpp$(PreprocessSuffix) "UCD/CharDistribution.cpp"

$(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix): UCD/JpCntx.cpp $(IntermediateDirectory)/UCD_JpCntx.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/JpCntx.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_JpCntx.cpp$(DependSuffix): UCD/JpCntx.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_JpCntx.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_JpCntx.cpp$(DependSuffix) -MM "UCD/JpCntx.cpp"

$(IntermediateDirectory)/UCD_JpCntx.cpp$(PreprocessSuffix): UCD/JpCntx.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_JpCntx.cpp$(PreprocessSuffix) "UCD/JpCntx.cpp"

$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix): UCD/LangBulgarianModel.cpp $(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangBulgarianModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(DependSuffix): UCD/LangBulgarianModel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(DependSuffix) -MM "UCD/LangBulgarianModel.cpp"

$(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(PreprocessSuffix): UCD/LangBulgarianModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangBulgarianModel.cpp$(PreprocessSuffix) "UCD/LangBulgarianModel.cpp"

$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix): UCD/LangCyrillicModel.cpp $(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangCyrillicModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(DependSuffix): UCD/LangCyrillicModel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(DependSuffix) -MM "UCD/LangCyrillicModel.cpp"

$(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(PreprocessSuffix): UCD/LangCyrillicModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangCyrillicModel.cpp$(PreprocessSuffix) "UCD/LangCyrillicModel.cpp"

$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix): UCD/LangGreekModel.cpp $(IntermediateDirectory)/UCD_LangGreekModel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangGreekModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(DependSuffix): UCD/LangGreekModel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(DependSuffix) -MM "UCD/LangGreekModel.cpp"

$(IntermediateDirectory)/UCD_LangGreekModel.cpp$(PreprocessSuffix): UCD/LangGreekModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangGreekModel.cpp$(PreprocessSuffix) "UCD/LangGreekModel.cpp"

$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix): UCD/LangHebrewModel.cpp $(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangHebrewModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(DependSuffix): UCD/LangHebrewModel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(DependSuffix) -MM "UCD/LangHebrewModel.cpp"

$(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(PreprocessSuffix): UCD/LangHebrewModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangHebrewModel.cpp$(PreprocessSuffix) "UCD/LangHebrewModel.cpp"

$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix): UCD/LangHungarianModel.cpp $(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangHungarianModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(DependSuffix): UCD/LangHungarianModel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(DependSuffix) -MM "UCD/LangHungarianModel.cpp"

$(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(PreprocessSuffix): UCD/LangHungarianModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangHungarianModel.cpp$(PreprocessSuffix) "UCD/LangHungarianModel.cpp"

$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix): UCD/LangThaiModel.cpp $(IntermediateDirectory)/UCD_LangThaiModel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/LangThaiModel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(DependSuffix): UCD/LangThaiModel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(DependSuffix) -MM "UCD/LangThaiModel.cpp"

$(IntermediateDirectory)/UCD_LangThaiModel.cpp$(PreprocessSuffix): UCD/LangThaiModel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_LangThaiModel.cpp$(PreprocessSuffix) "UCD/LangThaiModel.cpp"

$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix): UCD/nsBig5Prober.cpp $(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsBig5Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(DependSuffix): UCD/nsBig5Prober.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(DependSuffix) -MM "UCD/nsBig5Prober.cpp"

$(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(PreprocessSuffix): UCD/nsBig5Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsBig5Prober.cpp$(PreprocessSuffix) "UCD/nsBig5Prober.cpp"

$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix): UCD/nsCharSetProber.cpp $(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsCharSetProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(DependSuffix): UCD/nsCharSetProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(DependSuffix) -MM "UCD/nsCharSetProber.cpp"

$(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(PreprocessSuffix): UCD/nsCharSetProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsCharSetProber.cpp$(PreprocessSuffix) "UCD/nsCharSetProber.cpp"

$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix): UCD/nsEscCharsetProber.cpp $(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEscCharsetProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(DependSuffix): UCD/nsEscCharsetProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(DependSuffix) -MM "UCD/nsEscCharsetProber.cpp"

$(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(PreprocessSuffix): UCD/nsEscCharsetProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEscCharsetProber.cpp$(PreprocessSuffix) "UCD/nsEscCharsetProber.cpp"

$(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix): UCD/nsEscSM.cpp $(IntermediateDirectory)/UCD_nsEscSM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEscSM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEscSM.cpp$(DependSuffix): UCD/nsEscSM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsEscSM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsEscSM.cpp$(DependSuffix) -MM "UCD/nsEscSM.cpp"

$(IntermediateDirectory)/UCD_nsEscSM.cpp$(PreprocessSuffix): UCD/nsEscSM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEscSM.cpp$(PreprocessSuffix) "UCD/nsEscSM.cpp"

$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix): UCD/nsEUCJPProber.cpp $(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEUCJPProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(DependSuffix): UCD/nsEUCJPProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(DependSuffix) -MM "UCD/nsEUCJPProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(PreprocessSuffix): UCD/nsEUCJPProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEUCJPProber.cpp$(PreprocessSuffix) "UCD/nsEUCJPProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix): UCD/nsEUCKRProber.cpp $(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEUCKRProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(DependSuffix): UCD/nsEUCKRProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(DependSuffix) -MM "UCD/nsEUCKRProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(PreprocessSuffix): UCD/nsEUCKRProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEUCKRProber.cpp$(PreprocessSuffix) "UCD/nsEUCKRProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix): UCD/nsEUCTWProber.cpp $(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsEUCTWProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(DependSuffix): UCD/nsEUCTWProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(DependSuffix) -MM "UCD/nsEUCTWProber.cpp"

$(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(PreprocessSuffix): UCD/nsEUCTWProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsEUCTWProber.cpp$(PreprocessSuffix) "UCD/nsEUCTWProber.cpp"

$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix): UCD/nsGB2312Prober.cpp $(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsGB2312Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(DependSuffix): UCD/nsGB2312Prober.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(DependSuffix) -MM "UCD/nsGB2312Prober.cpp"

$(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(PreprocessSuffix): UCD/nsGB2312Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsGB2312Prober.cpp$(PreprocessSuffix) "UCD/nsGB2312Prober.cpp"

$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix): UCD/nsHebrewProber.cpp $(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsHebrewProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(DependSuffix): UCD/nsHebrewProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(DependSuffix) -MM "UCD/nsHebrewProber.cpp"

$(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(PreprocessSuffix): UCD/nsHebrewProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsHebrewProber.cpp$(PreprocessSuffix) "UCD/nsHebrewProber.cpp"

$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix): UCD/nsLatin1Prober.cpp $(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsLatin1Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(DependSuffix): UCD/nsLatin1Prober.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(DependSuffix) -MM "UCD/nsLatin1Prober.cpp"

$(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(PreprocessSuffix): UCD/nsLatin1Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsLatin1Prober.cpp$(PreprocessSuffix) "UCD/nsLatin1Prober.cpp"

$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix): UCD/nsMBCSGroupProber.cpp $(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsMBCSGroupProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(DependSuffix): UCD/nsMBCSGroupProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(DependSuffix) -MM "UCD/nsMBCSGroupProber.cpp"

$(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(PreprocessSuffix): UCD/nsMBCSGroupProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsMBCSGroupProber.cpp$(PreprocessSuffix) "UCD/nsMBCSGroupProber.cpp"

$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix): UCD/nsMBCSSM.cpp $(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsMBCSSM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(DependSuffix): UCD/nsMBCSSM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(DependSuffix) -MM "UCD/nsMBCSSM.cpp"

$(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(PreprocessSuffix): UCD/nsMBCSSM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsMBCSSM.cpp$(PreprocessSuffix) "UCD/nsMBCSSM.cpp"

$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix): UCD/nsSBCharSetProber.cpp $(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsSBCharSetProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(DependSuffix): UCD/nsSBCharSetProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(DependSuffix) -MM "UCD/nsSBCharSetProber.cpp"

$(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(PreprocessSuffix): UCD/nsSBCharSetProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsSBCharSetProber.cpp$(PreprocessSuffix) "UCD/nsSBCharSetProber.cpp"

$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix): UCD/nsSBCSGroupProber.cpp $(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsSBCSGroupProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(DependSuffix): UCD/nsSBCSGroupProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(DependSuffix) -MM "UCD/nsSBCSGroupProber.cpp"

$(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(PreprocessSuffix): UCD/nsSBCSGroupProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsSBCSGroupProber.cpp$(PreprocessSuffix) "UCD/nsSBCSGroupProber.cpp"

$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix): UCD/nsSJISProber.cpp $(IntermediateDirectory)/UCD_nsSJISProber.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsSJISProber.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(DependSuffix): UCD/nsSJISProber.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(DependSuffix) -MM "UCD/nsSJISProber.cpp"

$(IntermediateDirectory)/UCD_nsSJISProber.cpp$(PreprocessSuffix): UCD/nsSJISProber.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsSJISProber.cpp$(PreprocessSuffix) "UCD/nsSJISProber.cpp"

$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix): UCD/nsUniversalDetector.cpp $(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsUniversalDetector.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(DependSuffix): UCD/nsUniversalDetector.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(DependSuffix) -MM "UCD/nsUniversalDetector.cpp"

$(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(PreprocessSuffix): UCD/nsUniversalDetector.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsUniversalDetector.cpp$(PreprocessSuffix) "UCD/nsUniversalDetector.cpp"

$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix): UCD/nsUTF8Prober.cpp $(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./UCD/nsUTF8Prober.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(DependSuffix): UCD/nsUTF8Prober.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(DependSuffix) -MM "UCD/nsUTF8Prober.cpp"

$(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(PreprocessSuffix): UCD/nsUTF8Prober.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_nsUTF8Prober.cpp$(PreprocessSuffix) "UCD/nsUTF8Prober.cpp"

$(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix): UCD/prmem.c $(IntermediateDirectory)/UCD_prmem.c$(DependSuffix)
	$(CC) $(SourceSwitch) "./UCD/prmem.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UCD_prmem.c$(DependSuffix): UCD/prmem.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/UCD_prmem.c$(ObjectSuffix) -MF$(IntermediateDirectory)/UCD_prmem.c$(DependSuffix) -MM "UCD/prmem.c"

$(IntermediateDirectory)/UCD_prmem.c$(PreprocessSuffix): UCD/prmem.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UCD_prmem.c$(PreprocessSuffix) "UCD/prmem.c"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


