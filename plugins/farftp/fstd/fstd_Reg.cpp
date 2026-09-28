#include <all_far.h>

#include "fstdlib.h"

BOOL WINAPI FP_CheckRegKey(const char *Key)
{
	HKEY hKey = FP_OpenRegKey(Key);

	if (hKey != NULL)
		WINPORT(RegCloseKey)(hKey);

	return hKey != NULL;
}

HKEY WINAPI FP_CreateRegKey(const char *Key)
{
	HKEY hKey;
	DWORD Disposition;
	char name[FAR_MAX_REG];
	CHK_INITED

	if (Key && *Key)
		sprintf(name,
				"%s"
				"/"
				"%s",
				FP_PluginRootKey, Key);
	else
		sprintf(name, "%s", FP_PluginRootKey);

	// WINPORT(SetLastError)(WINPORT(RegOpenKey)(HKEY_CURRENT_USER, MB2Wide(name).c_str(),&hKey));

	// if(WINPORT(GetLastError)() == ERROR_SUCCESS)
	//	return hKey;

	hKey = NULL;
	WINPORT(SetLastError)
	(WINPORT(RegCreateKeyEx)(HKEY_CURRENT_USER, MB2Wide(name).c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey,
			&Disposition));
	return hKey;
}

BOOL WINAPI FP_DeleteRegKey(const char *Key)
{
	char name[FAR_MAX_REG];
	CHK_INITED

	if (!Key || !Key[0])
		return FALSE;

	sprintf(name,
			"%s"
			"/"
			"%s",
			FP_PluginRootKey, Key);
	return FP_DeleteRegKeyFull(name);
}

BOOL WINAPI FP_DeleteRegKeyFull(const char *Key)
{
	WINPORT(SetLastError)(WINPORT(RegDeleteKey)(HKEY_CURRENT_USER, MB2Wide(Key).c_str()));
	return WINPORT(GetLastError)() == ERROR_SUCCESS;
}

BOOL WINAPI FP_DeleteRegKeyAll(LPCSTR hParentKey, LPCSTR Key)
{
	char name[FAR_MAX_REG];

	if (!Key || !Key[0])
		return FALSE;

	if (!hParentKey || !hParentKey[0])
		return FP_DeleteRegKeyAll(HKEY_CURRENT_USER, Key);
	else {
		sprintf(name,
				"%s"
				"/"
				"%s",
				hParentKey, Key);
		return FP_DeleteRegKeyAll(HKEY_CURRENT_USER, name);
	}
}

BOOL WINAPI FP_DeleteRegKeyAll(LPCSTR Key)
{
	char name[FAR_MAX_REG];
	CHK_INITED

	if (!Key || !Key[0])
		return FALSE;

	sprintf(name,
			"%s"
			"/"
			"%s",
			FP_PluginRootKey, Key);
	return FP_DeleteRegKeyAll(HKEY_CURRENT_USER, name);
}

BOOL WINAPI FP_DeleteRegKeyAll(HKEY hParentKey, LPCSTR szKey)
{
	// PROC(( "FP_DeleteRegKeyAll","%s",szKey ))
	WCHAR szSubKey[FAR_MAX_REG];
	HKEY hKey = NULL;
	LONG nRes;
	FILETIME tTime;
	DWORD nSize;

	do {
		nRes = WINPORT(RegOpenKeyEx)(hParentKey, MB2Wide(szKey).c_str(), 0,
				KEY_ENUMERATE_SUB_KEYS | KEY_READ | KEY_WRITE, &hKey);
		WINPORT(SetLastError)(nRes);	// Log(( "open [%s] rc: %s",szKey,__WINError() ));

		if (nRes != ERROR_SUCCESS)
			break;

		for (nSize = sizeof(szSubKey); 1; nSize = sizeof(szSubKey)) {
			szSubKey[0] = 0;
			nRes = WINPORT(RegEnumKeyEx)(hKey, 0, szSubKey, &nSize, 0, NULL, NULL, &tTime);
			WINPORT(SetLastError)(nRes);	// Log(( "enum [%s] rc: %s",szSubKey,__WINError() ));

			if (nRes == ERROR_NO_MORE_ITEMS) {
				nRes = ERROR_SUCCESS;
				break;
			}

			if (nRes != ERROR_SUCCESS)
				break;

			if (!FP_DeleteRegKeyAll(hKey, Wide2MB(szSubKey).c_str())) {
				nRes = WINPORT(GetLastError)();
				break;
			}
		}

		if (nRes == ERROR_SUCCESS) {
			nRes = WINPORT(RegDeleteKey)(hParentKey, MB2Wide(szKey).c_str());
			WINPORT(SetLastError)(nRes);	// Log(( "delete rc: %s", __WINError() ));
		}
	} while (0);

	if (hKey)
		WINPORT(RegCloseKey)(hKey);

	return nRes == ERROR_SUCCESS;
}

BYTE *WINAPI
FP_GetRegKey(const char *Key, const char *ValueName, BYTE *ValueData, const BYTE *Default, DWORD DataSize)
{
	DWORD Type, sz = DataSize;
	int ExitCode = -1;	// not ERROR_SUCCESS, not ERROR_MORE_DATA
	Assert(ValueData);
	HKEY hKey = FP_OpenRegKey(Key);

	if (hKey) {
		ExitCode = WINPORT(RegQueryValueEx)(hKey, MB2Wide(ValueName).c_str(), nullptr, &Type, ValueData, &sz);
		WINPORT(RegCloseKey)(hKey);
	}

	if (!hKey || (ExitCode != ERROR_SUCCESS && ExitCode != ERROR_MORE_DATA)) {
		if (Default)
			memmove(ValueData, Default, DataSize);
		else
			memset(ValueData, 0, DataSize);
	}

	return ValueData;
}

char *WINAPI
FP_GetRegKey(const char *Key, const char *ValueName, char *ValueData, LPCSTR Default, DWORD DataSize)
{
	DWORD Type, sz = DataSize;
	int ExitCode = -1;	// not ERROR_SUCCESS, not ERROR_MORE_DATA
	Assert(ValueData);
	HKEY hKey = FP_OpenRegKey(Key);

	if (hKey) {
		ExitCode = WINPORT(
				RegQueryValueEx)(hKey, MB2Wide(ValueName).c_str(), nullptr, &Type, (LPBYTE)ValueData, &sz);
		WINPORT(RegCloseKey)(hKey);
	}

	if (!hKey || (ExitCode != ERROR_SUCCESS && ExitCode != ERROR_MORE_DATA)) {
		if (Default)
			StrCpy(ValueData, Default, DataSize);
		else
			memset(ValueData, 0, DataSize);
	}

	return ValueData;
}

int WINAPI FP_GetRegKey(const char *Key, const char *ValueName, DWORD Default)
{
	int data;
	DWORD Type, Size = sizeof(data);
	HKEY hKey = FP_OpenRegKey(Key);

	if (hKey) {
		if (WINPORT(RegQueryValueEx)(hKey, MB2Wide(ValueName).c_str(), 0, &Type, (BYTE *)&data, &Size)
				!= ERROR_SUCCESS)
			data = Default;

		WINPORT(RegCloseKey)(hKey);
	} else
		data = Default;

	return data;
}

HKEY WINAPI FP_OpenRegKey(const char *Key)
{
	HKEY hKey;
	char name[FAR_MAX_REG];
	CHK_INITED

	if (Key && *Key)
		snprintf(name, sizeof(name),
				"%s"
				"/"
				"%s",
				FP_PluginRootKey, Key);
	else
		snprintf(name, sizeof(name), "%s", FP_PluginRootKey);

	if (WINPORT(RegOpenKeyEx)(HKEY_CURRENT_USER, MB2Wide(name).c_str(), 0,
				KEY_ENUMERATE_SUB_KEYS | KEY_READ | KEY_WRITE, &hKey)
			!= ERROR_SUCCESS) {
		return NULL;
	}

	return hKey;
}

BOOL WINAPI FP_SetRegKey(LPCSTR Key, LPCSTR ValueName, const BYTE *ValueData, DWORD ValueSize)
{
	HKEY hKey = FP_CreateRegKey(Key);
	BOOL rc = hKey
			&& WINPORT(RegSetValueEx)(hKey, MB2Wide(ValueName).c_str(), 0, REG_BINARY, ValueData, ValueSize)
					== ERROR_SUCCESS;
	WINPORT(RegCloseKey)(hKey);
	return rc;
}

BOOL WINAPI FP_SetRegKey(LPCSTR Key, LPCSTR ValueName, LPCSTR ValueData)
{
	HKEY hKey = FP_CreateRegKey(Key);
	BOOL rc = hKey
			&& WINPORT(RegSetValueEx)(hKey, MB2Wide(ValueName).c_str(), 0, REG_SZ_MB, (const BYTE *)ValueData,
						(int)strlen(ValueData) + 1)
					== ERROR_SUCCESS;
	WINPORT(RegCloseKey)(hKey);
	return rc;
}

BOOL WINAPI FP_SetRegKey(LPCSTR Key, LPCSTR ValueName, DWORD ValueData)
{
	HKEY hKey = FP_CreateRegKey(Key);
	BOOL rc = hKey
			&& WINPORT(RegSetValueEx)(hKey, MB2Wide(ValueName).c_str(), 0, REG_DWORD, (BYTE *)&ValueData,
						sizeof(ValueData))
					== ERROR_SUCCESS;
	WINPORT(RegCloseKey)(hKey);
	return rc;
}
