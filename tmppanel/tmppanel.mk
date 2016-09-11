##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=tmppanel
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./$(ConfigurationName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=11/09/16
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
OutputFile             :=../Build/Plugins/tmppanel/bin/$(ProjectName).far-plug-wide
Preprocessors          :=$(PreprocessorSwitch)WINPORT_DIRECT $(PreprocessorSwitch)UNICODE $(PreprocessorSwitch)FAR_DONT_USE_INTERNALS 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="tmppanel.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)../far2l/ $(IncludeSwitch)../far2l/Include $(IncludeSwitch)../WinPort 
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
CXXFLAGS :=  -g -O2 -std=c++11 -fPIC -fvisibility=hidden $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 -fPIC -fvisibility=hidden $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_TmpCfg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_TmpClass.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_TmpMix.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_TmpPanel.cpp$(ObjectSuffix) 



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
	@echo rebuilt > "../.build-debug/tmppanel"

MakeIntermediateDirs:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)

PreBuild:
	@echo Executing Pre Build commands ...
	mkdir -p ../Build/Plugins/tmppanel/
	cp -R ./configs/* ../Build/Plugins/tmppanel/
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/src_TmpCfg.cpp$(ObjectSuffix): src/TmpCfg.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/TmpCfg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_TmpCfg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_TmpCfg.cpp$(PreprocessSuffix): src/TmpCfg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_TmpCfg.cpp$(PreprocessSuffix) "src/TmpCfg.cpp"

$(IntermediateDirectory)/src_TmpClass.cpp$(ObjectSuffix): src/TmpClass.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/TmpClass.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_TmpClass.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_TmpClass.cpp$(PreprocessSuffix): src/TmpClass.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_TmpClass.cpp$(PreprocessSuffix) "src/TmpClass.cpp"

$(IntermediateDirectory)/src_TmpMix.cpp$(ObjectSuffix): src/TmpMix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/TmpMix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_TmpMix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_TmpMix.cpp$(PreprocessSuffix): src/TmpMix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_TmpMix.cpp$(PreprocessSuffix) "src/TmpMix.cpp"

$(IntermediateDirectory)/src_TmpPanel.cpp$(ObjectSuffix): src/TmpPanel.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/TmpPanel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_TmpPanel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_TmpPanel.cpp$(PreprocessSuffix): src/TmpPanel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_TmpPanel.cpp$(PreprocessSuffix) "src/TmpPanel.cpp"

##
## Clean
##
clean:
	$(RM) -r ./$(ConfigurationName)/


