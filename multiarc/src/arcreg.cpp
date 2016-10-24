#include "MultiArc.hpp"

HKEY CreateRegKey(HKEY hRoot,const char *Key);
HKEY OpenRegKey(HKEY hRoot,const char *Key);

void SetRegKey(HKEY hRoot,const char *Key,const char *ValueName,char *ValueData)
{
  HKEY hKey=CreateRegKey(hRoot,Key);
  WINPORT(RegSetValueEx)(hKey,MB2Wide(ValueName).c_str(),0,REG_SZ_MB,(BYTE*)ValueData,strlen(ValueData)+1);
  WINPORT(RegCloseKey)(hKey);
}


void SetRegKey(HKEY hRoot,const char *Key,const char *ValueName,DWORD ValueData)
{
  HKEY hKey=CreateRegKey(hRoot,Key);
  WINPORT(RegSetValueEx)(hKey,MB2Wide(ValueName).c_str(),0,REG_DWORD,(BYTE *)&ValueData,sizeof(ValueData));
  WINPORT(RegCloseKey)(hKey);
}


int GetRegKey(const char *Key,const char *ValueName,char *ValueData,const char *Default,DWORD DataSize)
{
  int Ret;
  if(0==(Ret=GetRegKey(HKEY_CURRENT_USER, Key,ValueName,ValueData,Default,DataSize)))
    Ret=GetRegKey(HKEY_LOCAL_MACHINE,Key,ValueName,ValueData,Default,DataSize);
  return Ret;
}


int GetRegKey(HKEY hRoot,const char *Key,const char *ValueName,char *ValueData,const char *Default,DWORD DataSize)
{
  HKEY hKey=OpenRegKey(hRoot,Key);
  DWORD Type;
  int ExitCode=WINPORT(RegQueryValueEx)(hKey,MB2Wide(ValueName).c_str(),0,&Type,(BYTE*)ValueData,&DataSize);
  WINPORT(RegCloseKey)(hKey);
  if (hKey==NULL || ExitCode!=ERROR_SUCCESS)
  {
    strcpy(ValueData,Default);
    return FALSE;
  }
  return TRUE;
}


int GetRegKey(HKEY hRoot,const char *Key,const char *ValueName,int &ValueData,DWORD Default)
{
  HKEY hKey=OpenRegKey(hRoot,Key);
  DWORD Type,Size=sizeof(ValueData);
  int ExitCode=WINPORT(RegQueryValueEx)(hKey, MB2Wide(ValueName).c_str(),0,&Type,(BYTE *)&ValueData,&Size);
  WINPORT(RegCloseKey)(hKey);
  if (hKey==NULL || ExitCode!=ERROR_SUCCESS)
  {
    ValueData=Default;
    return FALSE;
  }
  return TRUE;
}


int GetRegKey(HKEY hRoot,const char *Key,const char *ValueName,DWORD Default)
{
  int ValueData;
  GetRegKey(hRoot,Key,ValueName,ValueData,Default);
  return ValueData;
}

static char *CreateKeyName(char *FullKeyName, const char *Key)
{
  FSF.sprintf(FullKeyName,"%s%s%s",PluginRootKey,*Key ? "/":"",Key);
  return FullKeyName;
}

HKEY CreateRegKey(HKEY hRoot,const char *Key)
{
  HKEY hKey;
  DWORD Disposition;
  char FullKeyName[512];
  WINPORT(RegCreateKeyEx)(hRoot, MB2Wide(CreateKeyName(FullKeyName,Key)).c_str(),0,NULL,0,KEY_WRITE,NULL,
                 &hKey,&Disposition);
  return hKey;
}


HKEY OpenRegKey(HKEY hRoot,const char *Key)
{
  HKEY hKey;
  char FullKeyName[512];
  if (WINPORT(RegOpenKeyEx)(hRoot, MB2Wide(CreateKeyName(FullKeyName,Key)).c_str(), 0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS)
    return NULL;
  return hKey;
}

void DeleteRegKey(HKEY hRoot,const char *Key)
{
  char FullKeyName[512];
  WINPORT(RegDeleteKey)(hRoot, MB2Wide(CreateKeyName(FullKeyName,Key)).c_str());
}


void DeleteRegValue(HKEY hRoot,const char *Key,const char *ValueName)
{
  HKEY hKey;
  char FullKeyName[512];
  if (WINPORT(RegOpenKeyEx)(hRoot, MB2Wide(CreateKeyName(FullKeyName,Key)).c_str(),0,KEY_WRITE,&hKey)==ERROR_SUCCESS)
  {
    WINPORT(RegDeleteValue)(hKey, MB2Wide(ValueName).c_str());
    WINPORT(RegCloseKey)(hKey);
  }
}
