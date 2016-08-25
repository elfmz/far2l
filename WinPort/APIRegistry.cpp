#include "stdafx.h"
#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

#include "WinCompat.h"
#include "WinPort.h"
#include "Utils.h"

struct RegDescriptor
{
	std::string dir;
};

#ifdef _WIN32
# define WINPORT_REGISTRY_SUBROOT	"\\WinPort"
# define WINPORT_REG_DIV_KEY		"\\k-"
# define WINPORT_REG_DIV_VALUE	"\\v-"
#else
# define WINPORT_REGISTRY_SUBROOT	"/.WinPort"
# define WINPORT_REG_DIV_KEY		"/k-"
# define WINPORT_REG_DIV_VALUE	"/v-"
#endif

static std::string GetRegistryRoot()
{
#ifdef _WIN32
	std::string rv = "D:";
#else
	std::string rv = getenv("HOME");
	if (rv.empty())
		rv = "/tmp";
#endif	
	rv+= WINPORT_REGISTRY_SUBROOT;
	return rv;	
}

static std::string GetRegistrySubroot(const char *sub)
{
	static std::string s_root = GetRegistryRoot();
	std::string rv = s_root;
	rv+= sub;
	return rv;
}

struct RegDescriptors : std::set<RegDescriptor *>, std::mutex  {} g_reg_descriptors;
static std::string HKDir(HKEY hKey)
{
	switch ((ULONG_PTR)hKey) {
	case (ULONG_PTR)HKEY_CLASSES_ROOT: return GetRegistrySubroot("/hklm/software/classes"); 
	case (ULONG_PTR)HKEY_CURRENT_USER: return GetRegistrySubroot("/hku/c"); 
	case (ULONG_PTR)HKEY_LOCAL_MACHINE: return GetRegistrySubroot("/hklm"); 
	case (ULONG_PTR)HKEY_USERS: return GetRegistrySubroot("/hku") ;
	case (ULONG_PTR)HKEY_PERFORMANCE_DATA: return GetRegistrySubroot("/pd"); 
	case (ULONG_PTR)HKEY_PERFORMANCE_TEXT: return GetRegistrySubroot("/pt"); 
	}

	std::string out;
	RegDescriptor *rd = reinterpret_cast<RegDescriptor *>(hKey);
	std::lock_guard<std::mutex> lock(g_reg_descriptors);
	if (g_reg_descriptors.find(rd)!=g_reg_descriptors.end())
		out = rd->dir;

	//if (out.empty()) fprintf(stderr, "HKDir(0x%x) - not found\n", hKey);
	return out;
}


LONG RegXxxKeyEx(
	bool create,
	HKEY    hKey,
	LPCWSTR lpSubKey,
	PHKEY   phkResult,
	LPDWORD lpdwDisposition = NULL)
{
	std::string dir = HKDir(hKey);
	if (dir.empty())
		return ERROR_INVALID_HANDLE;

	AppendAndRectifyPath(dir, WINPORT_REG_DIV_KEY, lpSubKey);
	struct stat s = {0};
	if (stat(dir.c_str(), &s)==-1) {
		if (!create) {
			//fprintf(stderr, "RegXxxKeyEx: can't open %s\n", dir.c_str());
			return ERROR_FILE_NOT_FOUND;
		}

		for (size_t i = 0, ii = dir.size(); i<=ii; ++i)  {
			if (i==ii || dir[i]==GOOD_SLASH) {
				if (_mkdir(dir.substr(0, i).c_str())==0)
					fprintf(stderr, "RegXxxKeyEx: creating %s\n", dir.c_str());
					
			}
		}
		if (stat(dir.c_str(), &s)==-1)
			return ERROR_ACCESS_DENIED;
		if (lpdwDisposition)
			*lpdwDisposition = REG_CREATED_NEW_KEY;
	} else if (lpdwDisposition)
			*lpdwDisposition = REG_OPENED_EXISTING_KEY;


	RegDescriptor *rd = new RegDescriptor;
	if (!rd)
		return ERROR_OUTOFMEMORY;
	rd->dir.swap(dir);
	std::lock_guard<std::mutex> lock(g_reg_descriptors);
	g_reg_descriptors.insert(rd);
	*phkResult = reinterpret_cast<HKEY>(rd);
	//fprintf(stderr, "RegXxxKeyEx: opened %s\n", rd->dir.c_str());
	return ERROR_SUCCESS;
}

static bool StringStartsWith(const char *str, const char *beginning)
{
	for (;;) {
		if (!*beginning) return true;
		if (*str!=*beginning) return false;
		++str;
		++beginning;
	}
}

static std::string LookupIndexedRegItem(const std::string &root, const char *div,  DWORD index)
{
	std::string out;
	fprintf(stderr, "LookupIndexedRegItem: '%s' '%s' %u\n", root.c_str(), div, index);

#ifdef _WIN32
	std::string  path = root;
	path+= div;
	path+= '*';
	
	WIN32_FIND_DATAA wfd = {0};
	HANDLE fh = ::FindFirstFileA(path.c_str(), &wfd);
	if (fh!=INVALID_HANDLE_VALUE) {
		DWORD i = 0;
		do {
			if (i==index) break;
			++i;
		} while (::FindNextFileA(fh, &wfd));
		::FindClose(fh);
		if (i==index)
			out.assign(&wfd.cFileName[0]);
	}
#else
	DIR *d = opendir(root.c_str());
	if (d) {
		for (DWORD i = 0;;) {
			dirent *de = readdir(d);
			if (!de) break;
			if (StringStartsWith(de->d_name, div + 1))  {
				if (i==index) {
					out = de->d_name;
					break;
				}
				++i;
			}
		}
		closedir(d);
	} else
		fprintf(stderr, "LookupIndexedRegItem(%s) - opendir failed\n", root.c_str());
#endif
	return out;
}

extern "C" {
	LONG WINPORT(RegOpenKeyEx) (
		     HKEY    hKey,
		 LPCWSTR lpSubKey,
		     DWORD   ulOptions,
		     REGSAM  samDesired,
		    PHKEY   phkResult)
	{
		//return RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired,phkResult);
		return RegXxxKeyEx(false, hKey, lpSubKey, phkResult);
	}

	LONG WINPORT(RegCreateKeyEx)(
		       HKEY                  hKey,
		       LPCWSTR               lpSubKey,
		 DWORD                 Reserved,
		   LPWSTR                lpClass,
		       DWORD                 dwOptions,
		       REGSAM                samDesired,
		   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		      PHKEY                 phkResult,
		  LPDWORD               lpdwDisposition
		)
	{
		//return RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, ulOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
		return RegXxxKeyEx(true, hKey, lpSubKey, phkResult, lpdwDisposition);
	}

	LONG WINPORT(RegCloseKey)(HKEY hKey)
	{
		//return RegCloseKey(hKey);
		RegDescriptor *rd = reinterpret_cast<RegDescriptor *>(hKey);
		{
			std::lock_guard<std::mutex> lock(g_reg_descriptors);
			if (!g_reg_descriptors.erase(rd))
				return ERROR_INVALID_HANDLE;
		}

		delete rd;
		return ERROR_SUCCESS;
	}

	LONG WINPORT(RegDeleteKey)(
		 HKEY    hKey,
		 LPCWSTR lpSubKey
		)
	{
		std::string dir = HKDir(hKey);
		if (dir.empty())
			return ERROR_INVALID_HANDLE;

		if (lpSubKey)
			AppendAndRectifyPath(dir, WINPORT_REG_DIV_KEY, lpSubKey);

		if (_rmdir(dir.c_str())!=0) {
			struct stat s = {0};
			if (stat(dir.c_str(), &s)==0)
				return ERROR_DIR_NOT_EMPTY;
		}
		return ERROR_SUCCESS;
	}

	LONG WINPORT(RegDeleteValue)(
		    HKEY    hKey,
		 LPCWSTR lpValueName)
	{
		RegDescriptor *rd = reinterpret_cast<RegDescriptor *>(hKey);
		std::lock_guard<std::mutex> lock(g_reg_descriptors);
		if (g_reg_descriptors.find(rd)==g_reg_descriptors.end()) {
			fprintf(stderr, "RegDeleteValue: bad handle - %p, " WS_FMT "\n", hKey, lpValueName);
			return ERROR_INVALID_HANDLE;
		}
		std::string path = rd->dir; 
		path+= WINPORT_REG_DIV_VALUE;
		if (lpValueName) path+= Wide2MB(lpValueName);
		return (remove(path.c_str())==0) ? ERROR_SUCCESS : ERROR_PATH_NOT_FOUND;
	}


	LONG WINPORT(RegEnumKeyEx)(
		        HKEY      hKey,
		        DWORD     dwIndex,
		       LPWSTR    lpName,
		     LPDWORD   lpcName,
		  LPDWORD   lpReserved,
		     LPWSTR    lpClass,
		 LPDWORD   lpcClass,
		   PFILETIME lpftLastWriteTime
		)
	{
		const std::string &root = HKDir(hKey);
		if (root.empty()) {
			fprintf(stderr, "RegEnumKeyEx: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string name = LookupIndexedRegItem(root, WINPORT_REG_DIV_KEY,  dwIndex);
		if (name.empty()) 
			return ERROR_NO_MORE_ITEMS;

		name.erase(0, strlen(WINPORT_REG_DIV_KEY)-1);

		std::wstring name16 = StrMB2Wide(name);
		if (lpcClass) *lpcClass = 0;
		if (*lpcName <= name16.size()) {
			*lpcName = name16.size() + 1;
			return ERROR_MORE_DATA;
		}
		memcpy(lpName, name16.c_str(), (name16.size() + 1) * sizeof(*lpName));
		*lpcName = name16.size();
		return ERROR_SUCCESS;
	}


	LONG WINPORT(RegEnumKey)(
		  HKEY   hKey,
		  DWORD  dwIndex,
		 LPWSTR lpName,
		  DWORD  cchName
		)
	{
		DWORD reserved = 0;
		return WINPORT(RegEnumKeyEx)( hKey, dwIndex, lpName, &cchName, &reserved, NULL, NULL, NULL);
	}

	LONG CommonQueryValue(const std::string &root, const std::string &prefixed_name,
		LPWSTR  lpValueName, LPDWORD lpcchValueName, LPDWORD lpType, LPBYTE  lpData, LPDWORD lpcbData)
	{
		std::string name, type, value;
		std::ifstream is;
		std::string path = root; 
		path+= GOOD_SLASH;
		path+= prefixed_name;
		
		is.open(path.c_str());
		if (!is.is_open()) {
			fprintf(stderr, "RegQueryValue: not found %s\n", path.c_str());
			return ERROR_FILE_NOT_FOUND;
		}
		getline (is, name);
		getline (is, type);
		getline (is, value);
		//fprintf(stderr, "RegQueryValue: '%s' '%s' '%s' %p\n", prefixed_name.c_str(), type.c_str(), value.c_str(), lpData);
		if (lpValueName) {
			const std::wstring &u16name = StrMB2Wide(name);
			if (*lpcchValueName <= u16name.size())
				return ERROR_MORE_DATA;
			wcscpy(lpValueName, (const wchar_t *)u16name.c_str());
			*lpcchValueName = u16name.size();
		}
		DWORD tip = (DWORD)atoi(type.c_str());
		if (lpType) *lpType = tip;
		if (lpData && lpcbData) {
			for (DWORD i = 0;;++i)
			{
				if (i * 2 >= value.size())
					break;
				if (i>=*lpcbData) {
					*lpcbData = value.size() / 2;
					//fprintf(stderr, "RegQueryValue: '%s' '%s' '%s' SIZE %u  MORE_DATA\n", prefixed_name.c_str(), type.c_str(), value.c_str(),*lpcbData);
					return ERROR_MORE_DATA;
				}

				lpData[i] = Hex2Byte(value.c_str() + i * 2);
			}			
		}
		if (lpcbData)
			*lpcbData = value.size()/2;
		//if (!lpData) {while (!IsDebuggerPresent())Sleep(1000); DebugBreak();}
		/*
		if (tip==REG_SZ || tip==REG_MULTI_SZ|| tip==REG_EXPAND_SZ)
			fprintf(stderr, "RegQueryValue: '%s' '%s' '%s' SIZE %u '%ls'\n", prefixed_name.c_str(), type.c_str(), value.c_str(),*lpcbData, (WCHAR*)lpData);
		else if (lpData)
			fprintf(stderr, "RegQueryValue: '%s' '%s' '%s' SIZE %u 0x%x\n", prefixed_name.c_str(), type.c_str(), value.c_str(), *lpcbData, *lpData);
		else 
			fprintf(stderr, "RegQueryValue: '%s' '%s' '%s' SIZE %u \n", prefixed_name.c_str(), type.c_str(), value.c_str(), *lpcbData);
		*/
		return ERROR_SUCCESS;
	}

	LONG WINPORT(RegEnumValue)(
		        HKEY    hKey,
		        DWORD   dwIndex,
		       LPWSTR  lpValueName,
		     LPDWORD lpcchValueName,
		  LPDWORD lpReserved,
		   LPDWORD lpType,
		   LPBYTE  lpData,
		 LPDWORD lpcbData
		)
	{
		const std::string &root = HKDir(hKey);
		if (root.empty()) {
			fprintf(stderr, "RegEnumValue: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string name = LookupIndexedRegItem(root, WINPORT_REG_DIV_VALUE,  dwIndex);
		if (name.empty())
			return ERROR_NO_MORE_ITEMS;

		return CommonQueryValue(root, name, lpValueName, lpcchValueName, lpType, lpData, lpcbData);
	}

	LONG WINPORT(RegQueryValueEx) (HKEY hKey, LPCWSTR lpValueName, 
		LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
	{
		const std::string &root = HKDir(hKey);
		if (root.empty()) {
			fprintf(stderr, "RegQueryValueEx: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		
		std::string prefixed_name = WINPORT_REG_DIV_VALUE + 1;
		prefixed_name+= Wide2MB(lpValueName);
		return CommonQueryValue(root, prefixed_name,
			NULL, NULL, lpType, lpData, lpcbData);
	}

	LONG WINPORT(RegSetValueEx)(
		             HKEY    hKey,
		         LPCWSTR lpValueName,
		       DWORD   Reserved,
		             DWORD   dwType,
		       const BYTE    *lpData,
		             DWORD   cbData
		)
	{
		RegDescriptor *rd = reinterpret_cast<RegDescriptor *>(hKey);
		std::lock_guard<std::mutex> lock(g_reg_descriptors);
		if (g_reg_descriptors.find(rd)==g_reg_descriptors.end()) {
			fprintf(stderr, "RegSetValueEx: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string path = rd->dir; 
		path+= WINPORT_REG_DIV_VALUE;
		path+= Wide2MB(lpValueName);
		//fprintf(stderr, "RegSetValue type=%u cbData=%u\n", dwType, cbData);
		if  (dwType==REG_DWORD && cbData==8)
		{
			*(volatile int*)100 = 200;
		}
		std::ofstream os;
		os.open(path.c_str());
		std::string name = Wide2MB(lpValueName);
		os << name << std::endl;
		os << dwType << std::endl;
		for (DWORD i = 0; i < cbData; ++i) {
			if (lpData[i] < 0x10) os << '0';
			os << std::hex << (unsigned int)lpData[i];
		}
		os << std::endl;
		return ERROR_SUCCESS;
	}
/*

	LONG WINPORT(RegQueryValueExA) (HKEY hKey, LPCSTR lpValueName, 
		LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
	{
		return WINPORT(RegQueryValueEx)(hKey, MB2Wide(lpValueName).c_str(), 
			lpReserved, lpType, lpData, lpcbData);
	}

	LONG WINPORT(RegSetValueExA)(
		             HKEY    hKey,
		         LPCSTR lpValueName,
		       DWORD   Reserved,
		             DWORD   dwType,
		       const BYTE    *lpData,
		             DWORD   cbData
		)
	{
		std::vector<WCHAR> tmp;
		return WINPORT(RegSetValueEx)(hKey, MB2Wide(lpValueName).c_str(), 
			lpReserved, lpType, lpData, lpcbData);
	}
*/
	void WinPortInitRegistry()
	{
		if (_mkdir( GetRegistrySubroot("") .c_str()) <0)
			fprintf(stderr, "WinPortInitRegistry: errno=%d \n", errno);
		else
			fprintf(stderr, "WinPortInitRegistry: OK \n");
		_mkdir(HKDir(HKEY_LOCAL_MACHINE).c_str());
		_mkdir(HKDir(HKEY_USERS).c_str());
		_mkdir(HKDir(HKEY_CURRENT_USER).c_str());
	}

}
