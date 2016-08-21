##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=multiarc
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=21/08/16
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
OutputFile             :=../Build/Plugins/multiarc/bin/$(ProjectName).far-plug-mb
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="multiarc.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)src $(IncludeSwitch)src/libpcre $(IncludeSwitch)../far2l/ $(IncludeSwitch)../far2l/Include $(IncludeSwitch)../WinPort 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O2 -std=c++11 -fPIC $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 -fPIC $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_global.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix) $(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix) $(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix) $(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix) $(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix) $(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix) $(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix) $(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix) $(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix) $(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix) $(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(SharedObjectLinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)
	@$(MakeDirCommand) "../.build-debug"
	@echo rebuilt > "../.build-debug/multiarc"

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:
	@echo Executing Pre Build commands ...
	mkdir -p ../Build/Plugins/multiarc/
	cp -R ./configs/* ../Build/Plugins/multiarc/
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix): src/MultiArc.cpp $(IntermediateDirectory)/src_MultiArc.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/MultiArc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_MultiArc.cpp$(DependSuffix): src/MultiArc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_MultiArc.cpp$(DependSuffix) -MM "src/MultiArc.cpp"

$(IntermediateDirectory)/src_MultiArc.cpp$(PreprocessSuffix): src/MultiArc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_MultiArc.cpp$(PreprocessSuffix) "src/MultiArc.cpp"

$(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix): src/ArcPlg.cpp $(IntermediateDirectory)/src_ArcPlg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ArcPlg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ArcPlg.cpp$(DependSuffix): src/ArcPlg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ArcPlg.cpp$(DependSuffix) -MM "src/ArcPlg.cpp"

$(IntermediateDirectory)/src_ArcPlg.cpp$(PreprocessSuffix): src/ArcPlg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ArcPlg.cpp$(PreprocessSuffix) "src/ArcPlg.cpp"

$(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix): src/arccfg.cpp $(IntermediateDirectory)/src_arccfg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arccfg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arccfg.cpp$(DependSuffix): src/arccfg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_arccfg.cpp$(DependSuffix) -MM "src/arccfg.cpp"

$(IntermediateDirectory)/src_arccfg.cpp$(PreprocessSuffix): src/arccfg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arccfg.cpp$(PreprocessSuffix) "src/arccfg.cpp"

$(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix): src/arcget.cpp $(IntermediateDirectory)/src_arcget.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcget.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcget.cpp$(DependSuffix): src/arcget.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_arcget.cpp$(DependSuffix) -MM "src/arcget.cpp"

$(IntermediateDirectory)/src_arcget.cpp$(PreprocessSuffix): src/arcget.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcget.cpp$(PreprocessSuffix) "src/arcget.cpp"

$(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix): src/arcput.cpp $(IntermediateDirectory)/src_arcput.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcput.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcput.cpp$(DependSuffix): src/arcput.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_arcput.cpp$(DependSuffix) -MM "src/arcput.cpp"

$(IntermediateDirectory)/src_arcput.cpp$(PreprocessSuffix): src/arcput.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcput.cpp$(PreprocessSuffix) "src/arcput.cpp"

$(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix): src/arcreg.cpp $(IntermediateDirectory)/src_arcreg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcreg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcreg.cpp$(DependSuffix): src/arcreg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_arcreg.cpp$(DependSuffix) -MM "src/arcreg.cpp"

$(IntermediateDirectory)/src_arcreg.cpp$(PreprocessSuffix): src/arcreg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcreg.cpp$(PreprocessSuffix) "src/arcreg.cpp"

$(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix): src/ArcMix.cpp $(IntermediateDirectory)/src_ArcMix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ArcMix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ArcMix.cpp$(DependSuffix): src/ArcMix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ArcMix.cpp$(DependSuffix) -MM "src/ArcMix.cpp"

$(IntermediateDirectory)/src_ArcMix.cpp$(PreprocessSuffix): src/ArcMix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ArcMix.cpp$(PreprocessSuffix) "src/ArcMix.cpp"

$(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix): src/ArcProc.cpp $(IntermediateDirectory)/src_ArcProc.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ArcProc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ArcProc.cpp$(DependSuffix): src/ArcProc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ArcProc.cpp$(DependSuffix) -MM "src/ArcProc.cpp"

$(IntermediateDirectory)/src_ArcProc.cpp$(PreprocessSuffix): src/ArcProc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ArcProc.cpp$(PreprocessSuffix) "src/ArcProc.cpp"

$(IntermediateDirectory)/src_global.cpp$(ObjectSuffix): src/global.cpp $(IntermediateDirectory)/src_global.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/global.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_global.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_global.cpp$(DependSuffix): src/global.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_global.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_global.cpp$(DependSuffix) -MM "src/global.cpp"

$(IntermediateDirectory)/src_global.cpp$(PreprocessSuffix): src/global.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_global.cpp$(PreprocessSuffix) "src/global.cpp"

$(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix): src/arcread.cpp $(IntermediateDirectory)/src_arcread.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcread.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcread.cpp$(DependSuffix): src/arcread.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_arcread.cpp$(DependSuffix) -MM "src/arcread.cpp"

$(IntermediateDirectory)/src_arcread.cpp$(PreprocessSuffix): src/arcread.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcread.cpp$(PreprocessSuffix) "src/arcread.cpp"

$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix): src/arccmd.cpp $(IntermediateDirectory)/src_arccmd.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arccmd.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arccmd.cpp$(DependSuffix): src/arccmd.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_arccmd.cpp$(DependSuffix) -MM "src/arccmd.cpp"

$(IntermediateDirectory)/src_arccmd.cpp$(PreprocessSuffix): src/arccmd.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arccmd.cpp$(PreprocessSuffix) "src/arccmd.cpp"

$(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix): src/libpcre/get.c $(IntermediateDirectory)/libpcre_get.c$(DependSuffix)
	$(CC) $(SourceSwitch) "./src/libpcre/get.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_get.c$(DependSuffix): src/libpcre/get.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix) -MF$(IntermediateDirectory)/libpcre_get.c$(DependSuffix) -MM "src/libpcre/get.c"

$(IntermediateDirectory)/libpcre_get.c$(PreprocessSuffix): src/libpcre/get.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_get.c$(PreprocessSuffix) "src/libpcre/get.c"

$(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix): src/libpcre/pcre.c $(IntermediateDirectory)/libpcre_pcre.c$(DependSuffix)
	$(CC) $(SourceSwitch) "./src/libpcre/pcre.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_pcre.c$(DependSuffix): src/libpcre/pcre.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix) -MF$(IntermediateDirectory)/libpcre_pcre.c$(DependSuffix) -MM "src/libpcre/pcre.c"

$(IntermediateDirectory)/libpcre_pcre.c$(PreprocessSuffix): src/libpcre/pcre.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_pcre.c$(PreprocessSuffix) "src/libpcre/pcre.c"

$(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix): src/libpcre/chartables.c $(IntermediateDirectory)/libpcre_chartables.c$(DependSuffix)
	$(CC) $(SourceSwitch) "./src/libpcre/chartables.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_chartables.c$(DependSuffix): src/libpcre/chartables.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix) -MF$(IntermediateDirectory)/libpcre_chartables.c$(DependSuffix) -MM "src/libpcre/chartables.c"

$(IntermediateDirectory)/libpcre_chartables.c$(PreprocessSuffix): src/libpcre/chartables.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_chartables.c$(PreprocessSuffix) "src/libpcre/chartables.c"

$(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix): src/libpcre/study.c $(IntermediateDirectory)/libpcre_study.c$(DependSuffix)
	$(CC) $(SourceSwitch) "./src/libpcre/study.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_study.c$(DependSuffix): src/libpcre/study.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix) -MF$(IntermediateDirectory)/libpcre_study.c$(DependSuffix) -MM "src/libpcre/study.c"

$(IntermediateDirectory)/libpcre_study.c$(PreprocessSuffix): src/libpcre/study.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_study.c$(PreprocessSuffix) "src/libpcre/study.c"

$(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix): src/formats/ha/ha.cpp $(IntermediateDirectory)/ha_ha.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/ha/ha.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ha_ha.cpp$(DependSuffix): src/formats/ha/ha.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ha_ha.cpp$(DependSuffix) -MM "src/formats/ha/ha.cpp"

$(IntermediateDirectory)/ha_ha.cpp$(PreprocessSuffix): src/formats/ha/ha.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ha_ha.cpp$(PreprocessSuffix) "src/formats/ha/ha.cpp"

$(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix): src/formats/arj/arj.cpp $(IntermediateDirectory)/arj_arj.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/arj/arj.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/arj_arj.cpp$(DependSuffix): src/formats/arj/arj.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/arj_arj.cpp$(DependSuffix) -MM "src/formats/arj/arj.cpp"

$(IntermediateDirectory)/arj_arj.cpp$(PreprocessSuffix): src/formats/arj/arj.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/arj_arj.cpp$(PreprocessSuffix) "src/formats/arj/arj.cpp"

$(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix): src/formats/ace/ace.cpp $(IntermediateDirectory)/ace_ace.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/ace/ace.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ace_ace.cpp$(DependSuffix): src/formats/ace/ace.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ace_ace.cpp$(DependSuffix) -MM "src/formats/ace/ace.cpp"

$(IntermediateDirectory)/ace_ace.cpp$(PreprocessSuffix): src/formats/ace/ace.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ace_ace.cpp$(PreprocessSuffix) "src/formats/ace/ace.cpp"

$(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix): src/formats/arc/arc.cpp $(IntermediateDirectory)/arc_arc.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/arc/arc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/arc_arc.cpp$(DependSuffix): src/formats/arc/arc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/arc_arc.cpp$(DependSuffix) -MM "src/formats/arc/arc.cpp"

$(IntermediateDirectory)/arc_arc.cpp$(PreprocessSuffix): src/formats/arc/arc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/arc_arc.cpp$(PreprocessSuffix) "src/formats/arc/arc.cpp"

$(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix): src/formats/zip/zip.cpp $(IntermediateDirectory)/zip_zip.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/zip/zip.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zip_zip.cpp$(DependSuffix): src/formats/zip/zip.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/zip_zip.cpp$(DependSuffix) -MM "src/formats/zip/zip.cpp"

$(IntermediateDirectory)/zip_zip.cpp$(PreprocessSuffix): src/formats/zip/zip.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zip_zip.cpp$(PreprocessSuffix) "src/formats/zip/zip.cpp"

$(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix): src/formats/lzh/lzh.cpp $(IntermediateDirectory)/lzh_lzh.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/lzh/lzh.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/lzh_lzh.cpp$(DependSuffix): src/formats/lzh/lzh.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/lzh_lzh.cpp$(DependSuffix) -MM "src/formats/lzh/lzh.cpp"

$(IntermediateDirectory)/lzh_lzh.cpp$(PreprocessSuffix): src/formats/lzh/lzh.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/lzh_lzh.cpp$(PreprocessSuffix) "src/formats/lzh/lzh.cpp"

$(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix): src/formats/targz/targz.cpp $(IntermediateDirectory)/targz_targz.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/targz/targz.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/targz_targz.cpp$(DependSuffix): src/formats/targz/targz.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/targz_targz.cpp$(DependSuffix) -MM "src/formats/targz/targz.cpp"

$(IntermediateDirectory)/targz_targz.cpp$(PreprocessSuffix): src/formats/targz/targz.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/targz_targz.cpp$(PreprocessSuffix) "src/formats/targz/targz.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


