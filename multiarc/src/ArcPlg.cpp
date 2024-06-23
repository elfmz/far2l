#include "MultiArc.hpp"
#include "marclng.hpp"

extern std::string gMultiArcPluginPath;

void ArcPlugins::AddPluginItem(PLUGINISARCHIVE pIsArchive, PLUGINOPENARCHIVE pOpenArchive,
		PLUGINGETARCITEM pGetArcItem, PLUGINLOADFORMATMODULE pLoadFormatModule,
		PLUGINCLOSEARCHIVE pCloseArchive, PLUGINGETFORMATNAME pGetFormatName,
		PLUGINGETDEFAULTCOMMANDS pGetDefaultCommands, PLUGINSETFARINFO pSetFarInfo,
		PLUGINGETSFXPOS pGetSFXPos)
{
	PluginsData.emplace_back();
	PluginItem &item = PluginsData.back();

	item.pIsArchive = pIsArchive;
	item.pOpenArchive = pOpenArchive;
	item.pGetArcItem = pGetArcItem;
	item.pLoadFormatModule = pLoadFormatModule;
	item.pCloseArchive = pCloseArchive;
	item.pGetFormatName = pGetFormatName;
	item.pGetDefaultCommands = pGetDefaultCommands;
	item.pSetFarInfo = pSetFarInfo;
	item.pGetSFXPos = pGetSFXPos;

	if (pLoadFormatModule)
		pLoadFormatModule(gMultiArcPluginPath.c_str());

	if (pSetFarInfo) {
		// Дабы FMT не испортил оригинальный PluginStartupInfo дадим ему
		// временную "переменную"
		memcpy(&item.Info, &Info, sizeof(struct PluginStartupInfo));
		memcpy(&item.FSF, &FSF, sizeof(struct FarStandardFunctions));
		item.Info.FSF = &item.FSF;
		pSetFarInfo(&item.Info);
	}
}

#include "formats/all.h"	//use enum_all.sh to update it

ArcPlugins::ArcPlugins(const char *ModuleName)
{
	// AddPluginItem(_IsArchive, _OpenArchive, _GetArcItem, _LoadFormatModule,
	//   _CloseArchive, _GetFormatName, _GetDefaultCommands, _SetFarInfo, _GetSFXPos);

	// following initialization defines order in which IsArchive invoked, so most-likely formats should be first

#ifndef HAVE_LIBARCHIVE
	AddPluginItem(TARGZ_IsArchive, TARGZ_OpenArchive, TARGZ_GetArcItem, NULL, TARGZ_CloseArchive,
			TARGZ_GetFormatName, TARGZ_GetDefaultCommands, TARGZ_SetFarInfo, TARGZ_GetSFXPos);
#endif
#ifdef HAVE_UNRAR
	AddPluginItem(RAR_IsArchive, RAR_OpenArchive, RAR_GetArcItem, NULL, RAR_CloseArchive, RAR_GetFormatName,
			RAR_GetDefaultCommands, RAR_SetFarInfo, RAR_GetSFXPos);
#endif

	AddPluginItem(ZIP_IsArchive, ZIP_OpenArchive, ZIP_GetArcItem, NULL, ZIP_CloseArchive, ZIP_GetFormatName,
			ZIP_GetDefaultCommands, NULL, ZIP_GetSFXPos);

#ifndef ENDIAN_IS_BIG	// these not yet adaptated to big endian build
	AddPluginItem(SEVENZ_IsArchive, SEVENZ_OpenArchive, SEVENZ_GetArcItem, NULL, SEVENZ_CloseArchive,
			SEVENZ_GetFormatName, SEVENZ_GetDefaultCommands, NULL, NULL);

	AddPluginItem(ACE_IsArchive, ACE_OpenArchive, ACE_GetArcItem, NULL, ACE_CloseArchive, ACE_GetFormatName,
			ACE_GetDefaultCommands, NULL, ACE_GetSFXPos);

	AddPluginItem(ARC_IsArchive, ARC_OpenArchive, ARC_GetArcItem, NULL, ARC_CloseArchive, ARC_GetFormatName,
			ARC_GetDefaultCommands, NULL, ARC_GetSFXPos);

	AddPluginItem(ARJ_IsArchive, ARJ_OpenArchive, ARJ_GetArcItem, NULL, ARJ_CloseArchive, ARJ_GetFormatName,
			ARJ_GetDefaultCommands, NULL, ARJ_GetSFXPos);

#ifndef HAVE_LIBARCHIVE
	AddPluginItem(CAB_IsArchive, CAB_OpenArchive, CAB_GetArcItem, NULL, CAB_CloseArchive, CAB_GetFormatName,
			CAB_GetDefaultCommands, NULL, CAB_GetSFXPos);
#endif

	AddPluginItem(HA_IsArchive, HA_OpenArchive, HA_GetArcItem, NULL, HA_CloseArchive, HA_GetFormatName,
			HA_GetDefaultCommands, HA_SetFarInfo, NULL);

	AddPluginItem(LZH_IsArchive, LZH_OpenArchive, LZH_GetArcItem, NULL, LZH_CloseArchive, LZH_GetFormatName,
			LZH_GetDefaultCommands, LZH_SetFarInfo, LZH_GetSFXPos);
#endif

//#ifdef HAVE_PCRE
	AddPluginItem(CUSTOM_IsArchive, CUSTOM_OpenArchive, CUSTOM_GetArcItem, CUSTOM_LoadFormatModule,
			CUSTOM_CloseArchive, CUSTOM_GetFormatName, CUSTOM_GetDefaultCommands, CUSTOM_SetFarInfo,
			CUSTOM_GetSFXPos);
//#endif

#ifdef HAVE_LIBARCHIVE
	// must be last cuz recognizes essentially most of above formats, but not handles them full-featurable
	AddPluginItem(LIBARCH_IsArchive, LIBARCH_OpenArchive, LIBARCH_GetArcItem, NULL, LIBARCH_CloseArchive,
			LIBARCH_GetFormatName, LIBARCH_GetDefaultCommands, NULL, NULL);
#endif
}

ArcPlugins::~ArcPlugins() {}

int ArcPlugins::IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	DWORD MinSFXSize = 0xffffffff;
	int MinSFXSizeI = -1;

	for (size_t I = 0; I < PluginsData.size(); I++) {
		DWORD CurSFXSize = 0;
		if (IsArchive(I, Name, Data, DataSize, &CurSFXSize)) {
			if (MinSFXSize > CurSFXSize || MinSFXSizeI == -1) {
				MinSFXSize = CurSFXSize;
				MinSFXSizeI = I;
				if (CurSFXSize == 0)
					break;
			}
		}
	}
	return MinSFXSizeI;
}

BOOL ArcPlugins::IsArchive(int ArcPluginNumber, const char *Name, const unsigned char *Data, int DataSize,
		DWORD *SFXSize)
{
	struct PluginItem &CurPluginsData = PluginsData[ArcPluginNumber];
	if (CurPluginsData.pIsArchive && CurPluginsData.pIsArchive(Name, Data, DataSize)) {
		if (CurPluginsData.pGetSFXPos)
			*SFXSize = CurPluginsData.pGetSFXPos();
		else
			*SFXSize = 0;
		return TRUE;
	}
	return FALSE;
}

BOOL ArcPlugins::OpenArchive(int PluginNumber, const char *Name, int *Type, bool Silent)
{
	*Type = 0;	//$ AA 12.11.2001
	if ((DWORD)PluginNumber < PluginsData.size() && PluginsData[PluginNumber].pOpenArchive)
		return PluginsData[PluginNumber].pOpenArchive(Name, Type, Silent);
	return FALSE;
}

int ArcPlugins::GetArcItem(int PluginNumber, struct ArcItemInfo *Info)
{
	if ((DWORD)PluginNumber < PluginsData.size() && PluginsData[PluginNumber].pGetArcItem)
		return PluginsData[PluginNumber].pGetArcItem(Info);
	return FALSE;
}

void ArcPlugins::CloseArchive(int PluginNumber, struct ArcInfo *Info)
{
	if ((DWORD)PluginNumber < PluginsData.size() && PluginsData[PluginNumber].pCloseArchive)
		PluginsData[PluginNumber].pCloseArchive(Info);
}

BOOL ArcPlugins::GetFormatName(int PluginNumber, int Type, std::string &FormatName, std::string &DefaultExt)
{
	FormatName.clear();
	if ((DWORD)PluginNumber < PluginsData.size() && PluginsData[PluginNumber].pGetFormatName)
		return PluginsData[PluginNumber].pGetFormatName(Type, FormatName, DefaultExt);
	return FALSE;
}

BOOL ArcPlugins::GetDefaultCommands(int PluginNumber, int Type, int Command, std::string &Dest)
{
	Dest.clear();
	if ((DWORD)PluginNumber < PluginsData.size() && PluginsData[PluginNumber].pGetDefaultCommands)
		return PluginsData[PluginNumber].pGetDefaultCommands(Type, Command, Dest);
	return FALSE;
}
