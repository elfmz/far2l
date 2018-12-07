#include "MultiArc.hpp"
#include "marclng.hpp"

extern std::string gMultiArcPluginPath;

typedef int (__cdecl *FCMP)(const void *, const void *);

bool ArcPlugins::AddPluginItem(const char *name,
				PLUGINISARCHIVE pIsArchive,
				PLUGINOPENARCHIVE pOpenArchive,
				PLUGINGETARCITEM pGetArcItem,
				PLUGINLOADFORMATMODULE pLoadFormatModule,
				PLUGINCLOSEARCHIVE pCloseArchive,
				PLUGINGETFORMATNAME pGetFormatName,
				PLUGINGETDEFAULTCOMMANDS pGetDefaultCommands,
				PLUGINSETFARINFO pSetFarInfo,
				PLUGINGETSFXPOS pGetSFXPos)
{
	PluginItem item{};
	strcpy(item.ModuleName, name); 
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
		memcpy(&item.Info,&Info,sizeof(struct PluginStartupInfo));
		memcpy(&item.FSF, &FSF, sizeof(struct FarStandardFunctions));
		item.Info.FSF=&item.FSF;
		pSetFarInfo(&item.Info);
	}

	struct PluginItem *NewPluginsData=(struct PluginItem *)realloc(PluginsData,
				sizeof(struct PluginItem)*(PluginsCount+1));
	if (NewPluginsData==NULL) {
		return false;
	}
	PluginsData=NewPluginsData;
	memcpy(&PluginsData[PluginsCount], &item, sizeof(struct PluginItem));
	PluginsCount++;
	return true;
}


#include "formats/all.h" //use enum_all.sh to update it 

ArcPlugins::ArcPlugins(const char *ModuleName) : PluginsData(NULL), PluginsCount(0)
{
	//AddPluginItem("", _IsArchive, _OpenArchive, _GetArcItem, _LoadFormatModule, 
	//	_CloseArchive, _GetFormatName, _GetDefaultCommands, _SetFarInfo, _GetSFXPos);
	
	AddPluginItem("RAR", RAR_IsArchive, RAR_OpenArchive, RAR_GetArcItem, NULL, 
		RAR_CloseArchive, RAR_GetFormatName, RAR_GetDefaultCommands, RAR_SetFarInfo, RAR_GetSFXPos);

	AddPluginItem("HA", HA_IsArchive, HA_OpenArchive, HA_GetArcItem, NULL, 
		HA_CloseArchive, HA_GetFormatName, HA_GetDefaultCommands, HA_SetFarInfo, NULL);

	AddPluginItem("CUSTOM", CUSTOM_IsArchive, CUSTOM_OpenArchive, CUSTOM_GetArcItem, CUSTOM_LoadFormatModule, 
		CUSTOM_CloseArchive, CUSTOM_GetFormatName, CUSTOM_GetDefaultCommands, CUSTOM_SetFarInfo, CUSTOM_GetSFXPos);

	AddPluginItem("ARJ", ARJ_IsArchive, ARJ_OpenArchive, ARJ_GetArcItem, NULL, 
		ARJ_CloseArchive, ARJ_GetFormatName, ARJ_GetDefaultCommands, NULL, ARJ_GetSFXPos);

	AddPluginItem("ACE", ACE_IsArchive, ACE_OpenArchive, ACE_GetArcItem, NULL, 
		ACE_CloseArchive, ACE_GetFormatName, ACE_GetDefaultCommands, NULL, ACE_GetSFXPos);

	AddPluginItem("ARC", ARC_IsArchive, ARC_OpenArchive, ARC_GetArcItem, NULL, 
		ARC_CloseArchive, ARC_GetFormatName, ARC_GetDefaultCommands, NULL, ARC_GetSFXPos);

	AddPluginItem("ZIP", ZIP_IsArchive, ZIP_OpenArchive, ZIP_GetArcItem, NULL, 
		ZIP_CloseArchive, ZIP_GetFormatName, ZIP_GetDefaultCommands, NULL, ZIP_GetSFXPos);
		
	AddPluginItem("LZH", LZH_IsArchive, LZH_OpenArchive, LZH_GetArcItem, NULL, 
		LZH_CloseArchive, LZH_GetFormatName, LZH_GetDefaultCommands, LZH_SetFarInfo, LZH_GetSFXPos);

	AddPluginItem("CAB", CAB_IsArchive, CAB_OpenArchive, CAB_GetArcItem, NULL, 
		CAB_CloseArchive, CAB_GetFormatName, CAB_GetDefaultCommands, NULL, CAB_GetSFXPos);
	
	AddPluginItem("TARGZ", TARGZ_IsArchive, TARGZ_OpenArchive, TARGZ_GetArcItem, NULL, 
		TARGZ_CloseArchive, TARGZ_GetFormatName, TARGZ_GetDefaultCommands, TARGZ_SetFarInfo, TARGZ_GetSFXPos);
		
	AddPluginItem("7z", SEVENZ_IsArchive, SEVENZ_OpenArchive, SEVENZ_GetArcItem, NULL, 
		SEVENZ_CloseArchive, SEVENZ_GetFormatName, SEVENZ_GetDefaultCommands, NULL, NULL);
		
	FSF.qsort(PluginsData,PluginsCount,sizeof(struct PluginItem),(FCMP)CompareFmtModules);
}

int __cdecl ArcPlugins::CompareFmtModules(const void *elem1, const void *elem2)
{
  char *left = (((struct PluginItem *)elem1)->ModuleName);
  char *right = (((struct PluginItem *)elem2)->ModuleName);
  DWORD try_left=(((struct PluginItem *)elem1)->TryIfNoOther);
  DWORD try_right=(((struct PluginItem *)elem2)->TryIfNoOther);

  if (try_left && !try_right)
    return 1;
  if (!try_left && try_right)
    return -1;

  return strcmp(left,right);
}

ArcPlugins::~ArcPlugins()
{
  if(PluginsData)
    free(PluginsData);
}

int ArcPlugins::IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
  DWORD MinSFXSize=0xFFFFFFFF;
  DWORD CurSFXSize;
  int TrueArc=-1;
  int I;

  for (I=0; I < PluginsCount; I++)
  {
    if (TrueArc>-1 && PluginsData[I].TryIfNoOther)
      break;
    if (IsArchive(I, Name, Data, DataSize, &CurSFXSize))
    {
      if(CurSFXSize <= MinSFXSize)
      {
        MinSFXSize=CurSFXSize;
        TrueArc=I;
      }
    }
  }
  return TrueArc;
}

BOOL ArcPlugins::IsArchive(int ArcPluginNumber, const char *Name,const unsigned char *Data,int DataSize, DWORD* SFXSize)
{
  struct PluginItem *CurPluginsData=PluginsData+ArcPluginNumber;
  if (CurPluginsData->pIsArchive &&
    CurPluginsData->pIsArchive(Name,Data,DataSize))
  {
      if(CurPluginsData->pGetSFXPos)
        *SFXSize=CurPluginsData->pGetSFXPos();
      else
        *SFXSize=0;
    return TRUE;
  }
  return FALSE;
}

BOOL ArcPlugins::OpenArchive(int PluginNumber, const char *Name,int *Type)
{
  *Type=0; //$ AA 12.11.2001
  if ((DWORD)PluginNumber < (DWORD)PluginsCount && PluginsData[PluginNumber].pOpenArchive)
    return PluginsData[PluginNumber].pOpenArchive(Name,Type);
  return FALSE;
}

int ArcPlugins::GetArcItem(int PluginNumber,struct PluginPanelItem *Item,
                           struct ArcItemInfo *Info)
{
  if ((DWORD)PluginNumber < (DWORD)PluginsCount && PluginsData[PluginNumber].pGetArcItem)
    return PluginsData[PluginNumber].pGetArcItem(Item,Info);
  return FALSE;
}


void ArcPlugins::CloseArchive(int PluginNumber,struct ArcInfo *Info)
{
  if ((DWORD)PluginNumber < (DWORD)PluginsCount && PluginsData[PluginNumber].pCloseArchive)
    PluginsData[PluginNumber].pCloseArchive(Info);
}


BOOL ArcPlugins::GetFormatName(int PluginNumber,int Type,char *FormatName,
                               char *DefaultExt)
{
  *FormatName=0;
  if ((DWORD)PluginNumber < (DWORD)PluginsCount && PluginsData[PluginNumber].pGetFormatName)
    return PluginsData[PluginNumber].pGetFormatName(Type,FormatName,DefaultExt);
  return FALSE;
}


BOOL ArcPlugins::GetDefaultCommands(int PluginNumber,int Type,int Command,
                                    char *Dest)
{
  *Dest=0;
  if ((DWORD)PluginNumber < (DWORD)PluginsCount && PluginsData[PluginNumber].pGetDefaultCommands)
    return PluginsData[PluginNumber].pGetDefaultCommands(Type,Command,Dest);
  return FALSE;
}
