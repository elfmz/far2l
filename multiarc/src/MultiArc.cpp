#include "MultiArc.hpp"
#include "marclng.hpp"
#include <string>
#include <unistd.h>
#include <locale.h>

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  (void) lpReserved;
  (void) dwReason;
  (void) hDll;
  return TRUE;
}
#endif

SHAREDSYMBOL int WINAPI _export GetMinFarVersion(void)
{
	#define MAKEFARVERSION(major,minor) ( ((major)<<16) | (minor))
  return MAKEFARVERSION(2, 1);
}

SHAREDSYMBOL void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
  ::Info=*Info;
  FSF=*Info->FSF;
  ::Info.FSF=&FSF;

  ::Info.FSF->Reserved[0]=(DWORD_PTR)malloc;
  ::Info.FSF->Reserved[1]=(DWORD_PTR)free;

  FSF.sprintf(PluginRootKey,"%s/MultiArc",Info->RootKey);

  if (ArcPlugin==NULL)
    ArcPlugin=new ArcPlugins(Info->ModuleName);

  Opt.HideOutput=GetRegKey(HKEY_CURRENT_USER,"","HideOutput",0);
  Opt.ProcessShiftF1=GetRegKey(HKEY_CURRENT_USER,"","ProcessShiftF1",1);
  Opt.UseLastHistory=GetRegKey(HKEY_CURRENT_USER,"","UseLastHistory",0);

  //Opt.DeleteExtFile=GetRegKey(HKEY_CURRENT_USER,"","DeleteExtFile",1);
  //Opt.AddExtArchive=GetRegKey(HKEY_CURRENT_USER,"","AddExtArchive",0);

  //Opt.AutoResetExactArcName=GetRegKey(HKEY_CURRENT_USER,"","AutoResetExactArcName",1);
  //Opt.ExactArcName=GetRegKey(HKEY_CURRENT_USER, "", "ExactArcName", 0);
  Opt.AdvFlags=GetRegKey(HKEY_CURRENT_USER, "", "AdvFlags", 2);

  GetRegKey(HKEY_CURRENT_USER,"","DescriptionNames",Opt.DescriptionNames,
            "descript.ion,files.bbs",sizeof(Opt.DescriptionNames));
  Opt.ReadDescriptions=GetRegKey(HKEY_CURRENT_USER,"","ReadDescriptions",0);
  Opt.UpdateDescriptions=GetRegKey(HKEY_CURRENT_USER,"","UpdateDescriptions",0);
  //Opt.UserBackground=GetRegKey(HKEY_CURRENT_USER,"","Background",0); // $ 06.02.2002 AA
  Opt.OldUserBackground=0; // $ 02.07.2002 AY
  Opt.AllowChangeDir=GetRegKey(HKEY_CURRENT_USER,"","AllowChangeDir",0);

  GetRegKey(HKEY_CURRENT_USER,"","Prefix1",Opt.CommandPrefix1,"ma",sizeof(Opt.CommandPrefix1));

  #ifdef _NEW_ARC_SORT_
  strcpy(IniFile, Info->ModuleName);
  *((int *)(IniFile+strlen(IniFile)-3))=0x696E69; // :)
  #endif
  Opt.PriorityClass=2; // default: NORMAL

}


SHAREDSYMBOL HANDLE WINAPI _export OpenFilePlugin(const char *Name,const unsigned char *Data,int DataSize,int OpMode)
{

  detect_oem_cp();

  if (ArcPlugin==NULL)
    return INVALID_HANDLE_VALUE;

  int ArcPluginNumber=-1;
  if (Name==NULL)
  {
    if (!Opt.ProcessShiftF1)
      return INVALID_HANDLE_VALUE;
  }
  else
  {
    ArcPluginNumber=ArcPlugin->IsArchive(Name,Data,DataSize);
    if (ArcPluginNumber==-1)
      return INVALID_HANDLE_VALUE;
  }

  try {
    PluginClass *pPlugin = new PluginClass(ArcPluginNumber);
    if (Name!=NULL && !pPlugin->PreReadArchive(Name)) {
        delete pPlugin;
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)pPlugin;

  } catch (std::exception &e) {
        fprintf(stderr, "OpenFilePlugin: %s\n", e.what());
        return INVALID_HANDLE_VALUE;
  }
}

std::string MakeFullName(const char *name)
{
	if (name[0]=='/')
		return name;
	
	std::string out;
	char cd[PATH_MAX];
	if (getcwd(cd, sizeof(cd))) {
		if (name[0] == '.' && name[1] == '/') {
			name += 2;
		}
		out = cd;
		out += '/';
	}
	
	out += name;
	return out;
}

SHAREDSYMBOL HANDLE WINAPI _export OpenPlugin(int OpenFrom, INT_PTR Item)
{
  if (ArcPlugin==NULL)
    return INVALID_HANDLE_VALUE;
  if (OpenFrom==OPEN_COMMANDLINE)
  {
    if (strncmp(Opt.CommandPrefix1,(char *)Item,strlen(Opt.CommandPrefix1))==0 &&
        (((char *)Item)[strlen(Opt.CommandPrefix1)]==':'))
    {
      if (((char *)Item)[strlen(Opt.CommandPrefix1)+1]==0)
        return INVALID_HANDLE_VALUE;
      char oldfilename[NM+2];
      strncpy(oldfilename,&((char *)Item)[strlen(Opt.CommandPrefix1)+1],sizeof(oldfilename));
      FSF.Unquote(oldfilename);
      FSF.ExpandEnvironmentStr(oldfilename,oldfilename,NM);
      std::string filename = MakeFullName(oldfilename);
      if (filename.empty())
        return INVALID_HANDLE_VALUE;
      HANDLE h = WINPORT(CreateFile)(StrMB2Wide(filename).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
      if (h == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;
      DWORD size = (DWORD)Info.AdvControl(Info.ModuleNumber,ACTL_GETPLUGINMAXREADDATA,(void *)0);
      unsigned char *data;
      data = (unsigned char *) malloc(size*sizeof(unsigned char));
      if (!data)
        return INVALID_HANDLE_VALUE;
      DWORD datasize;
      BOOL b = WINPORT(ReadFile)(h,data,size,&datasize,NULL);
      WINPORT(CloseHandle)(h);
      if (!(b&&datasize))
      {
        free(data);
        return INVALID_HANDLE_VALUE;
      }
      h = OpenFilePlugin(filename.c_str(), data, datasize, OPM_COMMANDS);
      free(data);
      if ((h==(HANDLE)-2) || h==INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;
      return h;
    }
  }
  return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI _export ClosePlugin(HANDLE hPlugin)
{
  delete (PluginClass *)hPlugin;
}


SHAREDSYMBOL int WINAPI _export GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->GetFindData(pPanelItem,pItemsNumber,OpMode);
}


SHAREDSYMBOL void WINAPI _export FreeFindData(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  Plugin->FreeFindData(PanelItem,ItemsNumber);
}


SHAREDSYMBOL int WINAPI _export SetDirectory(HANDLE hPlugin,const char *Dir,int OpMode)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->SetDirectory(Dir,OpMode);
}


SHAREDSYMBOL int WINAPI _export DeleteFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->DeleteFiles(PanelItem,ItemsNumber,OpMode);
}


SHAREDSYMBOL int WINAPI _export GetFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
                   int ItemsNumber,int Move,char *DestPath,int OpMode)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->GetFiles(PanelItem,ItemsNumber,Move,DestPath,OpMode);
}


SHAREDSYMBOL int WINAPI _export PutFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
                   int ItemsNumber,int Move,int OpMode)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->PutFiles(PanelItem,ItemsNumber,Move,OpMode);
}


SHAREDSYMBOL void WINAPI _export ExitFAR()
{
  delete ArcPlugin;
}


SHAREDSYMBOL void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
  Info->StructSize=sizeof(*Info);
  Info->Flags=PF_FULLCMDLINE;
  static const char *PluginCfgStrings[1];
  PluginCfgStrings[0]=(char*)GetMsg(MCfgLine0);
  Info->PluginConfigStrings=PluginCfgStrings;
  Info->PluginConfigStringsNumber=ARRAYSIZE(PluginCfgStrings);
  static char CommandPrefix[sizeof(Opt.CommandPrefix1)];
  strcpy(CommandPrefix,Opt.CommandPrefix1);
  Info->CommandPrefix=CommandPrefix;
}


SHAREDSYMBOL void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  Plugin->GetOpenPluginInfo(Info);
}


SHAREDSYMBOL int WINAPI _export ProcessHostFile(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->ProcessHostFile(PanelItem,ItemsNumber,OpMode);
}


SHAREDSYMBOL int WINAPI _export ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState)
{
  PluginClass *Plugin=(PluginClass *)hPlugin;
  return Plugin->ProcessKey(Key,ControlState);

}

SHAREDSYMBOL int WINAPI _export Configure(int ItemNumber)
{
  struct FarMenuItem MenuItems[2];
  memset(MenuItems,0,sizeof(MenuItems));
  strcpy(MenuItems[0].Text,GetMsg(MCfgLine1));
  strcpy(MenuItems[1].Text,GetMsg(MCfgLine2));
  MenuItems[0].Selected=TRUE;

  do{
    ItemNumber=Info.Menu(Info.ModuleNumber,-1,-1,0,FMENU_WRAPMODE,
                       GetMsg(MCfgLine0),NULL,"Config",NULL,NULL,MenuItems,
                       ARRAYSIZE(MenuItems));
    switch(ItemNumber)
    {
      case -1:
        return FALSE;
      case 0:
        ConfigGeneral();
        break;
      case 1:
      {
        char ArcFormat[100];
        *ArcFormat=0;
        while(PluginClass::SelectFormat(ArcFormat))
          ;//ConfigCommands(ArcFormat);
        break;
      }
    }
    MenuItems[0].Selected=FALSE;
    MenuItems[1].Selected=FALSE;
    MenuItems[ItemNumber].Selected=TRUE;
  }while(1);

  //return FALSE;
}

std::string gMultiArcPluginPath;

SHAREDSYMBOL void WINPORT_DllStartup(const char *path)
{
	gMultiArcPluginPath = path;
}

int oem_cp;

void detect_oem_cp() {

  // detect oem cp from system locale

  oem_cp = 437; // default

  char *lc = setlocale(LC_CTYPE, NULL);
  if (!lc) { return; }
  int lc_len;
  for (lc_len = 0; lc[lc_len] != '.' && lc[lc_len] != '\0'; ++lc_len)
    ;
  lc[lc_len] = 0;

  if (!strcmp(lc, "af_ZA")) { oem_cp = 850; }  if (!strcmp(lc, "ar_SA")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_LB")) { oem_cp = 720; }  if (!strcmp(lc, "ar_EG")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_DZ")) { oem_cp = 720; }  if (!strcmp(lc, "ar_BH")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_IQ")) { oem_cp = 720; }  if (!strcmp(lc, "ar_JO")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_KW")) { oem_cp = 720; }  if (!strcmp(lc, "ar_LY")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_MA")) { oem_cp = 720; }  if (!strcmp(lc, "ar_OM")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_QA")) { oem_cp = 720; }  if (!strcmp(lc, "ar_SY")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_TN")) { oem_cp = 720; }  if (!strcmp(lc, "ar_AE")) { oem_cp = 720; }
  if (!strcmp(lc, "ar_YE")) { oem_cp = 720; }  if (!strcmp(lc, "ast_ES")) { oem_cp = 850; }
  if (!strcmp(lc, "az_AZ")) { oem_cp = 866; }  if (!strcmp(lc, "az_AZ")) { oem_cp = 857; }
  if (!strcmp(lc, "be_BY")) { oem_cp = 866; }  if (!strcmp(lc, "bg_BG")) { oem_cp = 866; }
  if (!strcmp(lc, "br_FR")) { oem_cp = 850; }  if (!strcmp(lc, "ca_ES")) { oem_cp = 850; }
  if (!strcmp(lc, "zh_CN")) { oem_cp = 936; }  if (!strcmp(lc, "zh_TW")) { oem_cp = 950; }
  if (!strcmp(lc, "kw_GB")) { oem_cp = 850; }  if (!strcmp(lc, "cs_CZ")) { oem_cp = 852; }
  if (!strcmp(lc, "cy_GB")) { oem_cp = 850; }  if (!strcmp(lc, "da_DK")) { oem_cp = 850; }
  if (!strcmp(lc, "de_AT")) { oem_cp = 850; }  if (!strcmp(lc, "de_LI")) { oem_cp = 850; }
  if (!strcmp(lc, "de_LU")) { oem_cp = 850; }  if (!strcmp(lc, "de_CH")) { oem_cp = 850; }
  if (!strcmp(lc, "de_DE")) { oem_cp = 850; }  if (!strcmp(lc, "el_GR")) { oem_cp = 737; }
  if (!strcmp(lc, "en_AU")) { oem_cp = 850; }  if (!strcmp(lc, "en_CA")) { oem_cp = 850; }
  if (!strcmp(lc, "en_GB")) { oem_cp = 850; }  if (!strcmp(lc, "en_IE")) { oem_cp = 850; }
  if (!strcmp(lc, "en_JM")) { oem_cp = 850; }  if (!strcmp(lc, "en_BZ")) { oem_cp = 850; }
  if (!strcmp(lc, "en_PH")) { oem_cp = 437; }  if (!strcmp(lc, "en_ZA")) { oem_cp = 437; }
  if (!strcmp(lc, "en_TT")) { oem_cp = 850; }  if (!strcmp(lc, "en_US")) { oem_cp = 437; }
  if (!strcmp(lc, "en_ZW")) { oem_cp = 437; }  if (!strcmp(lc, "en_NZ")) { oem_cp = 850; }
  if (!strcmp(lc, "es_PA")) { oem_cp = 850; }  if (!strcmp(lc, "es_BO")) { oem_cp = 850; }
  if (!strcmp(lc, "es_CR")) { oem_cp = 850; }  if (!strcmp(lc, "es_DO")) { oem_cp = 850; }
  if (!strcmp(lc, "es_SV")) { oem_cp = 850; }  if (!strcmp(lc, "es_EC")) { oem_cp = 850; }
  if (!strcmp(lc, "es_GT")) { oem_cp = 850; }  if (!strcmp(lc, "es_HN")) { oem_cp = 850; }
  if (!strcmp(lc, "es_NI")) { oem_cp = 850; }  if (!strcmp(lc, "es_CL")) { oem_cp = 850; }
  if (!strcmp(lc, "es_MX")) { oem_cp = 850; }  if (!strcmp(lc, "es_ES")) { oem_cp = 850; }
  if (!strcmp(lc, "es_CO")) { oem_cp = 850; }  if (!strcmp(lc, "es_ES")) { oem_cp = 850; }
  if (!strcmp(lc, "es_PE")) { oem_cp = 850; }  if (!strcmp(lc, "es_AR")) { oem_cp = 850; }
  if (!strcmp(lc, "es_PR")) { oem_cp = 850; }  if (!strcmp(lc, "es_VE")) { oem_cp = 850; }
  if (!strcmp(lc, "es_UY")) { oem_cp = 850; }  if (!strcmp(lc, "es_PY")) { oem_cp = 850; }
  if (!strcmp(lc, "et_EE")) { oem_cp = 775; }  if (!strcmp(lc, "eu_ES")) { oem_cp = 850; }
  if (!strcmp(lc, "fa_IR")) { oem_cp = 720; }  if (!strcmp(lc, "fi_FI")) { oem_cp = 850; }
  if (!strcmp(lc, "fo_FO")) { oem_cp = 850; }  if (!strcmp(lc, "fr_FR")) { oem_cp = 850; }
  if (!strcmp(lc, "fr_BE")) { oem_cp = 850; }  if (!strcmp(lc, "fr_CA")) { oem_cp = 850; }
  if (!strcmp(lc, "fr_LU")) { oem_cp = 850; }  if (!strcmp(lc, "fr_MC")) { oem_cp = 850; }
  if (!strcmp(lc, "fr_CH")) { oem_cp = 850; }  if (!strcmp(lc, "ga_IE")) { oem_cp = 437; }
  if (!strcmp(lc, "gd_GB")) { oem_cp = 850; }  if (!strcmp(lc, "gv_IM")) { oem_cp = 850; }
  if (!strcmp(lc, "gl_ES")) { oem_cp = 850; }  if (!strcmp(lc, "he_IL")) { oem_cp = 862; }
  if (!strcmp(lc, "hr_HR")) { oem_cp = 852; }  if (!strcmp(lc, "hu_HU")) { oem_cp = 852; }
  if (!strcmp(lc, "id_ID")) { oem_cp = 850; }  if (!strcmp(lc, "is_IS")) { oem_cp = 850; }
  if (!strcmp(lc, "it_IT")) { oem_cp = 850; }  if (!strcmp(lc, "it_CH")) { oem_cp = 850; }
  if (!strcmp(lc, "iv_IV")) { oem_cp = 437; }  if (!strcmp(lc, "ja_JP")) { oem_cp = 932; }
  if (!strcmp(lc, "kk_KZ")) { oem_cp = 866; }  if (!strcmp(lc, "ko_KR")) { oem_cp = 949; }
  if (!strcmp(lc, "ky_KG")) { oem_cp = 866; }  if (!strcmp(lc, "lt_LT")) { oem_cp = 775; }
  if (!strcmp(lc, "lv_LV")) { oem_cp = 775; }  if (!strcmp(lc, "mk_MK")) { oem_cp = 866; }
  if (!strcmp(lc, "mn_MN")) { oem_cp = 866; }  if (!strcmp(lc, "ms_BN")) { oem_cp = 850; }
  if (!strcmp(lc, "ms_MY")) { oem_cp = 850; }  if (!strcmp(lc, "nl_BE")) { oem_cp = 850; }
  if (!strcmp(lc, "nl_NL")) { oem_cp = 850; }  if (!strcmp(lc, "nl_SR")) { oem_cp = 850; }
  if (!strcmp(lc, "nn_NO")) { oem_cp = 850; }  if (!strcmp(lc, "nb_NO")) { oem_cp = 850; }
  if (!strcmp(lc, "pl_PL")) { oem_cp = 852; }  if (!strcmp(lc, "pt_BR")) { oem_cp = 850; }
  if (!strcmp(lc, "pt_PT")) { oem_cp = 850; }  if (!strcmp(lc, "rm_CH")) { oem_cp = 850; }
  if (!strcmp(lc, "ro_RO")) { oem_cp = 852; }  if (!strcmp(lc, "ru_RU")) { oem_cp = 866; }
  if (!strcmp(lc, "sk_SK")) { oem_cp = 852; }  if (!strcmp(lc, "sl_SI")) { oem_cp = 852; }
  if (!strcmp(lc, "sq_AL")) { oem_cp = 852; }  if (!strcmp(lc, "sr_RS")) { oem_cp = 855; }
  if (!strcmp(lc, "sr_RS")) { oem_cp = 852; }  if (!strcmp(lc, "sv_SE")) { oem_cp = 850; }
  if (!strcmp(lc, "sv_FI")) { oem_cp = 850; }  if (!strcmp(lc, "sw_KE")) { oem_cp = 437; }
  if (!strcmp(lc, "th_TH")) { oem_cp = 874; }  if (!strcmp(lc, "tr_TR")) { oem_cp = 857; }
  if (!strcmp(lc, "tt_RU")) { oem_cp = 866; }  if (!strcmp(lc, "uk_UA")) { oem_cp = 866; }
  if (!strcmp(lc, "ur_PK")) { oem_cp = 720; }  if (!strcmp(lc, "uz_UZ")) { oem_cp = 866; }
  if (!strcmp(lc, "uz_UZ")) { oem_cp = 857; }  if (!strcmp(lc, "vi_VN")) { oem_cp = 1258; }
  if (!strcmp(lc, "wa_BE")) { oem_cp = 850; }  if (!strcmp(lc, "zh_HK")) { oem_cp = 950; }
  if (!strcmp(lc, "zh_SG")) { oem_cp = 936; }  if (!strcmp(lc, "zh_MO")) { oem_cp = 950; }
}
