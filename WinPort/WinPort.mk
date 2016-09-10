##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=WinPort
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./Debug
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
OutputFile             :=$(IntermediateDirectory)/lib$(ProjectName).a
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="WinPort.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell wx-config --debug=yes --libs --unicode=yes)
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)src 
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
CXXFLAGS :=  -g -O2 -std=c++11 -Wall $(shell wx-config --cxxflags --debug=yes --unicode=yes) $(shell pkg-config glib-2.0 --cflags)  -Wno-unused-function -fvisibility=hidden -Wno-unused-function $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 -Wall $(shell wx-config --cxxflags --debug=yes --unicode=yes) $(shell pkg-config glib-2.0 --cflags) -Wno-unused-function -fvisibility=hidden -Wno-unused-function $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_APIClipboard.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIConsole.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIFiles.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIFSNotify.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIKeyboard.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIMemory.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIOther.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIPrintFormat.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIRegistry.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APIStringCodepages.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_APIStringMap.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APISynch.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_APITime.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConsoleBuffer.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConsoleInput.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConsoleOutput.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_KeyFileHelper.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_stdafx.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_WinPortHandle.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_WinPortSynch.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConvertUTF.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix) \
	$(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix) $(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix) $(IntermediateDirectory)/UI_Paint.cpp$(ObjectSuffix) $(IntermediateDirectory)/UI_Main.cpp$(ObjectSuffix) $(IntermediateDirectory)/UI_wxWinTranslations.cpp$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix) \
	$(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix) $(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix) 



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
	@$(MakeDirCommand) "../.build-debug"
	@echo rebuilt > "../.build-debug/WinPort"

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


./Debug:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_APIClipboard.cpp$(ObjectSuffix): src/APIClipboard.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIClipboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIClipboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIClipboard.cpp$(PreprocessSuffix): src/APIClipboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIClipboard.cpp$(PreprocessSuffix) "src/APIClipboard.cpp"

$(IntermediateDirectory)/src_APIConsole.cpp$(ObjectSuffix): src/APIConsole.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIConsole.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIConsole.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIConsole.cpp$(PreprocessSuffix): src/APIConsole.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIConsole.cpp$(PreprocessSuffix) "src/APIConsole.cpp"

$(IntermediateDirectory)/src_APIFiles.cpp$(ObjectSuffix): src/APIFiles.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIFiles.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIFiles.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIFiles.cpp$(PreprocessSuffix): src/APIFiles.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIFiles.cpp$(PreprocessSuffix) "src/APIFiles.cpp"

$(IntermediateDirectory)/src_APIFSNotify.cpp$(ObjectSuffix): src/APIFSNotify.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIFSNotify.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIFSNotify.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIFSNotify.cpp$(PreprocessSuffix): src/APIFSNotify.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIFSNotify.cpp$(PreprocessSuffix) "src/APIFSNotify.cpp"

$(IntermediateDirectory)/src_APIKeyboard.cpp$(ObjectSuffix): src/APIKeyboard.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIKeyboard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIKeyboard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIKeyboard.cpp$(PreprocessSuffix): src/APIKeyboard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIKeyboard.cpp$(PreprocessSuffix) "src/APIKeyboard.cpp"

$(IntermediateDirectory)/src_APIMemory.cpp$(ObjectSuffix): src/APIMemory.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIMemory.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIMemory.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIMemory.cpp$(PreprocessSuffix): src/APIMemory.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIMemory.cpp$(PreprocessSuffix) "src/APIMemory.cpp"

$(IntermediateDirectory)/src_APIOther.cpp$(ObjectSuffix): src/APIOther.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIOther.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIOther.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIOther.cpp$(PreprocessSuffix): src/APIOther.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIOther.cpp$(PreprocessSuffix) "src/APIOther.cpp"

$(IntermediateDirectory)/src_APIPrintFormat.cpp$(ObjectSuffix): src/APIPrintFormat.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIPrintFormat.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIPrintFormat.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIPrintFormat.cpp$(PreprocessSuffix): src/APIPrintFormat.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIPrintFormat.cpp$(PreprocessSuffix) "src/APIPrintFormat.cpp"

$(IntermediateDirectory)/src_APIRegistry.cpp$(ObjectSuffix): src/APIRegistry.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIRegistry.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIRegistry.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIRegistry.cpp$(PreprocessSuffix): src/APIRegistry.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIRegistry.cpp$(PreprocessSuffix) "src/APIRegistry.cpp"

$(IntermediateDirectory)/src_APIStringCodepages.cpp$(ObjectSuffix): src/APIStringCodepages.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIStringCodepages.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIStringCodepages.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIStringCodepages.cpp$(PreprocessSuffix): src/APIStringCodepages.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIStringCodepages.cpp$(PreprocessSuffix) "src/APIStringCodepages.cpp"

$(IntermediateDirectory)/src_APIStringMap.cpp$(ObjectSuffix): src/APIStringMap.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APIStringMap.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APIStringMap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APIStringMap.cpp$(PreprocessSuffix): src/APIStringMap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APIStringMap.cpp$(PreprocessSuffix) "src/APIStringMap.cpp"

$(IntermediateDirectory)/src_APISynch.cpp$(ObjectSuffix): src/APISynch.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APISynch.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APISynch.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APISynch.cpp$(PreprocessSuffix): src/APISynch.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APISynch.cpp$(PreprocessSuffix) "src/APISynch.cpp"

$(IntermediateDirectory)/src_APITime.cpp$(ObjectSuffix): src/APITime.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/APITime.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_APITime.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_APITime.cpp$(PreprocessSuffix): src/APITime.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_APITime.cpp$(PreprocessSuffix) "src/APITime.cpp"

$(IntermediateDirectory)/src_ConsoleBuffer.cpp$(ObjectSuffix): src/ConsoleBuffer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConsoleBuffer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConsoleBuffer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConsoleBuffer.cpp$(PreprocessSuffix): src/ConsoleBuffer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConsoleBuffer.cpp$(PreprocessSuffix) "src/ConsoleBuffer.cpp"

$(IntermediateDirectory)/src_ConsoleInput.cpp$(ObjectSuffix): src/ConsoleInput.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConsoleInput.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConsoleInput.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConsoleInput.cpp$(PreprocessSuffix): src/ConsoleInput.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConsoleInput.cpp$(PreprocessSuffix) "src/ConsoleInput.cpp"

$(IntermediateDirectory)/src_ConsoleOutput.cpp$(ObjectSuffix): src/ConsoleOutput.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConsoleOutput.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConsoleOutput.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConsoleOutput.cpp$(PreprocessSuffix): src/ConsoleOutput.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConsoleOutput.cpp$(PreprocessSuffix) "src/ConsoleOutput.cpp"

$(IntermediateDirectory)/src_KeyFileHelper.cpp$(ObjectSuffix): src/KeyFileHelper.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/KeyFileHelper.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_KeyFileHelper.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_KeyFileHelper.cpp$(PreprocessSuffix): src/KeyFileHelper.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_KeyFileHelper.cpp$(PreprocessSuffix) "src/KeyFileHelper.cpp"

$(IntermediateDirectory)/src_stdafx.cpp$(ObjectSuffix): src/stdafx.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/stdafx.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_stdafx.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_stdafx.cpp$(PreprocessSuffix): src/stdafx.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_stdafx.cpp$(PreprocessSuffix) "src/stdafx.cpp"

$(IntermediateDirectory)/src_Utils.cpp$(ObjectSuffix): src/Utils.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Utils.cpp$(PreprocessSuffix): src/Utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Utils.cpp$(PreprocessSuffix) "src/Utils.cpp"

$(IntermediateDirectory)/src_WinPortHandle.cpp$(ObjectSuffix): src/WinPortHandle.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/WinPortHandle.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_WinPortHandle.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_WinPortHandle.cpp$(PreprocessSuffix): src/WinPortHandle.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_WinPortHandle.cpp$(PreprocessSuffix) "src/WinPortHandle.cpp"

$(IntermediateDirectory)/src_WinPortSynch.cpp$(ObjectSuffix): src/WinPortSynch.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/WinPortSynch.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_WinPortSynch.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_WinPortSynch.cpp$(PreprocessSuffix): src/WinPortSynch.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_WinPortSynch.cpp$(PreprocessSuffix) "src/WinPortSynch.cpp"

$(IntermediateDirectory)/src_ConvertUTF.c$(ObjectSuffix): src/ConvertUTF.c 
	$(CC) $(SourceSwitch) "./src/ConvertUTF.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConvertUTF.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConvertUTF.c$(PreprocessSuffix): src/ConvertUTF.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConvertUTF.c$(PreprocessSuffix) "src/ConvertUTF.c"

$(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix): wineguts/casemap.c 
	$(CC) $(SourceSwitch) "./wineguts/casemap.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_casemap.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_casemap.c$(PreprocessSuffix): wineguts/casemap.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_casemap.c$(PreprocessSuffix) "wineguts/casemap.c"

$(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix): wineguts/collation.c 
	$(CC) $(SourceSwitch) "./wineguts/collation.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_collation.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_collation.c$(PreprocessSuffix): wineguts/collation.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_collation.c$(PreprocessSuffix) "wineguts/collation.c"

$(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix): wineguts/compose.c 
	$(CC) $(SourceSwitch) "./wineguts/compose.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_compose.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_compose.c$(PreprocessSuffix): wineguts/compose.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_compose.c$(PreprocessSuffix) "wineguts/compose.c"

$(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix): wineguts/cpsymbol.c 
	$(CC) $(SourceSwitch) "./wineguts/cpsymbol.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_cpsymbol.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_cpsymbol.c$(PreprocessSuffix): wineguts/cpsymbol.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_cpsymbol.c$(PreprocessSuffix) "wineguts/cpsymbol.c"

$(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix): wineguts/cptable.c 
	$(CC) $(SourceSwitch) "./wineguts/cptable.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_cptable.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_cptable.c$(PreprocessSuffix): wineguts/cptable.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_cptable.c$(PreprocessSuffix) "wineguts/cptable.c"

$(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix): wineguts/decompose.c 
	$(CC) $(SourceSwitch) "./wineguts/decompose.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_decompose.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_decompose.c$(PreprocessSuffix): wineguts/decompose.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_decompose.c$(PreprocessSuffix) "wineguts/decompose.c"

$(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix): wineguts/locale.c 
	$(CC) $(SourceSwitch) "./wineguts/locale.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_locale.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_locale.c$(PreprocessSuffix): wineguts/locale.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_locale.c$(PreprocessSuffix) "wineguts/locale.c"

$(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix): wineguts/mbtowc.c 
	$(CC) $(SourceSwitch) "./wineguts/mbtowc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_mbtowc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_mbtowc.c$(PreprocessSuffix): wineguts/mbtowc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_mbtowc.c$(PreprocessSuffix) "wineguts/mbtowc.c"

$(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix): wineguts/sortkey.c 
	$(CC) $(SourceSwitch) "./wineguts/sortkey.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_sortkey.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_sortkey.c$(PreprocessSuffix): wineguts/sortkey.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_sortkey.c$(PreprocessSuffix) "wineguts/sortkey.c"

$(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix): wineguts/utf8.c 
	$(CC) $(SourceSwitch) "./wineguts/utf8.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_utf8.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_utf8.c$(PreprocessSuffix): wineguts/utf8.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_utf8.c$(PreprocessSuffix) "wineguts/utf8.c"

$(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix): wineguts/wctomb.c 
	$(CC) $(SourceSwitch) "./wineguts/wctomb.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_wctomb.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_wctomb.c$(PreprocessSuffix): wineguts/wctomb.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_wctomb.c$(PreprocessSuffix) "wineguts/wctomb.c"

$(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix): wineguts/wctype.c 
	$(CC) $(SourceSwitch) "./wineguts/wctype.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wineguts_wctype.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wineguts_wctype.c$(PreprocessSuffix): wineguts/wctype.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wineguts_wctype.c$(PreprocessSuffix) "wineguts/wctype.c"

$(IntermediateDirectory)/UI_Paint.cpp$(ObjectSuffix): src/UI/Paint.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/UI/Paint.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UI_Paint.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UI_Paint.cpp$(PreprocessSuffix): src/UI/Paint.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UI_Paint.cpp$(PreprocessSuffix) "src/UI/Paint.cpp"

$(IntermediateDirectory)/UI_Main.cpp$(ObjectSuffix): src/UI/Main.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/UI/Main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UI_Main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UI_Main.cpp$(PreprocessSuffix): src/UI/Main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UI_Main.cpp$(PreprocessSuffix) "src/UI/Main.cpp"

$(IntermediateDirectory)/UI_wxWinTranslations.cpp$(ObjectSuffix): src/UI/wxWinTranslations.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/UI/wxWinTranslations.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/UI_wxWinTranslations.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/UI_wxWinTranslations.cpp$(PreprocessSuffix): src/UI/wxWinTranslations.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/UI_wxWinTranslations.cpp$(PreprocessSuffix) "src/UI/wxWinTranslations.cpp"

$(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix): wineguts/codepages/c_037.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_037.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_037.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_037.c$(PreprocessSuffix): wineguts/codepages/c_037.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_037.c$(PreprocessSuffix) "wineguts/codepages/c_037.c"

$(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix): wineguts/codepages/c_424.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_424.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_424.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_424.c$(PreprocessSuffix): wineguts/codepages/c_424.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_424.c$(PreprocessSuffix) "wineguts/codepages/c_424.c"

$(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix): wineguts/codepages/c_437.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_437.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_437.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_437.c$(PreprocessSuffix): wineguts/codepages/c_437.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_437.c$(PreprocessSuffix) "wineguts/codepages/c_437.c"

$(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix): wineguts/codepages/c_500.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_500.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_500.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_500.c$(PreprocessSuffix): wineguts/codepages/c_500.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_500.c$(PreprocessSuffix) "wineguts/codepages/c_500.c"

$(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix): wineguts/codepages/c_737.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_737.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_737.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_737.c$(PreprocessSuffix): wineguts/codepages/c_737.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_737.c$(PreprocessSuffix) "wineguts/codepages/c_737.c"

$(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix): wineguts/codepages/c_775.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_775.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_775.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_775.c$(PreprocessSuffix): wineguts/codepages/c_775.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_775.c$(PreprocessSuffix) "wineguts/codepages/c_775.c"

$(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix): wineguts/codepages/c_850.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_850.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_850.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_850.c$(PreprocessSuffix): wineguts/codepages/c_850.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_850.c$(PreprocessSuffix) "wineguts/codepages/c_850.c"

$(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix): wineguts/codepages/c_852.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_852.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_852.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_852.c$(PreprocessSuffix): wineguts/codepages/c_852.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_852.c$(PreprocessSuffix) "wineguts/codepages/c_852.c"

$(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix): wineguts/codepages/c_855.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_855.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_855.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_855.c$(PreprocessSuffix): wineguts/codepages/c_855.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_855.c$(PreprocessSuffix) "wineguts/codepages/c_855.c"

$(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix): wineguts/codepages/c_856.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_856.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_856.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_856.c$(PreprocessSuffix): wineguts/codepages/c_856.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_856.c$(PreprocessSuffix) "wineguts/codepages/c_856.c"

$(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix): wineguts/codepages/c_857.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_857.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_857.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_857.c$(PreprocessSuffix): wineguts/codepages/c_857.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_857.c$(PreprocessSuffix) "wineguts/codepages/c_857.c"

$(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix): wineguts/codepages/c_860.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_860.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_860.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_860.c$(PreprocessSuffix): wineguts/codepages/c_860.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_860.c$(PreprocessSuffix) "wineguts/codepages/c_860.c"

$(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix): wineguts/codepages/c_861.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_861.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_861.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_861.c$(PreprocessSuffix): wineguts/codepages/c_861.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_861.c$(PreprocessSuffix) "wineguts/codepages/c_861.c"

$(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix): wineguts/codepages/c_862.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_862.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_862.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_862.c$(PreprocessSuffix): wineguts/codepages/c_862.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_862.c$(PreprocessSuffix) "wineguts/codepages/c_862.c"

$(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix): wineguts/codepages/c_863.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_863.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_863.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_863.c$(PreprocessSuffix): wineguts/codepages/c_863.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_863.c$(PreprocessSuffix) "wineguts/codepages/c_863.c"

$(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix): wineguts/codepages/c_864.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_864.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_864.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_864.c$(PreprocessSuffix): wineguts/codepages/c_864.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_864.c$(PreprocessSuffix) "wineguts/codepages/c_864.c"

$(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix): wineguts/codepages/c_865.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_865.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_865.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_865.c$(PreprocessSuffix): wineguts/codepages/c_865.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_865.c$(PreprocessSuffix) "wineguts/codepages/c_865.c"

$(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix): wineguts/codepages/c_866.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_866.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_866.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_866.c$(PreprocessSuffix): wineguts/codepages/c_866.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_866.c$(PreprocessSuffix) "wineguts/codepages/c_866.c"

$(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix): wineguts/codepages/c_869.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_869.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_869.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_869.c$(PreprocessSuffix): wineguts/codepages/c_869.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_869.c$(PreprocessSuffix) "wineguts/codepages/c_869.c"

$(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix): wineguts/codepages/c_874.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_874.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_874.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_874.c$(PreprocessSuffix): wineguts/codepages/c_874.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_874.c$(PreprocessSuffix) "wineguts/codepages/c_874.c"

$(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix): wineguts/codepages/c_875.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_875.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_875.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_875.c$(PreprocessSuffix): wineguts/codepages/c_875.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_875.c$(PreprocessSuffix) "wineguts/codepages/c_875.c"

$(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix): wineguts/codepages/c_878.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_878.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_878.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_878.c$(PreprocessSuffix): wineguts/codepages/c_878.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_878.c$(PreprocessSuffix) "wineguts/codepages/c_878.c"

$(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix): wineguts/codepages/c_932.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_932.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_932.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_932.c$(PreprocessSuffix): wineguts/codepages/c_932.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_932.c$(PreprocessSuffix) "wineguts/codepages/c_932.c"

$(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix): wineguts/codepages/c_936.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_936.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_936.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_936.c$(PreprocessSuffix): wineguts/codepages/c_936.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_936.c$(PreprocessSuffix) "wineguts/codepages/c_936.c"

$(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix): wineguts/codepages/c_949.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_949.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_949.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_949.c$(PreprocessSuffix): wineguts/codepages/c_949.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_949.c$(PreprocessSuffix) "wineguts/codepages/c_949.c"

$(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix): wineguts/codepages/c_950.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_950.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_950.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_950.c$(PreprocessSuffix): wineguts/codepages/c_950.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_950.c$(PreprocessSuffix) "wineguts/codepages/c_950.c"

$(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix): wineguts/codepages/c_1006.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1006.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1006.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1006.c$(PreprocessSuffix): wineguts/codepages/c_1006.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1006.c$(PreprocessSuffix) "wineguts/codepages/c_1006.c"

$(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix): wineguts/codepages/c_1026.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1026.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1026.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1026.c$(PreprocessSuffix): wineguts/codepages/c_1026.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1026.c$(PreprocessSuffix) "wineguts/codepages/c_1026.c"

$(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix): wineguts/codepages/c_1250.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1250.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1250.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1250.c$(PreprocessSuffix): wineguts/codepages/c_1250.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1250.c$(PreprocessSuffix) "wineguts/codepages/c_1250.c"

$(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix): wineguts/codepages/c_1251.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1251.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1251.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1251.c$(PreprocessSuffix): wineguts/codepages/c_1251.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1251.c$(PreprocessSuffix) "wineguts/codepages/c_1251.c"

$(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix): wineguts/codepages/c_1252.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1252.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1252.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1252.c$(PreprocessSuffix): wineguts/codepages/c_1252.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1252.c$(PreprocessSuffix) "wineguts/codepages/c_1252.c"

$(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix): wineguts/codepages/c_1253.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1253.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1253.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1253.c$(PreprocessSuffix): wineguts/codepages/c_1253.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1253.c$(PreprocessSuffix) "wineguts/codepages/c_1253.c"

$(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix): wineguts/codepages/c_1254.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1254.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1254.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1254.c$(PreprocessSuffix): wineguts/codepages/c_1254.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1254.c$(PreprocessSuffix) "wineguts/codepages/c_1254.c"

$(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix): wineguts/codepages/c_1255.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1255.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1255.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1255.c$(PreprocessSuffix): wineguts/codepages/c_1255.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1255.c$(PreprocessSuffix) "wineguts/codepages/c_1255.c"

$(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix): wineguts/codepages/c_1256.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1256.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1256.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1256.c$(PreprocessSuffix): wineguts/codepages/c_1256.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1256.c$(PreprocessSuffix) "wineguts/codepages/c_1256.c"

$(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix): wineguts/codepages/c_1257.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1257.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1257.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1257.c$(PreprocessSuffix): wineguts/codepages/c_1257.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1257.c$(PreprocessSuffix) "wineguts/codepages/c_1257.c"

$(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix): wineguts/codepages/c_1258.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1258.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1258.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1258.c$(PreprocessSuffix): wineguts/codepages/c_1258.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1258.c$(PreprocessSuffix) "wineguts/codepages/c_1258.c"

$(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix): wineguts/codepages/c_1361.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_1361.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_1361.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_1361.c$(PreprocessSuffix): wineguts/codepages/c_1361.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_1361.c$(PreprocessSuffix) "wineguts/codepages/c_1361.c"

$(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix): wineguts/codepages/c_10000.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10000.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10000.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10000.c$(PreprocessSuffix): wineguts/codepages/c_10000.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10000.c$(PreprocessSuffix) "wineguts/codepages/c_10000.c"

$(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix): wineguts/codepages/c_10001.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10001.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10001.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10001.c$(PreprocessSuffix): wineguts/codepages/c_10001.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10001.c$(PreprocessSuffix) "wineguts/codepages/c_10001.c"

$(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix): wineguts/codepages/c_10002.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10002.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10002.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10002.c$(PreprocessSuffix): wineguts/codepages/c_10002.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10002.c$(PreprocessSuffix) "wineguts/codepages/c_10002.c"

$(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix): wineguts/codepages/c_10003.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10003.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10003.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10003.c$(PreprocessSuffix): wineguts/codepages/c_10003.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10003.c$(PreprocessSuffix) "wineguts/codepages/c_10003.c"

$(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix): wineguts/codepages/c_10004.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10004.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10004.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10004.c$(PreprocessSuffix): wineguts/codepages/c_10004.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10004.c$(PreprocessSuffix) "wineguts/codepages/c_10004.c"

$(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix): wineguts/codepages/c_10005.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10005.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10005.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10005.c$(PreprocessSuffix): wineguts/codepages/c_10005.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10005.c$(PreprocessSuffix) "wineguts/codepages/c_10005.c"

$(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix): wineguts/codepages/c_10006.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10006.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10006.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10006.c$(PreprocessSuffix): wineguts/codepages/c_10006.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10006.c$(PreprocessSuffix) "wineguts/codepages/c_10006.c"

$(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix): wineguts/codepages/c_10007.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10007.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10007.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10007.c$(PreprocessSuffix): wineguts/codepages/c_10007.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10007.c$(PreprocessSuffix) "wineguts/codepages/c_10007.c"

$(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix): wineguts/codepages/c_10008.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10008.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10008.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10008.c$(PreprocessSuffix): wineguts/codepages/c_10008.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10008.c$(PreprocessSuffix) "wineguts/codepages/c_10008.c"

$(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix): wineguts/codepages/c_10010.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10010.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10010.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10010.c$(PreprocessSuffix): wineguts/codepages/c_10010.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10010.c$(PreprocessSuffix) "wineguts/codepages/c_10010.c"

$(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix): wineguts/codepages/c_10017.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10017.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10017.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10017.c$(PreprocessSuffix): wineguts/codepages/c_10017.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10017.c$(PreprocessSuffix) "wineguts/codepages/c_10017.c"

$(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix): wineguts/codepages/c_10021.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10021.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10021.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10021.c$(PreprocessSuffix): wineguts/codepages/c_10021.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10021.c$(PreprocessSuffix) "wineguts/codepages/c_10021.c"

$(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix): wineguts/codepages/c_10029.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10029.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10029.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10029.c$(PreprocessSuffix): wineguts/codepages/c_10029.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10029.c$(PreprocessSuffix) "wineguts/codepages/c_10029.c"

$(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix): wineguts/codepages/c_10079.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10079.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10079.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10079.c$(PreprocessSuffix): wineguts/codepages/c_10079.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10079.c$(PreprocessSuffix) "wineguts/codepages/c_10079.c"

$(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix): wineguts/codepages/c_10081.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10081.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10081.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10081.c$(PreprocessSuffix): wineguts/codepages/c_10081.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10081.c$(PreprocessSuffix) "wineguts/codepages/c_10081.c"

$(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix): wineguts/codepages/c_10082.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_10082.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_10082.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_10082.c$(PreprocessSuffix): wineguts/codepages/c_10082.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_10082.c$(PreprocessSuffix) "wineguts/codepages/c_10082.c"

$(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix): wineguts/codepages/c_20127.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_20127.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_20127.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_20127.c$(PreprocessSuffix): wineguts/codepages/c_20127.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_20127.c$(PreprocessSuffix) "wineguts/codepages/c_20127.c"

$(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix): wineguts/codepages/c_20866.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_20866.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_20866.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_20866.c$(PreprocessSuffix): wineguts/codepages/c_20866.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_20866.c$(PreprocessSuffix) "wineguts/codepages/c_20866.c"

$(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix): wineguts/codepages/c_20932.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_20932.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_20932.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_20932.c$(PreprocessSuffix): wineguts/codepages/c_20932.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_20932.c$(PreprocessSuffix) "wineguts/codepages/c_20932.c"

$(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix): wineguts/codepages/c_21866.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_21866.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_21866.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_21866.c$(PreprocessSuffix): wineguts/codepages/c_21866.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_21866.c$(PreprocessSuffix) "wineguts/codepages/c_21866.c"

$(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix): wineguts/codepages/c_28591.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28591.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28591.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28591.c$(PreprocessSuffix): wineguts/codepages/c_28591.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28591.c$(PreprocessSuffix) "wineguts/codepages/c_28591.c"

$(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix): wineguts/codepages/c_28592.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28592.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28592.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28592.c$(PreprocessSuffix): wineguts/codepages/c_28592.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28592.c$(PreprocessSuffix) "wineguts/codepages/c_28592.c"

$(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix): wineguts/codepages/c_28593.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28593.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28593.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28593.c$(PreprocessSuffix): wineguts/codepages/c_28593.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28593.c$(PreprocessSuffix) "wineguts/codepages/c_28593.c"

$(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix): wineguts/codepages/c_28594.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28594.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28594.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28594.c$(PreprocessSuffix): wineguts/codepages/c_28594.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28594.c$(PreprocessSuffix) "wineguts/codepages/c_28594.c"

$(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix): wineguts/codepages/c_28595.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28595.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28595.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28595.c$(PreprocessSuffix): wineguts/codepages/c_28595.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28595.c$(PreprocessSuffix) "wineguts/codepages/c_28595.c"

$(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix): wineguts/codepages/c_28596.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28596.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28596.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28596.c$(PreprocessSuffix): wineguts/codepages/c_28596.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28596.c$(PreprocessSuffix) "wineguts/codepages/c_28596.c"

$(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix): wineguts/codepages/c_28597.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28597.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28597.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28597.c$(PreprocessSuffix): wineguts/codepages/c_28597.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28597.c$(PreprocessSuffix) "wineguts/codepages/c_28597.c"

$(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix): wineguts/codepages/c_28598.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28598.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28598.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28598.c$(PreprocessSuffix): wineguts/codepages/c_28598.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28598.c$(PreprocessSuffix) "wineguts/codepages/c_28598.c"

$(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix): wineguts/codepages/c_28599.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28599.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28599.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28599.c$(PreprocessSuffix): wineguts/codepages/c_28599.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28599.c$(PreprocessSuffix) "wineguts/codepages/c_28599.c"

$(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix): wineguts/codepages/c_28600.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28600.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28600.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28600.c$(PreprocessSuffix): wineguts/codepages/c_28600.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28600.c$(PreprocessSuffix) "wineguts/codepages/c_28600.c"

$(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix): wineguts/codepages/c_28603.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28603.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28603.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28603.c$(PreprocessSuffix): wineguts/codepages/c_28603.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28603.c$(PreprocessSuffix) "wineguts/codepages/c_28603.c"

$(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix): wineguts/codepages/c_28604.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28604.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28604.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28604.c$(PreprocessSuffix): wineguts/codepages/c_28604.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28604.c$(PreprocessSuffix) "wineguts/codepages/c_28604.c"

$(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix): wineguts/codepages/c_28605.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28605.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28605.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28605.c$(PreprocessSuffix): wineguts/codepages/c_28605.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28605.c$(PreprocessSuffix) "wineguts/codepages/c_28605.c"

$(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix): wineguts/codepages/c_28606.c 
	$(CC) $(SourceSwitch) "./wineguts/codepages/c_28606.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/codepages_c_28606.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/codepages_c_28606.c$(PreprocessSuffix): wineguts/codepages/c_28606.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/codepages_c_28606.c$(PreprocessSuffix) "wineguts/codepages/c_28606.c"

##
## Clean
##
clean:
	$(RM) -r ./Debug/


