##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=farlng
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
OutputFile             :=../tools/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="farlng.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell wx-config --debug=yes --libs --unicode=yes)
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)src $(IncludeSwitch)../WinPort $(IncludeSwitch)../utils/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)WinPort $(LibrarySwitch)utils $(LibrarySwitch)dl $(LibrarySwitch)glib-2.0 
ArLibs                 :=  "WinPort" "utils" "dl" "glib-2.0" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)../WinPort/$(ConfigurationName) $(LibraryPathSwitch)../utils/$(ConfigurationName) 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O2 -std=c++11 -Wall -Wno-unused-function $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 -Wall -Wno-unused-function $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_lng.generator.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_farlng.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_lng.convertor.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_lng.common.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_lng.inserter.cpp$(ObjectSuffix) 



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
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_lng.generator.cpp$(ObjectSuffix): src/lng.generator.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/lng.generator.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_lng.generator.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_lng.generator.cpp$(PreprocessSuffix): src/lng.generator.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_lng.generator.cpp$(PreprocessSuffix) "src/lng.generator.cpp"

$(IntermediateDirectory)/src_farlng.cpp$(ObjectSuffix): src/farlng.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/farlng.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_farlng.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_farlng.cpp$(PreprocessSuffix): src/farlng.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_farlng.cpp$(PreprocessSuffix) "src/farlng.cpp"

$(IntermediateDirectory)/src_lng.convertor.cpp$(ObjectSuffix): src/lng.convertor.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/lng.convertor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_lng.convertor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_lng.convertor.cpp$(PreprocessSuffix): src/lng.convertor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_lng.convertor.cpp$(PreprocessSuffix) "src/lng.convertor.cpp"

$(IntermediateDirectory)/src_lng.common.cpp$(ObjectSuffix): src/lng.common.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/lng.common.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_lng.common.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_lng.common.cpp$(PreprocessSuffix): src/lng.common.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_lng.common.cpp$(PreprocessSuffix) "src/lng.common.cpp"

$(IntermediateDirectory)/src_lng.inserter.cpp$(ObjectSuffix): src/lng.inserter.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/lng.inserter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_lng.inserter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_lng.inserter.cpp$(PreprocessSuffix): src/lng.inserter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_lng.inserter.cpp$(PreprocessSuffix) "src/lng.inserter.cpp"

##
## Clean
##
clean:
	$(RM) -r ./$(ConfigurationName)/


