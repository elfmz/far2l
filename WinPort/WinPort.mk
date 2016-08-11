##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=WinPort
ConfigurationName      :=Debug
WorkspacePath          := "/home/user/projects/far2l"
ProjectPath            := "/home/user/projects/far2l/WinPort"
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=11/08/16
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
ObjectsFileList        :="WinPort.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell wx-config --debug=yes --libs --unicode=yes)
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)/usr/src/wxWidgets-3.0.2/lib/wx/include/gtk3-unicode-3.0 
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
CXXFLAGS :=  -g -O1 -O -O0 -O2 -std=c++11 -Wall $(shell wx-config --cxxflags --debug=yes --unicode=yes)  -Wno-unused-function $(Preprocessors)
CFLAGS   :=  -g -O1 -O -O0 -O2 -std=c++11 -Wall $(shell wx-config --cxxflags --debug=yes --unicode=yes) -Wno-unused-function $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/APIClipboard.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIFiles.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIFSNotify.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIKeyboard.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIMemory.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIOther.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIRegistry.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIStringCodepages.cpp$(ObjectSuffix) $(IntermediateDirectory)/APIStringMap.cpp$(ObjectSuffix) $(IntermediateDirectory)/APISynch.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/APITime.cpp$(ObjectSuffix) $(IntermediateDirectory)/ConsoleBuffer.cpp$(ObjectSuffix) $(IntermediateDirectory)/ConsoleInput.cpp$(ObjectSuffix) $(IntermediateDirectory)/ConsoleOutput.cpp$(ObjectSuffix) $(IntermediateDirectory)/Main.cpp$(ObjectSuffix) $(IntermediateDirectory)/stdafx.cpp$(ObjectSuffix) $(IntermediateDirectory)/StrUtils.cpp$(ObjectSuffix) $(IntermediateDirectory)/WinPortConsole.cpp$(ObjectSuffix) $(IntermediateDirectory)/WinPortHandle.cpp$(ObjectSuffix) $(IntermediateDirectory)/wxWinTranslations.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/WinPortSynch.cpp$(ObjectSuffix) $(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix) \
	$(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(IntermediateDirectory) $(OutputFile)

$(OutputFile): $(Objects)
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	$(AR) $(ArchiveOutputSwitch)$(OutputFile) @$(ObjectsFileList) $(ArLibs)
	@$(MakeDirCommand) "/home/user/projects/far2l/.build-debug"
	@echo rebuilt > "/home/user/projects/far2l/.build-debug/WinPort"

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


./Debug:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/APIClipboard.cpp$(ObjectSuffix): APIClipboard.cpp $(IntermediateDirectory)/APIClipboard.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIClipboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIClipboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIClipboard.cpp$(DependSuffix): APIClipboard.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIClipboard.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIClipboard.cpp$(DependSuffix) -MM "APIClipboard.cpp"

$(IntermediateDirectory)/APIClipboard.cpp$(PreprocessSuffix): APIClipboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIClipboard.cpp$(PreprocessSuffix) "APIClipboard.cpp"

$(IntermediateDirectory)/APIFiles.cpp$(ObjectSuffix): APIFiles.cpp $(IntermediateDirectory)/APIFiles.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIFiles.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIFiles.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIFiles.cpp$(DependSuffix): APIFiles.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIFiles.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIFiles.cpp$(DependSuffix) -MM "APIFiles.cpp"

$(IntermediateDirectory)/APIFiles.cpp$(PreprocessSuffix): APIFiles.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIFiles.cpp$(PreprocessSuffix) "APIFiles.cpp"

$(IntermediateDirectory)/APIFSNotify.cpp$(ObjectSuffix): APIFSNotify.cpp $(IntermediateDirectory)/APIFSNotify.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIFSNotify.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIFSNotify.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIFSNotify.cpp$(DependSuffix): APIFSNotify.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIFSNotify.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIFSNotify.cpp$(DependSuffix) -MM "APIFSNotify.cpp"

$(IntermediateDirectory)/APIFSNotify.cpp$(PreprocessSuffix): APIFSNotify.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIFSNotify.cpp$(PreprocessSuffix) "APIFSNotify.cpp"

$(IntermediateDirectory)/APIKeyboard.cpp$(ObjectSuffix): APIKeyboard.cpp $(IntermediateDirectory)/APIKeyboard.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIKeyboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIKeyboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIKeyboard.cpp$(DependSuffix): APIKeyboard.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIKeyboard.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIKeyboard.cpp$(DependSuffix) -MM "APIKeyboard.cpp"

$(IntermediateDirectory)/APIKeyboard.cpp$(PreprocessSuffix): APIKeyboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIKeyboard.cpp$(PreprocessSuffix) "APIKeyboard.cpp"

$(IntermediateDirectory)/APIMemory.cpp$(ObjectSuffix): APIMemory.cpp $(IntermediateDirectory)/APIMemory.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIMemory.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIMemory.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIMemory.cpp$(DependSuffix): APIMemory.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIMemory.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIMemory.cpp$(DependSuffix) -MM "APIMemory.cpp"

$(IntermediateDirectory)/APIMemory.cpp$(PreprocessSuffix): APIMemory.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIMemory.cpp$(PreprocessSuffix) "APIMemory.cpp"

$(IntermediateDirectory)/APIOther.cpp$(ObjectSuffix): APIOther.cpp $(IntermediateDirectory)/APIOther.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIOther.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIOther.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIOther.cpp$(DependSuffix): APIOther.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIOther.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIOther.cpp$(DependSuffix) -MM "APIOther.cpp"

$(IntermediateDirectory)/APIOther.cpp$(PreprocessSuffix): APIOther.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIOther.cpp$(PreprocessSuffix) "APIOther.cpp"

$(IntermediateDirectory)/APIRegistry.cpp$(ObjectSuffix): APIRegistry.cpp $(IntermediateDirectory)/APIRegistry.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIRegistry.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIRegistry.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIRegistry.cpp$(DependSuffix): APIRegistry.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIRegistry.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIRegistry.cpp$(DependSuffix) -MM "APIRegistry.cpp"

$(IntermediateDirectory)/APIRegistry.cpp$(PreprocessSuffix): APIRegistry.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIRegistry.cpp$(PreprocessSuffix) "APIRegistry.cpp"

$(IntermediateDirectory)/APIStringCodepages.cpp$(ObjectSuffix): APIStringCodepages.cpp $(IntermediateDirectory)/APIStringCodepages.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIStringCodepages.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIStringCodepages.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIStringCodepages.cpp$(DependSuffix): APIStringCodepages.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIStringCodepages.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIStringCodepages.cpp$(DependSuffix) -MM "APIStringCodepages.cpp"

$(IntermediateDirectory)/APIStringCodepages.cpp$(PreprocessSuffix): APIStringCodepages.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIStringCodepages.cpp$(PreprocessSuffix) "APIStringCodepages.cpp"

$(IntermediateDirectory)/APIStringMap.cpp$(ObjectSuffix): APIStringMap.cpp $(IntermediateDirectory)/APIStringMap.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APIStringMap.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APIStringMap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APIStringMap.cpp$(DependSuffix): APIStringMap.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APIStringMap.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APIStringMap.cpp$(DependSuffix) -MM "APIStringMap.cpp"

$(IntermediateDirectory)/APIStringMap.cpp$(PreprocessSuffix): APIStringMap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APIStringMap.cpp$(PreprocessSuffix) "APIStringMap.cpp"

$(IntermediateDirectory)/APISynch.cpp$(ObjectSuffix): APISynch.cpp $(IntermediateDirectory)/APISynch.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APISynch.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APISynch.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APISynch.cpp$(DependSuffix): APISynch.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APISynch.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APISynch.cpp$(DependSuffix) -MM "APISynch.cpp"

$(IntermediateDirectory)/APISynch.cpp$(PreprocessSuffix): APISynch.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APISynch.cpp$(PreprocessSuffix) "APISynch.cpp"

$(IntermediateDirectory)/APITime.cpp$(ObjectSuffix): APITime.cpp $(IntermediateDirectory)/APITime.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/APITime.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/APITime.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/APITime.cpp$(DependSuffix): APITime.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/APITime.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/APITime.cpp$(DependSuffix) -MM "APITime.cpp"

$(IntermediateDirectory)/APITime.cpp$(PreprocessSuffix): APITime.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/APITime.cpp$(PreprocessSuffix) "APITime.cpp"

$(IntermediateDirectory)/ConsoleBuffer.cpp$(ObjectSuffix): ConsoleBuffer.cpp $(IntermediateDirectory)/ConsoleBuffer.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/ConsoleBuffer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ConsoleBuffer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ConsoleBuffer.cpp$(DependSuffix): ConsoleBuffer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ConsoleBuffer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ConsoleBuffer.cpp$(DependSuffix) -MM "ConsoleBuffer.cpp"

$(IntermediateDirectory)/ConsoleBuffer.cpp$(PreprocessSuffix): ConsoleBuffer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ConsoleBuffer.cpp$(PreprocessSuffix) "ConsoleBuffer.cpp"

$(IntermediateDirectory)/ConsoleInput.cpp$(ObjectSuffix): ConsoleInput.cpp $(IntermediateDirectory)/ConsoleInput.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/ConsoleInput.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ConsoleInput.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ConsoleInput.cpp$(DependSuffix): ConsoleInput.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ConsoleInput.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ConsoleInput.cpp$(DependSuffix) -MM "ConsoleInput.cpp"

$(IntermediateDirectory)/ConsoleInput.cpp$(PreprocessSuffix): ConsoleInput.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ConsoleInput.cpp$(PreprocessSuffix) "ConsoleInput.cpp"

$(IntermediateDirectory)/ConsoleOutput.cpp$(ObjectSuffix): ConsoleOutput.cpp $(IntermediateDirectory)/ConsoleOutput.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/ConsoleOutput.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ConsoleOutput.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ConsoleOutput.cpp$(DependSuffix): ConsoleOutput.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ConsoleOutput.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ConsoleOutput.cpp$(DependSuffix) -MM "ConsoleOutput.cpp"

$(IntermediateDirectory)/ConsoleOutput.cpp$(PreprocessSuffix): ConsoleOutput.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ConsoleOutput.cpp$(PreprocessSuffix) "ConsoleOutput.cpp"

$(IntermediateDirectory)/Main.cpp$(ObjectSuffix): Main.cpp $(IntermediateDirectory)/Main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/Main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Main.cpp$(DependSuffix): Main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Main.cpp$(DependSuffix) -MM "Main.cpp"

$(IntermediateDirectory)/Main.cpp$(PreprocessSuffix): Main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Main.cpp$(PreprocessSuffix) "Main.cpp"

$(IntermediateDirectory)/stdafx.cpp$(ObjectSuffix): stdafx.cpp $(IntermediateDirectory)/stdafx.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/stdafx.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/stdafx.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/stdafx.cpp$(DependSuffix): stdafx.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/stdafx.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/stdafx.cpp$(DependSuffix) -MM "stdafx.cpp"

$(IntermediateDirectory)/stdafx.cpp$(PreprocessSuffix): stdafx.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/stdafx.cpp$(PreprocessSuffix) "stdafx.cpp"

$(IntermediateDirectory)/StrUtils.cpp$(ObjectSuffix): StrUtils.cpp $(IntermediateDirectory)/StrUtils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/StrUtils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/StrUtils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/StrUtils.cpp$(DependSuffix): StrUtils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/StrUtils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/StrUtils.cpp$(DependSuffix) -MM "StrUtils.cpp"

$(IntermediateDirectory)/StrUtils.cpp$(PreprocessSuffix): StrUtils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/StrUtils.cpp$(PreprocessSuffix) "StrUtils.cpp"

$(IntermediateDirectory)/WinPortConsole.cpp$(ObjectSuffix): WinPortConsole.cpp $(IntermediateDirectory)/WinPortConsole.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/WinPortConsole.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/WinPortConsole.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/WinPortConsole.cpp$(DependSuffix): WinPortConsole.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/WinPortConsole.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/WinPortConsole.cpp$(DependSuffix) -MM "WinPortConsole.cpp"

$(IntermediateDirectory)/WinPortConsole.cpp$(PreprocessSuffix): WinPortConsole.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/WinPortConsole.cpp$(PreprocessSuffix) "WinPortConsole.cpp"

$(IntermediateDirectory)/WinPortHandle.cpp$(ObjectSuffix): WinPortHandle.cpp $(IntermediateDirectory)/WinPortHandle.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/WinPortHandle.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/WinPortHandle.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/WinPortHandle.cpp$(DependSuffix): WinPortHandle.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/WinPortHandle.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/WinPortHandle.cpp$(DependSuffix) -MM "WinPortHandle.cpp"

$(IntermediateDirectory)/WinPortHandle.cpp$(PreprocessSuffix): WinPortHandle.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/WinPortHandle.cpp$(PreprocessSuffix) "WinPortHandle.cpp"

$(IntermediateDirectory)/wxWinTranslations.cpp$(ObjectSuffix): wxWinTranslations.cpp $(IntermediateDirectory)/wxWinTranslations.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wxWinTranslations.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wxWinTranslations.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wxWinTranslations.cpp$(DependSuffix): wxWinTranslations.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wxWinTranslations.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/wxWinTranslations.cpp$(DependSuffix) -MM "wxWinTranslations.cpp"

$(IntermediateDirectory)/wxWinTranslations.cpp$(PreprocessSuffix): wxWinTranslations.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wxWinTranslations.cpp$(PreprocessSuffix) "wxWinTranslations.cpp"

$(IntermediateDirectory)/WinPortSynch.cpp$(ObjectSuffix): WinPortSynch.cpp $(IntermediateDirectory)/WinPortSynch.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/user/projects/far2l/WinPort/WinPortSynch.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/WinPortSynch.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/WinPortSynch.cpp$(DependSuffix): WinPortSynch.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/WinPortSynch.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/WinPortSynch.cpp$(DependSuffix) -MM "WinPortSynch.cpp"

$(IntermediateDirectory)/WinPortSynch.cpp$(PreprocessSuffix): WinPortSynch.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/WinPortSynch.cpp$(PreprocessSuffix) "WinPortSynch.cpp"

$(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix): wineguts/casemap.c $(IntermediateDirectory)/wineguts_casemap.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/casemap.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_casemap.c$(DependSuffix): wineguts/casemap.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_casemap.c$(DependSuffix) -MM "wineguts/casemap.c"

$(IntermediateDirectory)/wineguts_casemap.c$(PreprocessSuffix): wineguts/casemap.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_casemap.c$(PreprocessSuffix) "wineguts/casemap.c"

$(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix): wineguts/collation.c $(IntermediateDirectory)/wineguts_collation.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/collation.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_collation.c$(DependSuffix): wineguts/collation.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_collation.c$(DependSuffix) -MM "wineguts/collation.c"

$(IntermediateDirectory)/wineguts_collation.c$(PreprocessSuffix): wineguts/collation.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_collation.c$(PreprocessSuffix) "wineguts/collation.c"

$(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix): wineguts/compose.c $(IntermediateDirectory)/wineguts_compose.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/compose.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_compose.c$(DependSuffix): wineguts/compose.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_compose.c$(DependSuffix) -MM "wineguts/compose.c"

$(IntermediateDirectory)/wineguts_compose.c$(PreprocessSuffix): wineguts/compose.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_compose.c$(PreprocessSuffix) "wineguts/compose.c"

$(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix): wineguts/cpsymbol.c $(IntermediateDirectory)/wineguts_cpsymbol.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/cpsymbol.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_cpsymbol.c$(DependSuffix): wineguts/cpsymbol.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_cpsymbol.c$(DependSuffix) -MM "wineguts/cpsymbol.c"

$(IntermediateDirectory)/wineguts_cpsymbol.c$(PreprocessSuffix): wineguts/cpsymbol.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_cpsymbol.c$(PreprocessSuffix) "wineguts/cpsymbol.c"

$(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix): wineguts/cptable.c $(IntermediateDirectory)/wineguts_cptable.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/cptable.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_cptable.c$(DependSuffix): wineguts/cptable.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_cptable.c$(DependSuffix) -MM "wineguts/cptable.c"

$(IntermediateDirectory)/wineguts_cptable.c$(PreprocessSuffix): wineguts/cptable.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_cptable.c$(PreprocessSuffix) "wineguts/cptable.c"

$(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix): wineguts/decompose.c $(IntermediateDirectory)/wineguts_decompose.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/decompose.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_decompose.c$(DependSuffix): wineguts/decompose.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_decompose.c$(DependSuffix) -MM "wineguts/decompose.c"

$(IntermediateDirectory)/wineguts_decompose.c$(PreprocessSuffix): wineguts/decompose.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_decompose.c$(PreprocessSuffix) "wineguts/decompose.c"

$(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix): wineguts/locale.c $(IntermediateDirectory)/wineguts_locale.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/locale.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_locale.c$(DependSuffix): wineguts/locale.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_locale.c$(DependSuffix) -MM "wineguts/locale.c"

$(IntermediateDirectory)/wineguts_locale.c$(PreprocessSuffix): wineguts/locale.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_locale.c$(PreprocessSuffix) "wineguts/locale.c"

$(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix): wineguts/mbtowc.c $(IntermediateDirectory)/wineguts_mbtowc.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/mbtowc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_mbtowc.c$(DependSuffix): wineguts/mbtowc.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_mbtowc.c$(DependSuffix) -MM "wineguts/mbtowc.c"

$(IntermediateDirectory)/wineguts_mbtowc.c$(PreprocessSuffix): wineguts/mbtowc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_mbtowc.c$(PreprocessSuffix) "wineguts/mbtowc.c"

$(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix): wineguts/sortkey.c $(IntermediateDirectory)/wineguts_sortkey.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/sortkey.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_sortkey.c$(DependSuffix): wineguts/sortkey.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_sortkey.c$(DependSuffix) -MM "wineguts/sortkey.c"

$(IntermediateDirectory)/wineguts_sortkey.c$(PreprocessSuffix): wineguts/sortkey.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_sortkey.c$(PreprocessSuffix) "wineguts/sortkey.c"

$(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix): wineguts/utf8.c $(IntermediateDirectory)/wineguts_utf8.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/utf8.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_utf8.c$(DependSuffix): wineguts/utf8.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_utf8.c$(DependSuffix) -MM "wineguts/utf8.c"

$(IntermediateDirectory)/wineguts_utf8.c$(PreprocessSuffix): wineguts/utf8.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_utf8.c$(PreprocessSuffix) "wineguts/utf8.c"

$(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix): wineguts/wctomb.c $(IntermediateDirectory)/wineguts_wctomb.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/wctomb.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_wctomb.c$(DependSuffix): wineguts/wctomb.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_wctomb.c$(DependSuffix) -MM "wineguts/wctomb.c"

$(IntermediateDirectory)/wineguts_wctomb.c$(PreprocessSuffix): wineguts/wctomb.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_wctomb.c$(PreprocessSuffix) "wineguts/wctomb.c"

$(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix): wineguts/wctype.c $(IntermediateDirectory)/wineguts_wctype.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/wctype.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_wctype.c$(DependSuffix): wineguts/wctype.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix) -MF$(IntermediateDirectory)/wineguts_wctype.c$(DependSuffix) -MM "wineguts/wctype.c"

$(IntermediateDirectory)/wineguts_wctype.c$(PreprocessSuffix): wineguts/wctype.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_wctype.c$(PreprocessSuffix) "wineguts/wctype.c"

$(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix): wineguts/codepages/c_037.c $(IntermediateDirectory)/codepages_c_037.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_037.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_037.c$(DependSuffix): wineguts/codepages/c_037.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_037.c$(DependSuffix) -MM "wineguts/codepages/c_037.c"

$(IntermediateDirectory)/codepages_c_037.c$(PreprocessSuffix): wineguts/codepages/c_037.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_037.c$(PreprocessSuffix) "wineguts/codepages/c_037.c"

$(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix): wineguts/codepages/c_424.c $(IntermediateDirectory)/codepages_c_424.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_424.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_424.c$(DependSuffix): wineguts/codepages/c_424.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_424.c$(DependSuffix) -MM "wineguts/codepages/c_424.c"

$(IntermediateDirectory)/codepages_c_424.c$(PreprocessSuffix): wineguts/codepages/c_424.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_424.c$(PreprocessSuffix) "wineguts/codepages/c_424.c"

$(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix): wineguts/codepages/c_437.c $(IntermediateDirectory)/codepages_c_437.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_437.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_437.c$(DependSuffix): wineguts/codepages/c_437.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_437.c$(DependSuffix) -MM "wineguts/codepages/c_437.c"

$(IntermediateDirectory)/codepages_c_437.c$(PreprocessSuffix): wineguts/codepages/c_437.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_437.c$(PreprocessSuffix) "wineguts/codepages/c_437.c"

$(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix): wineguts/codepages/c_500.c $(IntermediateDirectory)/codepages_c_500.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_500.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_500.c$(DependSuffix): wineguts/codepages/c_500.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_500.c$(DependSuffix) -MM "wineguts/codepages/c_500.c"

$(IntermediateDirectory)/codepages_c_500.c$(PreprocessSuffix): wineguts/codepages/c_500.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_500.c$(PreprocessSuffix) "wineguts/codepages/c_500.c"

$(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix): wineguts/codepages/c_737.c $(IntermediateDirectory)/codepages_c_737.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_737.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_737.c$(DependSuffix): wineguts/codepages/c_737.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_737.c$(DependSuffix) -MM "wineguts/codepages/c_737.c"

$(IntermediateDirectory)/codepages_c_737.c$(PreprocessSuffix): wineguts/codepages/c_737.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_737.c$(PreprocessSuffix) "wineguts/codepages/c_737.c"

$(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix): wineguts/codepages/c_775.c $(IntermediateDirectory)/codepages_c_775.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_775.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_775.c$(DependSuffix): wineguts/codepages/c_775.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_775.c$(DependSuffix) -MM "wineguts/codepages/c_775.c"

$(IntermediateDirectory)/codepages_c_775.c$(PreprocessSuffix): wineguts/codepages/c_775.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_775.c$(PreprocessSuffix) "wineguts/codepages/c_775.c"

$(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix): wineguts/codepages/c_850.c $(IntermediateDirectory)/codepages_c_850.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_850.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_850.c$(DependSuffix): wineguts/codepages/c_850.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_850.c$(DependSuffix) -MM "wineguts/codepages/c_850.c"

$(IntermediateDirectory)/codepages_c_850.c$(PreprocessSuffix): wineguts/codepages/c_850.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_850.c$(PreprocessSuffix) "wineguts/codepages/c_850.c"

$(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix): wineguts/codepages/c_852.c $(IntermediateDirectory)/codepages_c_852.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_852.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_852.c$(DependSuffix): wineguts/codepages/c_852.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_852.c$(DependSuffix) -MM "wineguts/codepages/c_852.c"

$(IntermediateDirectory)/codepages_c_852.c$(PreprocessSuffix): wineguts/codepages/c_852.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_852.c$(PreprocessSuffix) "wineguts/codepages/c_852.c"

$(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix): wineguts/codepages/c_855.c $(IntermediateDirectory)/codepages_c_855.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_855.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_855.c$(DependSuffix): wineguts/codepages/c_855.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_855.c$(DependSuffix) -MM "wineguts/codepages/c_855.c"

$(IntermediateDirectory)/codepages_c_855.c$(PreprocessSuffix): wineguts/codepages/c_855.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_855.c$(PreprocessSuffix) "wineguts/codepages/c_855.c"

$(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix): wineguts/codepages/c_856.c $(IntermediateDirectory)/codepages_c_856.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_856.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_856.c$(DependSuffix): wineguts/codepages/c_856.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_856.c$(DependSuffix) -MM "wineguts/codepages/c_856.c"

$(IntermediateDirectory)/codepages_c_856.c$(PreprocessSuffix): wineguts/codepages/c_856.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_856.c$(PreprocessSuffix) "wineguts/codepages/c_856.c"

$(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix): wineguts/codepages/c_857.c $(IntermediateDirectory)/codepages_c_857.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_857.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_857.c$(DependSuffix): wineguts/codepages/c_857.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_857.c$(DependSuffix) -MM "wineguts/codepages/c_857.c"

$(IntermediateDirectory)/codepages_c_857.c$(PreprocessSuffix): wineguts/codepages/c_857.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_857.c$(PreprocessSuffix) "wineguts/codepages/c_857.c"

$(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix): wineguts/codepages/c_860.c $(IntermediateDirectory)/codepages_c_860.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_860.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_860.c$(DependSuffix): wineguts/codepages/c_860.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_860.c$(DependSuffix) -MM "wineguts/codepages/c_860.c"

$(IntermediateDirectory)/codepages_c_860.c$(PreprocessSuffix): wineguts/codepages/c_860.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_860.c$(PreprocessSuffix) "wineguts/codepages/c_860.c"

$(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix): wineguts/codepages/c_861.c $(IntermediateDirectory)/codepages_c_861.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_861.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_861.c$(DependSuffix): wineguts/codepages/c_861.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_861.c$(DependSuffix) -MM "wineguts/codepages/c_861.c"

$(IntermediateDirectory)/codepages_c_861.c$(PreprocessSuffix): wineguts/codepages/c_861.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_861.c$(PreprocessSuffix) "wineguts/codepages/c_861.c"

$(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix): wineguts/codepages/c_862.c $(IntermediateDirectory)/codepages_c_862.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_862.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_862.c$(DependSuffix): wineguts/codepages/c_862.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_862.c$(DependSuffix) -MM "wineguts/codepages/c_862.c"

$(IntermediateDirectory)/codepages_c_862.c$(PreprocessSuffix): wineguts/codepages/c_862.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_862.c$(PreprocessSuffix) "wineguts/codepages/c_862.c"

$(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix): wineguts/codepages/c_863.c $(IntermediateDirectory)/codepages_c_863.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_863.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_863.c$(DependSuffix): wineguts/codepages/c_863.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_863.c$(DependSuffix) -MM "wineguts/codepages/c_863.c"

$(IntermediateDirectory)/codepages_c_863.c$(PreprocessSuffix): wineguts/codepages/c_863.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_863.c$(PreprocessSuffix) "wineguts/codepages/c_863.c"

$(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix): wineguts/codepages/c_864.c $(IntermediateDirectory)/codepages_c_864.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_864.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_864.c$(DependSuffix): wineguts/codepages/c_864.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_864.c$(DependSuffix) -MM "wineguts/codepages/c_864.c"

$(IntermediateDirectory)/codepages_c_864.c$(PreprocessSuffix): wineguts/codepages/c_864.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_864.c$(PreprocessSuffix) "wineguts/codepages/c_864.c"

$(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix): wineguts/codepages/c_865.c $(IntermediateDirectory)/codepages_c_865.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_865.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_865.c$(DependSuffix): wineguts/codepages/c_865.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_865.c$(DependSuffix) -MM "wineguts/codepages/c_865.c"

$(IntermediateDirectory)/codepages_c_865.c$(PreprocessSuffix): wineguts/codepages/c_865.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_865.c$(PreprocessSuffix) "wineguts/codepages/c_865.c"

$(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix): wineguts/codepages/c_866.c $(IntermediateDirectory)/codepages_c_866.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_866.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_866.c$(DependSuffix): wineguts/codepages/c_866.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_866.c$(DependSuffix) -MM "wineguts/codepages/c_866.c"

$(IntermediateDirectory)/codepages_c_866.c$(PreprocessSuffix): wineguts/codepages/c_866.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_866.c$(PreprocessSuffix) "wineguts/codepages/c_866.c"

$(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix): wineguts/codepages/c_869.c $(IntermediateDirectory)/codepages_c_869.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_869.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_869.c$(DependSuffix): wineguts/codepages/c_869.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_869.c$(DependSuffix) -MM "wineguts/codepages/c_869.c"

$(IntermediateDirectory)/codepages_c_869.c$(PreprocessSuffix): wineguts/codepages/c_869.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_869.c$(PreprocessSuffix) "wineguts/codepages/c_869.c"

$(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix): wineguts/codepages/c_874.c $(IntermediateDirectory)/codepages_c_874.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_874.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_874.c$(DependSuffix): wineguts/codepages/c_874.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_874.c$(DependSuffix) -MM "wineguts/codepages/c_874.c"

$(IntermediateDirectory)/codepages_c_874.c$(PreprocessSuffix): wineguts/codepages/c_874.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_874.c$(PreprocessSuffix) "wineguts/codepages/c_874.c"

$(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix): wineguts/codepages/c_875.c $(IntermediateDirectory)/codepages_c_875.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_875.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_875.c$(DependSuffix): wineguts/codepages/c_875.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_875.c$(DependSuffix) -MM "wineguts/codepages/c_875.c"

$(IntermediateDirectory)/codepages_c_875.c$(PreprocessSuffix): wineguts/codepages/c_875.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_875.c$(PreprocessSuffix) "wineguts/codepages/c_875.c"

$(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix): wineguts/codepages/c_878.c $(IntermediateDirectory)/codepages_c_878.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_878.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_878.c$(DependSuffix): wineguts/codepages/c_878.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_878.c$(DependSuffix) -MM "wineguts/codepages/c_878.c"

$(IntermediateDirectory)/codepages_c_878.c$(PreprocessSuffix): wineguts/codepages/c_878.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_878.c$(PreprocessSuffix) "wineguts/codepages/c_878.c"

$(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix): wineguts/codepages/c_932.c $(IntermediateDirectory)/codepages_c_932.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_932.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_932.c$(DependSuffix): wineguts/codepages/c_932.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_932.c$(DependSuffix) -MM "wineguts/codepages/c_932.c"

$(IntermediateDirectory)/codepages_c_932.c$(PreprocessSuffix): wineguts/codepages/c_932.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_932.c$(PreprocessSuffix) "wineguts/codepages/c_932.c"

$(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix): wineguts/codepages/c_936.c $(IntermediateDirectory)/codepages_c_936.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_936.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_936.c$(DependSuffix): wineguts/codepages/c_936.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_936.c$(DependSuffix) -MM "wineguts/codepages/c_936.c"

$(IntermediateDirectory)/codepages_c_936.c$(PreprocessSuffix): wineguts/codepages/c_936.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_936.c$(PreprocessSuffix) "wineguts/codepages/c_936.c"

$(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix): wineguts/codepages/c_949.c $(IntermediateDirectory)/codepages_c_949.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_949.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_949.c$(DependSuffix): wineguts/codepages/c_949.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_949.c$(DependSuffix) -MM "wineguts/codepages/c_949.c"

$(IntermediateDirectory)/codepages_c_949.c$(PreprocessSuffix): wineguts/codepages/c_949.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_949.c$(PreprocessSuffix) "wineguts/codepages/c_949.c"

$(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix): wineguts/codepages/c_950.c $(IntermediateDirectory)/codepages_c_950.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_950.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_950.c$(DependSuffix): wineguts/codepages/c_950.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_950.c$(DependSuffix) -MM "wineguts/codepages/c_950.c"

$(IntermediateDirectory)/codepages_c_950.c$(PreprocessSuffix): wineguts/codepages/c_950.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_950.c$(PreprocessSuffix) "wineguts/codepages/c_950.c"

$(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix): wineguts/codepages/c_1006.c $(IntermediateDirectory)/codepages_c_1006.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1006.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1006.c$(DependSuffix): wineguts/codepages/c_1006.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1006.c$(DependSuffix) -MM "wineguts/codepages/c_1006.c"

$(IntermediateDirectory)/codepages_c_1006.c$(PreprocessSuffix): wineguts/codepages/c_1006.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1006.c$(PreprocessSuffix) "wineguts/codepages/c_1006.c"

$(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix): wineguts/codepages/c_1026.c $(IntermediateDirectory)/codepages_c_1026.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1026.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1026.c$(DependSuffix): wineguts/codepages/c_1026.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1026.c$(DependSuffix) -MM "wineguts/codepages/c_1026.c"

$(IntermediateDirectory)/codepages_c_1026.c$(PreprocessSuffix): wineguts/codepages/c_1026.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1026.c$(PreprocessSuffix) "wineguts/codepages/c_1026.c"

$(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix): wineguts/codepages/c_1250.c $(IntermediateDirectory)/codepages_c_1250.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1250.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1250.c$(DependSuffix): wineguts/codepages/c_1250.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1250.c$(DependSuffix) -MM "wineguts/codepages/c_1250.c"

$(IntermediateDirectory)/codepages_c_1250.c$(PreprocessSuffix): wineguts/codepages/c_1250.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1250.c$(PreprocessSuffix) "wineguts/codepages/c_1250.c"

$(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix): wineguts/codepages/c_1251.c $(IntermediateDirectory)/codepages_c_1251.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1251.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1251.c$(DependSuffix): wineguts/codepages/c_1251.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1251.c$(DependSuffix) -MM "wineguts/codepages/c_1251.c"

$(IntermediateDirectory)/codepages_c_1251.c$(PreprocessSuffix): wineguts/codepages/c_1251.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1251.c$(PreprocessSuffix) "wineguts/codepages/c_1251.c"

$(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix): wineguts/codepages/c_1252.c $(IntermediateDirectory)/codepages_c_1252.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1252.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1252.c$(DependSuffix): wineguts/codepages/c_1252.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1252.c$(DependSuffix) -MM "wineguts/codepages/c_1252.c"

$(IntermediateDirectory)/codepages_c_1252.c$(PreprocessSuffix): wineguts/codepages/c_1252.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1252.c$(PreprocessSuffix) "wineguts/codepages/c_1252.c"

$(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix): wineguts/codepages/c_1253.c $(IntermediateDirectory)/codepages_c_1253.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1253.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1253.c$(DependSuffix): wineguts/codepages/c_1253.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1253.c$(DependSuffix) -MM "wineguts/codepages/c_1253.c"

$(IntermediateDirectory)/codepages_c_1253.c$(PreprocessSuffix): wineguts/codepages/c_1253.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1253.c$(PreprocessSuffix) "wineguts/codepages/c_1253.c"

$(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix): wineguts/codepages/c_1254.c $(IntermediateDirectory)/codepages_c_1254.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1254.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1254.c$(DependSuffix): wineguts/codepages/c_1254.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1254.c$(DependSuffix) -MM "wineguts/codepages/c_1254.c"

$(IntermediateDirectory)/codepages_c_1254.c$(PreprocessSuffix): wineguts/codepages/c_1254.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1254.c$(PreprocessSuffix) "wineguts/codepages/c_1254.c"

$(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix): wineguts/codepages/c_1255.c $(IntermediateDirectory)/codepages_c_1255.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1255.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1255.c$(DependSuffix): wineguts/codepages/c_1255.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1255.c$(DependSuffix) -MM "wineguts/codepages/c_1255.c"

$(IntermediateDirectory)/codepages_c_1255.c$(PreprocessSuffix): wineguts/codepages/c_1255.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1255.c$(PreprocessSuffix) "wineguts/codepages/c_1255.c"

$(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix): wineguts/codepages/c_1256.c $(IntermediateDirectory)/codepages_c_1256.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1256.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1256.c$(DependSuffix): wineguts/codepages/c_1256.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1256.c$(DependSuffix) -MM "wineguts/codepages/c_1256.c"

$(IntermediateDirectory)/codepages_c_1256.c$(PreprocessSuffix): wineguts/codepages/c_1256.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1256.c$(PreprocessSuffix) "wineguts/codepages/c_1256.c"

$(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix): wineguts/codepages/c_1257.c $(IntermediateDirectory)/codepages_c_1257.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1257.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1257.c$(DependSuffix): wineguts/codepages/c_1257.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1257.c$(DependSuffix) -MM "wineguts/codepages/c_1257.c"

$(IntermediateDirectory)/codepages_c_1257.c$(PreprocessSuffix): wineguts/codepages/c_1257.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1257.c$(PreprocessSuffix) "wineguts/codepages/c_1257.c"

$(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix): wineguts/codepages/c_1258.c $(IntermediateDirectory)/codepages_c_1258.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1258.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1258.c$(DependSuffix): wineguts/codepages/c_1258.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1258.c$(DependSuffix) -MM "wineguts/codepages/c_1258.c"

$(IntermediateDirectory)/codepages_c_1258.c$(PreprocessSuffix): wineguts/codepages/c_1258.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1258.c$(PreprocessSuffix) "wineguts/codepages/c_1258.c"

$(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix): wineguts/codepages/c_1361.c $(IntermediateDirectory)/codepages_c_1361.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_1361.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1361.c$(DependSuffix): wineguts/codepages/c_1361.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_1361.c$(DependSuffix) -MM "wineguts/codepages/c_1361.c"

$(IntermediateDirectory)/codepages_c_1361.c$(PreprocessSuffix): wineguts/codepages/c_1361.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1361.c$(PreprocessSuffix) "wineguts/codepages/c_1361.c"

$(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix): wineguts/codepages/c_10000.c $(IntermediateDirectory)/codepages_c_10000.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10000.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10000.c$(DependSuffix): wineguts/codepages/c_10000.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10000.c$(DependSuffix) -MM "wineguts/codepages/c_10000.c"

$(IntermediateDirectory)/codepages_c_10000.c$(PreprocessSuffix): wineguts/codepages/c_10000.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10000.c$(PreprocessSuffix) "wineguts/codepages/c_10000.c"

$(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix): wineguts/codepages/c_10001.c $(IntermediateDirectory)/codepages_c_10001.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10001.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10001.c$(DependSuffix): wineguts/codepages/c_10001.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10001.c$(DependSuffix) -MM "wineguts/codepages/c_10001.c"

$(IntermediateDirectory)/codepages_c_10001.c$(PreprocessSuffix): wineguts/codepages/c_10001.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10001.c$(PreprocessSuffix) "wineguts/codepages/c_10001.c"

$(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix): wineguts/codepages/c_10002.c $(IntermediateDirectory)/codepages_c_10002.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10002.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10002.c$(DependSuffix): wineguts/codepages/c_10002.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10002.c$(DependSuffix) -MM "wineguts/codepages/c_10002.c"

$(IntermediateDirectory)/codepages_c_10002.c$(PreprocessSuffix): wineguts/codepages/c_10002.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10002.c$(PreprocessSuffix) "wineguts/codepages/c_10002.c"

$(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix): wineguts/codepages/c_10003.c $(IntermediateDirectory)/codepages_c_10003.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10003.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10003.c$(DependSuffix): wineguts/codepages/c_10003.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10003.c$(DependSuffix) -MM "wineguts/codepages/c_10003.c"

$(IntermediateDirectory)/codepages_c_10003.c$(PreprocessSuffix): wineguts/codepages/c_10003.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10003.c$(PreprocessSuffix) "wineguts/codepages/c_10003.c"

$(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix): wineguts/codepages/c_10004.c $(IntermediateDirectory)/codepages_c_10004.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10004.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10004.c$(DependSuffix): wineguts/codepages/c_10004.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10004.c$(DependSuffix) -MM "wineguts/codepages/c_10004.c"

$(IntermediateDirectory)/codepages_c_10004.c$(PreprocessSuffix): wineguts/codepages/c_10004.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10004.c$(PreprocessSuffix) "wineguts/codepages/c_10004.c"

$(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix): wineguts/codepages/c_10005.c $(IntermediateDirectory)/codepages_c_10005.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10005.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10005.c$(DependSuffix): wineguts/codepages/c_10005.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10005.c$(DependSuffix) -MM "wineguts/codepages/c_10005.c"

$(IntermediateDirectory)/codepages_c_10005.c$(PreprocessSuffix): wineguts/codepages/c_10005.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10005.c$(PreprocessSuffix) "wineguts/codepages/c_10005.c"

$(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix): wineguts/codepages/c_10006.c $(IntermediateDirectory)/codepages_c_10006.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10006.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10006.c$(DependSuffix): wineguts/codepages/c_10006.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10006.c$(DependSuffix) -MM "wineguts/codepages/c_10006.c"

$(IntermediateDirectory)/codepages_c_10006.c$(PreprocessSuffix): wineguts/codepages/c_10006.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10006.c$(PreprocessSuffix) "wineguts/codepages/c_10006.c"

$(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix): wineguts/codepages/c_10007.c $(IntermediateDirectory)/codepages_c_10007.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10007.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10007.c$(DependSuffix): wineguts/codepages/c_10007.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10007.c$(DependSuffix) -MM "wineguts/codepages/c_10007.c"

$(IntermediateDirectory)/codepages_c_10007.c$(PreprocessSuffix): wineguts/codepages/c_10007.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10007.c$(PreprocessSuffix) "wineguts/codepages/c_10007.c"

$(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix): wineguts/codepages/c_10008.c $(IntermediateDirectory)/codepages_c_10008.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10008.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10008.c$(DependSuffix): wineguts/codepages/c_10008.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10008.c$(DependSuffix) -MM "wineguts/codepages/c_10008.c"

$(IntermediateDirectory)/codepages_c_10008.c$(PreprocessSuffix): wineguts/codepages/c_10008.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10008.c$(PreprocessSuffix) "wineguts/codepages/c_10008.c"

$(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix): wineguts/codepages/c_10010.c $(IntermediateDirectory)/codepages_c_10010.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10010.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10010.c$(DependSuffix): wineguts/codepages/c_10010.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10010.c$(DependSuffix) -MM "wineguts/codepages/c_10010.c"

$(IntermediateDirectory)/codepages_c_10010.c$(PreprocessSuffix): wineguts/codepages/c_10010.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10010.c$(PreprocessSuffix) "wineguts/codepages/c_10010.c"

$(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix): wineguts/codepages/c_10017.c $(IntermediateDirectory)/codepages_c_10017.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10017.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10017.c$(DependSuffix): wineguts/codepages/c_10017.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10017.c$(DependSuffix) -MM "wineguts/codepages/c_10017.c"

$(IntermediateDirectory)/codepages_c_10017.c$(PreprocessSuffix): wineguts/codepages/c_10017.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10017.c$(PreprocessSuffix) "wineguts/codepages/c_10017.c"

$(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix): wineguts/codepages/c_10021.c $(IntermediateDirectory)/codepages_c_10021.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10021.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10021.c$(DependSuffix): wineguts/codepages/c_10021.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10021.c$(DependSuffix) -MM "wineguts/codepages/c_10021.c"

$(IntermediateDirectory)/codepages_c_10021.c$(PreprocessSuffix): wineguts/codepages/c_10021.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10021.c$(PreprocessSuffix) "wineguts/codepages/c_10021.c"

$(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix): wineguts/codepages/c_10029.c $(IntermediateDirectory)/codepages_c_10029.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10029.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10029.c$(DependSuffix): wineguts/codepages/c_10029.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10029.c$(DependSuffix) -MM "wineguts/codepages/c_10029.c"

$(IntermediateDirectory)/codepages_c_10029.c$(PreprocessSuffix): wineguts/codepages/c_10029.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10029.c$(PreprocessSuffix) "wineguts/codepages/c_10029.c"

$(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix): wineguts/codepages/c_10079.c $(IntermediateDirectory)/codepages_c_10079.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10079.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10079.c$(DependSuffix): wineguts/codepages/c_10079.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10079.c$(DependSuffix) -MM "wineguts/codepages/c_10079.c"

$(IntermediateDirectory)/codepages_c_10079.c$(PreprocessSuffix): wineguts/codepages/c_10079.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10079.c$(PreprocessSuffix) "wineguts/codepages/c_10079.c"

$(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix): wineguts/codepages/c_10081.c $(IntermediateDirectory)/codepages_c_10081.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10081.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10081.c$(DependSuffix): wineguts/codepages/c_10081.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10081.c$(DependSuffix) -MM "wineguts/codepages/c_10081.c"

$(IntermediateDirectory)/codepages_c_10081.c$(PreprocessSuffix): wineguts/codepages/c_10081.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10081.c$(PreprocessSuffix) "wineguts/codepages/c_10081.c"

$(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix): wineguts/codepages/c_10082.c $(IntermediateDirectory)/codepages_c_10082.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_10082.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10082.c$(DependSuffix): wineguts/codepages/c_10082.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_10082.c$(DependSuffix) -MM "wineguts/codepages/c_10082.c"

$(IntermediateDirectory)/codepages_c_10082.c$(PreprocessSuffix): wineguts/codepages/c_10082.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10082.c$(PreprocessSuffix) "wineguts/codepages/c_10082.c"

$(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix): wineguts/codepages/c_20127.c $(IntermediateDirectory)/codepages_c_20127.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_20127.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_20127.c$(DependSuffix): wineguts/codepages/c_20127.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_20127.c$(DependSuffix) -MM "wineguts/codepages/c_20127.c"

$(IntermediateDirectory)/codepages_c_20127.c$(PreprocessSuffix): wineguts/codepages/c_20127.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_20127.c$(PreprocessSuffix) "wineguts/codepages/c_20127.c"

$(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix): wineguts/codepages/c_20866.c $(IntermediateDirectory)/codepages_c_20866.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_20866.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_20866.c$(DependSuffix): wineguts/codepages/c_20866.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_20866.c$(DependSuffix) -MM "wineguts/codepages/c_20866.c"

$(IntermediateDirectory)/codepages_c_20866.c$(PreprocessSuffix): wineguts/codepages/c_20866.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_20866.c$(PreprocessSuffix) "wineguts/codepages/c_20866.c"

$(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix): wineguts/codepages/c_20932.c $(IntermediateDirectory)/codepages_c_20932.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_20932.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_20932.c$(DependSuffix): wineguts/codepages/c_20932.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_20932.c$(DependSuffix) -MM "wineguts/codepages/c_20932.c"

$(IntermediateDirectory)/codepages_c_20932.c$(PreprocessSuffix): wineguts/codepages/c_20932.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_20932.c$(PreprocessSuffix) "wineguts/codepages/c_20932.c"

$(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix): wineguts/codepages/c_21866.c $(IntermediateDirectory)/codepages_c_21866.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_21866.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_21866.c$(DependSuffix): wineguts/codepages/c_21866.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_21866.c$(DependSuffix) -MM "wineguts/codepages/c_21866.c"

$(IntermediateDirectory)/codepages_c_21866.c$(PreprocessSuffix): wineguts/codepages/c_21866.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_21866.c$(PreprocessSuffix) "wineguts/codepages/c_21866.c"

$(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix): wineguts/codepages/c_28591.c $(IntermediateDirectory)/codepages_c_28591.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28591.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28591.c$(DependSuffix): wineguts/codepages/c_28591.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28591.c$(DependSuffix) -MM "wineguts/codepages/c_28591.c"

$(IntermediateDirectory)/codepages_c_28591.c$(PreprocessSuffix): wineguts/codepages/c_28591.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28591.c$(PreprocessSuffix) "wineguts/codepages/c_28591.c"

$(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix): wineguts/codepages/c_28592.c $(IntermediateDirectory)/codepages_c_28592.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28592.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28592.c$(DependSuffix): wineguts/codepages/c_28592.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28592.c$(DependSuffix) -MM "wineguts/codepages/c_28592.c"

$(IntermediateDirectory)/codepages_c_28592.c$(PreprocessSuffix): wineguts/codepages/c_28592.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28592.c$(PreprocessSuffix) "wineguts/codepages/c_28592.c"

$(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix): wineguts/codepages/c_28593.c $(IntermediateDirectory)/codepages_c_28593.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28593.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28593.c$(DependSuffix): wineguts/codepages/c_28593.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28593.c$(DependSuffix) -MM "wineguts/codepages/c_28593.c"

$(IntermediateDirectory)/codepages_c_28593.c$(PreprocessSuffix): wineguts/codepages/c_28593.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28593.c$(PreprocessSuffix) "wineguts/codepages/c_28593.c"

$(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix): wineguts/codepages/c_28594.c $(IntermediateDirectory)/codepages_c_28594.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28594.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28594.c$(DependSuffix): wineguts/codepages/c_28594.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28594.c$(DependSuffix) -MM "wineguts/codepages/c_28594.c"

$(IntermediateDirectory)/codepages_c_28594.c$(PreprocessSuffix): wineguts/codepages/c_28594.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28594.c$(PreprocessSuffix) "wineguts/codepages/c_28594.c"

$(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix): wineguts/codepages/c_28595.c $(IntermediateDirectory)/codepages_c_28595.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28595.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28595.c$(DependSuffix): wineguts/codepages/c_28595.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28595.c$(DependSuffix) -MM "wineguts/codepages/c_28595.c"

$(IntermediateDirectory)/codepages_c_28595.c$(PreprocessSuffix): wineguts/codepages/c_28595.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28595.c$(PreprocessSuffix) "wineguts/codepages/c_28595.c"

$(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix): wineguts/codepages/c_28596.c $(IntermediateDirectory)/codepages_c_28596.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28596.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28596.c$(DependSuffix): wineguts/codepages/c_28596.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28596.c$(DependSuffix) -MM "wineguts/codepages/c_28596.c"

$(IntermediateDirectory)/codepages_c_28596.c$(PreprocessSuffix): wineguts/codepages/c_28596.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28596.c$(PreprocessSuffix) "wineguts/codepages/c_28596.c"

$(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix): wineguts/codepages/c_28597.c $(IntermediateDirectory)/codepages_c_28597.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28597.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28597.c$(DependSuffix): wineguts/codepages/c_28597.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28597.c$(DependSuffix) -MM "wineguts/codepages/c_28597.c"

$(IntermediateDirectory)/codepages_c_28597.c$(PreprocessSuffix): wineguts/codepages/c_28597.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28597.c$(PreprocessSuffix) "wineguts/codepages/c_28597.c"

$(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix): wineguts/codepages/c_28598.c $(IntermediateDirectory)/codepages_c_28598.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28598.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28598.c$(DependSuffix): wineguts/codepages/c_28598.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28598.c$(DependSuffix) -MM "wineguts/codepages/c_28598.c"

$(IntermediateDirectory)/codepages_c_28598.c$(PreprocessSuffix): wineguts/codepages/c_28598.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28598.c$(PreprocessSuffix) "wineguts/codepages/c_28598.c"

$(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix): wineguts/codepages/c_28599.c $(IntermediateDirectory)/codepages_c_28599.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28599.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28599.c$(DependSuffix): wineguts/codepages/c_28599.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28599.c$(DependSuffix) -MM "wineguts/codepages/c_28599.c"

$(IntermediateDirectory)/codepages_c_28599.c$(PreprocessSuffix): wineguts/codepages/c_28599.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28599.c$(PreprocessSuffix) "wineguts/codepages/c_28599.c"

$(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix): wineguts/codepages/c_28600.c $(IntermediateDirectory)/codepages_c_28600.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28600.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28600.c$(DependSuffix): wineguts/codepages/c_28600.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28600.c$(DependSuffix) -MM "wineguts/codepages/c_28600.c"

$(IntermediateDirectory)/codepages_c_28600.c$(PreprocessSuffix): wineguts/codepages/c_28600.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28600.c$(PreprocessSuffix) "wineguts/codepages/c_28600.c"

$(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix): wineguts/codepages/c_28603.c $(IntermediateDirectory)/codepages_c_28603.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28603.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28603.c$(DependSuffix): wineguts/codepages/c_28603.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28603.c$(DependSuffix) -MM "wineguts/codepages/c_28603.c"

$(IntermediateDirectory)/codepages_c_28603.c$(PreprocessSuffix): wineguts/codepages/c_28603.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28603.c$(PreprocessSuffix) "wineguts/codepages/c_28603.c"

$(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix): wineguts/codepages/c_28604.c $(IntermediateDirectory)/codepages_c_28604.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28604.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28604.c$(DependSuffix): wineguts/codepages/c_28604.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28604.c$(DependSuffix) -MM "wineguts/codepages/c_28604.c"

$(IntermediateDirectory)/codepages_c_28604.c$(PreprocessSuffix): wineguts/codepages/c_28604.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28604.c$(PreprocessSuffix) "wineguts/codepages/c_28604.c"

$(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix): wineguts/codepages/c_28605.c $(IntermediateDirectory)/codepages_c_28605.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28605.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28605.c$(DependSuffix): wineguts/codepages/c_28605.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28605.c$(DependSuffix) -MM "wineguts/codepages/c_28605.c"

$(IntermediateDirectory)/codepages_c_28605.c$(PreprocessSuffix): wineguts/codepages/c_28605.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28605.c$(PreprocessSuffix) "wineguts/codepages/c_28605.c"

$(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix): wineguts/codepages/c_28606.c $(IntermediateDirectory)/codepages_c_28606.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/user/projects/far2l/WinPort/wineguts/codepages/c_28606.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28606.c$(DependSuffix): wineguts/codepages/c_28606.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix) -MF$(IntermediateDirectory)/codepages_c_28606.c$(DependSuffix) -MM "wineguts/codepages/c_28606.c"

$(IntermediateDirectory)/codepages_c_28606.c$(PreprocessSuffix): wineguts/codepages/c_28606.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28606.c$(PreprocessSuffix) "wineguts/codepages/c_28606.c"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


