struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  DWORD_PTR Selected;
  unsigned int Flags;
  unsigned char DefaultButton;
  const WCHAR *Data;
};

void SetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,DWORD ValueData);
void SetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,WCHAR *ValueData);
int GetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,int &ValueData,DWORD Default);
int GetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,DWORD Default);
int GetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,WCHAR *ValueData,const WCHAR *Default,DWORD DataSize);
const WCHAR *GetMsg(int MsgId);
void InitDialogItems(const struct InitDialogItem *Init,struct FarDialogItem *Item,int ItemsNumber);


static struct PluginStartupInfo Info;
struct FarStandardFunctions FSF;
WCHAR PluginRootKey[80];
