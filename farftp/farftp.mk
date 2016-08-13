##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=farftp
ConfigurationName      :=Debug
WorkspacePath          := ".."
ProjectPath            := "."
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=14/08/16
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
OutputFile             :=../Build/Plugin/farftp/bin/$(ProjectName).far-plug-utf8
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="farftp.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)./src $(IncludeSwitch)./src/FStdLib $(IncludeSwitch)./src/FStdLib/FARStdlib $(IncludeSwitch)../far2l/ $(IncludeSwitch)../far2l/Include $(IncludeSwitch)../WinPort $(IncludeSwitch)/usr/include/glib-2.0 $(IncludeSwitch)/usr/lib/x86_64-linux-gnu/glib-2.0/include/ 
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
CXXFLAGS :=  -g -std=c++11 -fPIC -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
CFLAGS   :=  -g -std=c99 -fPIC -fvisibility=hidden -Wno-unused-function -Wno-unknown-pragmas $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/lib_All.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_SetDir.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_cnInit.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Cfg.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_DeleteFile.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Key.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_HPut.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FTPConnect.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Mem.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FAR.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_ConnectNB.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConnectCmds.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_sock.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_fUtils.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FGet.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_EnumHost.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConnectMain.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_MakeDir.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Ftp.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Mix.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_FPut.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConnectIO.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_GetOpenInfo.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Queque.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Url.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_cnUpload.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FileList.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Plugin.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_Event.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FtpDlg.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_Shortcut.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_ConnectSock.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FtpAPI.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FTPHost.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_CmdLine.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_cnDownload.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_JM.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_FTPBlock.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_HGet.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_AskOver.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/src_Connect.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_pctcp.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_vx.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_os2.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_vms.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_unix.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_os400.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_dos.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_netware.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_skirdin.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/DirList_eplf.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_mvs.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_Main.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_tcpc.cpp$(ObjectSuffix) $(IntermediateDirectory)/DirList_cms.cpp$(ObjectSuffix) $(IntermediateDirectory)/Notify_Main.cpp$(ObjectSuffix) $(IntermediateDirectory)/Progress_cbFmt.cpp$(ObjectSuffix) $(IntermediateDirectory)/Progress_TraficCB.cpp$(ObjectSuffix) $(IntermediateDirectory)/Progress_Main.cpp$(ObjectSuffix) $(IntermediateDirectory)/Progress_Utils.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(ObjectSuffix) $(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(ObjectSuffix) 



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
	@echo rebuilt > "../.build-debug/farftp"

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/lib_All.cpp$(ObjectSuffix): lib/All.cpp $(IntermediateDirectory)/lib_All.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/All.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/lib_All.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/lib_All.cpp$(DependSuffix): lib/All.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/lib_All.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/lib_All.cpp$(DependSuffix) -MM "lib/All.cpp"

$(IntermediateDirectory)/lib_All.cpp$(PreprocessSuffix): lib/All.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/lib_All.cpp$(PreprocessSuffix) "lib/All.cpp"

$(IntermediateDirectory)/src_SetDir.cpp$(ObjectSuffix): src/SetDir.cpp $(IntermediateDirectory)/src_SetDir.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/SetDir.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_SetDir.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_SetDir.cpp$(DependSuffix): src/SetDir.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_SetDir.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_SetDir.cpp$(DependSuffix) -MM "src/SetDir.cpp"

$(IntermediateDirectory)/src_SetDir.cpp$(PreprocessSuffix): src/SetDir.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_SetDir.cpp$(PreprocessSuffix) "src/SetDir.cpp"

$(IntermediateDirectory)/src_cnInit.cpp$(ObjectSuffix): src/cnInit.cpp $(IntermediateDirectory)/src_cnInit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/cnInit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_cnInit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_cnInit.cpp$(DependSuffix): src/cnInit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_cnInit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_cnInit.cpp$(DependSuffix) -MM "src/cnInit.cpp"

$(IntermediateDirectory)/src_cnInit.cpp$(PreprocessSuffix): src/cnInit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_cnInit.cpp$(PreprocessSuffix) "src/cnInit.cpp"

$(IntermediateDirectory)/src_Cfg.cpp$(ObjectSuffix): src/Cfg.cpp $(IntermediateDirectory)/src_Cfg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Cfg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Cfg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Cfg.cpp$(DependSuffix): src/Cfg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Cfg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Cfg.cpp$(DependSuffix) -MM "src/Cfg.cpp"

$(IntermediateDirectory)/src_Cfg.cpp$(PreprocessSuffix): src/Cfg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Cfg.cpp$(PreprocessSuffix) "src/Cfg.cpp"

$(IntermediateDirectory)/src_DeleteFile.cpp$(ObjectSuffix): src/DeleteFile.cpp $(IntermediateDirectory)/src_DeleteFile.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/DeleteFile.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_DeleteFile.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_DeleteFile.cpp$(DependSuffix): src/DeleteFile.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_DeleteFile.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_DeleteFile.cpp$(DependSuffix) -MM "src/DeleteFile.cpp"

$(IntermediateDirectory)/src_DeleteFile.cpp$(PreprocessSuffix): src/DeleteFile.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_DeleteFile.cpp$(PreprocessSuffix) "src/DeleteFile.cpp"

$(IntermediateDirectory)/src_Key.cpp$(ObjectSuffix): src/Key.cpp $(IntermediateDirectory)/src_Key.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Key.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Key.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Key.cpp$(DependSuffix): src/Key.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Key.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Key.cpp$(DependSuffix) -MM "src/Key.cpp"

$(IntermediateDirectory)/src_Key.cpp$(PreprocessSuffix): src/Key.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Key.cpp$(PreprocessSuffix) "src/Key.cpp"

$(IntermediateDirectory)/src_HPut.cpp$(ObjectSuffix): src/HPut.cpp $(IntermediateDirectory)/src_HPut.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/HPut.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_HPut.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_HPut.cpp$(DependSuffix): src/HPut.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_HPut.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_HPut.cpp$(DependSuffix) -MM "src/HPut.cpp"

$(IntermediateDirectory)/src_HPut.cpp$(PreprocessSuffix): src/HPut.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_HPut.cpp$(PreprocessSuffix) "src/HPut.cpp"

$(IntermediateDirectory)/src_FTPConnect.cpp$(ObjectSuffix): src/FTPConnect.cpp $(IntermediateDirectory)/src_FTPConnect.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FTPConnect.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FTPConnect.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FTPConnect.cpp$(DependSuffix): src/FTPConnect.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FTPConnect.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FTPConnect.cpp$(DependSuffix) -MM "src/FTPConnect.cpp"

$(IntermediateDirectory)/src_FTPConnect.cpp$(PreprocessSuffix): src/FTPConnect.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FTPConnect.cpp$(PreprocessSuffix) "src/FTPConnect.cpp"

$(IntermediateDirectory)/src_Mem.cpp$(ObjectSuffix): src/Mem.cpp $(IntermediateDirectory)/src_Mem.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Mem.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Mem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Mem.cpp$(DependSuffix): src/Mem.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Mem.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Mem.cpp$(DependSuffix) -MM "src/Mem.cpp"

$(IntermediateDirectory)/src_Mem.cpp$(PreprocessSuffix): src/Mem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Mem.cpp$(PreprocessSuffix) "src/Mem.cpp"

$(IntermediateDirectory)/src_FAR.cpp$(ObjectSuffix): src/FAR.cpp $(IntermediateDirectory)/src_FAR.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FAR.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FAR.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FAR.cpp$(DependSuffix): src/FAR.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FAR.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FAR.cpp$(DependSuffix) -MM "src/FAR.cpp"

$(IntermediateDirectory)/src_FAR.cpp$(PreprocessSuffix): src/FAR.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FAR.cpp$(PreprocessSuffix) "src/FAR.cpp"

$(IntermediateDirectory)/src_ConnectNB.cpp$(ObjectSuffix): src/ConnectNB.cpp $(IntermediateDirectory)/src_ConnectNB.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConnectNB.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConnectNB.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConnectNB.cpp$(DependSuffix): src/ConnectNB.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ConnectNB.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ConnectNB.cpp$(DependSuffix) -MM "src/ConnectNB.cpp"

$(IntermediateDirectory)/src_ConnectNB.cpp$(PreprocessSuffix): src/ConnectNB.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConnectNB.cpp$(PreprocessSuffix) "src/ConnectNB.cpp"

$(IntermediateDirectory)/src_ConnectCmds.cpp$(ObjectSuffix): src/ConnectCmds.cpp $(IntermediateDirectory)/src_ConnectCmds.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConnectCmds.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConnectCmds.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConnectCmds.cpp$(DependSuffix): src/ConnectCmds.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ConnectCmds.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ConnectCmds.cpp$(DependSuffix) -MM "src/ConnectCmds.cpp"

$(IntermediateDirectory)/src_ConnectCmds.cpp$(PreprocessSuffix): src/ConnectCmds.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConnectCmds.cpp$(PreprocessSuffix) "src/ConnectCmds.cpp"

$(IntermediateDirectory)/src_sock.cpp$(ObjectSuffix): src/sock.cpp $(IntermediateDirectory)/src_sock.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/sock.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_sock.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_sock.cpp$(DependSuffix): src/sock.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_sock.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_sock.cpp$(DependSuffix) -MM "src/sock.cpp"

$(IntermediateDirectory)/src_sock.cpp$(PreprocessSuffix): src/sock.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_sock.cpp$(PreprocessSuffix) "src/sock.cpp"

$(IntermediateDirectory)/src_fUtils.cpp$(ObjectSuffix): src/fUtils.cpp $(IntermediateDirectory)/src_fUtils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/fUtils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_fUtils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_fUtils.cpp$(DependSuffix): src/fUtils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_fUtils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_fUtils.cpp$(DependSuffix) -MM "src/fUtils.cpp"

$(IntermediateDirectory)/src_fUtils.cpp$(PreprocessSuffix): src/fUtils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_fUtils.cpp$(PreprocessSuffix) "src/fUtils.cpp"

$(IntermediateDirectory)/src_FGet.cpp$(ObjectSuffix): src/FGet.cpp $(IntermediateDirectory)/src_FGet.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FGet.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FGet.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FGet.cpp$(DependSuffix): src/FGet.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FGet.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FGet.cpp$(DependSuffix) -MM "src/FGet.cpp"

$(IntermediateDirectory)/src_FGet.cpp$(PreprocessSuffix): src/FGet.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FGet.cpp$(PreprocessSuffix) "src/FGet.cpp"

$(IntermediateDirectory)/src_EnumHost.cpp$(ObjectSuffix): src/EnumHost.cpp $(IntermediateDirectory)/src_EnumHost.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/EnumHost.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_EnumHost.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_EnumHost.cpp$(DependSuffix): src/EnumHost.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_EnumHost.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_EnumHost.cpp$(DependSuffix) -MM "src/EnumHost.cpp"

$(IntermediateDirectory)/src_EnumHost.cpp$(PreprocessSuffix): src/EnumHost.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_EnumHost.cpp$(PreprocessSuffix) "src/EnumHost.cpp"

$(IntermediateDirectory)/src_ConnectMain.cpp$(ObjectSuffix): src/ConnectMain.cpp $(IntermediateDirectory)/src_ConnectMain.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConnectMain.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConnectMain.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConnectMain.cpp$(DependSuffix): src/ConnectMain.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ConnectMain.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ConnectMain.cpp$(DependSuffix) -MM "src/ConnectMain.cpp"

$(IntermediateDirectory)/src_ConnectMain.cpp$(PreprocessSuffix): src/ConnectMain.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConnectMain.cpp$(PreprocessSuffix) "src/ConnectMain.cpp"

$(IntermediateDirectory)/src_MakeDir.cpp$(ObjectSuffix): src/MakeDir.cpp $(IntermediateDirectory)/src_MakeDir.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/MakeDir.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_MakeDir.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_MakeDir.cpp$(DependSuffix): src/MakeDir.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_MakeDir.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_MakeDir.cpp$(DependSuffix) -MM "src/MakeDir.cpp"

$(IntermediateDirectory)/src_MakeDir.cpp$(PreprocessSuffix): src/MakeDir.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_MakeDir.cpp$(PreprocessSuffix) "src/MakeDir.cpp"

$(IntermediateDirectory)/src_Ftp.cpp$(ObjectSuffix): src/Ftp.cpp $(IntermediateDirectory)/src_Ftp.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Ftp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Ftp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Ftp.cpp$(DependSuffix): src/Ftp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Ftp.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Ftp.cpp$(DependSuffix) -MM "src/Ftp.cpp"

$(IntermediateDirectory)/src_Ftp.cpp$(PreprocessSuffix): src/Ftp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Ftp.cpp$(PreprocessSuffix) "src/Ftp.cpp"

$(IntermediateDirectory)/src_Mix.cpp$(ObjectSuffix): src/Mix.cpp $(IntermediateDirectory)/src_Mix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Mix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Mix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Mix.cpp$(DependSuffix): src/Mix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Mix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Mix.cpp$(DependSuffix) -MM "src/Mix.cpp"

$(IntermediateDirectory)/src_Mix.cpp$(PreprocessSuffix): src/Mix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Mix.cpp$(PreprocessSuffix) "src/Mix.cpp"

$(IntermediateDirectory)/src_FPut.cpp$(ObjectSuffix): src/FPut.cpp $(IntermediateDirectory)/src_FPut.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FPut.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FPut.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FPut.cpp$(DependSuffix): src/FPut.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FPut.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FPut.cpp$(DependSuffix) -MM "src/FPut.cpp"

$(IntermediateDirectory)/src_FPut.cpp$(PreprocessSuffix): src/FPut.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FPut.cpp$(PreprocessSuffix) "src/FPut.cpp"

$(IntermediateDirectory)/src_ConnectIO.cpp$(ObjectSuffix): src/ConnectIO.cpp $(IntermediateDirectory)/src_ConnectIO.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConnectIO.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConnectIO.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConnectIO.cpp$(DependSuffix): src/ConnectIO.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ConnectIO.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ConnectIO.cpp$(DependSuffix) -MM "src/ConnectIO.cpp"

$(IntermediateDirectory)/src_ConnectIO.cpp$(PreprocessSuffix): src/ConnectIO.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConnectIO.cpp$(PreprocessSuffix) "src/ConnectIO.cpp"

$(IntermediateDirectory)/src_GetOpenInfo.cpp$(ObjectSuffix): src/GetOpenInfo.cpp $(IntermediateDirectory)/src_GetOpenInfo.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/GetOpenInfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_GetOpenInfo.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_GetOpenInfo.cpp$(DependSuffix): src/GetOpenInfo.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_GetOpenInfo.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_GetOpenInfo.cpp$(DependSuffix) -MM "src/GetOpenInfo.cpp"

$(IntermediateDirectory)/src_GetOpenInfo.cpp$(PreprocessSuffix): src/GetOpenInfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_GetOpenInfo.cpp$(PreprocessSuffix) "src/GetOpenInfo.cpp"

$(IntermediateDirectory)/src_Queque.cpp$(ObjectSuffix): src/Queque.cpp $(IntermediateDirectory)/src_Queque.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Queque.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Queque.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Queque.cpp$(DependSuffix): src/Queque.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Queque.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Queque.cpp$(DependSuffix) -MM "src/Queque.cpp"

$(IntermediateDirectory)/src_Queque.cpp$(PreprocessSuffix): src/Queque.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Queque.cpp$(PreprocessSuffix) "src/Queque.cpp"

$(IntermediateDirectory)/src_Url.cpp$(ObjectSuffix): src/Url.cpp $(IntermediateDirectory)/src_Url.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Url.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Url.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Url.cpp$(DependSuffix): src/Url.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Url.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Url.cpp$(DependSuffix) -MM "src/Url.cpp"

$(IntermediateDirectory)/src_Url.cpp$(PreprocessSuffix): src/Url.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Url.cpp$(PreprocessSuffix) "src/Url.cpp"

$(IntermediateDirectory)/src_cnUpload.cpp$(ObjectSuffix): src/cnUpload.cpp $(IntermediateDirectory)/src_cnUpload.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/cnUpload.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_cnUpload.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_cnUpload.cpp$(DependSuffix): src/cnUpload.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_cnUpload.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_cnUpload.cpp$(DependSuffix) -MM "src/cnUpload.cpp"

$(IntermediateDirectory)/src_cnUpload.cpp$(PreprocessSuffix): src/cnUpload.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_cnUpload.cpp$(PreprocessSuffix) "src/cnUpload.cpp"

$(IntermediateDirectory)/src_FileList.cpp$(ObjectSuffix): src/FileList.cpp $(IntermediateDirectory)/src_FileList.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FileList.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FileList.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FileList.cpp$(DependSuffix): src/FileList.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FileList.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FileList.cpp$(DependSuffix) -MM "src/FileList.cpp"

$(IntermediateDirectory)/src_FileList.cpp$(PreprocessSuffix): src/FileList.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FileList.cpp$(PreprocessSuffix) "src/FileList.cpp"

$(IntermediateDirectory)/src_Plugin.cpp$(ObjectSuffix): src/Plugin.cpp $(IntermediateDirectory)/src_Plugin.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Plugin.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Plugin.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Plugin.cpp$(DependSuffix): src/Plugin.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Plugin.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Plugin.cpp$(DependSuffix) -MM "src/Plugin.cpp"

$(IntermediateDirectory)/src_Plugin.cpp$(PreprocessSuffix): src/Plugin.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Plugin.cpp$(PreprocessSuffix) "src/Plugin.cpp"

$(IntermediateDirectory)/src_Event.cpp$(ObjectSuffix): src/Event.cpp $(IntermediateDirectory)/src_Event.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Event.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Event.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Event.cpp$(DependSuffix): src/Event.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Event.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Event.cpp$(DependSuffix) -MM "src/Event.cpp"

$(IntermediateDirectory)/src_Event.cpp$(PreprocessSuffix): src/Event.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Event.cpp$(PreprocessSuffix) "src/Event.cpp"

$(IntermediateDirectory)/src_FtpDlg.cpp$(ObjectSuffix): src/FtpDlg.cpp $(IntermediateDirectory)/src_FtpDlg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FtpDlg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FtpDlg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FtpDlg.cpp$(DependSuffix): src/FtpDlg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FtpDlg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FtpDlg.cpp$(DependSuffix) -MM "src/FtpDlg.cpp"

$(IntermediateDirectory)/src_FtpDlg.cpp$(PreprocessSuffix): src/FtpDlg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FtpDlg.cpp$(PreprocessSuffix) "src/FtpDlg.cpp"

$(IntermediateDirectory)/src_Shortcut.cpp$(ObjectSuffix): src/Shortcut.cpp $(IntermediateDirectory)/src_Shortcut.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Shortcut.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Shortcut.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Shortcut.cpp$(DependSuffix): src/Shortcut.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Shortcut.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Shortcut.cpp$(DependSuffix) -MM "src/Shortcut.cpp"

$(IntermediateDirectory)/src_Shortcut.cpp$(PreprocessSuffix): src/Shortcut.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Shortcut.cpp$(PreprocessSuffix) "src/Shortcut.cpp"

$(IntermediateDirectory)/src_ConnectSock.cpp$(ObjectSuffix): src/ConnectSock.cpp $(IntermediateDirectory)/src_ConnectSock.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/ConnectSock.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ConnectSock.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ConnectSock.cpp$(DependSuffix): src/ConnectSock.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ConnectSock.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ConnectSock.cpp$(DependSuffix) -MM "src/ConnectSock.cpp"

$(IntermediateDirectory)/src_ConnectSock.cpp$(PreprocessSuffix): src/ConnectSock.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ConnectSock.cpp$(PreprocessSuffix) "src/ConnectSock.cpp"

$(IntermediateDirectory)/src_FtpAPI.cpp$(ObjectSuffix): src/FtpAPI.cpp $(IntermediateDirectory)/src_FtpAPI.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FtpAPI.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FtpAPI.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FtpAPI.cpp$(DependSuffix): src/FtpAPI.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FtpAPI.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FtpAPI.cpp$(DependSuffix) -MM "src/FtpAPI.cpp"

$(IntermediateDirectory)/src_FtpAPI.cpp$(PreprocessSuffix): src/FtpAPI.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FtpAPI.cpp$(PreprocessSuffix) "src/FtpAPI.cpp"

$(IntermediateDirectory)/src_FTPHost.cpp$(ObjectSuffix): src/FTPHost.cpp $(IntermediateDirectory)/src_FTPHost.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FTPHost.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FTPHost.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FTPHost.cpp$(DependSuffix): src/FTPHost.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FTPHost.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FTPHost.cpp$(DependSuffix) -MM "src/FTPHost.cpp"

$(IntermediateDirectory)/src_FTPHost.cpp$(PreprocessSuffix): src/FTPHost.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FTPHost.cpp$(PreprocessSuffix) "src/FTPHost.cpp"

$(IntermediateDirectory)/src_CmdLine.cpp$(ObjectSuffix): src/CmdLine.cpp $(IntermediateDirectory)/src_CmdLine.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/CmdLine.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_CmdLine.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_CmdLine.cpp$(DependSuffix): src/CmdLine.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_CmdLine.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_CmdLine.cpp$(DependSuffix) -MM "src/CmdLine.cpp"

$(IntermediateDirectory)/src_CmdLine.cpp$(PreprocessSuffix): src/CmdLine.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_CmdLine.cpp$(PreprocessSuffix) "src/CmdLine.cpp"

$(IntermediateDirectory)/src_cnDownload.cpp$(ObjectSuffix): src/cnDownload.cpp $(IntermediateDirectory)/src_cnDownload.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/cnDownload.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_cnDownload.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_cnDownload.cpp$(DependSuffix): src/cnDownload.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_cnDownload.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_cnDownload.cpp$(DependSuffix) -MM "src/cnDownload.cpp"

$(IntermediateDirectory)/src_cnDownload.cpp$(PreprocessSuffix): src/cnDownload.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_cnDownload.cpp$(PreprocessSuffix) "src/cnDownload.cpp"

$(IntermediateDirectory)/src_JM.cpp$(ObjectSuffix): src/JM.cpp $(IntermediateDirectory)/src_JM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/JM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_JM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_JM.cpp$(DependSuffix): src/JM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_JM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_JM.cpp$(DependSuffix) -MM "src/JM.cpp"

$(IntermediateDirectory)/src_JM.cpp$(PreprocessSuffix): src/JM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_JM.cpp$(PreprocessSuffix) "src/JM.cpp"

$(IntermediateDirectory)/src_FTPBlock.cpp$(ObjectSuffix): src/FTPBlock.cpp $(IntermediateDirectory)/src_FTPBlock.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FTPBlock.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_FTPBlock.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_FTPBlock.cpp$(DependSuffix): src/FTPBlock.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_FTPBlock.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_FTPBlock.cpp$(DependSuffix) -MM "src/FTPBlock.cpp"

$(IntermediateDirectory)/src_FTPBlock.cpp$(PreprocessSuffix): src/FTPBlock.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_FTPBlock.cpp$(PreprocessSuffix) "src/FTPBlock.cpp"

$(IntermediateDirectory)/src_HGet.cpp$(ObjectSuffix): src/HGet.cpp $(IntermediateDirectory)/src_HGet.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/HGet.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_HGet.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_HGet.cpp$(DependSuffix): src/HGet.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_HGet.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_HGet.cpp$(DependSuffix) -MM "src/HGet.cpp"

$(IntermediateDirectory)/src_HGet.cpp$(PreprocessSuffix): src/HGet.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_HGet.cpp$(PreprocessSuffix) "src/HGet.cpp"

$(IntermediateDirectory)/src_AskOver.cpp$(ObjectSuffix): src/AskOver.cpp $(IntermediateDirectory)/src_AskOver.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/AskOver.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_AskOver.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_AskOver.cpp$(DependSuffix): src/AskOver.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_AskOver.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_AskOver.cpp$(DependSuffix) -MM "src/AskOver.cpp"

$(IntermediateDirectory)/src_AskOver.cpp$(PreprocessSuffix): src/AskOver.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_AskOver.cpp$(PreprocessSuffix) "src/AskOver.cpp"

$(IntermediateDirectory)/src_Connect.cpp$(ObjectSuffix): src/Connect.cpp $(IntermediateDirectory)/src_Connect.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/Connect.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Connect.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Connect.cpp$(DependSuffix): src/Connect.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Connect.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Connect.cpp$(DependSuffix) -MM "src/Connect.cpp"

$(IntermediateDirectory)/src_Connect.cpp$(PreprocessSuffix): src/Connect.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Connect.cpp$(PreprocessSuffix) "src/Connect.cpp"

$(IntermediateDirectory)/DirList_pctcp.cpp$(ObjectSuffix): lib/DirList/pctcp.cpp $(IntermediateDirectory)/DirList_pctcp.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/pctcp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_pctcp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_pctcp.cpp$(DependSuffix): lib/DirList/pctcp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_pctcp.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_pctcp.cpp$(DependSuffix) -MM "lib/DirList/pctcp.cpp"

$(IntermediateDirectory)/DirList_pctcp.cpp$(PreprocessSuffix): lib/DirList/pctcp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_pctcp.cpp$(PreprocessSuffix) "lib/DirList/pctcp.cpp"

$(IntermediateDirectory)/DirList_vx.cpp$(ObjectSuffix): lib/DirList/vx.cpp $(IntermediateDirectory)/DirList_vx.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/vx.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_vx.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_vx.cpp$(DependSuffix): lib/DirList/vx.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_vx.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_vx.cpp$(DependSuffix) -MM "lib/DirList/vx.cpp"

$(IntermediateDirectory)/DirList_vx.cpp$(PreprocessSuffix): lib/DirList/vx.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_vx.cpp$(PreprocessSuffix) "lib/DirList/vx.cpp"

$(IntermediateDirectory)/DirList_os2.cpp$(ObjectSuffix): lib/DirList/os2.cpp $(IntermediateDirectory)/DirList_os2.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/os2.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_os2.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_os2.cpp$(DependSuffix): lib/DirList/os2.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_os2.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_os2.cpp$(DependSuffix) -MM "lib/DirList/os2.cpp"

$(IntermediateDirectory)/DirList_os2.cpp$(PreprocessSuffix): lib/DirList/os2.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_os2.cpp$(PreprocessSuffix) "lib/DirList/os2.cpp"

$(IntermediateDirectory)/DirList_vms.cpp$(ObjectSuffix): lib/DirList/vms.cpp $(IntermediateDirectory)/DirList_vms.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/vms.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_vms.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_vms.cpp$(DependSuffix): lib/DirList/vms.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_vms.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_vms.cpp$(DependSuffix) -MM "lib/DirList/vms.cpp"

$(IntermediateDirectory)/DirList_vms.cpp$(PreprocessSuffix): lib/DirList/vms.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_vms.cpp$(PreprocessSuffix) "lib/DirList/vms.cpp"

$(IntermediateDirectory)/DirList_unix.cpp$(ObjectSuffix): lib/DirList/unix.cpp $(IntermediateDirectory)/DirList_unix.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/unix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_unix.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_unix.cpp$(DependSuffix): lib/DirList/unix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_unix.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_unix.cpp$(DependSuffix) -MM "lib/DirList/unix.cpp"

$(IntermediateDirectory)/DirList_unix.cpp$(PreprocessSuffix): lib/DirList/unix.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_unix.cpp$(PreprocessSuffix) "lib/DirList/unix.cpp"

$(IntermediateDirectory)/DirList_os400.cpp$(ObjectSuffix): lib/DirList/os400.cpp $(IntermediateDirectory)/DirList_os400.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/os400.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_os400.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_os400.cpp$(DependSuffix): lib/DirList/os400.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_os400.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_os400.cpp$(DependSuffix) -MM "lib/DirList/os400.cpp"

$(IntermediateDirectory)/DirList_os400.cpp$(PreprocessSuffix): lib/DirList/os400.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_os400.cpp$(PreprocessSuffix) "lib/DirList/os400.cpp"

$(IntermediateDirectory)/DirList_dos.cpp$(ObjectSuffix): lib/DirList/dos.cpp $(IntermediateDirectory)/DirList_dos.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/dos.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_dos.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_dos.cpp$(DependSuffix): lib/DirList/dos.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_dos.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_dos.cpp$(DependSuffix) -MM "lib/DirList/dos.cpp"

$(IntermediateDirectory)/DirList_dos.cpp$(PreprocessSuffix): lib/DirList/dos.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_dos.cpp$(PreprocessSuffix) "lib/DirList/dos.cpp"

$(IntermediateDirectory)/DirList_netware.cpp$(ObjectSuffix): lib/DirList/netware.cpp $(IntermediateDirectory)/DirList_netware.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/netware.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_netware.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_netware.cpp$(DependSuffix): lib/DirList/netware.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_netware.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_netware.cpp$(DependSuffix) -MM "lib/DirList/netware.cpp"

$(IntermediateDirectory)/DirList_netware.cpp$(PreprocessSuffix): lib/DirList/netware.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_netware.cpp$(PreprocessSuffix) "lib/DirList/netware.cpp"

$(IntermediateDirectory)/DirList_skirdin.cpp$(ObjectSuffix): lib/DirList/skirdin.cpp $(IntermediateDirectory)/DirList_skirdin.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/skirdin.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_skirdin.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_skirdin.cpp$(DependSuffix): lib/DirList/skirdin.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_skirdin.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_skirdin.cpp$(DependSuffix) -MM "lib/DirList/skirdin.cpp"

$(IntermediateDirectory)/DirList_skirdin.cpp$(PreprocessSuffix): lib/DirList/skirdin.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_skirdin.cpp$(PreprocessSuffix) "lib/DirList/skirdin.cpp"

$(IntermediateDirectory)/DirList_eplf.cpp$(ObjectSuffix): lib/DirList/eplf.cpp $(IntermediateDirectory)/DirList_eplf.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/eplf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_eplf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_eplf.cpp$(DependSuffix): lib/DirList/eplf.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_eplf.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_eplf.cpp$(DependSuffix) -MM "lib/DirList/eplf.cpp"

$(IntermediateDirectory)/DirList_eplf.cpp$(PreprocessSuffix): lib/DirList/eplf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_eplf.cpp$(PreprocessSuffix) "lib/DirList/eplf.cpp"

$(IntermediateDirectory)/DirList_mvs.cpp$(ObjectSuffix): lib/DirList/mvs.cpp $(IntermediateDirectory)/DirList_mvs.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/mvs.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_mvs.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_mvs.cpp$(DependSuffix): lib/DirList/mvs.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_mvs.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_mvs.cpp$(DependSuffix) -MM "lib/DirList/mvs.cpp"

$(IntermediateDirectory)/DirList_mvs.cpp$(PreprocessSuffix): lib/DirList/mvs.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_mvs.cpp$(PreprocessSuffix) "lib/DirList/mvs.cpp"

$(IntermediateDirectory)/DirList_Main.cpp$(ObjectSuffix): lib/DirList/Main.cpp $(IntermediateDirectory)/DirList_Main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/Main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_Main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_Main.cpp$(DependSuffix): lib/DirList/Main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_Main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_Main.cpp$(DependSuffix) -MM "lib/DirList/Main.cpp"

$(IntermediateDirectory)/DirList_Main.cpp$(PreprocessSuffix): lib/DirList/Main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_Main.cpp$(PreprocessSuffix) "lib/DirList/Main.cpp"

$(IntermediateDirectory)/DirList_tcpc.cpp$(ObjectSuffix): lib/DirList/tcpc.cpp $(IntermediateDirectory)/DirList_tcpc.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/tcpc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_tcpc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_tcpc.cpp$(DependSuffix): lib/DirList/tcpc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_tcpc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_tcpc.cpp$(DependSuffix) -MM "lib/DirList/tcpc.cpp"

$(IntermediateDirectory)/DirList_tcpc.cpp$(PreprocessSuffix): lib/DirList/tcpc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_tcpc.cpp$(PreprocessSuffix) "lib/DirList/tcpc.cpp"

$(IntermediateDirectory)/DirList_cms.cpp$(ObjectSuffix): lib/DirList/cms.cpp $(IntermediateDirectory)/DirList_cms.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/DirList/cms.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/DirList_cms.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/DirList_cms.cpp$(DependSuffix): lib/DirList/cms.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/DirList_cms.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/DirList_cms.cpp$(DependSuffix) -MM "lib/DirList/cms.cpp"

$(IntermediateDirectory)/DirList_cms.cpp$(PreprocessSuffix): lib/DirList/cms.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/DirList_cms.cpp$(PreprocessSuffix) "lib/DirList/cms.cpp"

$(IntermediateDirectory)/Notify_Main.cpp$(ObjectSuffix): lib/Notify/Main.cpp $(IntermediateDirectory)/Notify_Main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/Notify/Main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Notify_Main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Notify_Main.cpp$(DependSuffix): lib/Notify/Main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Notify_Main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Notify_Main.cpp$(DependSuffix) -MM "lib/Notify/Main.cpp"

$(IntermediateDirectory)/Notify_Main.cpp$(PreprocessSuffix): lib/Notify/Main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Notify_Main.cpp$(PreprocessSuffix) "lib/Notify/Main.cpp"

$(IntermediateDirectory)/Progress_cbFmt.cpp$(ObjectSuffix): lib/Progress/cbFmt.cpp $(IntermediateDirectory)/Progress_cbFmt.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/Progress/cbFmt.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Progress_cbFmt.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Progress_cbFmt.cpp$(DependSuffix): lib/Progress/cbFmt.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Progress_cbFmt.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Progress_cbFmt.cpp$(DependSuffix) -MM "lib/Progress/cbFmt.cpp"

$(IntermediateDirectory)/Progress_cbFmt.cpp$(PreprocessSuffix): lib/Progress/cbFmt.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Progress_cbFmt.cpp$(PreprocessSuffix) "lib/Progress/cbFmt.cpp"

$(IntermediateDirectory)/Progress_TraficCB.cpp$(ObjectSuffix): lib/Progress/TraficCB.cpp $(IntermediateDirectory)/Progress_TraficCB.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/Progress/TraficCB.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Progress_TraficCB.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Progress_TraficCB.cpp$(DependSuffix): lib/Progress/TraficCB.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Progress_TraficCB.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Progress_TraficCB.cpp$(DependSuffix) -MM "lib/Progress/TraficCB.cpp"

$(IntermediateDirectory)/Progress_TraficCB.cpp$(PreprocessSuffix): lib/Progress/TraficCB.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Progress_TraficCB.cpp$(PreprocessSuffix) "lib/Progress/TraficCB.cpp"

$(IntermediateDirectory)/Progress_Main.cpp$(ObjectSuffix): lib/Progress/Main.cpp $(IntermediateDirectory)/Progress_Main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/Progress/Main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Progress_Main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Progress_Main.cpp$(DependSuffix): lib/Progress/Main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Progress_Main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Progress_Main.cpp$(DependSuffix) -MM "lib/Progress/Main.cpp"

$(IntermediateDirectory)/Progress_Main.cpp$(PreprocessSuffix): lib/Progress/Main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Progress_Main.cpp$(PreprocessSuffix) "lib/Progress/Main.cpp"

$(IntermediateDirectory)/Progress_Utils.cpp$(ObjectSuffix): lib/Progress/Utils.cpp $(IntermediateDirectory)/Progress_Utils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./lib/Progress/Utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Progress_Utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Progress_Utils.cpp$(DependSuffix): lib/Progress/Utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Progress_Utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Progress_Utils.cpp$(DependSuffix) -MM "lib/Progress/Utils.cpp"

$(IntermediateDirectory)/Progress_Utils.cpp$(PreprocessSuffix): lib/Progress/Utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Progress_Utils.cpp$(PreprocessSuffix) "lib/Progress/Utils.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_scr.cpp $(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_scr.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_scr.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_scr.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_scr.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_scr.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_scr.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_stdlibCS.cpp $(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_stdlibCS.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_stdlibCS.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_stdlibCS.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_stdlibCS.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_stdlibCS.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_stdlibCS.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_asrt.cpp $(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_asrt.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_asrt.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_asrt.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_asrt.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_asrt.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_asrt.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Patt.cpp $(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Patt.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Patt.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Patt.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Patt.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Patt.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Patt.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_OEM.cpp $(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_OEM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_OEM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_OEM.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_OEM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_OEM.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_OEM.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_SText.cpp $(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_SText.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_SText.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_SText.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_SText.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_SText.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_SText.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSCHC.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSCHC.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSCHC.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSCHC.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSCHC.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSCHC.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSCHC.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Reg.cpp $(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Reg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Reg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Reg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Reg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Reg.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Reg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_plg.cpp $(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_plg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_plg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_plg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_plg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_plg.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_plg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Con.cpp $(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Con.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Con.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Con.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Con.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Con.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Con.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSCPY.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSCPY.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSCPY.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSCPY.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSCPY.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSCPY.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSCPY.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_menu.cpp $(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_menu.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_menu.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_menu.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_menu.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_menu.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_menu.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSCAT.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSCAT.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSCAT.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSCAT.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSCAT.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSCAT.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSCAT.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_log.cpp $(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_log.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_log.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_log.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_log.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_log.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_log.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_mklog.cpp $(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_mklog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_mklog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_mklog.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_mklog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_mklog.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_mklog.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_String.cpp $(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_String.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_String.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_String.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_String.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_String.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_String.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_err.cpp $(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_err.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_err.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_err.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_err.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_err.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_err.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_mesg.cpp $(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_mesg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_mesg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_mesg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_mesg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_mesg.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_mesg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_SCol.cpp $(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_SCol.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_SCol.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_SCol.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_SCol.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_SCol.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_SCol.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Msg.cpp $(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Msg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Msg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Msg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Msg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Msg.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Msg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSCMP.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSCMP.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSCMP.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSCMP.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSCMP.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSCMP.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSCMP.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSPCH.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSPCH.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSPCH.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSPCH.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSPCH.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSPCH.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSPCH.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_crc32.cpp $(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_crc32.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_crc32.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_crc32.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_crc32.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_crc32.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_crc32.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exit.cpp $(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exit.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exit.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exit.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exit.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSNCH.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSNCH.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSNCH.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSNCH.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSNCH.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSNCH.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSNCH.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Dialog.cpp $(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Dialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Dialog.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Dialog.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Dialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Dialog.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Dialog.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_per.cpp $(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_per.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_per.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_per.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_per.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_per.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_per.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Utils.cpp $(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Utils.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Utils.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Utils.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_ilist.cpp $(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_ilist.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_ilist.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_ilist.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_ilist.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_ilist.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_ilist.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_Arg.cpp $(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_Arg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_Arg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_Arg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_Arg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_Arg.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_Arg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_FMsg.cpp $(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_FMsg.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_FMsg.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_FMsg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_FMsg.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_FMsg.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_FMsg.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_exSPS.cpp $(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_exSPS.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_exSPS.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_exSPS.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_exSPS.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_exSPS.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_exSPS.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(ObjectSuffix): src/FStdLib/FARStdlib/fstd_FUtils.cpp $(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/FStdLib/FARStdlib/fstd_FUtils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(DependSuffix): src/FStdLib/FARStdlib/fstd_FUtils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(DependSuffix) -MM "src/FStdLib/FARStdlib/fstd_FUtils.cpp"

$(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(PreprocessSuffix): src/FStdLib/FARStdlib/fstd_FUtils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/FARStdlib_fstd_FUtils.cpp$(PreprocessSuffix) "src/FStdLib/FARStdlib/fstd_FUtils.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


