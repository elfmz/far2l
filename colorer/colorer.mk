##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=colorer
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./$(ConfigurationName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=12/09/16
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
OutputFile             :=../Build/Plugins/Colorer/bin/$(ProjectName).far-plug-wide
Preprocessors          :=$(PreprocessorSwitch)_UNICODE $(PreprocessorSwitch)UNICODE $(PreprocessorSwitch)__unix__ 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="colorer.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell wx-config --debug=yes --libs --unicode=yes) -fPIC
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)src $(IncludeSwitch)src/zlib $(IncludeSwitch)src/pcolorer2 $(IncludeSwitch)src/shared $(IncludeSwitch)src/shared/colorer $(IncludeSwitch)src/shared/common $(IncludeSwitch)src/shared/cregexp $(IncludeSwitch)src/shared/misc $(IncludeSwitch)src/shared/xml $(IncludeSwitch)../far2l/ $(IncludeSwitch)../far2l/Include $(IncludeSwitch)../WinPort $(IncludeSwitch)../utils/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)utils 
ArLibs                 :=  "utils" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)../WinPort/$(ConfigurationName) $(LibraryPathSwitch)../utils/$(ConfigurationName) 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O2 -std=c++11 $(shell wx-config --cxxflags --debug=yes --unicode=yes) -fPIC -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
CFLAGS   :=  -g -O2 -std=c99 $(shell wx-config --cxxflags --debug=yes --unicode=yes) -fPIC -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/zlib_uncompr.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_deflate.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_inftrees.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_trees.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_gzwrite.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_infback.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_crc32.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_gzclose.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_zutil.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_inflate.c$(ObjectSuffix) \
	$(IntermediateDirectory)/zlib_gzlib.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_gzread.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_adler32.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_compress.c$(ObjectSuffix) $(IntermediateDirectory)/zlib_inffast.c$(ObjectSuffix) $(IntermediateDirectory)/pcolorer2_FarEditor.cpp$(ObjectSuffix) $(IntermediateDirectory)/pcolorer2_ChooseTypeMenu.cpp$(ObjectSuffix) $(IntermediateDirectory)/pcolorer2_tools.cpp$(ObjectSuffix) $(IntermediateDirectory)/pcolorer2_registry_wide.cpp$(ObjectSuffix) $(IntermediateDirectory)/pcolorer2_FarHrcSettings.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/pcolorer2_FarEditorSet.cpp$(ObjectSuffix) $(IntermediateDirectory)/pcolorer2_pcolorer.cpp$(ObjectSuffix) $(IntermediateDirectory)/common_Exception.cpp$(ObjectSuffix) $(IntermediateDirectory)/common_Logging.cpp$(ObjectSuffix) $(IntermediateDirectory)/common_MemoryChunks.cpp$(ObjectSuffix) $(IntermediateDirectory)/colorer_ParserFactory.cpp$(ObjectSuffix) $(IntermediateDirectory)/misc_malloc.c$(ObjectSuffix) $(IntermediateDirectory)/unicode_BitArray.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_CharacterClass.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_DString.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/unicode_Encodings.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_String.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_SString.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_UnicodeTools.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_Character.cpp$(ObjectSuffix) $(IntermediateDirectory)/unicode_StringBuffer.cpp$(ObjectSuffix) $(IntermediateDirectory)/cregexp_cregexp.cpp$(ObjectSuffix) $(IntermediateDirectory)/xml_xmldom.cpp$(ObjectSuffix) $(IntermediateDirectory)/minizip_mztools.c$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/minizip_zip.c$(ObjectSuffix) \
	$(IntermediateDirectory)/minizip_ioapi.c$(ObjectSuffix) $(IntermediateDirectory)/minizip_unzip.c$(ObjectSuffix) $(IntermediateDirectory)/io_Writer.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_FileInputSource.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_HTTPInputSource.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_SharedInputSource.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_FileWriter.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_StreamWriter.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_InputSource.cpp$(ObjectSuffix) $(IntermediateDirectory)/io_JARInputSource.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/parsers_HRCParserImpl.cpp$(ObjectSuffix) $(IntermediateDirectory)/parsers_TextParserImpl.cpp$(ObjectSuffix) $(IntermediateDirectory)/viewer_TextLinesStore.cpp$(ObjectSuffix) $(IntermediateDirectory)/viewer_TextConsoleViewer.cpp$(ObjectSuffix) $(IntermediateDirectory)/viewer_ConsoleTools.cpp$(ObjectSuffix) $(IntermediateDirectory)/editor_Outliner.cpp$(ObjectSuffix) $(IntermediateDirectory)/editor_BaseEditor.cpp$(ObjectSuffix) $(IntermediateDirectory)/handlers_FileErrorHandler.cpp$(ObjectSuffix) $(IntermediateDirectory)/handlers_TextHRDMapper.cpp$(ObjectSuffix) $(IntermediateDirectory)/handlers_LineRegionsSupport.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/handlers_StyledHRDMapper.cpp$(ObjectSuffix) $(IntermediateDirectory)/handlers_LineRegionsCompactSupport.cpp$(ObjectSuffix) $(IntermediateDirectory)/handlers_RegionMapperImpl.cpp$(ObjectSuffix) $(IntermediateDirectory)/handlers_ErrorHandlerWriter.cpp$(ObjectSuffix) $(IntermediateDirectory)/helpers_HRCParserHelpers.cpp$(ObjectSuffix) $(IntermediateDirectory)/helpers_TextParserHelpers.cpp$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) 

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
	$(SharedObjectLinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)
	@$(MakeDirCommand) "../.build-debug"
	@echo rebuilt > "../.build-debug/colorer"

PostBuild:
	@echo Executing Post Build commands ...
	
	@echo Done

MakeIntermediateDirs:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)

PreBuild:
	@echo Executing Pre Build commands ...
	mkdir -p ../Build/Plugins/Colorer/
	cp -R ./configs/* ../Build/Plugins/Colorer/
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/zlib_uncompr.c$(ObjectSuffix): src/zlib/uncompr.c 
	$(CC) $(SourceSwitch) "./src/zlib/uncompr.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_uncompr.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_uncompr.c$(PreprocessSuffix): src/zlib/uncompr.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_uncompr.c$(PreprocessSuffix) "src/zlib/uncompr.c"

$(IntermediateDirectory)/zlib_deflate.c$(ObjectSuffix): src/zlib/deflate.c 
	$(CC) $(SourceSwitch) "./src/zlib/deflate.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_deflate.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_deflate.c$(PreprocessSuffix): src/zlib/deflate.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_deflate.c$(PreprocessSuffix) "src/zlib/deflate.c"

$(IntermediateDirectory)/zlib_inftrees.c$(ObjectSuffix): src/zlib/inftrees.c 
	$(CC) $(SourceSwitch) "./src/zlib/inftrees.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_inftrees.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_inftrees.c$(PreprocessSuffix): src/zlib/inftrees.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_inftrees.c$(PreprocessSuffix) "src/zlib/inftrees.c"

$(IntermediateDirectory)/zlib_trees.c$(ObjectSuffix): src/zlib/trees.c 
	$(CC) $(SourceSwitch) "./src/zlib/trees.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_trees.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_trees.c$(PreprocessSuffix): src/zlib/trees.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_trees.c$(PreprocessSuffix) "src/zlib/trees.c"

$(IntermediateDirectory)/zlib_gzwrite.c$(ObjectSuffix): src/zlib/gzwrite.c 
	$(CC) $(SourceSwitch) "./src/zlib/gzwrite.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_gzwrite.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_gzwrite.c$(PreprocessSuffix): src/zlib/gzwrite.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_gzwrite.c$(PreprocessSuffix) "src/zlib/gzwrite.c"

$(IntermediateDirectory)/zlib_infback.c$(ObjectSuffix): src/zlib/infback.c 
	$(CC) $(SourceSwitch) "./src/zlib/infback.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_infback.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_infback.c$(PreprocessSuffix): src/zlib/infback.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_infback.c$(PreprocessSuffix) "src/zlib/infback.c"

$(IntermediateDirectory)/zlib_crc32.c$(ObjectSuffix): src/zlib/crc32.c 
	$(CC) $(SourceSwitch) "./src/zlib/crc32.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_crc32.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_crc32.c$(PreprocessSuffix): src/zlib/crc32.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_crc32.c$(PreprocessSuffix) "src/zlib/crc32.c"

$(IntermediateDirectory)/zlib_gzclose.c$(ObjectSuffix): src/zlib/gzclose.c 
	$(CC) $(SourceSwitch) "./src/zlib/gzclose.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_gzclose.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_gzclose.c$(PreprocessSuffix): src/zlib/gzclose.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_gzclose.c$(PreprocessSuffix) "src/zlib/gzclose.c"

$(IntermediateDirectory)/zlib_zutil.c$(ObjectSuffix): src/zlib/zutil.c 
	$(CC) $(SourceSwitch) "./src/zlib/zutil.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_zutil.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_zutil.c$(PreprocessSuffix): src/zlib/zutil.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_zutil.c$(PreprocessSuffix) "src/zlib/zutil.c"

$(IntermediateDirectory)/zlib_inflate.c$(ObjectSuffix): src/zlib/inflate.c 
	$(CC) $(SourceSwitch) "./src/zlib/inflate.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_inflate.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_inflate.c$(PreprocessSuffix): src/zlib/inflate.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_inflate.c$(PreprocessSuffix) "src/zlib/inflate.c"

$(IntermediateDirectory)/zlib_gzlib.c$(ObjectSuffix): src/zlib/gzlib.c 
	$(CC) $(SourceSwitch) "./src/zlib/gzlib.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_gzlib.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_gzlib.c$(PreprocessSuffix): src/zlib/gzlib.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_gzlib.c$(PreprocessSuffix) "src/zlib/gzlib.c"

$(IntermediateDirectory)/zlib_gzread.c$(ObjectSuffix): src/zlib/gzread.c 
	$(CC) $(SourceSwitch) "./src/zlib/gzread.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_gzread.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_gzread.c$(PreprocessSuffix): src/zlib/gzread.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_gzread.c$(PreprocessSuffix) "src/zlib/gzread.c"

$(IntermediateDirectory)/zlib_adler32.c$(ObjectSuffix): src/zlib/adler32.c 
	$(CC) $(SourceSwitch) "./src/zlib/adler32.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_adler32.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_adler32.c$(PreprocessSuffix): src/zlib/adler32.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_adler32.c$(PreprocessSuffix) "src/zlib/adler32.c"

$(IntermediateDirectory)/zlib_compress.c$(ObjectSuffix): src/zlib/compress.c 
	$(CC) $(SourceSwitch) "./src/zlib/compress.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_compress.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_compress.c$(PreprocessSuffix): src/zlib/compress.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_compress.c$(PreprocessSuffix) "src/zlib/compress.c"

$(IntermediateDirectory)/zlib_inffast.c$(ObjectSuffix): src/zlib/inffast.c 
	$(CC) $(SourceSwitch) "./src/zlib/inffast.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zlib_inffast.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zlib_inffast.c$(PreprocessSuffix): src/zlib/inffast.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zlib_inffast.c$(PreprocessSuffix) "src/zlib/inffast.c"

$(IntermediateDirectory)/pcolorer2_FarEditor.cpp$(ObjectSuffix): src/pcolorer2/FarEditor.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/FarEditor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_FarEditor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_FarEditor.cpp$(PreprocessSuffix): src/pcolorer2/FarEditor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_FarEditor.cpp$(PreprocessSuffix) "src/pcolorer2/FarEditor.cpp"

$(IntermediateDirectory)/pcolorer2_ChooseTypeMenu.cpp$(ObjectSuffix): src/pcolorer2/ChooseTypeMenu.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/ChooseTypeMenu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_ChooseTypeMenu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_ChooseTypeMenu.cpp$(PreprocessSuffix): src/pcolorer2/ChooseTypeMenu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_ChooseTypeMenu.cpp$(PreprocessSuffix) "src/pcolorer2/ChooseTypeMenu.cpp"

$(IntermediateDirectory)/pcolorer2_tools.cpp$(ObjectSuffix): src/pcolorer2/tools.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/tools.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_tools.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_tools.cpp$(PreprocessSuffix): src/pcolorer2/tools.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_tools.cpp$(PreprocessSuffix) "src/pcolorer2/tools.cpp"

$(IntermediateDirectory)/pcolorer2_registry_wide.cpp$(ObjectSuffix): src/pcolorer2/registry_wide.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/registry_wide.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_registry_wide.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_registry_wide.cpp$(PreprocessSuffix): src/pcolorer2/registry_wide.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_registry_wide.cpp$(PreprocessSuffix) "src/pcolorer2/registry_wide.cpp"

$(IntermediateDirectory)/pcolorer2_FarHrcSettings.cpp$(ObjectSuffix): src/pcolorer2/FarHrcSettings.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/FarHrcSettings.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_FarHrcSettings.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_FarHrcSettings.cpp$(PreprocessSuffix): src/pcolorer2/FarHrcSettings.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_FarHrcSettings.cpp$(PreprocessSuffix) "src/pcolorer2/FarHrcSettings.cpp"

$(IntermediateDirectory)/pcolorer2_FarEditorSet.cpp$(ObjectSuffix): src/pcolorer2/FarEditorSet.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/FarEditorSet.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_FarEditorSet.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_FarEditorSet.cpp$(PreprocessSuffix): src/pcolorer2/FarEditorSet.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_FarEditorSet.cpp$(PreprocessSuffix) "src/pcolorer2/FarEditorSet.cpp"

$(IntermediateDirectory)/pcolorer2_pcolorer.cpp$(ObjectSuffix): src/pcolorer2/pcolorer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/pcolorer2/pcolorer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pcolorer2_pcolorer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pcolorer2_pcolorer.cpp$(PreprocessSuffix): src/pcolorer2/pcolorer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pcolorer2_pcolorer.cpp$(PreprocessSuffix) "src/pcolorer2/pcolorer.cpp"

$(IntermediateDirectory)/common_Exception.cpp$(ObjectSuffix): src/shared/common/Exception.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/Exception.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/common_Exception.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/common_Exception.cpp$(PreprocessSuffix): src/shared/common/Exception.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/common_Exception.cpp$(PreprocessSuffix) "src/shared/common/Exception.cpp"

$(IntermediateDirectory)/common_Logging.cpp$(ObjectSuffix): src/shared/common/Logging.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/Logging.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/common_Logging.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/common_Logging.cpp$(PreprocessSuffix): src/shared/common/Logging.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/common_Logging.cpp$(PreprocessSuffix) "src/shared/common/Logging.cpp"

$(IntermediateDirectory)/common_MemoryChunks.cpp$(ObjectSuffix): src/shared/common/MemoryChunks.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/MemoryChunks.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/common_MemoryChunks.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/common_MemoryChunks.cpp$(PreprocessSuffix): src/shared/common/MemoryChunks.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/common_MemoryChunks.cpp$(PreprocessSuffix) "src/shared/common/MemoryChunks.cpp"

$(IntermediateDirectory)/colorer_ParserFactory.cpp$(ObjectSuffix): src/shared/colorer/ParserFactory.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/ParserFactory.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/colorer_ParserFactory.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/colorer_ParserFactory.cpp$(PreprocessSuffix): src/shared/colorer/ParserFactory.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/colorer_ParserFactory.cpp$(PreprocessSuffix) "src/shared/colorer/ParserFactory.cpp"

$(IntermediateDirectory)/misc_malloc.c$(ObjectSuffix): src/shared/misc/malloc.c 
	$(CC) $(SourceSwitch) "./src/shared/misc/malloc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/misc_malloc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/misc_malloc.c$(PreprocessSuffix): src/shared/misc/malloc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/misc_malloc.c$(PreprocessSuffix) "src/shared/misc/malloc.c"

$(IntermediateDirectory)/unicode_BitArray.cpp$(ObjectSuffix): src/shared/unicode/BitArray.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/BitArray.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_BitArray.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_BitArray.cpp$(PreprocessSuffix): src/shared/unicode/BitArray.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_BitArray.cpp$(PreprocessSuffix) "src/shared/unicode/BitArray.cpp"

$(IntermediateDirectory)/unicode_CharacterClass.cpp$(ObjectSuffix): src/shared/unicode/CharacterClass.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/CharacterClass.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_CharacterClass.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_CharacterClass.cpp$(PreprocessSuffix): src/shared/unicode/CharacterClass.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_CharacterClass.cpp$(PreprocessSuffix) "src/shared/unicode/CharacterClass.cpp"

$(IntermediateDirectory)/unicode_DString.cpp$(ObjectSuffix): src/shared/unicode/DString.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/DString.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_DString.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_DString.cpp$(PreprocessSuffix): src/shared/unicode/DString.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_DString.cpp$(PreprocessSuffix) "src/shared/unicode/DString.cpp"

$(IntermediateDirectory)/unicode_Encodings.cpp$(ObjectSuffix): src/shared/unicode/Encodings.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/Encodings.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_Encodings.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_Encodings.cpp$(PreprocessSuffix): src/shared/unicode/Encodings.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_Encodings.cpp$(PreprocessSuffix) "src/shared/unicode/Encodings.cpp"

$(IntermediateDirectory)/unicode_String.cpp$(ObjectSuffix): src/shared/unicode/String.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/String.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_String.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_String.cpp$(PreprocessSuffix): src/shared/unicode/String.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_String.cpp$(PreprocessSuffix) "src/shared/unicode/String.cpp"

$(IntermediateDirectory)/unicode_SString.cpp$(ObjectSuffix): src/shared/unicode/SString.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/SString.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_SString.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_SString.cpp$(PreprocessSuffix): src/shared/unicode/SString.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_SString.cpp$(PreprocessSuffix) "src/shared/unicode/SString.cpp"

$(IntermediateDirectory)/unicode_UnicodeTools.cpp$(ObjectSuffix): src/shared/unicode/UnicodeTools.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/UnicodeTools.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_UnicodeTools.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_UnicodeTools.cpp$(PreprocessSuffix): src/shared/unicode/UnicodeTools.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_UnicodeTools.cpp$(PreprocessSuffix) "src/shared/unicode/UnicodeTools.cpp"

$(IntermediateDirectory)/unicode_Character.cpp$(ObjectSuffix): src/shared/unicode/Character.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/Character.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_Character.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_Character.cpp$(PreprocessSuffix): src/shared/unicode/Character.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_Character.cpp$(PreprocessSuffix) "src/shared/unicode/Character.cpp"

$(IntermediateDirectory)/unicode_StringBuffer.cpp$(ObjectSuffix): src/shared/unicode/StringBuffer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/unicode/StringBuffer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unicode_StringBuffer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unicode_StringBuffer.cpp$(PreprocessSuffix): src/shared/unicode/StringBuffer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unicode_StringBuffer.cpp$(PreprocessSuffix) "src/shared/unicode/StringBuffer.cpp"

$(IntermediateDirectory)/cregexp_cregexp.cpp$(ObjectSuffix): src/shared/cregexp/cregexp.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/cregexp/cregexp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cregexp_cregexp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cregexp_cregexp.cpp$(PreprocessSuffix): src/shared/cregexp/cregexp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cregexp_cregexp.cpp$(PreprocessSuffix) "src/shared/cregexp/cregexp.cpp"

$(IntermediateDirectory)/xml_xmldom.cpp$(ObjectSuffix): src/shared/xml/xmldom.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/xml/xmldom.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/xml_xmldom.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/xml_xmldom.cpp$(PreprocessSuffix): src/shared/xml/xmldom.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/xml_xmldom.cpp$(PreprocessSuffix) "src/shared/xml/xmldom.cpp"

$(IntermediateDirectory)/minizip_mztools.c$(ObjectSuffix): src/zlib/contrib/minizip/mztools.c 
	$(CC) $(SourceSwitch) "./src/zlib/contrib/minizip/mztools.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/minizip_mztools.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/minizip_mztools.c$(PreprocessSuffix): src/zlib/contrib/minizip/mztools.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/minizip_mztools.c$(PreprocessSuffix) "src/zlib/contrib/minizip/mztools.c"

$(IntermediateDirectory)/minizip_zip.c$(ObjectSuffix): src/zlib/contrib/minizip/zip.c 
	$(CC) $(SourceSwitch) "./src/zlib/contrib/minizip/zip.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/minizip_zip.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/minizip_zip.c$(PreprocessSuffix): src/zlib/contrib/minizip/zip.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/minizip_zip.c$(PreprocessSuffix) "src/zlib/contrib/minizip/zip.c"

$(IntermediateDirectory)/minizip_ioapi.c$(ObjectSuffix): src/zlib/contrib/minizip/ioapi.c 
	$(CC) $(SourceSwitch) "./src/zlib/contrib/minizip/ioapi.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/minizip_ioapi.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/minizip_ioapi.c$(PreprocessSuffix): src/zlib/contrib/minizip/ioapi.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/minizip_ioapi.c$(PreprocessSuffix) "src/zlib/contrib/minizip/ioapi.c"

$(IntermediateDirectory)/minizip_unzip.c$(ObjectSuffix): src/zlib/contrib/minizip/unzip.c 
	$(CC) $(SourceSwitch) "./src/zlib/contrib/minizip/unzip.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/minizip_unzip.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/minizip_unzip.c$(PreprocessSuffix): src/zlib/contrib/minizip/unzip.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/minizip_unzip.c$(PreprocessSuffix) "src/zlib/contrib/minizip/unzip.c"

$(IntermediateDirectory)/io_Writer.cpp$(ObjectSuffix): src/shared/common/io/Writer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/Writer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_Writer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_Writer.cpp$(PreprocessSuffix): src/shared/common/io/Writer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_Writer.cpp$(PreprocessSuffix) "src/shared/common/io/Writer.cpp"

$(IntermediateDirectory)/io_FileInputSource.cpp$(ObjectSuffix): src/shared/common/io/FileInputSource.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/FileInputSource.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_FileInputSource.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_FileInputSource.cpp$(PreprocessSuffix): src/shared/common/io/FileInputSource.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_FileInputSource.cpp$(PreprocessSuffix) "src/shared/common/io/FileInputSource.cpp"

$(IntermediateDirectory)/io_HTTPInputSource.cpp$(ObjectSuffix): src/shared/common/io/HTTPInputSource.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/HTTPInputSource.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_HTTPInputSource.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_HTTPInputSource.cpp$(PreprocessSuffix): src/shared/common/io/HTTPInputSource.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_HTTPInputSource.cpp$(PreprocessSuffix) "src/shared/common/io/HTTPInputSource.cpp"

$(IntermediateDirectory)/io_SharedInputSource.cpp$(ObjectSuffix): src/shared/common/io/SharedInputSource.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/SharedInputSource.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_SharedInputSource.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_SharedInputSource.cpp$(PreprocessSuffix): src/shared/common/io/SharedInputSource.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_SharedInputSource.cpp$(PreprocessSuffix) "src/shared/common/io/SharedInputSource.cpp"

$(IntermediateDirectory)/io_FileWriter.cpp$(ObjectSuffix): src/shared/common/io/FileWriter.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/FileWriter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_FileWriter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_FileWriter.cpp$(PreprocessSuffix): src/shared/common/io/FileWriter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_FileWriter.cpp$(PreprocessSuffix) "src/shared/common/io/FileWriter.cpp"

$(IntermediateDirectory)/io_StreamWriter.cpp$(ObjectSuffix): src/shared/common/io/StreamWriter.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/StreamWriter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_StreamWriter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_StreamWriter.cpp$(PreprocessSuffix): src/shared/common/io/StreamWriter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_StreamWriter.cpp$(PreprocessSuffix) "src/shared/common/io/StreamWriter.cpp"

$(IntermediateDirectory)/io_InputSource.cpp$(ObjectSuffix): src/shared/common/io/InputSource.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/InputSource.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_InputSource.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_InputSource.cpp$(PreprocessSuffix): src/shared/common/io/InputSource.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_InputSource.cpp$(PreprocessSuffix) "src/shared/common/io/InputSource.cpp"

$(IntermediateDirectory)/io_JARInputSource.cpp$(ObjectSuffix): src/shared/common/io/JARInputSource.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/common/io/JARInputSource.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/io_JARInputSource.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/io_JARInputSource.cpp$(PreprocessSuffix): src/shared/common/io/JARInputSource.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/io_JARInputSource.cpp$(PreprocessSuffix) "src/shared/common/io/JARInputSource.cpp"

$(IntermediateDirectory)/parsers_HRCParserImpl.cpp$(ObjectSuffix): src/shared/colorer/parsers/HRCParserImpl.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/parsers/HRCParserImpl.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/parsers_HRCParserImpl.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/parsers_HRCParserImpl.cpp$(PreprocessSuffix): src/shared/colorer/parsers/HRCParserImpl.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/parsers_HRCParserImpl.cpp$(PreprocessSuffix) "src/shared/colorer/parsers/HRCParserImpl.cpp"

$(IntermediateDirectory)/parsers_TextParserImpl.cpp$(ObjectSuffix): src/shared/colorer/parsers/TextParserImpl.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/parsers/TextParserImpl.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/parsers_TextParserImpl.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/parsers_TextParserImpl.cpp$(PreprocessSuffix): src/shared/colorer/parsers/TextParserImpl.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/parsers_TextParserImpl.cpp$(PreprocessSuffix) "src/shared/colorer/parsers/TextParserImpl.cpp"

$(IntermediateDirectory)/viewer_TextLinesStore.cpp$(ObjectSuffix): src/shared/colorer/viewer/TextLinesStore.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/viewer/TextLinesStore.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/viewer_TextLinesStore.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/viewer_TextLinesStore.cpp$(PreprocessSuffix): src/shared/colorer/viewer/TextLinesStore.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/viewer_TextLinesStore.cpp$(PreprocessSuffix) "src/shared/colorer/viewer/TextLinesStore.cpp"

$(IntermediateDirectory)/viewer_TextConsoleViewer.cpp$(ObjectSuffix): src/shared/colorer/viewer/TextConsoleViewer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/viewer/TextConsoleViewer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/viewer_TextConsoleViewer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/viewer_TextConsoleViewer.cpp$(PreprocessSuffix): src/shared/colorer/viewer/TextConsoleViewer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/viewer_TextConsoleViewer.cpp$(PreprocessSuffix) "src/shared/colorer/viewer/TextConsoleViewer.cpp"

$(IntermediateDirectory)/viewer_ConsoleTools.cpp$(ObjectSuffix): src/shared/colorer/viewer/ConsoleTools.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/viewer/ConsoleTools.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/viewer_ConsoleTools.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/viewer_ConsoleTools.cpp$(PreprocessSuffix): src/shared/colorer/viewer/ConsoleTools.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/viewer_ConsoleTools.cpp$(PreprocessSuffix) "src/shared/colorer/viewer/ConsoleTools.cpp"

$(IntermediateDirectory)/editor_Outliner.cpp$(ObjectSuffix): src/shared/colorer/editor/Outliner.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/editor/Outliner.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/editor_Outliner.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/editor_Outliner.cpp$(PreprocessSuffix): src/shared/colorer/editor/Outliner.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/editor_Outliner.cpp$(PreprocessSuffix) "src/shared/colorer/editor/Outliner.cpp"

$(IntermediateDirectory)/editor_BaseEditor.cpp$(ObjectSuffix): src/shared/colorer/editor/BaseEditor.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/editor/BaseEditor.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/editor_BaseEditor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/editor_BaseEditor.cpp$(PreprocessSuffix): src/shared/colorer/editor/BaseEditor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/editor_BaseEditor.cpp$(PreprocessSuffix) "src/shared/colorer/editor/BaseEditor.cpp"

$(IntermediateDirectory)/handlers_FileErrorHandler.cpp$(ObjectSuffix): src/shared/colorer/handlers/FileErrorHandler.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/FileErrorHandler.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_FileErrorHandler.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_FileErrorHandler.cpp$(PreprocessSuffix): src/shared/colorer/handlers/FileErrorHandler.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_FileErrorHandler.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/FileErrorHandler.cpp"

$(IntermediateDirectory)/handlers_TextHRDMapper.cpp$(ObjectSuffix): src/shared/colorer/handlers/TextHRDMapper.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/TextHRDMapper.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_TextHRDMapper.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_TextHRDMapper.cpp$(PreprocessSuffix): src/shared/colorer/handlers/TextHRDMapper.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_TextHRDMapper.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/TextHRDMapper.cpp"

$(IntermediateDirectory)/handlers_LineRegionsSupport.cpp$(ObjectSuffix): src/shared/colorer/handlers/LineRegionsSupport.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/LineRegionsSupport.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_LineRegionsSupport.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_LineRegionsSupport.cpp$(PreprocessSuffix): src/shared/colorer/handlers/LineRegionsSupport.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_LineRegionsSupport.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/LineRegionsSupport.cpp"

$(IntermediateDirectory)/handlers_StyledHRDMapper.cpp$(ObjectSuffix): src/shared/colorer/handlers/StyledHRDMapper.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/StyledHRDMapper.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_StyledHRDMapper.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_StyledHRDMapper.cpp$(PreprocessSuffix): src/shared/colorer/handlers/StyledHRDMapper.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_StyledHRDMapper.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/StyledHRDMapper.cpp"

$(IntermediateDirectory)/handlers_LineRegionsCompactSupport.cpp$(ObjectSuffix): src/shared/colorer/handlers/LineRegionsCompactSupport.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/LineRegionsCompactSupport.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_LineRegionsCompactSupport.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_LineRegionsCompactSupport.cpp$(PreprocessSuffix): src/shared/colorer/handlers/LineRegionsCompactSupport.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_LineRegionsCompactSupport.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/LineRegionsCompactSupport.cpp"

$(IntermediateDirectory)/handlers_RegionMapperImpl.cpp$(ObjectSuffix): src/shared/colorer/handlers/RegionMapperImpl.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/RegionMapperImpl.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_RegionMapperImpl.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_RegionMapperImpl.cpp$(PreprocessSuffix): src/shared/colorer/handlers/RegionMapperImpl.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_RegionMapperImpl.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/RegionMapperImpl.cpp"

$(IntermediateDirectory)/handlers_ErrorHandlerWriter.cpp$(ObjectSuffix): src/shared/colorer/handlers/ErrorHandlerWriter.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/handlers/ErrorHandlerWriter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/handlers_ErrorHandlerWriter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/handlers_ErrorHandlerWriter.cpp$(PreprocessSuffix): src/shared/colorer/handlers/ErrorHandlerWriter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/handlers_ErrorHandlerWriter.cpp$(PreprocessSuffix) "src/shared/colorer/handlers/ErrorHandlerWriter.cpp"

$(IntermediateDirectory)/helpers_HRCParserHelpers.cpp$(ObjectSuffix): src/shared/colorer/parsers/helpers/HRCParserHelpers.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/parsers/helpers/HRCParserHelpers.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/helpers_HRCParserHelpers.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/helpers_HRCParserHelpers.cpp$(PreprocessSuffix): src/shared/colorer/parsers/helpers/HRCParserHelpers.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/helpers_HRCParserHelpers.cpp$(PreprocessSuffix) "src/shared/colorer/parsers/helpers/HRCParserHelpers.cpp"

$(IntermediateDirectory)/helpers_TextParserHelpers.cpp$(ObjectSuffix): src/shared/colorer/parsers/helpers/TextParserHelpers.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/shared/colorer/parsers/helpers/TextParserHelpers.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/helpers_TextParserHelpers.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/helpers_TextParserHelpers.cpp$(PreprocessSuffix): src/shared/colorer/parsers/helpers/TextParserHelpers.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/helpers_TextParserHelpers.cpp$(PreprocessSuffix) "src/shared/colorer/parsers/helpers/TextParserHelpers.cpp"

##
## Clean
##
clean:
	$(RM) -r ./$(ConfigurationName)/


