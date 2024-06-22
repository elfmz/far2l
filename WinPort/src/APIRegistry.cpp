#ifdef WINPORT_REGISTRY

#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "EnsureDir.h"
#include <utils.h>
#include <RandomString.h>
#include <os_call.hpp>

static std::atomic<int>	s_reg_wipe_count{0};

struct WinPortHandleReg : MagicWinPortHandle<1> // <1> - for reg handles
{
	std::string dir;

	virtual bool Cleanup() noexcept
	{
		return true;
	}
};

#define WINPORT_REG_PREFIX_KEY		"k-"
#define WINPORT_REG_PREFIX_VALUE	"v-"

#define WINPORT_REG_DIV_KEY		("/" WINPORT_REG_PREFIX_KEY)
#define WINPORT_REG_DIV_VALUE	("/" WINPORT_REG_PREFIX_VALUE)

static std::string GetRegistrySubroot(const char *sub)
{
	static std::string s_root = InMyConfig("REG");
	std::string rv = s_root;
	rv+= sub;
	return rv;
}

static std::string HKDir(HKEY hKey)
{
	if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_CLASSES_ROOT)
		return GetRegistrySubroot("/HKLM/software/classes"); else
	if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_CURRENT_USER)
		return GetRegistrySubroot("/HKU/c"); else
	if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_LOCAL_MACHINE)
		return GetRegistrySubroot("/HKLM"); else
	if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_USERS)
		return GetRegistrySubroot("/HKU"); else
	if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_PERFORMANCE_DATA)
		return GetRegistrySubroot("/PD"); else
	if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_PERFORMANCE_TEXT)
		return GetRegistrySubroot("/PT");

	std::string out;
	AutoWinPortHandle<WinPortHandleReg> wph(hKey);
	
	if (wph) out = wph->dir;

	return out;
}

static int try_open_rw(const char *path)
{
	return open(path, O_RDWR);
}

static std::string LitterFile(const char *path)
{
	int fd = os_call_int(try_open_rw, path);
	if (fd == -1) {
		if (errno != ENOENT)
			perror("LitterFile - open");
		return path;
	}

	struct stat s{};
	if (fstat(fd, &s) != 0)
		s.st_size = 0x10000;

	srand(fd ^ time(nullptr));

	char garbage[128];
	RandomStringBuffer(garbage, sizeof(garbage), sizeof(garbage), RNDF_NZ);
	for (off_t i = 0; i < s.st_size;) {
		const size_t piece = (s.st_size - i > (off_t)sizeof(garbage))
			? sizeof(garbage) : (size_t) (s.st_size - i);
		if (os_call_v<ssize_t, -1>(write, fd,
			(const void *)&garbage[0], piece) != (ssize_t)piece) {
			perror("LitterFile - write");
		}
		i+= piece;
	}
	//fprintf(stderr, "LitterFile - WRITTEN: '%s'\n", path);
	os_call_int(fsync, fd);
	os_call_int(close, fd);

	std::string garbage_path = path;
	size_t p = garbage_path.rfind('/');
	if (p != std::string::npos) {
		size_t l = garbage_path.size();
		if (l > p + 1) {
			garbage_path.resize(p + 1);
			RandomStringAppend(garbage_path, l - garbage_path.size(), l - garbage_path.size(), RNDF_ALNUM);
		}
	}
	if (os_call_int(rename, path, garbage_path.c_str()) == 0) {
		//fprintf(stderr, "LitterFile - RENAMED: '%s' -> '%s'\n", path, garbage_path.c_str());
		return garbage_path;
	}

	return path;
}

static int RemoveRegistryFile(const char *path)
{
	if (s_reg_wipe_count) {
		std::string garbage_path = LitterFile(path);
		garbage_path = LitterFile(garbage_path.c_str());
		garbage_path = LitterFile(garbage_path.c_str());
		return os_call_int(remove, garbage_path.c_str());
	}
	return os_call_int(remove, path);
}

LONG RegXxxKeyEx(
	bool    create,
	HKEY    hKey,
	LPCWSTR lpSubKey,
	PHKEY   phkResult,
	LPDWORD lpdwDisposition = nullptr)
{
	std::string dir = HKDir(hKey);
	if (dir.empty())
		return ERROR_INVALID_HANDLE;

	AppendAndRectifyPath(dir, WINPORT_REG_DIV_KEY, lpSubKey);
	struct stat s{};
	if (stat(dir.c_str(), &s)==-1) {
		if (!create) {
			//fprintf(stderr, "RegXxxKeyEx: can't open %s\n", dir.c_str());
			return ERROR_FILE_NOT_FOUND;
		}

		for (size_t i = 0, ii = dir.size(); i<=ii; ++i) {
			if (i==ii || dir[i]==GOOD_SLASH) {
				if (mkdir(dir.substr(0, i).c_str(), 0775)==0)
					fprintf(stderr, "RegXxxKeyEx: creating %s\n", dir.c_str());
					
			}
		}
		if (stat(dir.c_str(), &s)==-1)
			return ERROR_ACCESS_DENIED;
		if (lpdwDisposition)
			*lpdwDisposition = REG_CREATED_NEW_KEY;
	} else if (lpdwDisposition)
			*lpdwDisposition = REG_OPENED_EXISTING_KEY;


	WinPortHandleReg *rd = new(std::nothrow) WinPortHandleReg;
	if (UNLIKELY(!rd))
		return ERROR_OUTOFMEMORY;
	rd->dir.swap(dir);
	
	*phkResult = rd->Register();
	return ERROR_SUCCESS;
}

static std::string LookupIndexedRegItem(const std::string &root, const char *div, DWORD index)
{
	std::string out;
	fprintf(stderr, "LookupIndexedRegItem: '%s' '%s' %u\n", root.c_str(), div, index);

#ifdef _WIN32
	std::string path = root;
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
			if (StrStartsFrom(de->d_name, div + 1)) {
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



void RegEscape(std::string &s)
{
	for (std::string::iterator i = s.begin(); i != s.end(); ++i) {
		if (*i == '\\' || *i == '\r' || *i == '\n' || *i == 0) {
			 i = s.insert(i, '\\');
			 ++i;
			 switch (*i) {
				 case '\\': /*i = '\\';*/ break;
				 case '\r': *i = 'r'; break;
				 case '\n': *i = 'n'; break;
				 case 0: 
					*i = '0';
					if (i + 1 != s.end()) {
						++i;
						i = s.insert(i, '\n');
					}
				 break;
				 default:
					ABORT();
			 }
		}
	}
}
	
void RegUnescape(std::string &s)
{
	for (std::string::iterator i = s.begin(); i != s.end(); ) {
		if (*i == '\\') {
			i = s.erase(i);
			if (i == s.end()) break;
			switch (*i) {
				case '\\': /*i = '\\';*/ break;
				case 'r': *i = '\r'; break;
				case 'n': *i = '\n'; break;
				case '0': *i = 0; break;
				default: 
					fprintf(stderr, "RegUnescape: unexpected escape code 0x%x\n", (unsigned int)(unsigned char)*i);
			}
			++i;
		} else if (*i == '\r' || *i == '\n' || *i == 0) {
			i = s.erase(i);
		} else
			++i;
	}
}
	
	
static LONG RegValueDeserializeWide(const char *s, size_t l, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = 0;
	std::string us(s, l);
	RegUnescape(us);
	std::wstring ws;
	StrMB2Wide(us, ws);
	size_t total_len = ws.size() * sizeof(wchar_t);
	if(lpcbData) {
		cbData = *lpcbData;
		*lpcbData = total_len;
	}
	if (lpData) {
		if (total_len > cbData)
			return ERROR_MORE_DATA;

		if (total_len + sizeof(wchar_t) <= cbData)
			memcpy(lpData, ws.c_str(), total_len + sizeof(wchar_t)); //ensure null if have enough place
		else
			memcpy(lpData, ws.c_str(), total_len);
	}
	return ERROR_SUCCESS;
}
	
static LONG RegValueDeserializeMB(const char *s, size_t l, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = 0;
	std::string us(s, l);
	RegUnescape(us);
	size_t total_len = us.size();
	if(lpcbData) {
		cbData = *lpcbData;
		*lpcbData = total_len;
	}
	if (lpData) {
		if (total_len > cbData)
			return ERROR_MORE_DATA;
		if (total_len + 1 <= cbData)
			memcpy(lpData, us.c_str(), total_len + 1); //ensure null if have enough place
		else
			memcpy(lpData, us.c_str(), total_len);
	}
	return ERROR_SUCCESS;
}

inline void dosscanf(const char *s, unsigned int *var)
{ sscanf(s, "%x", var); }

inline void dosscanf(const char *s, unsigned long long *var)
{ sscanf(s, "%llx", var); }

template <class INT_T>
	static LONG RegValueDeserializeINT(const char *s, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = 0;
	if(lpcbData) {
		cbData = *lpcbData;
		*lpcbData = sizeof(INT_T);
	}
	if (lpData) {
		if (cbData < sizeof(INT_T))
			return ERROR_MORE_DATA;
		dosscanf(s, (INT_T *)lpData);
	}
	return ERROR_SUCCESS;
}

static LONG RegValueDeserializeBinary(const char *s, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = 0;
	if(lpcbData) {
		cbData = *lpcbData;
		*lpcbData = 0;
	}
	for (;;) {
		while (s[0]=='\n' || s[0]=='\r' || s[0]==' ') ++s;
		if (!s[0] || !s[1]) break;
		if (lpData && *lpcbData < cbData) {
			char tmp[] = {s[0], s[1], 0};
			unsigned int v = 0;
			sscanf(tmp, "%x", &v);
			lpData[*lpcbData] = (BYTE)v;
		}
		(*lpcbData)++;
		s+= 2;
	}
	return (!lpData || *lpcbData <= cbData) ? ERROR_SUCCESS : ERROR_MORE_DATA;
}

static LONG RegValueDeserialize(const std::string &tip, const std::string &s, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	static_assert(sizeof(DWORD) == sizeof(unsigned int ), "bad DWORD size");
	static_assert(sizeof(DWORD64) == sizeof(unsigned long long), "bad DWORD64 size");

	if(lpData && !lpcbData) {
		return ERROR_INVALID_PARAMETER;
	}
				
	if ( tip == "DWORD") {
		if (lpType)
			*lpType = REG_DWORD;
		return RegValueDeserializeINT<unsigned int> (s.c_str(), lpData, lpcbData);
	}
		
	if ( tip == "QWORD") {
		if (lpType)
			*lpType = REG_QWORD;
		return RegValueDeserializeINT<unsigned long long> (s.c_str(), lpData, lpcbData);
	}

	if ( tip == "SZ") {
		if (lpType)
			*lpType = REG_SZ;
		return RegValueDeserializeWide(s.c_str(), s.size(), lpData, lpcbData);
	}
		
	if ( tip == "MULTI_SZ") {
		if (lpType)
			*lpType = REG_MULTI_SZ;
		return RegValueDeserializeWide(s.c_str(), s.size(), lpData, lpcbData);
	}
		
	if ( tip == "EXPAND_SZ") {
		if (lpType)
			*lpType = REG_EXPAND_SZ;
		return RegValueDeserializeWide(s.c_str(), s.size(), lpData, lpcbData);
	}

	if ( tip == "SZ_MB") {
		if (lpType)
			*lpType = REG_SZ_MB;
		return RegValueDeserializeMB(s.c_str(), s.size(), lpData, lpcbData);
	}

	if ( tip == "MULTI_SZ_MB") {
		if (lpType)
			*lpType = REG_MULTI_SZ_MB;
		return RegValueDeserializeMB(s.c_str(), s.size(), lpData, lpcbData);
	}
		
	if ( tip == "EXPAND_SZ_MB") {
		if (lpType)
			*lpType = REG_EXPAND_SZ_MB;
		return RegValueDeserializeMB(s.c_str(), s.size(), lpData, lpcbData);
	}
	
	if (lpType) {
		*lpType = REG_BINARY;
		if ( tip != "BINARY") {
			sscanf(tip.c_str(), "%x", (unsigned int *)lpType);
		}
	}
		
	return RegValueDeserializeBinary(s.c_str(), lpData, lpcbData);
}
	
static void RegValueSerializeBinary(std::ofstream &os, const BYTE *lpData, DWORD cbData)
{
	os << std::hex;
	char tmp[8];
	for (DWORD i = 0; i < cbData; ++i) {
		if (i != 0 && (i % 16) == 0) {
			os << std::endl;
		}
		snprintf(tmp, sizeof(tmp), "%02x ", (unsigned int)(unsigned char)lpData[i]);
		os << tmp;
	}
}

static void RegValueSerialize(std::ofstream &os, DWORD Type, const VOID *lpData, DWORD cbData)
{
	static_assert(sizeof(DWORD) == sizeof(unsigned int ), "bad DWORD size");
	static_assert(sizeof(DWORD64) == sizeof(unsigned long long), "bad DWORD64 size");

	switch (Type) {
		case REG_DWORD: {
			if (cbData != sizeof(DWORD))
				fprintf(stderr, "RegValueSerialize: DWORD but cbData=%u\n", cbData);

			os << "DWORD" << std::endl << std::hex << (unsigned int)*(const DWORD *)lpData;
		} break;

		case REG_QWORD: {
			if (cbData != sizeof(DWORD64))
				fprintf(stderr, "RegValueSerialize: QWORD but cbData=%u\n", cbData);
				
			os << "QWORD" << std::endl << std::hex << (unsigned long long)*(const DWORD64 *)lpData;
		} break;
				
		case REG_SZ: case REG_MULTI_SZ: case REG_EXPAND_SZ: {
			if (cbData % sizeof(wchar_t))
				fprintf(stderr, "RegValueSerialize: WS(%u) but cbData=%u\n", Type, cbData);

			std::string s;
			StrWide2MB( std::wstring((const wchar_t *)lpData, cbData / sizeof(wchar_t)), s);
			RegEscape(s);
			
			switch (Type) {
				case REG_SZ: os << "SZ" << std::endl << s; break;
				case REG_MULTI_SZ: os << "MULTI_SZ" << std::endl << s; break;
				case REG_EXPAND_SZ: os << "EXPAND_SZ" << std::endl << s; break;
				default:
					ABORT();
			}
		} break;
		
		case REG_SZ_MB: case REG_MULTI_SZ_MB: case REG_EXPAND_SZ_MB: {
			std::string s((const char *)lpData, cbData);
			RegEscape(s);
			
			switch (Type) {
				case REG_SZ_MB: os << "SZ_MB" << std::endl << s; break;
				case REG_MULTI_SZ_MB: os << "MULTI_SZ_MB" << std::endl << s; break;
				case REG_EXPAND_SZ_MB: os << "EXPAND_SZ_MB" << std::endl << s; break;
			}				
		} break;
		
		case REG_BINARY: {
			os << "BINARY" << std::endl;
			RegValueSerializeBinary(os, (const BYTE *)lpData, cbData);
		} break;

		default: {
			os << std::hex << Type << std::endl;
			RegValueSerializeBinary(os, (const BYTE *)lpData, cbData);
		}
	}
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
		if (!hKey || hKey == (HKEY)INVALID_HANDLE_VALUE)
			return ERROR_INVALID_HANDLE;
		if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_CLASSES_ROOT)
			return ERROR_INVALID_HANDLE;
		if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_CURRENT_USER)
			return ERROR_INVALID_HANDLE;
		if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_LOCAL_MACHINE)
			return ERROR_INVALID_HANDLE;
		if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_USERS)
			return ERROR_INVALID_HANDLE;
		if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_PERFORMANCE_DATA)
			return ERROR_INVALID_HANDLE;
		if ((ULONG_PTR)hKey == (ULONG_PTR)HKEY_PERFORMANCE_TEXT)
			return ERROR_INVALID_HANDLE;

		if (!WinPortHandle::Deregister(hKey))
			return ERROR_INVALID_HANDLE;

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

		for (;;)
		{
			std::string name = LookupIndexedRegItem(dir, WINPORT_REG_DIV_VALUE, 0);
			if (name.empty())
				break;
			std::string file = dir + "/" + name;
			if (RemoveRegistryFile(file.c_str()) != 0)
				return ERROR_PATH_NOT_FOUND;
		}

		if (rmdir(dir.c_str())!=0) {
			struct stat s{};
			if (stat(dir.c_str(), &s)==0)
				return ERROR_DIR_NOT_EMPTY;
		}
		return ERROR_SUCCESS;
	}

	LONG WINPORT(RegDeleteValue)(
		HKEY    hKey,
		LPCWSTR lpValueName)
	{
		AutoWinPortHandle<WinPortHandleReg> wph(hKey);
		if (UNLIKELY(!wph)) {
			fprintf(stderr, "RegDeleteValue: bad handle - %p, %ls\n", hKey, lpValueName);
			return ERROR_INVALID_HANDLE;
		}
		
		std::string path = wph->dir; 
		path+= WINPORT_REG_DIV_VALUE;
		if (lpValueName) path+= Wide2MB(lpValueName);
		return (remove(path.c_str()) == 0) ? ERROR_SUCCESS : ERROR_PATH_NOT_FOUND;
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
			//fprintf(stderr, "RegEnumKeyEx: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string name = LookupIndexedRegItem(root, WINPORT_REG_DIV_KEY, dwIndex);
		if (name.empty()) {
			return ERROR_NO_MORE_ITEMS;
		}

		name.erase(0, strlen(WINPORT_REG_DIV_KEY)-1);

		std::wstring wname = StrMB2Wide(name);
		if (lpcClass) *lpcClass = 0;
		if (*lpcName <= wname.size()) {
			*lpcName = wname.size() + 1;
			return ERROR_MORE_DATA;
		}
		memcpy(lpName, wname.c_str(), (wname.size() + 1) * sizeof(*lpName));
		*lpcName = wname.size();
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
		return WINPORT(RegEnumKeyEx)( hKey, dwIndex, lpName, &cchName, &reserved, nullptr, nullptr, nullptr);
	}


	LONG CommonQueryValue(const std::string &root, const std::string &prefixed_name,
		LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
	{
		std::string name, tip, s;
		std::ifstream is;
		std::string path = root; 
		path+= GOOD_SLASH;
		path+= prefixed_name;
		
		is.open(path.c_str());
		if (!is.is_open()) {
			//fprintf(stderr, "RegQueryValue: not found %s\n", path.c_str());
			return ERROR_FILE_NOT_FOUND;
		}
		
		getline (is, name);
		getline (is, tip);
		getline (is, s, (char)0);
		//fprintf(stderr, "RegQueryValue: '%s' '%s' '%s' %p\n", prefixed_name.c_str(), type.c_str(), value.c_str(), lpData);
		LONG out = ERROR_SUCCESS;
		const std::wstring &namew = StrMB2Wide(name);
		if (lpValueName && lpcchValueName) {
			if (namew.size() < *lpcchValueName) {
				wcscpy(lpValueName, (const wchar_t *)namew.c_str());
			} else
				out = ERROR_MORE_DATA;
		}
		if (lpcchValueName)
			*lpcchValueName = namew.size();
		
		LONG out2 = RegValueDeserialize(tip, s, lpType, lpData, lpcbData);
		return (out2 == ERROR_SUCCESS) ? out : out2;
	}

	LONG WINPORT(RegEnumValue)(
		HKEY    hKey,
		DWORD   dwIndex,
		LPWSTR  lpValueName,
		LPDWORD lpcchValueName,
		LPDWORD lpReserved,
		LPDWORD lpType,
		LPBYTE  lpData,
		LPDWORD lpcbData)
	{
		const std::string &root = HKDir(hKey);
		if (root.empty()) {
			fprintf(stderr, "RegEnumValue: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string name = LookupIndexedRegItem(root, WINPORT_REG_DIV_VALUE, dwIndex);
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
		
		std::string prefixed_name = WINPORT_REG_PREFIX_VALUE;
		prefixed_name+= Wide2MB(lpValueName);
		return CommonQueryValue(root, prefixed_name,
			nullptr, nullptr, lpType, lpData, lpcbData);
	}

	LONG WINPORT(RegSetValueEx)(
			HKEY         hKey,
			LPCWSTR      lpValueName,
			DWORD        Reserved,
			DWORD        dwType,
			const BYTE  *lpData,
			DWORD        cbData
		)
	{
		AutoWinPortHandle<WinPortHandleReg> wph(hKey);
		if (UNLIKELY(!wph)) {
			fprintf(stderr, "RegSetValueEx: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string s = wph->dir; 
		s+= WINPORT_REG_DIV_VALUE;
		s+= Wide2MB(lpValueName);
		//fprintf(stderr, "RegSetValue type=%u cbData=%u\n", dwType, cbData);
		std::ofstream os;
		os.open(s.c_str());
		Wide2MB(lpValueName, s);
		os << s << std::endl;
		RegValueSerialize(os, dwType, lpData, cbData);
		os << std::endl;
		return ERROR_SUCCESS;
	}
	
	LONG WINPORT(RegQueryInfoKey)(
		_In_        HKEY      hKey,
		_Out_opt_   LPTSTR    lpClass,
		_Inout_opt_ LPDWORD   lpcClass,
		_Reserved_  LPDWORD   lpReserved,
		_Out_opt_   LPDWORD   lpcSubKeys,
		_Out_opt_   LPDWORD   lpcMaxSubKeyLen,
		_Out_opt_   LPDWORD   lpcMaxClassLen,
		_Out_opt_   LPDWORD   lpcValues,
		_Out_opt_   LPDWORD   lpcMaxValueNameLen,
		_Out_opt_   LPDWORD   lpcMaxValueLen,
		_Out_opt_   LPDWORD   lpcbSecurityDescriptor,
		_Out_opt_   PFILETIME lpftLastWriteTime)
	{
		AutoWinPortHandle<WinPortHandleReg> wph(hKey);
		if (UNLIKELY(!wph))
		{//TODO: FIXME: handle predefined HKEY_-s
			fprintf(stderr, "RegQueryInfoKey: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		
		if (lpcMaxSubKeyLen) *lpcMaxSubKeyLen = 0;
		if (lpcMaxClassLen) {
			if (lpClass) *lpClass = 0;
			*lpcMaxClassLen = 0;
		}
		if (lpcMaxValueNameLen) *lpcMaxValueNameLen = 0;
		if (lpcMaxValueLen) *lpcMaxValueLen = 0;
		if (lpcbSecurityDescriptor) *lpcbSecurityDescriptor = 0;
		if (lpftLastWriteTime) memset(lpftLastWriteTime, 0, sizeof(*lpftLastWriteTime));
		
		for (DWORD i = 0;;++i) {
			DWORD namelen = 0, classlen = 0;
			LONG r = WINPORT(RegEnumKeyEx)( hKey, i, nullptr, &namelen, nullptr, nullptr, &classlen, nullptr);
			if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA) {
				if (lpcSubKeys) *lpcSubKeys = i;
				break;
			}
			if (lpcMaxSubKeyLen) *lpcMaxSubKeyLen = std::max(*lpcMaxSubKeyLen, namelen);
			if (lpcMaxClassLen) *lpcMaxClassLen = std::max(*lpcMaxClassLen, classlen);
		}
		
		for (DWORD i = 0;;++i) {
			DWORD namelen = 0, datalen = 0;
			LONG r = WINPORT(RegEnumValue)(hKey, i, nullptr, &namelen, nullptr, nullptr, nullptr, &datalen);
			if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA) {
				if (lpcValues) *lpcValues = i;
				break;
			}
			if (lpcMaxValueNameLen) *lpcMaxValueNameLen = std::max(*lpcMaxValueNameLen, namelen);
			if (lpcMaxValueLen) *lpcMaxValueLen = std::max(*lpcMaxValueLen, datalen);
		}
		
		return ERROR_SUCCESS;
	}


	WINPORT_DECL(RegWipeBegin, VOID, ())
	{
		++s_reg_wipe_count;
	}
	
	WINPORT_DECL(RegWipeEnd, VOID, ())
	{
		--s_reg_wipe_count;
		ASSERT(s_reg_wipe_count >= 0);
	}

	void WinPortInitRegistry()
	{
		unsigned int fail_mask = 0;
		if (!EnsureDir(GetRegistrySubroot("").c_str())) fail_mask|= 0x1;
		if (!EnsureDir(HKDir(HKEY_LOCAL_MACHINE).c_str())) fail_mask|= 0x2;
		if (!EnsureDir(HKDir(HKEY_USERS).c_str())) fail_mask|= 0x4;
		if (!EnsureDir(HKDir(HKEY_CURRENT_USER).c_str())) fail_mask|= 0x8;
		if (fail_mask)
			fprintf(stderr, "WinPortInitRegistry: fail_mask=0x%x errno=%d \n", fail_mask, errno);
		else
			fprintf(stderr, "WinPortInitRegistry: OK \n");
	}

}
#else

extern "C" void WinPortInitRegistry() { }

#endif // WINPORT_REGISTRY
