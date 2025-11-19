#pragma once

// #include "CriticalSections.hpp"

#ifdef DEBUG
#define DEBUG_OUTPUT(msg) OutputDebugStringW(((msg) + L"\n").c_str())
#else
#define DEBUG_OUTPUT(msg)
#endif

#ifndef FILE_FLAG_CREATE_REPARSE_POINT
	#define FILE_FLAG_CREATE_REPARSE_POINT  0x00400000
#endif

#define _IDLE_PRIORITY_CLASS 19

#define CHECK_FILE(code, path)                                                                               \
	do {                                                                                                     \
		if (!(code))                                                                                         \
			throw Error(HRESULT_FROM_WIN32(WINPORT_GetLastError()), path, __FILE__, __LINE__);               \
	} while (false)

int _map_priority(int user_priority);
pid_t _GetCurrentProcess(void);
int _GetPriorityClass(pid_t pid);
void _SetPriorityClass(pid_t pid, int priority);

void CoCreateGuid(GUID *guid);
void StringFromGUID2A(GUID *guid, char *str, uint32_t size);
void StringFromGUID2(GUID *guid, wchar_t *str, uint32_t size);

extern HINSTANCE g_h_instance;

std::wstring get_system_message(HRESULT hr, DWORD lang_id = 0);
std::wstring get_console_title();
std::wstring ansi_to_unicode(const std::string &str, unsigned code_page);
std::string unicode_to_ansi(const std::wstring &str, unsigned code_page);
std::wstring expand_env_vars(const std::wstring &str);
std::wstring get_full_path_name(const std::wstring &path);
std::wstring get_current_directory();

CriticalSection &GetSync();
CriticalSection &GetExportSync();

/**
typedef struct _WIN32_FIND_DATAW {
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    uid_t UnixOwner;
    gid_t UnixGroup;
    DWORD64 UnixDevice;
    DWORD64 UnixNode;
    DWORD64 nPhysicalSize;
    DWORD64 nFileSize;
    DWORD dwFileAttributes;
    DWORD dwUnixMode;
    DWORD nHardLinks;
    DWORD nBlockSize;
    WCHAR cFileName[ MAX_NAME ];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW, WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _BY_HANDLE_FILE_INFORMATION {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    dwVolumeSerialNumber;
  DWORD64  nFileSize;
  DWORD    nNumberOfLinks;
  DWORD64  nFileIndex;
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

struct FAR_FIND_DATA
{
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	uint64_t nPhysicalSize;
	uint64_t nFileSize;
	DWORD    dwFileAttributes;
	DWORD    dwUnixMode;
	wchar_t *lpwszFileName;
};

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD64  nFileSize;
//  DWORD    nFileSizeHigh;
//  DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

**/

struct FindData : public WIN32_FIND_DATAW
{
	bool is_dir() const { return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
	UInt64 size() const
	{
		return nFileSize;
	}
	void set_size(UInt64 size)
	{
		nFileSize = size;
	}
};

class File : private NonCopyable
{
protected:
	HANDLE h_file;
	std::wstring m_file_path;
	char *symlinkdata;
	size_t symlinkdatasize;
	size_t symlinkRWptr;
	bool	is_symlink;

public:
	File() noexcept;
	~File() noexcept;
	File(const std::wstring &file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition,
			DWORD flags_and_attributes);
	void open(const std::wstring &file_path, DWORD desired_access, DWORD share_mode,
			DWORD creation_disposition, DWORD flags_and_attributes);
	bool open_nt(const std::wstring &file_path, DWORD desired_access, DWORD share_mode,
			DWORD creation_disposition, DWORD flags_and_attributes) noexcept;
	void close() noexcept;
	bool is_open() const noexcept { return h_file != INVALID_HANDLE_VALUE; }
	HANDLE handle() const noexcept { return h_file; }
	const std::wstring &path() const noexcept { return m_file_path; }
	char *getsymlink() noexcept { return symlinkdata;}
	const size_t getsymlinkdatasize() const noexcept {return symlinkdatasize;}

	UInt64 size();
	bool size_nt(UInt64 &file_size) noexcept;
	size_t read(void *data, size_t size);
	bool read_nt(void *data, size_t size, size_t &size_read) noexcept;
	size_t write(const void *data, size_t size);
	bool write_nt(const void *data, size_t size, size_t &size_written) noexcept;
	void set_time(const FILETIME &ctime, const FILETIME &atime, const FILETIME &mtime);
	bool set_time_nt(const FILETIME &ctime, const FILETIME &atime, const FILETIME &mtime) noexcept;
	bool copy_ctime_from(const std::wstring &source_file) noexcept;
	UInt64 set_pos(int64_t offset, DWORD method = FILE_BEGIN);
	bool set_pos_nt(int64_t offset, DWORD method = FILE_BEGIN, UInt64 *new_pos = nullptr) noexcept;
	void set_end();
	bool set_end_nt() noexcept;
	BY_HANDLE_FILE_INFORMATION get_info();
	bool get_info_nt(BY_HANDLE_FILE_INFORMATION &info) noexcept;
	template <typename Type>
	bool io_control_out_nt(DWORD code, Type &data) noexcept
	{
		DWORD size_ret;
		return DeviceIoControl(h_file, code, nullptr, 0, &data, sizeof(Type), &size_ret, nullptr) != 0;
	}
	static DWORD attributes(const std::wstring &file_path) noexcept;
	static bool attributes_ex(const std::wstring &file_path, WIN32_FILE_ATTRIBUTE_DATA *ex_attrs) noexcept;
	static bool exists(const std::wstring &file_path) noexcept;
	static void set_attr(const std::wstring &file_path, DWORD attr);
	static bool set_attr_nt(const std::wstring &file_path, DWORD attr) noexcept;
	static bool set_attr_posix(const std::wstring &file_path, DWORD attr) noexcept;

	static void delete_file(const std::wstring &file_path);
	static bool delete_file_nt(const std::wstring &file_path) noexcept;
	static void create_dir(const std::wstring &dir_path);
	static bool create_dir_nt(const std::wstring &dir_path) noexcept;
	static void remove_dir(const std::wstring &file_path);
	static bool remove_dir_nt(const std::wstring &file_path) noexcept;
	static void move_file(const std::wstring &file_path, const std::wstring &new_path, DWORD flags);
	static bool
	move_file_nt(const std::wstring &file_path, const std::wstring &new_path, DWORD flags) noexcept;
	static FindData get_find_data(const std::wstring &file_path);
	static bool get_find_data_nt(const std::wstring &file_path, FindData &find_data) noexcept;
};

/**
class Key : private NonCopyable
{
protected:
	HKEY h_key{};

public:
	Key() = default;
	~Key();
	Key(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create);
	Key &open(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create);
	bool open_nt(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create) noexcept;
	void close() noexcept;
	HKEY handle() const noexcept;
	bool query_bool(const wchar_t *name);
	bool query_bool_nt(bool &value, const wchar_t *name) noexcept;
	unsigned query_int(const wchar_t *name);
	bool query_int_nt(unsigned &value, const wchar_t *name) noexcept;
	std::wstring query_str(const wchar_t *name);
	bool query_str_nt(std::wstring &value, const wchar_t *name) noexcept;
	ByteVector query_binary(const wchar_t *name);
	bool query_binary_nt(ByteVector &value, const wchar_t *name) noexcept;
	void set_bool(const wchar_t *name, bool value);
	bool set_bool_nt(const wchar_t *name, bool value) noexcept;
	void set_int(const wchar_t *name, unsigned value);
	bool set_int_nt(const wchar_t *name, unsigned value) noexcept;
	void set_str(const wchar_t *name, const std::wstring &value);
	bool set_str_nt(const wchar_t *name, const std::wstring &value) noexcept;
	void set_binary(const wchar_t *name, const unsigned char *value, unsigned size);
	bool set_binary_nt(const wchar_t *name, const unsigned char *value, unsigned size) noexcept;
	void delete_value(const wchar_t *name);
	bool delete_value_nt(const wchar_t *name) noexcept;
	std::vector<std::wstring> enum_sub_keys();
	bool enum_sub_keys_nt(std::vector<std::wstring> &names) noexcept;
	void delete_sub_key(const wchar_t *name);
	bool delete_sub_key_nt(const wchar_t *name) noexcept;
};
**/

class FileEnum : private NonCopyable
{
protected:
	std::wstring file_mask;
	HANDLE h_find;
	FindData find_data;
	int n_far_items;
	std::list<FindData> far_items;

public:
	FileEnum(const std::wstring &file_mask) noexcept;
	~FileEnum();
	bool next();
	bool next_nt(bool &more) noexcept;
//	const FindData &data() const noexcept { return find_data; }
	FindData &data() noexcept { return find_data; }

public:
	int far_emum_cb(const FAR_FIND_DATA &item);
};

class DirList : public FileEnum
{
public:
	DirList(const std::wstring &dir_path) noexcept;
};

std::wstring get_temp_path();

class TempFile : private NonCopyable
{
private:
	std::wstring path;

public:
	TempFile();
	~TempFile();
	std::wstring get_path() const { return path; }
};

std::wstring format_file_time(const FILETIME &file_time);
std::wstring upcase(const std::wstring &str);
std::wstring create_guid();
void enable_lfh();
std::wstring search_path(const std::wstring &file_name);

class DisableSleepMode
{
private:
	EXECUTION_STATE saved_state;

public:
	DisableSleepMode() {
//		saved_state = WINPORT_SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
	}
	~DisableSleepMode()
	{
		if (saved_state) {
//			WINPORT_SetThreadExecutionState(saved_state);
		}
	}
};

class SudoRegionGuard {
private:
	bool active;
public:
	SudoRegionGuard() : active(true) {
		sudo_client_region_enter();
	}
	~SudoRegionGuard() {
		if (active) {
			sudo_client_region_leave();
		}
	}
	SudoRegionGuard(const SudoRegionGuard&) = delete;
	SudoRegionGuard& operator=(const SudoRegionGuard&) = delete;
	SudoRegionGuard(SudoRegionGuard&& other) noexcept : active(other.active) {
		other.active = false;
	}
	SudoRegionGuard& operator=(SudoRegionGuard&& other) noexcept {
		if (this != &other) {
			if (active) {
				fprintf(stderr, "WARNING: SudoRegionGuard assigned while active! Forcing leave.\n");
				sudo_client_region_leave();
			}
			active = other.active;
			other.active = false;
		}
		return *this;
	}
	void release() {
		active = false;
	}
};

class Patch7zCP
{
public:
	static void SetCP(UINT oemCP, UINT ansiCP, bool bRePatch = false);
	static int GetDefCP_OEM();
	static int GetDefCP_ANSI();
};

std::pair<DWORD, DWORD> get_posix_and_nt_attributes(DWORD RawAttributes);
