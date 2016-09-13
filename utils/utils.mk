##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=utils
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./$(ConfigurationName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=13/09/16
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
OutputFile             :=$(IntermediateDirectory)/lib$(ProjectName).a
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="utils.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)include $(IncludeSwitch)../WinPort 
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
CXXFLAGS :=  -g -O2 -std=c++11 $(shell pkg-config glib-2.0 --cflags) -fPIC -fvisibility=hidden $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 $(shell pkg-config glib-2.0 --cflags) -fPIC -fvisibility=hidden $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_KeyFileHelper.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_utils.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(IntermediateDirectory) $(OutputFile)

$(OutputFile): $(Objects)
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(AR) $(ArchiveOutputSwitch)$(OutputFile) @$(ObjectsFileList) $(ArLibs)
	@$(MakeDirCommand) "../.build-debug"
	@echo rebuilt > "../.build-debug/utils"

MakeIntermediateDirs:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)


./$(ConfigurationName):
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_KeyFileHelper.cpp$(ObjectSuffix): src/KeyFileHelper.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/KeyFileHelper.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_KeyFileHelper.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_KeyFileHelper.cpp$(PreprocessSuffix): src/KeyFileHelper.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_KeyFileHelper.cpp$(PreprocessSuffix) "src/KeyFileHelper.cpp"

$(IntermediateDirectory)/src_utils.cpp$(ObjectSuffix): src/utils.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_utils.cpp$(PreprocessSuffix): src/utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_utils.cpp$(PreprocessSuffix) "src/utils.cpp"

##
## Clean
##
clean:
	$(RM) -r ./$(ConfigurationName)/


