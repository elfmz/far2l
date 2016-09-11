##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=netbox
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
OutputFile             :=../Build/Plugins/netbox/bin/$(ProjectName).far-plug-wide
Preprocessors          :=$(PreprocessorSwitch)NO_FILEZILLA $(PreprocessorSwitch)MPEXT $(PreprocessorSwitch)WINPORT_DIRECT 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="netbox.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  -Wl, $(shell wx-config --debug=yes --libs --unicode=yes) -export-dynamic
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)../WinPort $(IncludeSwitch)src/base $(IncludeSwitch)src/resource $(IncludeSwitch)src/core $(IncludeSwitch)src/PluginSDK/Far2 $(IncludeSwitch)src/windows $(IncludeSwitch)src/NetBox $(IncludeSwitch)libs/Putty $(IncludeSwitch)libs/Putty/unix $(IncludeSwitch)libs/Putty/charset $(IncludeSwitch)../utils/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)neon $(LibrarySwitch)tinyxml2 $(LibrarySwitch)ssl $(LibrarySwitch)crypto $(LibrarySwitch)dl $(LibrarySwitch)utils 
ArLibs                 :=  "neon" "tinyxml2" "ssl" "crypto" "dl" "utils" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)../WinPort/$(ConfigurationName) $(LibraryPathSwitch)../utils/$(ConfigurationName) 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -std=c++14 -fpic $(Preprocessors)
CFLAGS   :=  -g -fpic $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/base_Classes.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_Common.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_FileBuffer.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_Global.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_LibraryLoader.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_local.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_Masks.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_rtti.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_StrUtils.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_Sysutils.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/base_UnicodeString.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_WideStrUtils.cpp$(ObjectSuffix) $(IntermediateDirectory)/base_Exceptions.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_FarConfiguration.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_FarDialog.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_FarInterface.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_FarPlugin.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_FarPluginStrings.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_FarUtil.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_NetBox.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/NetBox_WinSCPDialogs.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_WinSCPFileSystem.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_WinSCPPlugin.cpp$(ObjectSuffix) $(IntermediateDirectory)/NetBox_XmlStorage.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_Bookmarks.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_Configuration.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_CopyParam.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_CoreMain.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_Cryptography.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_FileInfo.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/core_FileMasks.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_FileOperationProgress.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_FileSystems.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_FtpFileSystem.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_HierarchicalStorage.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_Http.cpp$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/core_NamedObjs.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_NeonIntf.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_Option.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_PuttyIntf.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/core_Queue.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_RemoteFiles.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_ScpFileSystem.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_SecureShell.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_SessionData.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_SessionInfo.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_SftpFileSystem.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_Terminal.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_WebDAVFileSystem.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_WinSCPSecurity.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/windows_GUIConfiguration.cpp$(ObjectSuffix) $(IntermediateDirectory)/windows_GUITools.cpp$(ObjectSuffix) $(IntermediateDirectory)/windows_ProgParams.cpp$(ObjectSuffix) $(IntermediateDirectory)/windows_SynchronizeController.cpp$(ObjectSuffix) $(IntermediateDirectory)/windows_Tools.cpp$(ObjectSuffix) $(IntermediateDirectory)/windows_WinInterface.cpp$(ObjectSuffix) $(IntermediateDirectory)/Putty_callback.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_conf.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_cproxy.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_dialog.c$(ObjectSuffix) \
	$(IntermediateDirectory)/Putty_errsock.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_import.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_int64.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_ldiscucs.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_logging.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_minibidi.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_misc.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_miscucs.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_noshare.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_pgssapi.c$(ObjectSuffix) \
	$(IntermediateDirectory)/Putty_portfwd.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_proxy.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_ssh.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshaes.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_ssharcf.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshblowf.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshbn.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshcrc.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshcrcda.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshdes.c$(ObjectSuffix) \
	$(IntermediateDirectory)/Putty_sshdh.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshdss.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshdssg.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshgssc.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshmd5.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshprime.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshpubk.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshrand.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshrsa.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshrsag.c$(ObjectSuffix) \
	$(IntermediateDirectory)/Putty_sshsh256.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshsh512.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshsha.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshshare.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshzlib.c$(ObjectSuffix) 

Objects2=$(IntermediateDirectory)/Putty_telnet.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_time.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_tree234.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_wcwidth.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_wildcard.c$(ObjectSuffix) \
	$(IntermediateDirectory)/Putty_x11fwd.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_sshecc.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_ldisc.c$(ObjectSuffix) $(IntermediateDirectory)/Putty_noterm.c$(ObjectSuffix) $(IntermediateDirectory)/charset_fromucs.c$(ObjectSuffix) $(IntermediateDirectory)/charset_localenc.c$(ObjectSuffix) $(IntermediateDirectory)/charset_macenc.c$(ObjectSuffix) $(IntermediateDirectory)/charset_mimeenc.c$(ObjectSuffix) $(IntermediateDirectory)/charset_sbcs.c$(ObjectSuffix) $(IntermediateDirectory)/charset_sbcsdat.c$(ObjectSuffix) \
	$(IntermediateDirectory)/charset_slookup.c$(ObjectSuffix) $(IntermediateDirectory)/charset_toucs.c$(ObjectSuffix) $(IntermediateDirectory)/charset_utf8.c$(ObjectSuffix) $(IntermediateDirectory)/charset_xenc.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxgen.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxgss.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxmisc.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxnet.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxnoise.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxpeer.c$(ObjectSuffix) \
	$(IntermediateDirectory)/unix_uxprint.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxproxy.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxser.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxsignal.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxstore.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxucs.c$(ObjectSuffix) $(IntermediateDirectory)/unix_xkeysym.c$(ObjectSuffix) $(IntermediateDirectory)/unix_xpmpucfg.c$(ObjectSuffix) $(IntermediateDirectory)/unix_xpmputty.c$(ObjectSuffix) $(IntermediateDirectory)/unix_uxagentc.c$(ObjectSuffix) \
	$(IntermediateDirectory)/unix_uxsel.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) $(Objects2) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d "../.build-debug/utils" $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	@echo $(Objects2) >> $(ObjectsFileList)
	$(SharedObjectLinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)
	@$(MakeDirCommand) "../.build-debug"
	@echo rebuilt > "../.build-debug/netbox"

"../.build-debug/utils":
	@$(MakeDirCommand) "../.build-debug"
	@echo stam > "../.build-debug/utils"




PostBuild:
	@echo Executing Post Build commands ...
	cp ./src/NetBox/*.lng ../Build/Plugins/netbox/bin/
	@echo Done

MakeIntermediateDirs:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d ./$(ConfigurationName) || $(MakeDirCommand) ./$(ConfigurationName)

PreBuild:
	@echo Executing Pre Build commands ...
	mkdir -p ../Build/Plugins/netbox/
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/base_Classes.cpp$(ObjectSuffix): src/base/Classes.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/Classes.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_Classes.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_Classes.cpp$(PreprocessSuffix): src/base/Classes.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_Classes.cpp$(PreprocessSuffix) "src/base/Classes.cpp"

$(IntermediateDirectory)/base_Common.cpp$(ObjectSuffix): src/base/Common.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/Common.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_Common.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_Common.cpp$(PreprocessSuffix): src/base/Common.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_Common.cpp$(PreprocessSuffix) "src/base/Common.cpp"

$(IntermediateDirectory)/base_FileBuffer.cpp$(ObjectSuffix): src/base/FileBuffer.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/FileBuffer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_FileBuffer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_FileBuffer.cpp$(PreprocessSuffix): src/base/FileBuffer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_FileBuffer.cpp$(PreprocessSuffix) "src/base/FileBuffer.cpp"

$(IntermediateDirectory)/base_Global.cpp$(ObjectSuffix): src/base/Global.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/Global.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_Global.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_Global.cpp$(PreprocessSuffix): src/base/Global.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_Global.cpp$(PreprocessSuffix) "src/base/Global.cpp"

$(IntermediateDirectory)/base_LibraryLoader.cpp$(ObjectSuffix): src/base/LibraryLoader.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/LibraryLoader.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_LibraryLoader.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_LibraryLoader.cpp$(PreprocessSuffix): src/base/LibraryLoader.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_LibraryLoader.cpp$(PreprocessSuffix) "src/base/LibraryLoader.cpp"

$(IntermediateDirectory)/base_local.cpp$(ObjectSuffix): src/base/local.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/local.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_local.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_local.cpp$(PreprocessSuffix): src/base/local.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_local.cpp$(PreprocessSuffix) "src/base/local.cpp"

$(IntermediateDirectory)/base_Masks.cpp$(ObjectSuffix): src/base/Masks.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/Masks.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_Masks.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_Masks.cpp$(PreprocessSuffix): src/base/Masks.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_Masks.cpp$(PreprocessSuffix) "src/base/Masks.cpp"

$(IntermediateDirectory)/base_rtti.cpp$(ObjectSuffix): src/base/rtti.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/rtti.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_rtti.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_rtti.cpp$(PreprocessSuffix): src/base/rtti.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_rtti.cpp$(PreprocessSuffix) "src/base/rtti.cpp"

$(IntermediateDirectory)/base_StrUtils.cpp$(ObjectSuffix): src/base/StrUtils.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/StrUtils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_StrUtils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_StrUtils.cpp$(PreprocessSuffix): src/base/StrUtils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_StrUtils.cpp$(PreprocessSuffix) "src/base/StrUtils.cpp"

$(IntermediateDirectory)/base_Sysutils.cpp$(ObjectSuffix): src/base/Sysutils.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/Sysutils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_Sysutils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_Sysutils.cpp$(PreprocessSuffix): src/base/Sysutils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_Sysutils.cpp$(PreprocessSuffix) "src/base/Sysutils.cpp"

$(IntermediateDirectory)/base_UnicodeString.cpp$(ObjectSuffix): src/base/UnicodeString.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/UnicodeString.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_UnicodeString.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_UnicodeString.cpp$(PreprocessSuffix): src/base/UnicodeString.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_UnicodeString.cpp$(PreprocessSuffix) "src/base/UnicodeString.cpp"

$(IntermediateDirectory)/base_WideStrUtils.cpp$(ObjectSuffix): src/base/WideStrUtils.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/WideStrUtils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_WideStrUtils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_WideStrUtils.cpp$(PreprocessSuffix): src/base/WideStrUtils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_WideStrUtils.cpp$(PreprocessSuffix) "src/base/WideStrUtils.cpp"

$(IntermediateDirectory)/base_Exceptions.cpp$(ObjectSuffix): src/base/Exceptions.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/base/Exceptions.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/base_Exceptions.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/base_Exceptions.cpp$(PreprocessSuffix): src/base/Exceptions.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/base_Exceptions.cpp$(PreprocessSuffix) "src/base/Exceptions.cpp"

$(IntermediateDirectory)/NetBox_FarConfiguration.cpp$(ObjectSuffix): src/NetBox/FarConfiguration.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/FarConfiguration.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_FarConfiguration.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_FarConfiguration.cpp$(PreprocessSuffix): src/NetBox/FarConfiguration.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_FarConfiguration.cpp$(PreprocessSuffix) "src/NetBox/FarConfiguration.cpp"

$(IntermediateDirectory)/NetBox_FarDialog.cpp$(ObjectSuffix): src/NetBox/FarDialog.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/FarDialog.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_FarDialog.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_FarDialog.cpp$(PreprocessSuffix): src/NetBox/FarDialog.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_FarDialog.cpp$(PreprocessSuffix) "src/NetBox/FarDialog.cpp"

$(IntermediateDirectory)/NetBox_FarInterface.cpp$(ObjectSuffix): src/NetBox/FarInterface.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/FarInterface.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_FarInterface.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_FarInterface.cpp$(PreprocessSuffix): src/NetBox/FarInterface.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_FarInterface.cpp$(PreprocessSuffix) "src/NetBox/FarInterface.cpp"

$(IntermediateDirectory)/NetBox_FarPlugin.cpp$(ObjectSuffix): src/NetBox/FarPlugin.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/FarPlugin.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_FarPlugin.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_FarPlugin.cpp$(PreprocessSuffix): src/NetBox/FarPlugin.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_FarPlugin.cpp$(PreprocessSuffix) "src/NetBox/FarPlugin.cpp"

$(IntermediateDirectory)/NetBox_FarPluginStrings.cpp$(ObjectSuffix): src/NetBox/FarPluginStrings.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/FarPluginStrings.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_FarPluginStrings.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_FarPluginStrings.cpp$(PreprocessSuffix): src/NetBox/FarPluginStrings.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_FarPluginStrings.cpp$(PreprocessSuffix) "src/NetBox/FarPluginStrings.cpp"

$(IntermediateDirectory)/NetBox_FarUtil.cpp$(ObjectSuffix): src/NetBox/FarUtil.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/FarUtil.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_FarUtil.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_FarUtil.cpp$(PreprocessSuffix): src/NetBox/FarUtil.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_FarUtil.cpp$(PreprocessSuffix) "src/NetBox/FarUtil.cpp"

$(IntermediateDirectory)/NetBox_NetBox.cpp$(ObjectSuffix): src/NetBox/NetBox.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/NetBox.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_NetBox.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_NetBox.cpp$(PreprocessSuffix): src/NetBox/NetBox.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_NetBox.cpp$(PreprocessSuffix) "src/NetBox/NetBox.cpp"

$(IntermediateDirectory)/NetBox_WinSCPDialogs.cpp$(ObjectSuffix): src/NetBox/WinSCPDialogs.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/WinSCPDialogs.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_WinSCPDialogs.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_WinSCPDialogs.cpp$(PreprocessSuffix): src/NetBox/WinSCPDialogs.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_WinSCPDialogs.cpp$(PreprocessSuffix) "src/NetBox/WinSCPDialogs.cpp"

$(IntermediateDirectory)/NetBox_WinSCPFileSystem.cpp$(ObjectSuffix): src/NetBox/WinSCPFileSystem.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/WinSCPFileSystem.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_WinSCPFileSystem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_WinSCPFileSystem.cpp$(PreprocessSuffix): src/NetBox/WinSCPFileSystem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_WinSCPFileSystem.cpp$(PreprocessSuffix) "src/NetBox/WinSCPFileSystem.cpp"

$(IntermediateDirectory)/NetBox_WinSCPPlugin.cpp$(ObjectSuffix): src/NetBox/WinSCPPlugin.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/WinSCPPlugin.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_WinSCPPlugin.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_WinSCPPlugin.cpp$(PreprocessSuffix): src/NetBox/WinSCPPlugin.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_WinSCPPlugin.cpp$(PreprocessSuffix) "src/NetBox/WinSCPPlugin.cpp"

$(IntermediateDirectory)/NetBox_XmlStorage.cpp$(ObjectSuffix): src/NetBox/XmlStorage.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/NetBox/XmlStorage.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/NetBox_XmlStorage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/NetBox_XmlStorage.cpp$(PreprocessSuffix): src/NetBox/XmlStorage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/NetBox_XmlStorage.cpp$(PreprocessSuffix) "src/NetBox/XmlStorage.cpp"

$(IntermediateDirectory)/core_Bookmarks.cpp$(ObjectSuffix): src/core/Bookmarks.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Bookmarks.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Bookmarks.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Bookmarks.cpp$(PreprocessSuffix): src/core/Bookmarks.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Bookmarks.cpp$(PreprocessSuffix) "src/core/Bookmarks.cpp"

$(IntermediateDirectory)/core_Configuration.cpp$(ObjectSuffix): src/core/Configuration.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Configuration.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Configuration.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Configuration.cpp$(PreprocessSuffix): src/core/Configuration.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Configuration.cpp$(PreprocessSuffix) "src/core/Configuration.cpp"

$(IntermediateDirectory)/core_CopyParam.cpp$(ObjectSuffix): src/core/CopyParam.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/CopyParam.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_CopyParam.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_CopyParam.cpp$(PreprocessSuffix): src/core/CopyParam.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_CopyParam.cpp$(PreprocessSuffix) "src/core/CopyParam.cpp"

$(IntermediateDirectory)/core_CoreMain.cpp$(ObjectSuffix): src/core/CoreMain.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/CoreMain.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_CoreMain.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_CoreMain.cpp$(PreprocessSuffix): src/core/CoreMain.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_CoreMain.cpp$(PreprocessSuffix) "src/core/CoreMain.cpp"

$(IntermediateDirectory)/core_Cryptography.cpp$(ObjectSuffix): src/core/Cryptography.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Cryptography.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Cryptography.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Cryptography.cpp$(PreprocessSuffix): src/core/Cryptography.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Cryptography.cpp$(PreprocessSuffix) "src/core/Cryptography.cpp"

$(IntermediateDirectory)/core_FileInfo.cpp$(ObjectSuffix): src/core/FileInfo.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/FileInfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_FileInfo.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_FileInfo.cpp$(PreprocessSuffix): src/core/FileInfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_FileInfo.cpp$(PreprocessSuffix) "src/core/FileInfo.cpp"

$(IntermediateDirectory)/core_FileMasks.cpp$(ObjectSuffix): src/core/FileMasks.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/FileMasks.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_FileMasks.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_FileMasks.cpp$(PreprocessSuffix): src/core/FileMasks.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_FileMasks.cpp$(PreprocessSuffix) "src/core/FileMasks.cpp"

$(IntermediateDirectory)/core_FileOperationProgress.cpp$(ObjectSuffix): src/core/FileOperationProgress.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/FileOperationProgress.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_FileOperationProgress.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_FileOperationProgress.cpp$(PreprocessSuffix): src/core/FileOperationProgress.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_FileOperationProgress.cpp$(PreprocessSuffix) "src/core/FileOperationProgress.cpp"

$(IntermediateDirectory)/core_FileSystems.cpp$(ObjectSuffix): src/core/FileSystems.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/FileSystems.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_FileSystems.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_FileSystems.cpp$(PreprocessSuffix): src/core/FileSystems.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_FileSystems.cpp$(PreprocessSuffix) "src/core/FileSystems.cpp"

$(IntermediateDirectory)/core_FtpFileSystem.cpp$(ObjectSuffix): src/core/FtpFileSystem.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/FtpFileSystem.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_FtpFileSystem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_FtpFileSystem.cpp$(PreprocessSuffix): src/core/FtpFileSystem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_FtpFileSystem.cpp$(PreprocessSuffix) "src/core/FtpFileSystem.cpp"

$(IntermediateDirectory)/core_HierarchicalStorage.cpp$(ObjectSuffix): src/core/HierarchicalStorage.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/HierarchicalStorage.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_HierarchicalStorage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_HierarchicalStorage.cpp$(PreprocessSuffix): src/core/HierarchicalStorage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_HierarchicalStorage.cpp$(PreprocessSuffix) "src/core/HierarchicalStorage.cpp"

$(IntermediateDirectory)/core_Http.cpp$(ObjectSuffix): src/core/Http.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Http.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Http.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Http.cpp$(PreprocessSuffix): src/core/Http.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Http.cpp$(PreprocessSuffix) "src/core/Http.cpp"

$(IntermediateDirectory)/core_NamedObjs.cpp$(ObjectSuffix): src/core/NamedObjs.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/NamedObjs.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_NamedObjs.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_NamedObjs.cpp$(PreprocessSuffix): src/core/NamedObjs.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_NamedObjs.cpp$(PreprocessSuffix) "src/core/NamedObjs.cpp"

$(IntermediateDirectory)/core_NeonIntf.cpp$(ObjectSuffix): src/core/NeonIntf.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/NeonIntf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_NeonIntf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_NeonIntf.cpp$(PreprocessSuffix): src/core/NeonIntf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_NeonIntf.cpp$(PreprocessSuffix) "src/core/NeonIntf.cpp"

$(IntermediateDirectory)/core_Option.cpp$(ObjectSuffix): src/core/Option.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Option.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Option.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Option.cpp$(PreprocessSuffix): src/core/Option.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Option.cpp$(PreprocessSuffix) "src/core/Option.cpp"

$(IntermediateDirectory)/core_PuttyIntf.cpp$(ObjectSuffix): src/core/PuttyIntf.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/PuttyIntf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_PuttyIntf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_PuttyIntf.cpp$(PreprocessSuffix): src/core/PuttyIntf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_PuttyIntf.cpp$(PreprocessSuffix) "src/core/PuttyIntf.cpp"

$(IntermediateDirectory)/core_Queue.cpp$(ObjectSuffix): src/core/Queue.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Queue.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Queue.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Queue.cpp$(PreprocessSuffix): src/core/Queue.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Queue.cpp$(PreprocessSuffix) "src/core/Queue.cpp"

$(IntermediateDirectory)/core_RemoteFiles.cpp$(ObjectSuffix): src/core/RemoteFiles.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/RemoteFiles.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_RemoteFiles.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_RemoteFiles.cpp$(PreprocessSuffix): src/core/RemoteFiles.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_RemoteFiles.cpp$(PreprocessSuffix) "src/core/RemoteFiles.cpp"

$(IntermediateDirectory)/core_ScpFileSystem.cpp$(ObjectSuffix): src/core/ScpFileSystem.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/ScpFileSystem.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_ScpFileSystem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_ScpFileSystem.cpp$(PreprocessSuffix): src/core/ScpFileSystem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_ScpFileSystem.cpp$(PreprocessSuffix) "src/core/ScpFileSystem.cpp"

$(IntermediateDirectory)/core_SecureShell.cpp$(ObjectSuffix): src/core/SecureShell.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/SecureShell.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_SecureShell.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_SecureShell.cpp$(PreprocessSuffix): src/core/SecureShell.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_SecureShell.cpp$(PreprocessSuffix) "src/core/SecureShell.cpp"

$(IntermediateDirectory)/core_SessionData.cpp$(ObjectSuffix): src/core/SessionData.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/SessionData.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_SessionData.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_SessionData.cpp$(PreprocessSuffix): src/core/SessionData.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_SessionData.cpp$(PreprocessSuffix) "src/core/SessionData.cpp"

$(IntermediateDirectory)/core_SessionInfo.cpp$(ObjectSuffix): src/core/SessionInfo.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/SessionInfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_SessionInfo.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_SessionInfo.cpp$(PreprocessSuffix): src/core/SessionInfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_SessionInfo.cpp$(PreprocessSuffix) "src/core/SessionInfo.cpp"

$(IntermediateDirectory)/core_SftpFileSystem.cpp$(ObjectSuffix): src/core/SftpFileSystem.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/SftpFileSystem.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_SftpFileSystem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_SftpFileSystem.cpp$(PreprocessSuffix): src/core/SftpFileSystem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_SftpFileSystem.cpp$(PreprocessSuffix) "src/core/SftpFileSystem.cpp"

$(IntermediateDirectory)/core_Terminal.cpp$(ObjectSuffix): src/core/Terminal.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/Terminal.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_Terminal.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_Terminal.cpp$(PreprocessSuffix): src/core/Terminal.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_Terminal.cpp$(PreprocessSuffix) "src/core/Terminal.cpp"

$(IntermediateDirectory)/core_WebDAVFileSystem.cpp$(ObjectSuffix): src/core/WebDAVFileSystem.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/WebDAVFileSystem.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_WebDAVFileSystem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_WebDAVFileSystem.cpp$(PreprocessSuffix): src/core/WebDAVFileSystem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_WebDAVFileSystem.cpp$(PreprocessSuffix) "src/core/WebDAVFileSystem.cpp"

$(IntermediateDirectory)/core_WinSCPSecurity.cpp$(ObjectSuffix): src/core/WinSCPSecurity.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/core/WinSCPSecurity.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_WinSCPSecurity.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_WinSCPSecurity.cpp$(PreprocessSuffix): src/core/WinSCPSecurity.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_WinSCPSecurity.cpp$(PreprocessSuffix) "src/core/WinSCPSecurity.cpp"

$(IntermediateDirectory)/windows_GUIConfiguration.cpp$(ObjectSuffix): src/windows/GUIConfiguration.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/windows/GUIConfiguration.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/windows_GUIConfiguration.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/windows_GUIConfiguration.cpp$(PreprocessSuffix): src/windows/GUIConfiguration.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/windows_GUIConfiguration.cpp$(PreprocessSuffix) "src/windows/GUIConfiguration.cpp"

$(IntermediateDirectory)/windows_GUITools.cpp$(ObjectSuffix): src/windows/GUITools.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/windows/GUITools.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/windows_GUITools.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/windows_GUITools.cpp$(PreprocessSuffix): src/windows/GUITools.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/windows_GUITools.cpp$(PreprocessSuffix) "src/windows/GUITools.cpp"

$(IntermediateDirectory)/windows_ProgParams.cpp$(ObjectSuffix): src/windows/ProgParams.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/windows/ProgParams.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/windows_ProgParams.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/windows_ProgParams.cpp$(PreprocessSuffix): src/windows/ProgParams.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/windows_ProgParams.cpp$(PreprocessSuffix) "src/windows/ProgParams.cpp"

$(IntermediateDirectory)/windows_SynchronizeController.cpp$(ObjectSuffix): src/windows/SynchronizeController.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/windows/SynchronizeController.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/windows_SynchronizeController.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/windows_SynchronizeController.cpp$(PreprocessSuffix): src/windows/SynchronizeController.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/windows_SynchronizeController.cpp$(PreprocessSuffix) "src/windows/SynchronizeController.cpp"

$(IntermediateDirectory)/windows_Tools.cpp$(ObjectSuffix): src/windows/Tools.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/windows/Tools.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/windows_Tools.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/windows_Tools.cpp$(PreprocessSuffix): src/windows/Tools.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/windows_Tools.cpp$(PreprocessSuffix) "src/windows/Tools.cpp"

$(IntermediateDirectory)/windows_WinInterface.cpp$(ObjectSuffix): src/windows/WinInterface.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "./src/windows/WinInterface.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/windows_WinInterface.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/windows_WinInterface.cpp$(PreprocessSuffix): src/windows/WinInterface.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/windows_WinInterface.cpp$(PreprocessSuffix) "src/windows/WinInterface.cpp"

$(IntermediateDirectory)/Putty_callback.c$(ObjectSuffix): libs/Putty/callback.c 
	$(CC) $(SourceSwitch) "./libs/Putty/callback.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_callback.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_callback.c$(PreprocessSuffix): libs/Putty/callback.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_callback.c$(PreprocessSuffix) "libs/Putty/callback.c"

$(IntermediateDirectory)/Putty_conf.c$(ObjectSuffix): libs/Putty/conf.c 
	$(CC) $(SourceSwitch) "./libs/Putty/conf.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_conf.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_conf.c$(PreprocessSuffix): libs/Putty/conf.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_conf.c$(PreprocessSuffix) "libs/Putty/conf.c"

$(IntermediateDirectory)/Putty_cproxy.c$(ObjectSuffix): libs/Putty/cproxy.c 
	$(CC) $(SourceSwitch) "./libs/Putty/cproxy.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_cproxy.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_cproxy.c$(PreprocessSuffix): libs/Putty/cproxy.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_cproxy.c$(PreprocessSuffix) "libs/Putty/cproxy.c"

$(IntermediateDirectory)/Putty_dialog.c$(ObjectSuffix): libs/Putty/dialog.c 
	$(CC) $(SourceSwitch) "./libs/Putty/dialog.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_dialog.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_dialog.c$(PreprocessSuffix): libs/Putty/dialog.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_dialog.c$(PreprocessSuffix) "libs/Putty/dialog.c"

$(IntermediateDirectory)/Putty_errsock.c$(ObjectSuffix): libs/Putty/errsock.c 
	$(CC) $(SourceSwitch) "./libs/Putty/errsock.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_errsock.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_errsock.c$(PreprocessSuffix): libs/Putty/errsock.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_errsock.c$(PreprocessSuffix) "libs/Putty/errsock.c"

$(IntermediateDirectory)/Putty_import.c$(ObjectSuffix): libs/Putty/import.c 
	$(CC) $(SourceSwitch) "./libs/Putty/import.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_import.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_import.c$(PreprocessSuffix): libs/Putty/import.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_import.c$(PreprocessSuffix) "libs/Putty/import.c"

$(IntermediateDirectory)/Putty_int64.c$(ObjectSuffix): libs/Putty/int64.c 
	$(CC) $(SourceSwitch) "./libs/Putty/int64.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_int64.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_int64.c$(PreprocessSuffix): libs/Putty/int64.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_int64.c$(PreprocessSuffix) "libs/Putty/int64.c"

$(IntermediateDirectory)/Putty_ldiscucs.c$(ObjectSuffix): libs/Putty/ldiscucs.c 
	$(CC) $(SourceSwitch) "./libs/Putty/ldiscucs.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_ldiscucs.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_ldiscucs.c$(PreprocessSuffix): libs/Putty/ldiscucs.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_ldiscucs.c$(PreprocessSuffix) "libs/Putty/ldiscucs.c"

$(IntermediateDirectory)/Putty_logging.c$(ObjectSuffix): libs/Putty/logging.c 
	$(CC) $(SourceSwitch) "./libs/Putty/logging.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_logging.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_logging.c$(PreprocessSuffix): libs/Putty/logging.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_logging.c$(PreprocessSuffix) "libs/Putty/logging.c"

$(IntermediateDirectory)/Putty_minibidi.c$(ObjectSuffix): libs/Putty/minibidi.c 
	$(CC) $(SourceSwitch) "./libs/Putty/minibidi.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_minibidi.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_minibidi.c$(PreprocessSuffix): libs/Putty/minibidi.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_minibidi.c$(PreprocessSuffix) "libs/Putty/minibidi.c"

$(IntermediateDirectory)/Putty_misc.c$(ObjectSuffix): libs/Putty/misc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/misc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_misc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_misc.c$(PreprocessSuffix): libs/Putty/misc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_misc.c$(PreprocessSuffix) "libs/Putty/misc.c"

$(IntermediateDirectory)/Putty_miscucs.c$(ObjectSuffix): libs/Putty/miscucs.c 
	$(CC) $(SourceSwitch) "./libs/Putty/miscucs.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_miscucs.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_miscucs.c$(PreprocessSuffix): libs/Putty/miscucs.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_miscucs.c$(PreprocessSuffix) "libs/Putty/miscucs.c"

$(IntermediateDirectory)/Putty_noshare.c$(ObjectSuffix): libs/Putty/noshare.c 
	$(CC) $(SourceSwitch) "./libs/Putty/noshare.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_noshare.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_noshare.c$(PreprocessSuffix): libs/Putty/noshare.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_noshare.c$(PreprocessSuffix) "libs/Putty/noshare.c"

$(IntermediateDirectory)/Putty_pgssapi.c$(ObjectSuffix): libs/Putty/pgssapi.c 
	$(CC) $(SourceSwitch) "./libs/Putty/pgssapi.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_pgssapi.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_pgssapi.c$(PreprocessSuffix): libs/Putty/pgssapi.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_pgssapi.c$(PreprocessSuffix) "libs/Putty/pgssapi.c"

$(IntermediateDirectory)/Putty_portfwd.c$(ObjectSuffix): libs/Putty/portfwd.c 
	$(CC) $(SourceSwitch) "./libs/Putty/portfwd.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_portfwd.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_portfwd.c$(PreprocessSuffix): libs/Putty/portfwd.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_portfwd.c$(PreprocessSuffix) "libs/Putty/portfwd.c"

$(IntermediateDirectory)/Putty_proxy.c$(ObjectSuffix): libs/Putty/proxy.c 
	$(CC) $(SourceSwitch) "./libs/Putty/proxy.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_proxy.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_proxy.c$(PreprocessSuffix): libs/Putty/proxy.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_proxy.c$(PreprocessSuffix) "libs/Putty/proxy.c"

$(IntermediateDirectory)/Putty_ssh.c$(ObjectSuffix): libs/Putty/ssh.c 
	$(CC) $(SourceSwitch) "./libs/Putty/ssh.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_ssh.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_ssh.c$(PreprocessSuffix): libs/Putty/ssh.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_ssh.c$(PreprocessSuffix) "libs/Putty/ssh.c"

$(IntermediateDirectory)/Putty_sshaes.c$(ObjectSuffix): libs/Putty/sshaes.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshaes.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshaes.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshaes.c$(PreprocessSuffix): libs/Putty/sshaes.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshaes.c$(PreprocessSuffix) "libs/Putty/sshaes.c"

$(IntermediateDirectory)/Putty_ssharcf.c$(ObjectSuffix): libs/Putty/ssharcf.c 
	$(CC) $(SourceSwitch) "./libs/Putty/ssharcf.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_ssharcf.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_ssharcf.c$(PreprocessSuffix): libs/Putty/ssharcf.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_ssharcf.c$(PreprocessSuffix) "libs/Putty/ssharcf.c"

$(IntermediateDirectory)/Putty_sshblowf.c$(ObjectSuffix): libs/Putty/sshblowf.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshblowf.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshblowf.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshblowf.c$(PreprocessSuffix): libs/Putty/sshblowf.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshblowf.c$(PreprocessSuffix) "libs/Putty/sshblowf.c"

$(IntermediateDirectory)/Putty_sshbn.c$(ObjectSuffix): libs/Putty/sshbn.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshbn.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshbn.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshbn.c$(PreprocessSuffix): libs/Putty/sshbn.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshbn.c$(PreprocessSuffix) "libs/Putty/sshbn.c"

$(IntermediateDirectory)/Putty_sshcrc.c$(ObjectSuffix): libs/Putty/sshcrc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshcrc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshcrc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshcrc.c$(PreprocessSuffix): libs/Putty/sshcrc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshcrc.c$(PreprocessSuffix) "libs/Putty/sshcrc.c"

$(IntermediateDirectory)/Putty_sshcrcda.c$(ObjectSuffix): libs/Putty/sshcrcda.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshcrcda.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshcrcda.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshcrcda.c$(PreprocessSuffix): libs/Putty/sshcrcda.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshcrcda.c$(PreprocessSuffix) "libs/Putty/sshcrcda.c"

$(IntermediateDirectory)/Putty_sshdes.c$(ObjectSuffix): libs/Putty/sshdes.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshdes.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshdes.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshdes.c$(PreprocessSuffix): libs/Putty/sshdes.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshdes.c$(PreprocessSuffix) "libs/Putty/sshdes.c"

$(IntermediateDirectory)/Putty_sshdh.c$(ObjectSuffix): libs/Putty/sshdh.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshdh.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshdh.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshdh.c$(PreprocessSuffix): libs/Putty/sshdh.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshdh.c$(PreprocessSuffix) "libs/Putty/sshdh.c"

$(IntermediateDirectory)/Putty_sshdss.c$(ObjectSuffix): libs/Putty/sshdss.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshdss.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshdss.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshdss.c$(PreprocessSuffix): libs/Putty/sshdss.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshdss.c$(PreprocessSuffix) "libs/Putty/sshdss.c"

$(IntermediateDirectory)/Putty_sshdssg.c$(ObjectSuffix): libs/Putty/sshdssg.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshdssg.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshdssg.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshdssg.c$(PreprocessSuffix): libs/Putty/sshdssg.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshdssg.c$(PreprocessSuffix) "libs/Putty/sshdssg.c"

$(IntermediateDirectory)/Putty_sshgssc.c$(ObjectSuffix): libs/Putty/sshgssc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshgssc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshgssc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshgssc.c$(PreprocessSuffix): libs/Putty/sshgssc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshgssc.c$(PreprocessSuffix) "libs/Putty/sshgssc.c"

$(IntermediateDirectory)/Putty_sshmd5.c$(ObjectSuffix): libs/Putty/sshmd5.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshmd5.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshmd5.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshmd5.c$(PreprocessSuffix): libs/Putty/sshmd5.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshmd5.c$(PreprocessSuffix) "libs/Putty/sshmd5.c"

$(IntermediateDirectory)/Putty_sshprime.c$(ObjectSuffix): libs/Putty/sshprime.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshprime.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshprime.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshprime.c$(PreprocessSuffix): libs/Putty/sshprime.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshprime.c$(PreprocessSuffix) "libs/Putty/sshprime.c"

$(IntermediateDirectory)/Putty_sshpubk.c$(ObjectSuffix): libs/Putty/sshpubk.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshpubk.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshpubk.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshpubk.c$(PreprocessSuffix): libs/Putty/sshpubk.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshpubk.c$(PreprocessSuffix) "libs/Putty/sshpubk.c"

$(IntermediateDirectory)/Putty_sshrand.c$(ObjectSuffix): libs/Putty/sshrand.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshrand.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshrand.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshrand.c$(PreprocessSuffix): libs/Putty/sshrand.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshrand.c$(PreprocessSuffix) "libs/Putty/sshrand.c"

$(IntermediateDirectory)/Putty_sshrsa.c$(ObjectSuffix): libs/Putty/sshrsa.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshrsa.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshrsa.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshrsa.c$(PreprocessSuffix): libs/Putty/sshrsa.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshrsa.c$(PreprocessSuffix) "libs/Putty/sshrsa.c"

$(IntermediateDirectory)/Putty_sshrsag.c$(ObjectSuffix): libs/Putty/sshrsag.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshrsag.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshrsag.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshrsag.c$(PreprocessSuffix): libs/Putty/sshrsag.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshrsag.c$(PreprocessSuffix) "libs/Putty/sshrsag.c"

$(IntermediateDirectory)/Putty_sshsh256.c$(ObjectSuffix): libs/Putty/sshsh256.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshsh256.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshsh256.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshsh256.c$(PreprocessSuffix): libs/Putty/sshsh256.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshsh256.c$(PreprocessSuffix) "libs/Putty/sshsh256.c"

$(IntermediateDirectory)/Putty_sshsh512.c$(ObjectSuffix): libs/Putty/sshsh512.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshsh512.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshsh512.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshsh512.c$(PreprocessSuffix): libs/Putty/sshsh512.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshsh512.c$(PreprocessSuffix) "libs/Putty/sshsh512.c"

$(IntermediateDirectory)/Putty_sshsha.c$(ObjectSuffix): libs/Putty/sshsha.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshsha.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshsha.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshsha.c$(PreprocessSuffix): libs/Putty/sshsha.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshsha.c$(PreprocessSuffix) "libs/Putty/sshsha.c"

$(IntermediateDirectory)/Putty_sshshare.c$(ObjectSuffix): libs/Putty/sshshare.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshshare.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshshare.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshshare.c$(PreprocessSuffix): libs/Putty/sshshare.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshshare.c$(PreprocessSuffix) "libs/Putty/sshshare.c"

$(IntermediateDirectory)/Putty_sshzlib.c$(ObjectSuffix): libs/Putty/sshzlib.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshzlib.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshzlib.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshzlib.c$(PreprocessSuffix): libs/Putty/sshzlib.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshzlib.c$(PreprocessSuffix) "libs/Putty/sshzlib.c"

$(IntermediateDirectory)/Putty_telnet.c$(ObjectSuffix): libs/Putty/telnet.c 
	$(CC) $(SourceSwitch) "./libs/Putty/telnet.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_telnet.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_telnet.c$(PreprocessSuffix): libs/Putty/telnet.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_telnet.c$(PreprocessSuffix) "libs/Putty/telnet.c"

$(IntermediateDirectory)/Putty_time.c$(ObjectSuffix): libs/Putty/time.c 
	$(CC) $(SourceSwitch) "./libs/Putty/time.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_time.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_time.c$(PreprocessSuffix): libs/Putty/time.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_time.c$(PreprocessSuffix) "libs/Putty/time.c"

$(IntermediateDirectory)/Putty_tree234.c$(ObjectSuffix): libs/Putty/tree234.c 
	$(CC) $(SourceSwitch) "./libs/Putty/tree234.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_tree234.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_tree234.c$(PreprocessSuffix): libs/Putty/tree234.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_tree234.c$(PreprocessSuffix) "libs/Putty/tree234.c"

$(IntermediateDirectory)/Putty_wcwidth.c$(ObjectSuffix): libs/Putty/wcwidth.c 
	$(CC) $(SourceSwitch) "./libs/Putty/wcwidth.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_wcwidth.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_wcwidth.c$(PreprocessSuffix): libs/Putty/wcwidth.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_wcwidth.c$(PreprocessSuffix) "libs/Putty/wcwidth.c"

$(IntermediateDirectory)/Putty_wildcard.c$(ObjectSuffix): libs/Putty/wildcard.c 
	$(CC) $(SourceSwitch) "./libs/Putty/wildcard.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_wildcard.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_wildcard.c$(PreprocessSuffix): libs/Putty/wildcard.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_wildcard.c$(PreprocessSuffix) "libs/Putty/wildcard.c"

$(IntermediateDirectory)/Putty_x11fwd.c$(ObjectSuffix): libs/Putty/x11fwd.c 
	$(CC) $(SourceSwitch) "./libs/Putty/x11fwd.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_x11fwd.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_x11fwd.c$(PreprocessSuffix): libs/Putty/x11fwd.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_x11fwd.c$(PreprocessSuffix) "libs/Putty/x11fwd.c"

$(IntermediateDirectory)/Putty_sshecc.c$(ObjectSuffix): libs/Putty/sshecc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/sshecc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_sshecc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_sshecc.c$(PreprocessSuffix): libs/Putty/sshecc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_sshecc.c$(PreprocessSuffix) "libs/Putty/sshecc.c"

$(IntermediateDirectory)/Putty_ldisc.c$(ObjectSuffix): libs/Putty/ldisc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/ldisc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_ldisc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_ldisc.c$(PreprocessSuffix): libs/Putty/ldisc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_ldisc.c$(PreprocessSuffix) "libs/Putty/ldisc.c"

$(IntermediateDirectory)/Putty_noterm.c$(ObjectSuffix): libs/Putty/noterm.c 
	$(CC) $(SourceSwitch) "./libs/Putty/noterm.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Putty_noterm.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Putty_noterm.c$(PreprocessSuffix): libs/Putty/noterm.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Putty_noterm.c$(PreprocessSuffix) "libs/Putty/noterm.c"

$(IntermediateDirectory)/charset_fromucs.c$(ObjectSuffix): libs/Putty/charset/fromucs.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/fromucs.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_fromucs.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_fromucs.c$(PreprocessSuffix): libs/Putty/charset/fromucs.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_fromucs.c$(PreprocessSuffix) "libs/Putty/charset/fromucs.c"

$(IntermediateDirectory)/charset_localenc.c$(ObjectSuffix): libs/Putty/charset/localenc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/localenc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_localenc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_localenc.c$(PreprocessSuffix): libs/Putty/charset/localenc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_localenc.c$(PreprocessSuffix) "libs/Putty/charset/localenc.c"

$(IntermediateDirectory)/charset_macenc.c$(ObjectSuffix): libs/Putty/charset/macenc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/macenc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_macenc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_macenc.c$(PreprocessSuffix): libs/Putty/charset/macenc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_macenc.c$(PreprocessSuffix) "libs/Putty/charset/macenc.c"

$(IntermediateDirectory)/charset_mimeenc.c$(ObjectSuffix): libs/Putty/charset/mimeenc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/mimeenc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_mimeenc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_mimeenc.c$(PreprocessSuffix): libs/Putty/charset/mimeenc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_mimeenc.c$(PreprocessSuffix) "libs/Putty/charset/mimeenc.c"

$(IntermediateDirectory)/charset_sbcs.c$(ObjectSuffix): libs/Putty/charset/sbcs.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/sbcs.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_sbcs.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_sbcs.c$(PreprocessSuffix): libs/Putty/charset/sbcs.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_sbcs.c$(PreprocessSuffix) "libs/Putty/charset/sbcs.c"

$(IntermediateDirectory)/charset_sbcsdat.c$(ObjectSuffix): libs/Putty/charset/sbcsdat.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/sbcsdat.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_sbcsdat.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_sbcsdat.c$(PreprocessSuffix): libs/Putty/charset/sbcsdat.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_sbcsdat.c$(PreprocessSuffix) "libs/Putty/charset/sbcsdat.c"

$(IntermediateDirectory)/charset_slookup.c$(ObjectSuffix): libs/Putty/charset/slookup.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/slookup.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_slookup.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_slookup.c$(PreprocessSuffix): libs/Putty/charset/slookup.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_slookup.c$(PreprocessSuffix) "libs/Putty/charset/slookup.c"

$(IntermediateDirectory)/charset_toucs.c$(ObjectSuffix): libs/Putty/charset/toucs.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/toucs.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_toucs.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_toucs.c$(PreprocessSuffix): libs/Putty/charset/toucs.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_toucs.c$(PreprocessSuffix) "libs/Putty/charset/toucs.c"

$(IntermediateDirectory)/charset_utf8.c$(ObjectSuffix): libs/Putty/charset/utf8.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/utf8.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_utf8.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_utf8.c$(PreprocessSuffix): libs/Putty/charset/utf8.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_utf8.c$(PreprocessSuffix) "libs/Putty/charset/utf8.c"

$(IntermediateDirectory)/charset_xenc.c$(ObjectSuffix): libs/Putty/charset/xenc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/charset/xenc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/charset_xenc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/charset_xenc.c$(PreprocessSuffix): libs/Putty/charset/xenc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/charset_xenc.c$(PreprocessSuffix) "libs/Putty/charset/xenc.c"

$(IntermediateDirectory)/unix_uxgen.c$(ObjectSuffix): libs/Putty/unix/uxgen.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxgen.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxgen.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxgen.c$(PreprocessSuffix): libs/Putty/unix/uxgen.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxgen.c$(PreprocessSuffix) "libs/Putty/unix/uxgen.c"

$(IntermediateDirectory)/unix_uxgss.c$(ObjectSuffix): libs/Putty/unix/uxgss.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxgss.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxgss.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxgss.c$(PreprocessSuffix): libs/Putty/unix/uxgss.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxgss.c$(PreprocessSuffix) "libs/Putty/unix/uxgss.c"

$(IntermediateDirectory)/unix_uxmisc.c$(ObjectSuffix): libs/Putty/unix/uxmisc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxmisc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxmisc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxmisc.c$(PreprocessSuffix): libs/Putty/unix/uxmisc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxmisc.c$(PreprocessSuffix) "libs/Putty/unix/uxmisc.c"

$(IntermediateDirectory)/unix_uxnet.c$(ObjectSuffix): libs/Putty/unix/uxnet.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxnet.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxnet.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxnet.c$(PreprocessSuffix): libs/Putty/unix/uxnet.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxnet.c$(PreprocessSuffix) "libs/Putty/unix/uxnet.c"

$(IntermediateDirectory)/unix_uxnoise.c$(ObjectSuffix): libs/Putty/unix/uxnoise.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxnoise.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxnoise.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxnoise.c$(PreprocessSuffix): libs/Putty/unix/uxnoise.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxnoise.c$(PreprocessSuffix) "libs/Putty/unix/uxnoise.c"

$(IntermediateDirectory)/unix_uxpeer.c$(ObjectSuffix): libs/Putty/unix/uxpeer.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxpeer.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxpeer.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxpeer.c$(PreprocessSuffix): libs/Putty/unix/uxpeer.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxpeer.c$(PreprocessSuffix) "libs/Putty/unix/uxpeer.c"

$(IntermediateDirectory)/unix_uxprint.c$(ObjectSuffix): libs/Putty/unix/uxprint.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxprint.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxprint.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxprint.c$(PreprocessSuffix): libs/Putty/unix/uxprint.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxprint.c$(PreprocessSuffix) "libs/Putty/unix/uxprint.c"

$(IntermediateDirectory)/unix_uxproxy.c$(ObjectSuffix): libs/Putty/unix/uxproxy.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxproxy.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxproxy.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxproxy.c$(PreprocessSuffix): libs/Putty/unix/uxproxy.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxproxy.c$(PreprocessSuffix) "libs/Putty/unix/uxproxy.c"

$(IntermediateDirectory)/unix_uxser.c$(ObjectSuffix): libs/Putty/unix/uxser.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxser.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxser.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxser.c$(PreprocessSuffix): libs/Putty/unix/uxser.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxser.c$(PreprocessSuffix) "libs/Putty/unix/uxser.c"

$(IntermediateDirectory)/unix_uxsignal.c$(ObjectSuffix): libs/Putty/unix/uxsignal.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxsignal.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxsignal.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxsignal.c$(PreprocessSuffix): libs/Putty/unix/uxsignal.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxsignal.c$(PreprocessSuffix) "libs/Putty/unix/uxsignal.c"

$(IntermediateDirectory)/unix_uxstore.c$(ObjectSuffix): libs/Putty/unix/uxstore.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxstore.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxstore.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxstore.c$(PreprocessSuffix): libs/Putty/unix/uxstore.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxstore.c$(PreprocessSuffix) "libs/Putty/unix/uxstore.c"

$(IntermediateDirectory)/unix_uxucs.c$(ObjectSuffix): libs/Putty/unix/uxucs.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxucs.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxucs.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxucs.c$(PreprocessSuffix): libs/Putty/unix/uxucs.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxucs.c$(PreprocessSuffix) "libs/Putty/unix/uxucs.c"

$(IntermediateDirectory)/unix_xkeysym.c$(ObjectSuffix): libs/Putty/unix/xkeysym.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/xkeysym.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_xkeysym.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_xkeysym.c$(PreprocessSuffix): libs/Putty/unix/xkeysym.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_xkeysym.c$(PreprocessSuffix) "libs/Putty/unix/xkeysym.c"

$(IntermediateDirectory)/unix_xpmpucfg.c$(ObjectSuffix): libs/Putty/unix/xpmpucfg.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/xpmpucfg.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_xpmpucfg.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_xpmpucfg.c$(PreprocessSuffix): libs/Putty/unix/xpmpucfg.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_xpmpucfg.c$(PreprocessSuffix) "libs/Putty/unix/xpmpucfg.c"

$(IntermediateDirectory)/unix_xpmputty.c$(ObjectSuffix): libs/Putty/unix/xpmputty.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/xpmputty.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_xpmputty.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_xpmputty.c$(PreprocessSuffix): libs/Putty/unix/xpmputty.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_xpmputty.c$(PreprocessSuffix) "libs/Putty/unix/xpmputty.c"

$(IntermediateDirectory)/unix_uxagentc.c$(ObjectSuffix): libs/Putty/unix/uxagentc.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxagentc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxagentc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxagentc.c$(PreprocessSuffix): libs/Putty/unix/uxagentc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxagentc.c$(PreprocessSuffix) "libs/Putty/unix/uxagentc.c"

$(IntermediateDirectory)/unix_uxsel.c$(ObjectSuffix): libs/Putty/unix/uxsel.c 
	$(CC) $(SourceSwitch) "./libs/Putty/unix/uxsel.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unix_uxsel.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unix_uxsel.c$(PreprocessSuffix): libs/Putty/unix/uxsel.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unix_uxsel.c$(PreprocessSuffix) "libs/Putty/unix/uxsel.c"

##
## Clean
##
clean:
	$(RM) -r ./$(ConfigurationName)/


