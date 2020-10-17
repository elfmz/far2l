struct Options
{
  WCHAR FileMasks[512];
  WCHAR ExcludeFileMasks[512];
  int RightMargin;
  int Wrap;
} Opt;

void SetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,DWORD ValueData);
void SetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,WCHAR *ValueData);
int GetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,int &ValueData,DWORD Default);
int GetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,DWORD Default);
int GetRegKey(HKEY hRoot,const WCHAR *Key,const WCHAR *ValueName,WCHAR *ValueData,const WCHAR *Default,DWORD DataSize);
const WCHAR *GetMsg(int MsgId);
WCHAR *GetCommaWord(const WCHAR *Src,WCHAR *Word);


static struct PluginStartupInfo Info;
static FARSTANDARDFUNCTIONS FSF;
WCHAR PluginRootKey[80];
