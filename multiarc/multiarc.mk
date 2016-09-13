##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=multiarc
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
OutputFile             :=../Build/Plugins/multiarc/bin/$(ProjectName).far-plug-mb
Preprocessors          :=$(PreprocessorSwitch)RARDLL $(PreprocessorSwitch)_7ZIP_ST 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="multiarc.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)src $(IncludeSwitch)src/libpcre $(IncludeSwitch)../far2l/ $(IncludeSwitch)../far2l/Include $(IncludeSwitch)../WinPort $(IncludeSwitch)src/formats/rar/unrar $(IncludeSwitch)../utils/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)utils 
ArLibs                 :=  "utils" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)../utils/$(ConfigurationName) 

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
Objects0=$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_global.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix) $(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix) $(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix) $(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix) $(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix) $(IntermediateDirectory)/rar_rar.cpp$(ObjectSuffix) $(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix) $(IntermediateDirectory)/custom_custom.cpp$(ObjectSuffix) $(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix) $(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix) $(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix) $(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix) $(IntermediateDirectory)/cab_cab.cpp$(ObjectSuffix) $(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix) $(IntermediateDirectory)/7z_7z.cpp$(ObjectSuffix) $(IntermediateDirectory)/7z_7zMain.c$(ObjectSuffix) $(IntermediateDirectory)/unrar_scantree.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_sha1.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_threadpool.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/unrar_crc.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_hash.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_strlist.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rs16.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_resource.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_consio.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_secpassword.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_qopen.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_recvol.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rijndael.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/unrar_timefn.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_encname.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rawread.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_file.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_strfn.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_sha256.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_filefn.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_filcreat.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_headers.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_blake2s.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/unrar_options.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_volume.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_isnt.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_system.cpp$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/unrar_crypt.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rar.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_cmddata.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_extinfo.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_filestr.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_ui.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/unrar_list.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_find.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_smallfn.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_unicode.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_pathfn.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_global.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rarvm.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_getbits.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rs.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_errhnd.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/unrar_archive.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_dll.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_extract.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_match.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_unpack.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_arcread.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rdwrfn.cpp$(ObjectSuffix) $(IntermediateDirectory)/unrar_rarpch.cpp$(ObjectSuffix) $(IntermediateDirectory)/C_Lzma86Dec.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zDec.c$(ObjectSuffix) \
	$(IntermediateDirectory)/C_Ppmd7Dec.c$(ObjectSuffix) $(IntermediateDirectory)/C_Sha1.c$(ObjectSuffix) $(IntermediateDirectory)/C_BraIA64.c$(ObjectSuffix) $(IntermediateDirectory)/C_Xz.c$(ObjectSuffix) $(IntermediateDirectory)/C_Lzma2Dec.c$(ObjectSuffix) $(IntermediateDirectory)/C_AesOpt.c$(ObjectSuffix) $(IntermediateDirectory)/C_BwtSort.c$(ObjectSuffix) $(IntermediateDirectory)/C_Bra86.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zCrcOpt.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zAlloc.c$(ObjectSuffix) \
	$(IntermediateDirectory)/C_LzmaLib.c$(ObjectSuffix) $(IntermediateDirectory)/C_HuffEnc.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zBuf.c$(ObjectSuffix) $(IntermediateDirectory)/C_XzDec.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zBuf2.c$(ObjectSuffix) $(IntermediateDirectory)/C_Lzma86Enc.c$(ObjectSuffix) $(IntermediateDirectory)/C_Alloc.c$(ObjectSuffix) $(IntermediateDirectory)/C_XzCrc64.c$(ObjectSuffix) $(IntermediateDirectory)/C_XzEnc.c$(ObjectSuffix) $(IntermediateDirectory)/C_Ppmd8.c$(ObjectSuffix) \
	$(IntermediateDirectory)/C_Aes.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zCrc.c$(ObjectSuffix) $(IntermediateDirectory)/C_Bcj2.c$(ObjectSuffix) $(IntermediateDirectory)/C_Delta.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zFile.c$(ObjectSuffix) $(IntermediateDirectory)/C_Bcj2Enc.c$(ObjectSuffix) $(IntermediateDirectory)/C_Ppmd8Enc.c$(ObjectSuffix) 

Objects2=$(IntermediateDirectory)/C_LzmaEnc.c$(ObjectSuffix) $(IntermediateDirectory)/C_Ppmd7Enc.c$(ObjectSuffix) $(IntermediateDirectory)/C_7zStream.c$(ObjectSuffix) \
	$(IntermediateDirectory)/C_Ppmd8Dec.c$(ObjectSuffix) $(IntermediateDirectory)/C_XzCrc64Opt.c$(ObjectSuffix) $(IntermediateDirectory)/C_LzmaDec.c$(ObjectSuffix) $(IntermediateDirectory)/C_XzIn.c$(ObjectSuffix) $(IntermediateDirectory)/C_CpuArch.c$(ObjectSuffix) $(IntermediateDirectory)/C_LzFind.c$(ObjectSuffix) $(IntermediateDirectory)/C_Ppmd7.c$(ObjectSuffix) $(IntermediateDirectory)/C_Lzma2Enc.c$(ObjectSuffix) $(IntermediateDirectory)/C_Sha256.c$(ObjectSuffix) $(IntermediateDirectory)/C_Sort.c$(ObjectSuffix) \
	$(IntermediateDirectory)/C_7zArcIn.c$(ObjectSuffix) $(IntermediateDirectory)/C_Bra.c$(ObjectSuffix) $(IntermediateDirectory)/C_Blake2s.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) $(Objects2) 

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
	$(SharedObjectLinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)
	@$(MakeDirCommand) "../.build-debug"
	@echo rebuilt > "../.build-debug/multiarc"

MakeIntermediateDirs:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)

PreBuild:
	@echo Executing Pre Build commands ...
	mkdir -p ../Build/Plugins/multiarc/
	cp -R ./configs/* ../Build/Plugins/multiarc/
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix): src/MultiArc.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/MultiArc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_MultiArc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_MultiArc.cpp$(PreprocessSuffix): src/MultiArc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_MultiArc.cpp$(PreprocessSuffix) "src/MultiArc.cpp"

$(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix): src/ArcPlg.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ArcPlg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ArcPlg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ArcPlg.cpp$(PreprocessSuffix): src/ArcPlg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ArcPlg.cpp$(PreprocessSuffix) "src/ArcPlg.cpp"

$(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix): src/arccfg.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arccfg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arccfg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arccfg.cpp$(PreprocessSuffix): src/arccfg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arccfg.cpp$(PreprocessSuffix) "src/arccfg.cpp"

$(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix): src/arcget.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcget.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcget.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcget.cpp$(PreprocessSuffix): src/arcget.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcget.cpp$(PreprocessSuffix) "src/arcget.cpp"

$(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix): src/arcput.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcput.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcput.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcput.cpp$(PreprocessSuffix): src/arcput.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcput.cpp$(PreprocessSuffix) "src/arcput.cpp"

$(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix): src/arcreg.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcreg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcreg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcreg.cpp$(PreprocessSuffix): src/arcreg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcreg.cpp$(PreprocessSuffix) "src/arcreg.cpp"

$(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix): src/ArcMix.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ArcMix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ArcMix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ArcMix.cpp$(PreprocessSuffix): src/ArcMix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ArcMix.cpp$(PreprocessSuffix) "src/ArcMix.cpp"

$(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix): src/ArcProc.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ArcProc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ArcProc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ArcProc.cpp$(PreprocessSuffix): src/ArcProc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ArcProc.cpp$(PreprocessSuffix) "src/ArcProc.cpp"

$(IntermediateDirectory)/src_global.cpp$(ObjectSuffix): src/global.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/global.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_global.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_global.cpp$(PreprocessSuffix): src/global.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_global.cpp$(PreprocessSuffix) "src/global.cpp"

$(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix): src/arcread.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arcread.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arcread.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arcread.cpp$(PreprocessSuffix): src/arcread.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arcread.cpp$(PreprocessSuffix) "src/arcread.cpp"

$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix): src/arccmd.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/arccmd.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_arccmd.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_arccmd.cpp$(PreprocessSuffix): src/arccmd.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_arccmd.cpp$(PreprocessSuffix) "src/arccmd.cpp"

$(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix): src/libpcre/get.c 
	$(CC) $(SourceSwitch) "./src/libpcre/get.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_get.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_get.c$(PreprocessSuffix): src/libpcre/get.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_get.c$(PreprocessSuffix) "src/libpcre/get.c"

$(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix): src/libpcre/pcre.c 
	$(CC) $(SourceSwitch) "./src/libpcre/pcre.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_pcre.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_pcre.c$(PreprocessSuffix): src/libpcre/pcre.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_pcre.c$(PreprocessSuffix) "src/libpcre/pcre.c"

$(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix): src/libpcre/chartables.c 
	$(CC) $(SourceSwitch) "./src/libpcre/chartables.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_chartables.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_chartables.c$(PreprocessSuffix): src/libpcre/chartables.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_chartables.c$(PreprocessSuffix) "src/libpcre/chartables.c"

$(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix): src/libpcre/study.c 
	$(CC) $(SourceSwitch) "./src/libpcre/study.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libpcre_study.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/libpcre_study.c$(PreprocessSuffix): src/libpcre/study.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libpcre_study.c$(PreprocessSuffix) "src/libpcre/study.c"

$(IntermediateDirectory)/rar_rar.cpp$(ObjectSuffix): src/formats/rar/rar.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/rar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/rar_rar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/rar_rar.cpp$(PreprocessSuffix): src/formats/rar/rar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/rar_rar.cpp$(PreprocessSuffix) "src/formats/rar/rar.cpp"

$(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix): src/formats/ha/ha.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/ha/ha.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ha_ha.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ha_ha.cpp$(PreprocessSuffix): src/formats/ha/ha.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ha_ha.cpp$(PreprocessSuffix) "src/formats/ha/ha.cpp"

$(IntermediateDirectory)/custom_custom.cpp$(ObjectSuffix): src/formats/custom/custom.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/custom/custom.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/custom_custom.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/custom_custom.cpp$(PreprocessSuffix): src/formats/custom/custom.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/custom_custom.cpp$(PreprocessSuffix) "src/formats/custom/custom.cpp"

$(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix): src/formats/arj/arj.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/arj/arj.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/arj_arj.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/arj_arj.cpp$(PreprocessSuffix): src/formats/arj/arj.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/arj_arj.cpp$(PreprocessSuffix) "src/formats/arj/arj.cpp"

$(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix): src/formats/ace/ace.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/ace/ace.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ace_ace.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ace_ace.cpp$(PreprocessSuffix): src/formats/ace/ace.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ace_ace.cpp$(PreprocessSuffix) "src/formats/ace/ace.cpp"

$(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix): src/formats/arc/arc.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/arc/arc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/arc_arc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/arc_arc.cpp$(PreprocessSuffix): src/formats/arc/arc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/arc_arc.cpp$(PreprocessSuffix) "src/formats/arc/arc.cpp"

$(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix): src/formats/zip/zip.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/zip/zip.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/zip_zip.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/zip_zip.cpp$(PreprocessSuffix): src/formats/zip/zip.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/zip_zip.cpp$(PreprocessSuffix) "src/formats/zip/zip.cpp"

$(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix): src/formats/lzh/lzh.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/lzh/lzh.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/lzh_lzh.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/lzh_lzh.cpp$(PreprocessSuffix): src/formats/lzh/lzh.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/lzh_lzh.cpp$(PreprocessSuffix) "src/formats/lzh/lzh.cpp"

$(IntermediateDirectory)/cab_cab.cpp$(ObjectSuffix): src/formats/cab/cab.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/cab/cab.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/cab_cab.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/cab_cab.cpp$(PreprocessSuffix): src/formats/cab/cab.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/cab_cab.cpp$(PreprocessSuffix) "src/formats/cab/cab.cpp"

$(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix): src/formats/targz/targz.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/targz/targz.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/targz_targz.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/targz_targz.cpp$(PreprocessSuffix): src/formats/targz/targz.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/targz_targz.cpp$(PreprocessSuffix) "src/formats/targz/targz.cpp"

$(IntermediateDirectory)/7z_7z.cpp$(ObjectSuffix): src/formats/7z/7z.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/7z/7z.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/7z_7z.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/7z_7z.cpp$(PreprocessSuffix): src/formats/7z/7z.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/7z_7z.cpp$(PreprocessSuffix) "src/formats/7z/7z.cpp"

$(IntermediateDirectory)/7z_7zMain.c$(ObjectSuffix): src/formats/7z/7zMain.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/7zMain.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/7z_7zMain.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/7z_7zMain.c$(PreprocessSuffix): src/formats/7z/7zMain.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/7z_7zMain.c$(PreprocessSuffix) "src/formats/7z/7zMain.c"

$(IntermediateDirectory)/unrar_scantree.cpp$(ObjectSuffix): src/formats/rar/unrar/scantree.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/scantree.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_scantree.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_scantree.cpp$(PreprocessSuffix): src/formats/rar/unrar/scantree.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_scantree.cpp$(PreprocessSuffix) "src/formats/rar/unrar/scantree.cpp"

$(IntermediateDirectory)/unrar_sha1.cpp$(ObjectSuffix): src/formats/rar/unrar/sha1.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/sha1.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_sha1.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_sha1.cpp$(PreprocessSuffix): src/formats/rar/unrar/sha1.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_sha1.cpp$(PreprocessSuffix) "src/formats/rar/unrar/sha1.cpp"

$(IntermediateDirectory)/unrar_threadpool.cpp$(ObjectSuffix): src/formats/rar/unrar/threadpool.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/threadpool.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_threadpool.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_threadpool.cpp$(PreprocessSuffix): src/formats/rar/unrar/threadpool.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_threadpool.cpp$(PreprocessSuffix) "src/formats/rar/unrar/threadpool.cpp"

$(IntermediateDirectory)/unrar_crc.cpp$(ObjectSuffix): src/formats/rar/unrar/crc.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/crc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_crc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_crc.cpp$(PreprocessSuffix): src/formats/rar/unrar/crc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_crc.cpp$(PreprocessSuffix) "src/formats/rar/unrar/crc.cpp"

$(IntermediateDirectory)/unrar_hash.cpp$(ObjectSuffix): src/formats/rar/unrar/hash.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/hash.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_hash.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_hash.cpp$(PreprocessSuffix): src/formats/rar/unrar/hash.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_hash.cpp$(PreprocessSuffix) "src/formats/rar/unrar/hash.cpp"

$(IntermediateDirectory)/unrar_strlist.cpp$(ObjectSuffix): src/formats/rar/unrar/strlist.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/strlist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_strlist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_strlist.cpp$(PreprocessSuffix): src/formats/rar/unrar/strlist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_strlist.cpp$(PreprocessSuffix) "src/formats/rar/unrar/strlist.cpp"

$(IntermediateDirectory)/unrar_rs16.cpp$(ObjectSuffix): src/formats/rar/unrar/rs16.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rs16.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rs16.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rs16.cpp$(PreprocessSuffix): src/formats/rar/unrar/rs16.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rs16.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rs16.cpp"

$(IntermediateDirectory)/unrar_resource.cpp$(ObjectSuffix): src/formats/rar/unrar/resource.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/resource.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_resource.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_resource.cpp$(PreprocessSuffix): src/formats/rar/unrar/resource.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_resource.cpp$(PreprocessSuffix) "src/formats/rar/unrar/resource.cpp"

$(IntermediateDirectory)/unrar_consio.cpp$(ObjectSuffix): src/formats/rar/unrar/consio.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/consio.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_consio.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_consio.cpp$(PreprocessSuffix): src/formats/rar/unrar/consio.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_consio.cpp$(PreprocessSuffix) "src/formats/rar/unrar/consio.cpp"

$(IntermediateDirectory)/unrar_secpassword.cpp$(ObjectSuffix): src/formats/rar/unrar/secpassword.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/secpassword.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_secpassword.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_secpassword.cpp$(PreprocessSuffix): src/formats/rar/unrar/secpassword.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_secpassword.cpp$(PreprocessSuffix) "src/formats/rar/unrar/secpassword.cpp"

$(IntermediateDirectory)/unrar_qopen.cpp$(ObjectSuffix): src/formats/rar/unrar/qopen.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/qopen.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_qopen.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_qopen.cpp$(PreprocessSuffix): src/formats/rar/unrar/qopen.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_qopen.cpp$(PreprocessSuffix) "src/formats/rar/unrar/qopen.cpp"

$(IntermediateDirectory)/unrar_recvol.cpp$(ObjectSuffix): src/formats/rar/unrar/recvol.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/recvol.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_recvol.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_recvol.cpp$(PreprocessSuffix): src/formats/rar/unrar/recvol.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_recvol.cpp$(PreprocessSuffix) "src/formats/rar/unrar/recvol.cpp"

$(IntermediateDirectory)/unrar_rijndael.cpp$(ObjectSuffix): src/formats/rar/unrar/rijndael.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rijndael.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rijndael.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rijndael.cpp$(PreprocessSuffix): src/formats/rar/unrar/rijndael.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rijndael.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rijndael.cpp"

$(IntermediateDirectory)/unrar_timefn.cpp$(ObjectSuffix): src/formats/rar/unrar/timefn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/timefn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_timefn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_timefn.cpp$(PreprocessSuffix): src/formats/rar/unrar/timefn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_timefn.cpp$(PreprocessSuffix) "src/formats/rar/unrar/timefn.cpp"

$(IntermediateDirectory)/unrar_encname.cpp$(ObjectSuffix): src/formats/rar/unrar/encname.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/encname.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_encname.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_encname.cpp$(PreprocessSuffix): src/formats/rar/unrar/encname.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_encname.cpp$(PreprocessSuffix) "src/formats/rar/unrar/encname.cpp"

$(IntermediateDirectory)/unrar_rawread.cpp$(ObjectSuffix): src/formats/rar/unrar/rawread.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rawread.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rawread.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rawread.cpp$(PreprocessSuffix): src/formats/rar/unrar/rawread.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rawread.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rawread.cpp"

$(IntermediateDirectory)/unrar_file.cpp$(ObjectSuffix): src/formats/rar/unrar/file.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/file.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_file.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_file.cpp$(PreprocessSuffix): src/formats/rar/unrar/file.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_file.cpp$(PreprocessSuffix) "src/formats/rar/unrar/file.cpp"

$(IntermediateDirectory)/unrar_strfn.cpp$(ObjectSuffix): src/formats/rar/unrar/strfn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/strfn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_strfn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_strfn.cpp$(PreprocessSuffix): src/formats/rar/unrar/strfn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_strfn.cpp$(PreprocessSuffix) "src/formats/rar/unrar/strfn.cpp"

$(IntermediateDirectory)/unrar_sha256.cpp$(ObjectSuffix): src/formats/rar/unrar/sha256.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/sha256.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_sha256.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_sha256.cpp$(PreprocessSuffix): src/formats/rar/unrar/sha256.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_sha256.cpp$(PreprocessSuffix) "src/formats/rar/unrar/sha256.cpp"

$(IntermediateDirectory)/unrar_filefn.cpp$(ObjectSuffix): src/formats/rar/unrar/filefn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/filefn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_filefn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_filefn.cpp$(PreprocessSuffix): src/formats/rar/unrar/filefn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_filefn.cpp$(PreprocessSuffix) "src/formats/rar/unrar/filefn.cpp"

$(IntermediateDirectory)/unrar_filcreat.cpp$(ObjectSuffix): src/formats/rar/unrar/filcreat.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/filcreat.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_filcreat.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_filcreat.cpp$(PreprocessSuffix): src/formats/rar/unrar/filcreat.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_filcreat.cpp$(PreprocessSuffix) "src/formats/rar/unrar/filcreat.cpp"

$(IntermediateDirectory)/unrar_headers.cpp$(ObjectSuffix): src/formats/rar/unrar/headers.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/headers.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_headers.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_headers.cpp$(PreprocessSuffix): src/formats/rar/unrar/headers.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_headers.cpp$(PreprocessSuffix) "src/formats/rar/unrar/headers.cpp"

$(IntermediateDirectory)/unrar_blake2s.cpp$(ObjectSuffix): src/formats/rar/unrar/blake2s.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/blake2s.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_blake2s.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_blake2s.cpp$(PreprocessSuffix): src/formats/rar/unrar/blake2s.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_blake2s.cpp$(PreprocessSuffix) "src/formats/rar/unrar/blake2s.cpp"

$(IntermediateDirectory)/unrar_options.cpp$(ObjectSuffix): src/formats/rar/unrar/options.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/options.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_options.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_options.cpp$(PreprocessSuffix): src/formats/rar/unrar/options.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_options.cpp$(PreprocessSuffix) "src/formats/rar/unrar/options.cpp"

$(IntermediateDirectory)/unrar_volume.cpp$(ObjectSuffix): src/formats/rar/unrar/volume.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/volume.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_volume.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_volume.cpp$(PreprocessSuffix): src/formats/rar/unrar/volume.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_volume.cpp$(PreprocessSuffix) "src/formats/rar/unrar/volume.cpp"

$(IntermediateDirectory)/unrar_isnt.cpp$(ObjectSuffix): src/formats/rar/unrar/isnt.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/isnt.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_isnt.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_isnt.cpp$(PreprocessSuffix): src/formats/rar/unrar/isnt.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_isnt.cpp$(PreprocessSuffix) "src/formats/rar/unrar/isnt.cpp"

$(IntermediateDirectory)/unrar_system.cpp$(ObjectSuffix): src/formats/rar/unrar/system.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/system.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_system.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_system.cpp$(PreprocessSuffix): src/formats/rar/unrar/system.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_system.cpp$(PreprocessSuffix) "src/formats/rar/unrar/system.cpp"

$(IntermediateDirectory)/unrar_crypt.cpp$(ObjectSuffix): src/formats/rar/unrar/crypt.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/crypt.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_crypt.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_crypt.cpp$(PreprocessSuffix): src/formats/rar/unrar/crypt.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_crypt.cpp$(PreprocessSuffix) "src/formats/rar/unrar/crypt.cpp"

$(IntermediateDirectory)/unrar_rar.cpp$(ObjectSuffix): src/formats/rar/unrar/rar.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rar.cpp$(PreprocessSuffix): src/formats/rar/unrar/rar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rar.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rar.cpp"

$(IntermediateDirectory)/unrar_cmddata.cpp$(ObjectSuffix): src/formats/rar/unrar/cmddata.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/cmddata.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_cmddata.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_cmddata.cpp$(PreprocessSuffix): src/formats/rar/unrar/cmddata.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_cmddata.cpp$(PreprocessSuffix) "src/formats/rar/unrar/cmddata.cpp"

$(IntermediateDirectory)/unrar_extinfo.cpp$(ObjectSuffix): src/formats/rar/unrar/extinfo.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/extinfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_extinfo.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_extinfo.cpp$(PreprocessSuffix): src/formats/rar/unrar/extinfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_extinfo.cpp$(PreprocessSuffix) "src/formats/rar/unrar/extinfo.cpp"

$(IntermediateDirectory)/unrar_filestr.cpp$(ObjectSuffix): src/formats/rar/unrar/filestr.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/filestr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_filestr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_filestr.cpp$(PreprocessSuffix): src/formats/rar/unrar/filestr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_filestr.cpp$(PreprocessSuffix) "src/formats/rar/unrar/filestr.cpp"

$(IntermediateDirectory)/unrar_ui.cpp$(ObjectSuffix): src/formats/rar/unrar/ui.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/ui.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_ui.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_ui.cpp$(PreprocessSuffix): src/formats/rar/unrar/ui.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_ui.cpp$(PreprocessSuffix) "src/formats/rar/unrar/ui.cpp"

$(IntermediateDirectory)/unrar_list.cpp$(ObjectSuffix): src/formats/rar/unrar/list.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/list.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_list.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_list.cpp$(PreprocessSuffix): src/formats/rar/unrar/list.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_list.cpp$(PreprocessSuffix) "src/formats/rar/unrar/list.cpp"

$(IntermediateDirectory)/unrar_find.cpp$(ObjectSuffix): src/formats/rar/unrar/find.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/find.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_find.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_find.cpp$(PreprocessSuffix): src/formats/rar/unrar/find.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_find.cpp$(PreprocessSuffix) "src/formats/rar/unrar/find.cpp"

$(IntermediateDirectory)/unrar_smallfn.cpp$(ObjectSuffix): src/formats/rar/unrar/smallfn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/smallfn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_smallfn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_smallfn.cpp$(PreprocessSuffix): src/formats/rar/unrar/smallfn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_smallfn.cpp$(PreprocessSuffix) "src/formats/rar/unrar/smallfn.cpp"

$(IntermediateDirectory)/unrar_unicode.cpp$(ObjectSuffix): src/formats/rar/unrar/unicode.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/unicode.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_unicode.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_unicode.cpp$(PreprocessSuffix): src/formats/rar/unrar/unicode.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_unicode.cpp$(PreprocessSuffix) "src/formats/rar/unrar/unicode.cpp"

$(IntermediateDirectory)/unrar_pathfn.cpp$(ObjectSuffix): src/formats/rar/unrar/pathfn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/pathfn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_pathfn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_pathfn.cpp$(PreprocessSuffix): src/formats/rar/unrar/pathfn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_pathfn.cpp$(PreprocessSuffix) "src/formats/rar/unrar/pathfn.cpp"

$(IntermediateDirectory)/unrar_global.cpp$(ObjectSuffix): src/formats/rar/unrar/global.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/global.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_global.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_global.cpp$(PreprocessSuffix): src/formats/rar/unrar/global.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_global.cpp$(PreprocessSuffix) "src/formats/rar/unrar/global.cpp"

$(IntermediateDirectory)/unrar_rarvm.cpp$(ObjectSuffix): src/formats/rar/unrar/rarvm.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rarvm.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rarvm.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rarvm.cpp$(PreprocessSuffix): src/formats/rar/unrar/rarvm.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rarvm.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rarvm.cpp"

$(IntermediateDirectory)/unrar_getbits.cpp$(ObjectSuffix): src/formats/rar/unrar/getbits.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/getbits.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_getbits.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_getbits.cpp$(PreprocessSuffix): src/formats/rar/unrar/getbits.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_getbits.cpp$(PreprocessSuffix) "src/formats/rar/unrar/getbits.cpp"

$(IntermediateDirectory)/unrar_rs.cpp$(ObjectSuffix): src/formats/rar/unrar/rs.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rs.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rs.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rs.cpp$(PreprocessSuffix): src/formats/rar/unrar/rs.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rs.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rs.cpp"

$(IntermediateDirectory)/unrar_errhnd.cpp$(ObjectSuffix): src/formats/rar/unrar/errhnd.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/errhnd.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_errhnd.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_errhnd.cpp$(PreprocessSuffix): src/formats/rar/unrar/errhnd.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_errhnd.cpp$(PreprocessSuffix) "src/formats/rar/unrar/errhnd.cpp"

$(IntermediateDirectory)/unrar_archive.cpp$(ObjectSuffix): src/formats/rar/unrar/archive.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/archive.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_archive.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_archive.cpp$(PreprocessSuffix): src/formats/rar/unrar/archive.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_archive.cpp$(PreprocessSuffix) "src/formats/rar/unrar/archive.cpp"

$(IntermediateDirectory)/unrar_dll.cpp$(ObjectSuffix): src/formats/rar/unrar/dll.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/dll.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_dll.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_dll.cpp$(PreprocessSuffix): src/formats/rar/unrar/dll.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_dll.cpp$(PreprocessSuffix) "src/formats/rar/unrar/dll.cpp"

$(IntermediateDirectory)/unrar_extract.cpp$(ObjectSuffix): src/formats/rar/unrar/extract.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/extract.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_extract.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_extract.cpp$(PreprocessSuffix): src/formats/rar/unrar/extract.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_extract.cpp$(PreprocessSuffix) "src/formats/rar/unrar/extract.cpp"

$(IntermediateDirectory)/unrar_match.cpp$(ObjectSuffix): src/formats/rar/unrar/match.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/match.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_match.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_match.cpp$(PreprocessSuffix): src/formats/rar/unrar/match.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_match.cpp$(PreprocessSuffix) "src/formats/rar/unrar/match.cpp"

$(IntermediateDirectory)/unrar_unpack.cpp$(ObjectSuffix): src/formats/rar/unrar/unpack.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/unpack.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_unpack.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_unpack.cpp$(PreprocessSuffix): src/formats/rar/unrar/unpack.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_unpack.cpp$(PreprocessSuffix) "src/formats/rar/unrar/unpack.cpp"

$(IntermediateDirectory)/unrar_arcread.cpp$(ObjectSuffix): src/formats/rar/unrar/arcread.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/arcread.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_arcread.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_arcread.cpp$(PreprocessSuffix): src/formats/rar/unrar/arcread.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_arcread.cpp$(PreprocessSuffix) "src/formats/rar/unrar/arcread.cpp"

$(IntermediateDirectory)/unrar_rdwrfn.cpp$(ObjectSuffix): src/formats/rar/unrar/rdwrfn.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rdwrfn.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rdwrfn.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rdwrfn.cpp$(PreprocessSuffix): src/formats/rar/unrar/rdwrfn.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rdwrfn.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rdwrfn.cpp"

$(IntermediateDirectory)/unrar_rarpch.cpp$(ObjectSuffix): src/formats/rar/unrar/rarpch.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/formats/rar/unrar/rarpch.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unrar_rarpch.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unrar_rarpch.cpp$(PreprocessSuffix): src/formats/rar/unrar/rarpch.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unrar_rarpch.cpp$(PreprocessSuffix) "src/formats/rar/unrar/rarpch.cpp"

$(IntermediateDirectory)/C_Lzma86Dec.c$(ObjectSuffix): src/formats/7z/C/Lzma86Dec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Lzma86Dec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Lzma86Dec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Lzma86Dec.c$(PreprocessSuffix): src/formats/7z/C/Lzma86Dec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Lzma86Dec.c$(PreprocessSuffix) "src/formats/7z/C/Lzma86Dec.c"

$(IntermediateDirectory)/C_7zDec.c$(ObjectSuffix): src/formats/7z/C/7zDec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zDec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zDec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zDec.c$(PreprocessSuffix): src/formats/7z/C/7zDec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zDec.c$(PreprocessSuffix) "src/formats/7z/C/7zDec.c"

$(IntermediateDirectory)/C_Ppmd7Dec.c$(ObjectSuffix): src/formats/7z/C/Ppmd7Dec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Ppmd7Dec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Ppmd7Dec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Ppmd7Dec.c$(PreprocessSuffix): src/formats/7z/C/Ppmd7Dec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Ppmd7Dec.c$(PreprocessSuffix) "src/formats/7z/C/Ppmd7Dec.c"

$(IntermediateDirectory)/C_Sha1.c$(ObjectSuffix): src/formats/7z/C/Sha1.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Sha1.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Sha1.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Sha1.c$(PreprocessSuffix): src/formats/7z/C/Sha1.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Sha1.c$(PreprocessSuffix) "src/formats/7z/C/Sha1.c"

$(IntermediateDirectory)/C_BraIA64.c$(ObjectSuffix): src/formats/7z/C/BraIA64.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/BraIA64.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_BraIA64.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_BraIA64.c$(PreprocessSuffix): src/formats/7z/C/BraIA64.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_BraIA64.c$(PreprocessSuffix) "src/formats/7z/C/BraIA64.c"

$(IntermediateDirectory)/C_Xz.c$(ObjectSuffix): src/formats/7z/C/Xz.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Xz.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Xz.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Xz.c$(PreprocessSuffix): src/formats/7z/C/Xz.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Xz.c$(PreprocessSuffix) "src/formats/7z/C/Xz.c"

$(IntermediateDirectory)/C_Lzma2Dec.c$(ObjectSuffix): src/formats/7z/C/Lzma2Dec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Lzma2Dec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Lzma2Dec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Lzma2Dec.c$(PreprocessSuffix): src/formats/7z/C/Lzma2Dec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Lzma2Dec.c$(PreprocessSuffix) "src/formats/7z/C/Lzma2Dec.c"

$(IntermediateDirectory)/C_AesOpt.c$(ObjectSuffix): src/formats/7z/C/AesOpt.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/AesOpt.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_AesOpt.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_AesOpt.c$(PreprocessSuffix): src/formats/7z/C/AesOpt.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_AesOpt.c$(PreprocessSuffix) "src/formats/7z/C/AesOpt.c"

$(IntermediateDirectory)/C_BwtSort.c$(ObjectSuffix): src/formats/7z/C/BwtSort.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/BwtSort.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_BwtSort.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_BwtSort.c$(PreprocessSuffix): src/formats/7z/C/BwtSort.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_BwtSort.c$(PreprocessSuffix) "src/formats/7z/C/BwtSort.c"

$(IntermediateDirectory)/C_Bra86.c$(ObjectSuffix): src/formats/7z/C/Bra86.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Bra86.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Bra86.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Bra86.c$(PreprocessSuffix): src/formats/7z/C/Bra86.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Bra86.c$(PreprocessSuffix) "src/formats/7z/C/Bra86.c"

$(IntermediateDirectory)/C_7zCrcOpt.c$(ObjectSuffix): src/formats/7z/C/7zCrcOpt.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zCrcOpt.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zCrcOpt.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zCrcOpt.c$(PreprocessSuffix): src/formats/7z/C/7zCrcOpt.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zCrcOpt.c$(PreprocessSuffix) "src/formats/7z/C/7zCrcOpt.c"

$(IntermediateDirectory)/C_7zAlloc.c$(ObjectSuffix): src/formats/7z/C/7zAlloc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zAlloc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zAlloc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zAlloc.c$(PreprocessSuffix): src/formats/7z/C/7zAlloc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zAlloc.c$(PreprocessSuffix) "src/formats/7z/C/7zAlloc.c"

$(IntermediateDirectory)/C_LzmaLib.c$(ObjectSuffix): src/formats/7z/C/LzmaLib.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/LzmaLib.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_LzmaLib.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_LzmaLib.c$(PreprocessSuffix): src/formats/7z/C/LzmaLib.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_LzmaLib.c$(PreprocessSuffix) "src/formats/7z/C/LzmaLib.c"

$(IntermediateDirectory)/C_HuffEnc.c$(ObjectSuffix): src/formats/7z/C/HuffEnc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/HuffEnc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_HuffEnc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_HuffEnc.c$(PreprocessSuffix): src/formats/7z/C/HuffEnc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_HuffEnc.c$(PreprocessSuffix) "src/formats/7z/C/HuffEnc.c"

$(IntermediateDirectory)/C_7zBuf.c$(ObjectSuffix): src/formats/7z/C/7zBuf.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zBuf.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zBuf.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zBuf.c$(PreprocessSuffix): src/formats/7z/C/7zBuf.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zBuf.c$(PreprocessSuffix) "src/formats/7z/C/7zBuf.c"

$(IntermediateDirectory)/C_XzDec.c$(ObjectSuffix): src/formats/7z/C/XzDec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/XzDec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_XzDec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_XzDec.c$(PreprocessSuffix): src/formats/7z/C/XzDec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_XzDec.c$(PreprocessSuffix) "src/formats/7z/C/XzDec.c"

$(IntermediateDirectory)/C_7zBuf2.c$(ObjectSuffix): src/formats/7z/C/7zBuf2.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zBuf2.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zBuf2.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zBuf2.c$(PreprocessSuffix): src/formats/7z/C/7zBuf2.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zBuf2.c$(PreprocessSuffix) "src/formats/7z/C/7zBuf2.c"

$(IntermediateDirectory)/C_Lzma86Enc.c$(ObjectSuffix): src/formats/7z/C/Lzma86Enc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Lzma86Enc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Lzma86Enc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Lzma86Enc.c$(PreprocessSuffix): src/formats/7z/C/Lzma86Enc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Lzma86Enc.c$(PreprocessSuffix) "src/formats/7z/C/Lzma86Enc.c"

$(IntermediateDirectory)/C_Alloc.c$(ObjectSuffix): src/formats/7z/C/Alloc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Alloc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Alloc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Alloc.c$(PreprocessSuffix): src/formats/7z/C/Alloc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Alloc.c$(PreprocessSuffix) "src/formats/7z/C/Alloc.c"

$(IntermediateDirectory)/C_XzCrc64.c$(ObjectSuffix): src/formats/7z/C/XzCrc64.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/XzCrc64.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_XzCrc64.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_XzCrc64.c$(PreprocessSuffix): src/formats/7z/C/XzCrc64.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_XzCrc64.c$(PreprocessSuffix) "src/formats/7z/C/XzCrc64.c"

$(IntermediateDirectory)/C_XzEnc.c$(ObjectSuffix): src/formats/7z/C/XzEnc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/XzEnc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_XzEnc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_XzEnc.c$(PreprocessSuffix): src/formats/7z/C/XzEnc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_XzEnc.c$(PreprocessSuffix) "src/formats/7z/C/XzEnc.c"

$(IntermediateDirectory)/C_Ppmd8.c$(ObjectSuffix): src/formats/7z/C/Ppmd8.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Ppmd8.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Ppmd8.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Ppmd8.c$(PreprocessSuffix): src/formats/7z/C/Ppmd8.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Ppmd8.c$(PreprocessSuffix) "src/formats/7z/C/Ppmd8.c"

$(IntermediateDirectory)/C_Aes.c$(ObjectSuffix): src/formats/7z/C/Aes.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Aes.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Aes.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Aes.c$(PreprocessSuffix): src/formats/7z/C/Aes.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Aes.c$(PreprocessSuffix) "src/formats/7z/C/Aes.c"

$(IntermediateDirectory)/C_7zCrc.c$(ObjectSuffix): src/formats/7z/C/7zCrc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zCrc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zCrc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zCrc.c$(PreprocessSuffix): src/formats/7z/C/7zCrc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zCrc.c$(PreprocessSuffix) "src/formats/7z/C/7zCrc.c"

$(IntermediateDirectory)/C_Bcj2.c$(ObjectSuffix): src/formats/7z/C/Bcj2.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Bcj2.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Bcj2.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Bcj2.c$(PreprocessSuffix): src/formats/7z/C/Bcj2.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Bcj2.c$(PreprocessSuffix) "src/formats/7z/C/Bcj2.c"

$(IntermediateDirectory)/C_Delta.c$(ObjectSuffix): src/formats/7z/C/Delta.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Delta.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Delta.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Delta.c$(PreprocessSuffix): src/formats/7z/C/Delta.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Delta.c$(PreprocessSuffix) "src/formats/7z/C/Delta.c"

$(IntermediateDirectory)/C_7zFile.c$(ObjectSuffix): src/formats/7z/C/7zFile.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zFile.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zFile.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zFile.c$(PreprocessSuffix): src/formats/7z/C/7zFile.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zFile.c$(PreprocessSuffix) "src/formats/7z/C/7zFile.c"

$(IntermediateDirectory)/C_Bcj2Enc.c$(ObjectSuffix): src/formats/7z/C/Bcj2Enc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Bcj2Enc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Bcj2Enc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Bcj2Enc.c$(PreprocessSuffix): src/formats/7z/C/Bcj2Enc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Bcj2Enc.c$(PreprocessSuffix) "src/formats/7z/C/Bcj2Enc.c"

$(IntermediateDirectory)/C_Ppmd8Enc.c$(ObjectSuffix): src/formats/7z/C/Ppmd8Enc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Ppmd8Enc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Ppmd8Enc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Ppmd8Enc.c$(PreprocessSuffix): src/formats/7z/C/Ppmd8Enc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Ppmd8Enc.c$(PreprocessSuffix) "src/formats/7z/C/Ppmd8Enc.c"

$(IntermediateDirectory)/C_LzmaEnc.c$(ObjectSuffix): src/formats/7z/C/LzmaEnc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/LzmaEnc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_LzmaEnc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_LzmaEnc.c$(PreprocessSuffix): src/formats/7z/C/LzmaEnc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_LzmaEnc.c$(PreprocessSuffix) "src/formats/7z/C/LzmaEnc.c"

$(IntermediateDirectory)/C_Ppmd7Enc.c$(ObjectSuffix): src/formats/7z/C/Ppmd7Enc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Ppmd7Enc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Ppmd7Enc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Ppmd7Enc.c$(PreprocessSuffix): src/formats/7z/C/Ppmd7Enc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Ppmd7Enc.c$(PreprocessSuffix) "src/formats/7z/C/Ppmd7Enc.c"

$(IntermediateDirectory)/C_7zStream.c$(ObjectSuffix): src/formats/7z/C/7zStream.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zStream.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zStream.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zStream.c$(PreprocessSuffix): src/formats/7z/C/7zStream.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zStream.c$(PreprocessSuffix) "src/formats/7z/C/7zStream.c"

$(IntermediateDirectory)/C_Ppmd8Dec.c$(ObjectSuffix): src/formats/7z/C/Ppmd8Dec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Ppmd8Dec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Ppmd8Dec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Ppmd8Dec.c$(PreprocessSuffix): src/formats/7z/C/Ppmd8Dec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Ppmd8Dec.c$(PreprocessSuffix) "src/formats/7z/C/Ppmd8Dec.c"

$(IntermediateDirectory)/C_XzCrc64Opt.c$(ObjectSuffix): src/formats/7z/C/XzCrc64Opt.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/XzCrc64Opt.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_XzCrc64Opt.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_XzCrc64Opt.c$(PreprocessSuffix): src/formats/7z/C/XzCrc64Opt.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_XzCrc64Opt.c$(PreprocessSuffix) "src/formats/7z/C/XzCrc64Opt.c"

$(IntermediateDirectory)/C_LzmaDec.c$(ObjectSuffix): src/formats/7z/C/LzmaDec.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/LzmaDec.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_LzmaDec.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_LzmaDec.c$(PreprocessSuffix): src/formats/7z/C/LzmaDec.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_LzmaDec.c$(PreprocessSuffix) "src/formats/7z/C/LzmaDec.c"

$(IntermediateDirectory)/C_XzIn.c$(ObjectSuffix): src/formats/7z/C/XzIn.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/XzIn.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_XzIn.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_XzIn.c$(PreprocessSuffix): src/formats/7z/C/XzIn.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_XzIn.c$(PreprocessSuffix) "src/formats/7z/C/XzIn.c"

$(IntermediateDirectory)/C_CpuArch.c$(ObjectSuffix): src/formats/7z/C/CpuArch.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/CpuArch.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_CpuArch.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_CpuArch.c$(PreprocessSuffix): src/formats/7z/C/CpuArch.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_CpuArch.c$(PreprocessSuffix) "src/formats/7z/C/CpuArch.c"

$(IntermediateDirectory)/C_LzFind.c$(ObjectSuffix): src/formats/7z/C/LzFind.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/LzFind.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_LzFind.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_LzFind.c$(PreprocessSuffix): src/formats/7z/C/LzFind.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_LzFind.c$(PreprocessSuffix) "src/formats/7z/C/LzFind.c"

$(IntermediateDirectory)/C_Ppmd7.c$(ObjectSuffix): src/formats/7z/C/Ppmd7.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Ppmd7.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Ppmd7.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Ppmd7.c$(PreprocessSuffix): src/formats/7z/C/Ppmd7.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Ppmd7.c$(PreprocessSuffix) "src/formats/7z/C/Ppmd7.c"

$(IntermediateDirectory)/C_Lzma2Enc.c$(ObjectSuffix): src/formats/7z/C/Lzma2Enc.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Lzma2Enc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Lzma2Enc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Lzma2Enc.c$(PreprocessSuffix): src/formats/7z/C/Lzma2Enc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Lzma2Enc.c$(PreprocessSuffix) "src/formats/7z/C/Lzma2Enc.c"

$(IntermediateDirectory)/C_Sha256.c$(ObjectSuffix): src/formats/7z/C/Sha256.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Sha256.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Sha256.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Sha256.c$(PreprocessSuffix): src/formats/7z/C/Sha256.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Sha256.c$(PreprocessSuffix) "src/formats/7z/C/Sha256.c"

$(IntermediateDirectory)/C_Sort.c$(ObjectSuffix): src/formats/7z/C/Sort.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Sort.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Sort.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Sort.c$(PreprocessSuffix): src/formats/7z/C/Sort.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Sort.c$(PreprocessSuffix) "src/formats/7z/C/Sort.c"

$(IntermediateDirectory)/C_7zArcIn.c$(ObjectSuffix): src/formats/7z/C/7zArcIn.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/7zArcIn.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_7zArcIn.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_7zArcIn.c$(PreprocessSuffix): src/formats/7z/C/7zArcIn.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_7zArcIn.c$(PreprocessSuffix) "src/formats/7z/C/7zArcIn.c"

$(IntermediateDirectory)/C_Bra.c$(ObjectSuffix): src/formats/7z/C/Bra.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Bra.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Bra.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Bra.c$(PreprocessSuffix): src/formats/7z/C/Bra.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Bra.c$(PreprocessSuffix) "src/formats/7z/C/Bra.c"

$(IntermediateDirectory)/C_Blake2s.c$(ObjectSuffix): src/formats/7z/C/Blake2s.c 
	$(CC) $(SourceSwitch) "./src/formats/7z/C/Blake2s.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/C_Blake2s.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/C_Blake2s.c$(PreprocessSuffix): src/formats/7z/C/Blake2s.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/C_Blake2s.c$(PreprocessSuffix) "src/formats/7z/C/Blake2s.c"

##
## Clean
##
clean:
	$(RM) -r ./$(ConfigurationName)/


