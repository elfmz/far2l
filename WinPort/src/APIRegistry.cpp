#include <set>
#include <string>
#include <locale> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

#include <wx/wx.h>
#include <wx/display.h>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include <utils.h>


struct WinPortHandleReg : WinPortHandle
{
	std::string dir;
};

#ifdef _WIN32
# define WINPORT_REG_DIV_KEY		"\\k-"
# define WINPORT_REG_DIV_VALUE	"\\v-"
#else
# define WINPORT_REG_DIV_KEY		"/k-"
# define WINPORT_REG_DIV_VALUE	"/v-"
#endif

static std::string GetRegistrySubroot(const char *sub)
{
	static std::string s_root = InMyProfile();
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


	WinPortHandleReg *rd = new WinPortHandleReg;
	if (!rd)
		return ERROR_OUTOFMEMORY;
	rd->dir.swap(dir);
	
	*phkResult = WinPortHandle_Register(rd);
	return ERROR_SUCCESS;
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
			if (StrStartsFrom(de->d_name, div + 1))  {
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



static void RegEscape(std::string &s)
{
	for (std::string::iterator i = s.begin(); i != s.end(); ++i) {
		if (*i == '\\' || *i == '\r' || *i == '\n' || *i == 0) {
			 i = s.insert(i, '\\');
			 ++i;
			 switch (*i) {
				 case '\\': /*i = '\\';*/ break;
				 case '\r': *i = 'r'; break;
				 case '\n': *i = 'n'; break;
				 case 0: *i = '0'; break;
				 default: abort();
			 }
		}
	}
}
	
static void RegUnescape(std::string &s)
{
	for (std::string::iterator i = s.begin(); i != s.end(); ++i) {
		if (*i == '\\') {
			i = s.erase(i);
			switch (*i) {
				case '\\': /*i = '\\';*/ break;
				case 'r': *i = '\r'; break;
				case 'n': *i = '\n'; break;
				case '0': *i = 0; break;
				default: 
					fprintf(stderr, "RegUnescape: unexpected escape code 0x%x\n", (unsigned int)(unsigned char)*i);
			}
		}
	}
}
	
	
static LONG RegValueDeserializeWide(const char *s, size_t l, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = *lpcbData;
	std::string us(s, l);
	RegUnescape(us);
	std::wstring ws;
	StrMB2Wide(us, ws);
	size_t total_len = ws.size() * sizeof(wchar_t);
	*lpcbData = total_len;
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
	DWORD cbData = *lpcbData;
	std::string us(s, l);
	RegUnescape(us);
	size_t total_len = us.size();
	*lpcbData = total_len;
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
	
template <class INT_T>
	static LONG RegValueDeserializeINT(const char *s, const char *fmt, LPBYTE lpData, LPDWORD lpcbData)
{
	DWORD cbData = *lpcbData;
	*lpcbData = sizeof(INT_T);
	if (lpData) {
		if (cbData < sizeof(INT_T))
			return ERROR_MORE_DATA;
		sscanf(s, fmt, (INT_T *)lpData);
	}
	return ERROR_SUCCESS;
}
		
static LONG RegValueDeserialize(const std::string &s, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	static_assert(sizeof(DWORD) == sizeof(unsigned int ), "bad DWORD size");
	static_assert(sizeof(DWORD64) == sizeof(long unsigned int ), "bad DWORD64 size");
				
	size_t prefix;
		
	if ( (prefix = StrStartsFrom(s, "DWORD:")) != 0 ) {
		if (lpType)
			*lpType = REG_QWORD;
		return RegValueDeserializeINT<unsigned int> (s.c_str() + prefix, "%x", lpData, lpcbData);
	}
		
	if ( (prefix = StrStartsFrom(s, "QWORD:")) != 0 ) {
		if (lpType)
			*lpType = REG_QWORD;
		return RegValueDeserializeINT<long unsigned int> (s.c_str() + prefix, "%lx", lpData, lpcbData);
	}

	if ( (prefix = StrStartsFrom(s, "SZ:")) != 0 ) {
		if (lpType)
			*lpType = REG_SZ;
		return RegValueDeserializeWide(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}
		
	if ( (prefix = StrStartsFrom(s, "MULTI_SZ:")) != 0 ) {
		if (lpType)
			*lpType = REG_MULTI_SZ;
		return RegValueDeserializeWide(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}
		
	if ( (prefix = StrStartsFrom(s, "EXPAND_SZ:")) != 0 ) {
		if (lpType)
			*lpType = REG_EXPAND_SZ;
		return RegValueDeserializeWide(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}

	if ( (prefix = StrStartsFrom(s, "BIN:")) != 0 ) {
		if (lpType)
			*lpType = REG_BINARY;
		return RegValueDeserializeMB(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}

	if ( (prefix = StrStartsFrom(s, "SZ_MB:")) != 0 ) {
		if (lpType)
			*lpType = REG_SZ_MB;
		return RegValueDeserializeMB(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}

	if ( (prefix = StrStartsFrom(s, "MULTI_SZ_MB:")) != 0 ) {
		if (lpType)
			*lpType = REG_MULTI_SZ_MB;
		return RegValueDeserializeMB(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}
		
	if ( (prefix = StrStartsFrom(s, "EXPAND_SZ_MB:")) != 0 ) {
		if (lpType)
			*lpType = REG_EXPAND_SZ_MB;
		return RegValueDeserializeMB(s.c_str() + prefix, s.size() - prefix, lpData, lpcbData);
	}

	size_t p = s.find(':');
	if (p==std::string::npos) {
		fprintf(stderr, "RegValueDeserialize = untyped: '%s'\n", s.c_str());
		return ERROR_READ_FAULT;
	}
		
	if (lpType)
		*lpType = atoi(s.substr(0, p).c_str());
		
	return RegValueDeserializeMB(s.c_str() + p + 1, s.size() - p - 1, lpData, lpcbData);
}
	
static void RegValueSerialize(std::ofstream &os, DWORD Type, const BYTE *lpData, DWORD cbData)
{
	static_assert(sizeof(DWORD) == sizeof(unsigned int ), "bad DWORD size");
	static_assert(sizeof(DWORD64) == sizeof(long unsigned int ), "bad DWORD64 size");
		
	switch (Type) {
		case REG_DWORD: {
			if (cbData != sizeof(DWORD))
				fprintf(stderr, "RegValueSerialize: DWORD but cbData=%u\n", cbData);
					
			os << "DWORD:" << std::hex << (unsigned int)*(const DWORD *)lpData;
		} break;

		case REG_QWORD: {
			if (cbData != sizeof(DWORD64))
				fprintf(stderr, "RegValueSerialize: QWORD but cbData=%u\n", cbData);
				
			os << "QWORD:" << std::hex << (long unsigned int)*(const DWORD64 *)lpData;
		} break;
				
		case REG_SZ: case REG_MULTI_SZ: case REG_EXPAND_SZ: {
			if (cbData % sizeof(wchar_t))
				fprintf(stderr, "RegValueSerialize: WS(%u) but cbData=%u\n", Type, cbData);

			std::string s;
			StrWide2MB( std::wstring((const wchar_t *)lpData, cbData / sizeof(wchar_t)), s);
			RegEscape(s);
				
			switch (Type) {
				case REG_SZ: os << "SZ:" << s; break;
				case REG_MULTI_SZ: os << "MULTI_SZ:" << s; break;
				case REG_EXPAND_SZ: os << "EXPAND_SZ:" << s; break;
				default: abort();
			}
		} break;

		
		//case REG_BINARY: case REG_SZ_MB: case REG_MULTI_SZ_MB: case REG_EXPAND_SZ_MB: 
		default: {
			std::string s((const char *)lpData, cbData);
			RegEscape(s);
			
			switch (Type) {
				case REG_BINARY: os << "BIN:" << s; break;
				case REG_SZ_MB: os << "SZ_MB:" << s; break;
				case REG_MULTI_SZ_MB: os << "MULTI_SZ_MB:" << s; break;
				case REG_EXPAND_SZ_MB: os << "EXPAND_SZ_MB:" << s; break;
				default: os << std::hex << Type << ":" << s;
			}				
		} break;
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
		if (!WinPortHandle_Deregister(hKey)) {
			return ERROR_INVALID_HANDLE;
		}

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
			if (remove(file.c_str())) return ERROR_PATH_NOT_FOUND;
		}

		if (rmdir(dir.c_str())!=0) {
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
		AutoWinPortHandle<WinPortHandleReg> wph(hKey);
		if (!wph) {
			fprintf(stderr, "RegDeleteValue: bad handle - %p, " WS_FMT "\n", hKey, lpValueName);
			return ERROR_INVALID_HANDLE;
		}
		
		std::string path = wph->dir; 
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
		return WINPORT(RegEnumKeyEx)( hKey, dwIndex, lpName, &cchName, &reserved, NULL, NULL, NULL);
	}


	LONG CommonQueryValue(const std::string &root, const std::string &prefixed_name,
		LPWSTR  lpValueName, LPDWORD lpcchValueName, LPDWORD lpType, LPBYTE  lpData, LPDWORD lpcbData)
	{
		std::string name, s;
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
		getline (is, s);
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
		
		LONG out2 = RegValueDeserialize(s, lpType, lpData, lpcbData);
		return (out2 == ERROR_SUCCESS) ? out : out2;
	}

	LONG WINPORT(RegEnumValue)( HKEY    hKey,
		        DWORD   dwIndex, LPWSTR  lpValueName,
				LPDWORD lpcchValueName,
				LPDWORD lpReserved,  LPDWORD lpType,
				LPBYTE  lpData, LPDWORD lpcbData )
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
		AutoWinPortHandle<WinPortHandleReg> wph(hKey);
		if (!wph) {
			fprintf(stderr, "RegSetValueEx: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		std::string s = wph->dir; 
		s+= WINPORT_REG_DIV_VALUE;
		s+= Wide2MB(lpValueName);
		//fprintf(stderr, "RegSetValue type=%u cbData=%u\n", dwType, cbData);
		if  (dwType==REG_DWORD && cbData==8)
		{
			*(volatile int*)100 = 200;
		}
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
		if (!wph)
		{//TODO: FIXME: handle predefined HKEY_-s
			fprintf(stderr, "RegQueryInfoKey: bad handle - %p\n", hKey);
			return ERROR_INVALID_HANDLE;
		}
		
		if (lpcMaxSubKeyLen) *lpcMaxSubKeyLen = 0;
		if (lpcMaxClassLen) {
			if (lpClass && lpcMaxClassLen ) *lpClass = 0;
			*lpcMaxClassLen = 0;
		}
		if (lpcMaxValueNameLen) *lpcMaxValueNameLen = 0;
		if (lpcMaxValueLen) *lpcMaxValueLen = 0;
		if (lpcbSecurityDescriptor) *lpcbSecurityDescriptor = 0;
		if (lpftLastWriteTime) memset(lpftLastWriteTime, 0, sizeof(*lpftLastWriteTime));
		
		for (DWORD i = 0;;++i) {
			DWORD namelen = 0, classlen = 0;
			LONG r = WINPORT(RegEnumKeyEx)( hKey, i, NULL, &namelen, NULL, NULL, &classlen, NULL);
			if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA) {
				if (lpcSubKeys) *lpcSubKeys = i;
				break;
			}
			if (lpcMaxSubKeyLen) *lpcMaxSubKeyLen = std::max(*lpcMaxSubKeyLen, namelen);
			if (lpcMaxClassLen) *lpcMaxClassLen = std::max(*lpcMaxClassLen, classlen);
		}
		
		for (DWORD i = 0;;++i) {
			DWORD  namelen = 0, datalen = 0;
			LONG r = WINPORT(RegEnumValue)(hKey, i, NULL, &namelen, NULL, NULL, NULL, &datalen);
			if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA) {
				if (lpcValues) *lpcValues = i;
				break;
			}
			if (lpcMaxValueNameLen) *lpcMaxValueNameLen = std::max(*lpcMaxValueNameLen, namelen);
			if (lpcMaxValueLen) *lpcMaxValueLen = std::max(*lpcMaxValueLen, datalen);
		}
		
		return ERROR_SUCCESS;
	}


	void WinPortInitRegistry()
	{
		int ret = mkdir( GetRegistrySubroot("") .c_str(), 0775);
		if (ret < 0 && EEXIST != errno)
			fprintf(stderr, "WinPortInitRegistry: errno=%d \n", errno);
		else
			fprintf(stderr, "WinPortInitRegistry: OK \n");
		mkdir(HKDir(HKEY_LOCAL_MACHINE).c_str(), 0775);
		mkdir(HKDir(HKEY_USERS).c_str(), 0775);
		mkdir(HKDir(HKEY_CURRENT_USER).c_str(), 0775);
	}

}
