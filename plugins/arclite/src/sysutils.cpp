#include "headers.hpp"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"

#include "Environment.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

HINSTANCE g_h_instance = nullptr;

int _map_priority(int user_priority) {
    switch (user_priority) {
        case 0: return 19;   // Low
        case 1: return 10;   // Below Normal
        case 2: return 0;    // Normal
        case 3: return -5;   // Above Normal
        case 4: return -10;  // High
        case 5: return -20;  // Highest
        default:
            fprintf(stderr, "Invalid priority value. Using default (Normal).\n");
            return 0; // Default to Normal
    }
}

pid_t _GetCurrentProcess(void) {
	return getpid();
}

int _GetPriorityClass(pid_t pid) {
    int current_priority = getpriority(PRIO_PROCESS, pid);
    if (current_priority == -1) {
		return 0;
    }
    return current_priority;
}

void _SetPriorityClass(pid_t pid, int priority) {
    if (setpriority(PRIO_PROCESS, pid, priority) != 0) {
		fprintf(stderr, "arclite:SetPriorityClass(): Failed to set process priority\n");
    }
}

uint32_t xs30_seed[4] = {0x3D696D09, 0xCD6BEB33, 0x9D1A0022, 0x9D1B0022};
static inline uint32_t zRAND(uint32_t *zseed = &xs30_seed[0])
{
	uint32_t t;
	zseed[0] ^= zseed[0] << 16;
	zseed[0] ^= zseed[0] >> 5;
	zseed[0] ^= zseed[0] << 1;
	t = zseed[0];
	zseed[0] = zseed[1];
	zseed[1] = zseed[2];
	zseed[2] = t ^ zseed[0] ^ zseed[1];
	return zseed[0];
}

void CoCreateGuid(GUID *guid)
{
	uint32_t *ddd = (uint32_t *)guid;
	if (!ddd)
		return;
	ddd[0] = zRAND();
	ddd[1] = zRAND();
	ddd[2] = zRAND();
	ddd[3] = zRAND();
}

void StringFromGUID2A(GUID *guid, char *str, uint32_t size)
{
	char *_bytes = (char *)guid;
	snprintf(str, size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", _bytes[0] & 0xff,
			_bytes[1] & 0xff, _bytes[2] & 0xff, _bytes[3] & 0xff, _bytes[4] & 0xff, _bytes[5] & 0xff,
			_bytes[6] & 0xff, _bytes[7] & 0xff, _bytes[8] & 0xff, _bytes[9] & 0xff, _bytes[10] & 0xff,
			_bytes[11] & 0xff, _bytes[12] & 0xff, _bytes[13] & 0xff, _bytes[14] & 0xff, _bytes[15] & 0xff);
}

void StringFromGUID2(GUID *guid, wchar_t *str, uint32_t size)
{
	char *_bytes = (char *)guid;
	swprintf(str, size, L"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			_bytes[0] & 0xff, _bytes[1] & 0xff, _bytes[2] & 0xff, _bytes[3] & 0xff, _bytes[4] & 0xff,
			_bytes[5] & 0xff, _bytes[6] & 0xff, _bytes[7] & 0xff, _bytes[8] & 0xff, _bytes[9] & 0xff,
			_bytes[10] & 0xff, _bytes[11] & 0xff, _bytes[12] & 0xff, _bytes[13] & 0xff, _bytes[14] & 0xff,
			_bytes[15] & 0xff);
}

CriticalSection &GetSync()
{
	static CriticalSection sync;
	return sync;
}

CriticalSection &GetExportSync()
{
	static CriticalSection sync;
	return sync;
}

std::wstring get_system_message(HRESULT hr, DWORD lang_id)
{
	std::wostringstream st;
	const char *error_str = NULL;

	switch ((Int32)hr) {
    // case ERROR_NO_MORE_FILES   : s = "No more files"; break;
    // case ERROR_DIRECTORY       : s = "Error Directory"; break;

	//TODO: Localize:
    case E_NOTIMPL             : error_str = "E_NOTIMPL : Not implemented"; break;
    case E_NOINTERFACE         : error_str = "E_NOINTERFACE : No such interface supported"; break;
    case E_ABORT               : error_str = "E_ABORT : Operation aborted"; break;
    case E_FAIL                : error_str = "E_FAIL : Unspecified error"; break;

    case STG_E_INVALIDFUNCTION : error_str = "STG_E_INVALIDFUNCTION"; break;
    case CLASS_E_CLASSNOTAVAILABLE : error_str = "CLASS_E_CLASSNOTAVAILABLE"; break;

    case E_OUTOFMEMORY         : error_str = "E_OUTOFMEMORY : Can't allocate required memory"; break;
    case E_INVALIDARG          : error_str = "E_INVALIDARG : One or more arguments are invalid"; break;
	//TODO: Localize:

    // case MY__E_ERROR_NEGATIVE_SEEK : s = "MY__E_ERROR_NEGATIVE_SEEK"; break;
    default:
      break;
	}

	if (error_str == NULL)
		error_str = strerror(hr);

	if (error_str) {
		std::wstring message;
		StrMB2Wide(error_str, message);
	    st << strip(message) << L" (0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << hr << L")";
	}
	else {
		st << L"HRESULT: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << hr;
	}

#if 0
  wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, lang_id, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
  if (!len && lang_id && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND)
    len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, 0, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
  if (!len) {
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
      HMODULE h_winhttp = GetModuleHandle(L"winhttp");
      if (h_winhttp) {
        len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, h_winhttp, HRESULT_CODE(hr), lang_id, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
        if (!len && lang_id && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND)
          len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, h_winhttp, HRESULT_CODE(hr), 0, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
      }
    }
  }
  if (len) {
    std::wstring message;
    try {
      message = sys_msg;
    }
    catch (...) {
      LocalFree(static_cast<HLOCAL>(sys_msg));
      throw;
    }
    LocalFree(static_cast<HLOCAL>(sys_msg));
    st << strip(message) << L" (0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << hr << L")";
  }
  else {
    st << L"HRESULT: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << hr;
  }
#else
//	st << L"HRESULT: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << hr;
#endif

	return st.str();
}

std::wstring get_console_title()
{
	wchar_t buf[2048];
	DWORD size = WINPORT(GetConsoleTitle)(NULL, buf, 2048);
	return std::wstring(buf, size);
}

std::wstring ansi_to_unicode(const std::string &str, unsigned code_page)
{
	unsigned str_size = static_cast<unsigned>(str.size());
	if (str_size == 0)
		return std::wstring();
	int size = WINPORT_MultiByteToWideChar(code_page, 0, str.data(), str_size, nullptr, 0);
	Buffer<wchar_t> out(size);
	size = WINPORT_MultiByteToWideChar(code_page, 0, str.data(), str_size, out.data(), size);
	CHECK_SYS(size);
	return std::wstring(out.data(), size);
}

std::string unicode_to_ansi(const std::wstring &str, unsigned code_page)
{
	unsigned str_size = static_cast<unsigned>(str.size());
	if (str_size == 0)
		return std::string();
	int size = WINPORT_WideCharToMultiByte(code_page, 0, str.data(), str_size, nullptr, 0, nullptr, nullptr);
	Buffer<char> out(size);
	size = WINPORT_WideCharToMultiByte(code_page, 0, str.data(), str_size, out.data(), size, nullptr,
			nullptr);
	CHECK_SYS(size);
	return std::string(out.data(), size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t ExpandEnvironmentStringsW(const wchar_t *input, wchar_t *output, size_t output_size)
{
	const wchar_t *start = input;
	wchar_t *out_ptr = output;
	size_t remaining_size = output_size;

	while (*start && remaining_size > 1) {
		if (*start == L'$' && *(start + 1) == L'{') {
			start += 2;	   // skip '${'
			const wchar_t *end = wcschr(start, L'}');
			if (end) {
				size_t var_len = end - start;
				wchar_t var_name[256];
				wcsncpy(var_name, start, var_len);
				var_name[var_len] = L'\0';

				const char *var_value = getenv(Wide2MB(var_name).c_str());
				if (var_value) {
					wchar_t w_value[256];
					mbstowcs(w_value, var_value, 256);
					size_t value_len = wcslen(w_value);

					if (value_len < remaining_size) {
						wcscpy(out_ptr, w_value);
						out_ptr += value_len;
						remaining_size -= value_len;
					}
				}
				start = end + 1;	// skip '}'
			}
		} else {
			*out_ptr++ = *start++;
			remaining_size--;
		}
	}
	*out_ptr = L'\0';
	return (out_ptr - output);
}


std::wstring expand_env_vars(const std::wstring &str)
{
#if 1
	std::string new_path_mb;
	StrWide2MB(str, new_path_mb);
	Environment::ExpandString(new_path_mb, true);
	std::wstring result;
	StrMB2Wide(new_path_mb, result);
	return result;
#else
	Buffer<wchar_t> buf(MAX_PATH);
	unsigned size = ExpandEnvironmentStringsW(str.c_str(), buf.data(), static_cast<DWORD>(buf.size()));
	return std::wstring(buf.data(), size);
#endif
}

std::wstring get_full_path_name(const std::wstring &path)
{
	Buffer<wchar_t> buf(MAX_PATH);
	DWORD size = WINPORT_GetFullPathName(path.c_str(), static_cast<DWORD>(buf.size()), buf.data(), nullptr);
	if (size > buf.size()) {
		buf.resize(size);
		size = WINPORT_GetFullPathName(path.c_str(), static_cast<DWORD>(buf.size()), buf.data(), nullptr);
	}
	CHECK_SYS(size);
	return std::wstring(buf.data(), size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::wstring get_current_directory()
{
	Buffer<wchar_t> buf(MAX_PATH);
	DWORD size = WINPORT_GetCurrentDirectory(static_cast<DWORD>(buf.size()), buf.data());
	if (size > buf.size()) {
		buf.resize(size);
		size = WINPORT_GetCurrentDirectory(static_cast<DWORD>(buf.size()), buf.data());
	}
	CHECK_SYS(size);
	return std::wstring(buf.data(), size);
}

File::File() noexcept {
	h_file = INVALID_HANDLE_VALUE;
	is_symlink = false;
	symlinkdatasize = symlinkRWptr = 0;
	symlinkdata = NULL;
}

File::~File() noexcept
{	close();
}

File::File(const std::wstring &file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition,
		DWORD flags_and_attributes)
{
	h_file = INVALID_HANDLE_VALUE;
	is_symlink = false;
	symlinkdatasize = symlinkRWptr = 0;
	symlinkdata = NULL;
	open(file_path, desired_access, share_mode, creation_disposition, flags_and_attributes);
}

void File::open(const std::wstring &file_path, DWORD desired_access, DWORD share_mode,
		DWORD creation_disposition, DWORD flags_and_attributes)
{
	CHECK_FILE(open_nt(file_path, desired_access, share_mode, creation_disposition, flags_and_attributes),
			file_path);
}

bool File::open_nt(const std::wstring &file_path, DWORD desired_access, DWORD share_mode,
		DWORD creation_disposition, DWORD flags_and_attributes) noexcept
{
	close();
	m_file_path = file_path;

	if (flags_and_attributes & FILE_FLAG_CREATE_REPARSE_POINT) {
		is_symlink = true;
		symlinkdata = (char *)malloc(PATH_MAX + 1);
		if (!symlinkdata) {
			WINPORT_SetLastError(E_OUTOFMEMORY);
			return false;
		}
		symlinkRWptr = 0;
		symlinkdatasize = 0;
		h_file = (HANDLE)0x999;
		return true;
	}

	if (flags_and_attributes & FILE_FLAG_OPEN_REPARSE_POINT) {

		char symtp[PATH_MAX + 1];
		int symtp_len;
		symlinkdata = (char *)malloc(PATH_MAX + 1);
		if (!symlinkdata) {
			WINPORT_SetLastError(E_OUTOFMEMORY);
			return false;
		}
		symlinkRWptr = 0;

		symtp_len = sdc_readlink(Wide2MB(file_path.c_str()).c_str(), symtp, PATH_MAX);
		if (symtp_len <= 0) {
			//fprintf(stderr, "symtp_len <= 0 - return false for %ls\n", file_path.c_str() );
			return false;
		}
		symtp[symtp_len] = 0;

		std::string final_target;

		if (creation_disposition == 1) {
			final_target = make_absolute_symlink_target(Wide2MB(file_path.c_str()), std::string(symtp));
		} else if (creation_disposition == 2) {
			final_target = make_relative_symlink_target(Wide2MB(file_path.c_str()), std::string(symtp));
		} else {
			final_target = symtp;
		}

		if (final_target.size() >= PATH_MAX) {
			final_target.resize(PATH_MAX - 1);
		}
		memcpy(symlinkdata, final_target.c_str(), final_target.size() + 1);
		symlinkdatasize = final_target.size();

		is_symlink = true;
		h_file = (HANDLE)0x999;
		return true;
	}

	h_file = WINPORT_CreateFile(long_path_norm(file_path).c_str(), desired_access, share_mode, nullptr,
			creation_disposition, flags_and_attributes, nullptr);

	//fprintf(stderr, "h_file = %p\n", h_file );

	return h_file != INVALID_HANDLE_VALUE;
}

void File::close() noexcept
{
	if (symlinkdata) {
		free(symlinkdata);
		symlinkdata = NULL;
	}

	symlinkdatasize = symlinkRWptr = 0;

	if (is_symlink) {
		h_file = INVALID_HANDLE_VALUE;
		is_symlink = false;
		return;
	}

	if (h_file != INVALID_HANDLE_VALUE) {
		WINPORT_CloseHandle(h_file);
		h_file = INVALID_HANDLE_VALUE;
	}
}

UInt64 File::size()
{
	UInt64 file_size;
	CHECK_FILE(size_nt(file_size), m_file_path);
	return file_size;
}

bool File::size_nt(UInt64 &file_size) noexcept
{
	LARGE_INTEGER fs;
	if (is_symlink) {
		file_size = symlinkdatasize;
		return true;
	}

	if (WINPORT_GetFileSizeEx(h_file, &fs)) {
		file_size = fs.QuadPart;
		return true;
	} else {
		return false;
	}
}

size_t File::read(void *data, size_t size)
{
	size_t size_read;
	CHECK_FILE(read_nt(data, size, size_read), m_file_path);
	return size_read;
}

bool File::read_nt(void *data, size_t size, size_t &size_read) noexcept
{
	DWORD sz;
	if (is_symlink) {
		if (symlinkRWptr >= symlinkdatasize || size <= 0) {
			size_read = 0;
			return true;
		}
		if(size >= symlinkdatasize - symlinkRWptr) {
			memcpy(data, symlinkdata, symlinkdatasize - symlinkRWptr);
			size_read = symlinkdatasize - symlinkRWptr;
			symlinkRWptr = symlinkdatasize;
		}
		else {
			memcpy(data, symlinkdata, size);
			size_read = size;
			symlinkRWptr += size;
		}
		return true;
	}
	if (WINPORT_ReadFile(h_file, data, static_cast<DWORD>(size), &sz, nullptr)) {
		size_read = sz;
		return true;
	} else
		return false;
}

size_t File::write(const void *data, size_t size)
{
	size_t size_written = 0;
	CHECK_FILE(write_nt(data, size, size_written), m_file_path);
	return size_written;
}

bool File::write_nt(const void *data, size_t size, size_t &size_written) noexcept
{
	DWORD sz;
	if (is_symlink) {
		if (symlinkRWptr >= PATH_MAX || size <= 0) {
			size_written = 0;
			return true;
		}
		if(size >= PATH_MAX - symlinkRWptr) {
			memcpy(symlinkdata, data, PATH_MAX - symlinkRWptr);
			size_written = PATH_MAX - symlinkRWptr;
			symlinkRWptr = PATH_MAX;
		}
		else {
			memcpy(symlinkdata, data, size);
			size_written = size;
			symlinkRWptr += size;
		}
		if (symlinkdatasize < symlinkRWptr)
			symlinkdatasize = symlinkRWptr;

		return true;
	}
	if (WINPORT_WriteFile(h_file, data, static_cast<DWORD>(size), &sz, nullptr)) {
		size_written = sz;
		return true;
	} else
		return false;
}

void File::set_time(const FILETIME &ctime, const FILETIME &atime, const FILETIME &mtime)
{
	CHECK_FILE(set_time_nt(ctime, atime, mtime), m_file_path);
};

bool File::set_time_nt(const FILETIME &ctime, const FILETIME &atime, const FILETIME &mtime) noexcept
{
	if (is_symlink) return true;
	return WINPORT_SetFileTime(h_file, &ctime, &atime, &mtime) != 0;
};

bool File::copy_ctime_from(const std::wstring &source_file) noexcept
{
	WIN32_FILE_ATTRIBUTE_DATA fa;
	if (!attributes_ex(source_file, &fa))
		return false;
	FILETIME crft;
	WINPORT(GetSystemTimeAsFileTime)(&crft);
	return set_time_nt(fa.ftCreationTime, crft, crft);
}

UInt64 File::set_pos(int64_t offset, DWORD method)
{
	UInt64 new_pos = 0;
	CHECK_FILE(set_pos_nt(offset, method, &new_pos), m_file_path);
	return new_pos;
}

bool File::set_pos_nt(int64_t offset, DWORD method, UInt64 *new_pos) noexcept
{
	LARGE_INTEGER distance_to_move, new_file_pointer;
	distance_to_move.QuadPart = offset;

	if (is_symlink) {
		switch(method) {
		case FILE_BEGIN: 
			symlinkRWptr = *new_pos;
			break;
		case FILE_CURRENT: 
			symlinkRWptr += *new_pos;
			break;
		case FILE_END: 
			if (*new_pos >= symlinkdatasize)
				symlinkRWptr = 0;
			else
				symlinkRWptr = symlinkdatasize - *new_pos;
			break;
		}
		if (symlinkRWptr > symlinkdatasize)
			symlinkRWptr = symlinkdatasize;
		return true;
	}

	if (!WINPORT_SetFilePointerEx(h_file, distance_to_move, &new_file_pointer, method))
		return false;
	if (new_pos)
		*new_pos = new_file_pointer.QuadPart;
	return true;
}

void File::set_end()
{
	CHECK_FILE(set_end_nt(), m_file_path);
}

bool File::set_end_nt() noexcept
{
	if (is_symlink) {
		symlinkRWptr = symlinkdatasize;
		return true;
	}
	return WINPORT_SetEndOfFile(h_file) != 0;
}

BY_HANDLE_FILE_INFORMATION File::get_info()
{
	BY_HANDLE_FILE_INFORMATION info;
	CHECK_FILE(get_info_nt(info), m_file_path);
	return info;
}

bool File::get_info_nt(BY_HANDLE_FILE_INFORMATION &info) noexcept
{
	if (is_symlink) return true;

	info.dwFileAttributes = 0;
	WINPORT_GetFileTime(h_file, &info.ftCreationTime, &info.ftLastAccessTime, &info.ftLastWriteTime);
	WINPORT_GetFileSizeEx(h_file, (PLARGE_INTEGER)&info.nFileSize);
	return true;
}

bool File::exists(const std::wstring &file_path) noexcept
{
	return attributes(file_path) != INVALID_FILE_ATTRIBUTES;
}

DWORD File::attributes(const std::wstring &file_path) noexcept
{
	return WINPORT_GetFileAttributes(long_path_norm(file_path).c_str());
}

bool File::attributes_ex(const std::wstring &file_path, WIN32_FILE_ATTRIBUTE_DATA *ex_attrs) noexcept
{
	static int have_attributes_ex = 0;
	//fprintf(stderr, " (!) File::attributes_ex %ls\n", file_path.c_str());

#if 0
  static BOOL (WINAPI *pfGetFileAttributesExW)(LPCWSTR pname, GET_FILEEX_INFO_LEVELS level, LPVOID pinfo) = nullptr;
  if (have_attributes_ex == 0) {
	auto pf = GetProcAddress(GetModuleHandleW(L"kernel32"), "GetFileAttributesExW");
	if (pf == nullptr)
	  have_attributes_ex = -1;
	else {
	  pfGetFileAttributesExW = reinterpret_cast<decltype(pfGetFileAttributesExW)>(reinterpret_cast<void*>(pf));
	  have_attributes_ex = +1;
	}
  }
#endif

	auto norm_path = long_path_norm(file_path);
	if (have_attributes_ex > 0) {
		//	return pfGetFileAttributesExW(norm_path.c_str(), GetFileExInfoStandard, ex_attrs) != FALSE;
	} else {
		WIN32_FIND_DATAW ff;
		auto hfind = WINPORT_FindFirstFile(norm_path.c_str(), &ff);
		if (hfind == INVALID_HANDLE_VALUE)
			return false;
		WINPORT_FindClose(hfind);
		ex_attrs->dwFileAttributes = ff.dwFileAttributes;
		ex_attrs->ftCreationTime = ff.ftCreationTime;
		ex_attrs->ftLastWriteTime = ff.ftLastWriteTime;
		ex_attrs->ftLastAccessTime = ff.ftLastAccessTime;
		ex_attrs->nFileSize = ff.nFileSize;
		return true;
	}
	return true;
}

void File::set_attr(const std::wstring &file_path, DWORD attr)
{
	//  CHECK_FILE(set_attr_nt(file_path, attr), file_path);
	set_attr_nt(file_path, attr);
}

bool File::set_attr_nt(const std::wstring &file_path, DWORD attr) noexcept
{
	//fprintf(stderr, " (!) File::set_attr_nt %ls\n", file_path.c_str());
	return WINPORT_SetFileAttributes(long_path_norm(file_path).c_str(), attr) != 0;
}

bool File::set_attr_posix(const std::wstring &file_path, DWORD attr) noexcept
{
	//fprintf(stderr, " (!) File::set_attr_posix %ls\n", file_path.c_str());
	//  const auto system_functions = Far::get_system_functions();
	//  if (system_functions)
	//	return system_functions->SetFileAttributes(long_path_norm(file_path).c_str(), attr) != 0;
	//  else
//	return WINPORT_SetFileAttributes(long_path_norm(file_path).c_str(), attr) != 0;
	return false;
}

void File::delete_file(const std::wstring &file_path)
{
	CHECK_FILE(delete_file_nt(file_path), file_path);
}

bool File::delete_file_nt(const std::wstring &file_path) noexcept
{
	//fprintf(stderr, "file delete_file_nt\n" );
	return WINPORT_DeleteFile(long_path_norm(file_path).c_str()) != 0;
}

void File::create_dir(const std::wstring &file_path)
{
	//fprintf(stderr, " (!) File::create_dir %ls\n", file_path.c_str());
	//  CHECK_FILE(create_dir_nt(file_path), file_path);

	create_dir_nt(file_path);
}

bool File::create_dir_nt(const std::wstring &file_path) noexcept
{
	//fprintf(stderr, "file remove_dir_nt %ls\n", file_path.c_str() );
	return WINPORT_CreateDirectory(long_path_norm(file_path).c_str(), nullptr) != 0;
}

void File::remove_dir(const std::wstring &file_path)
{
	CHECK_FILE(remove_dir_nt(file_path), file_path);
}

bool File::remove_dir_nt(const std::wstring &file_path) noexcept
{
	//fprintf(stderr, " (!) File::remove_dir_nt %ls\n", file_path.c_str());
	return WINPORT_RemoveDirectory(long_path_norm(file_path).c_str()) != 0;
}

void File::move_file(const std::wstring &file_path, const std::wstring &new_path, DWORD flags)
{
	CHECK_FILE(move_file_nt(file_path, new_path, flags), file_path);
}

bool File::move_file_nt(const std::wstring &file_path, const std::wstring &new_path, DWORD flags) noexcept
{
	//fprintf(stderr, " (!) File::move_file_nt %ls\n", file_path.c_str());
	return WINPORT_MoveFileEx(long_path_norm(file_path).c_str(), long_path_norm(new_path).c_str(), flags) != 0;
}

FindData File::get_find_data(const std::wstring &file_path)
{
	FindData find_data;
	CHECK_FILE(get_find_data_nt(file_path, find_data), file_path);
	return find_data;
}

bool File::get_find_data_nt(const std::wstring &file_path, FindData &find_data) noexcept
{
	if (!Far::g_fsf.GetFindData(file_path.c_str(), &find_data)) {
		return false;
	}

	return true;
}

#undef CHECK_FILE

FileEnum::FileEnum(const std::wstring &file_mask) noexcept
	: file_mask(file_mask), h_find(INVALID_HANDLE_VALUE)
{
	n_far_items = -1;
}

FileEnum::~FileEnum()
{
	if (h_find != INVALID_HANDLE_VALUE)
		WINPORT_FindClose(h_find);
}

bool FileEnum::next()
{
	bool more = false;

	next_nt(more);

//	if (!next_nt(more))
//		throw Error(HRESULT_FROM_WIN32(WINPORT_GetLastError()), file_mask, __FILE__, __LINE__);
	return more;
}

static int WINAPI find_cb(const struct FAR_FIND_DATA *FData, const wchar_t *FullName, void *Param)
{
	(void)FullName;
	return ((FileEnum *)Param)->far_emum_cb(*FData);
}

#ifndef TOOLS_TOOL
int FileEnum::far_emum_cb(const FAR_FIND_DATA &item)
{
	far_items.emplace_back();
	auto &fdata = far_items.back();

	fdata.dwFileAttributes = item.dwFileAttributes;
	fdata.ftCreationTime = item.ftCreationTime;
	fdata.ftLastAccessTime = item.ftLastAccessTime;
	fdata.ftLastWriteTime = item.ftLastWriteTime;

	fdata.nFileSize = item.nFileSize;

	std::wcsncpy(fdata.cFileName, null_to_empty(item.lpwszFileName),
			sizeof(fdata.cFileName) / sizeof(fdata.cFileName[0]));

	++n_far_items;
	return TRUE;
}
#endif

bool FileEnum::next_nt(bool &more) noexcept
{
	for (;;) {
		if (h_find == INVALID_HANDLE_VALUE) {
			if (n_far_items >= 0) {
				more = (n_far_items > 0);
				if (!more)
					return true;
				find_data = far_items.front();
				far_items.pop_front();
				--n_far_items;
			}
			else {
				if ((h_find = WINPORT_FindFirstFile(long_path(file_mask).c_str(), &find_data))
						== INVALID_HANDLE_VALUE) {
					if (WINPORT_GetLastError() == ERROR_ACCESS_DENIED) {	//
						auto dir = extract_file_path(file_mask);	// M$ FindFirst/NextFile doesn't work for
																	// junction/symlink folder.
						auto msk = extract_file_name(
								file_mask);					   // Try to use FarRecursiveSearch in such case.
						if (!dir.empty() && !msk.empty()) {	   //
							auto attr = File::attributes(dir);	  //
#ifndef TOOLS_TOOL
							if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)
									&& (attr & FILE_ATTRIBUTE_REPARSE_POINT)) {
								n_far_items = 0;
								Far::g_fsf.FarRecursiveSearch(long_path(dir).c_str(), msk.c_str(), find_cb,
										FRS_NONE, this);

								continue;
							}
#endif
						}
					}
					more = false;
					return false;
				}
			}
		}
		else {
			if (!WINPORT_FindNextFile(h_find, &find_data)) {
				if (WINPORT_GetLastError() == ERROR_NO_MORE_FILES) {
					more = false;
					return true;
				}
				return false;
			}
		}

		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if ((find_data.cFileName[0] == L'.')
					&& ((find_data.cFileName[1] == 0)
							|| ((find_data.cFileName[1] == L'.') && (find_data.cFileName[2] == 0))))
				continue;
		}

		auto mask_dot_pos = file_mask.find_last_of(L'.');	 // avoid found "name.ext_" using mask "*.ext"
		if (mask_dot_pos != std::wstring::npos
				&& file_mask.find_first_of(L'*', mask_dot_pos) == std::wstring::npos) {
			const auto last_dot_in_fname = std::wcsrchr(find_data.cFileName, L'.');
			if (nullptr != last_dot_in_fname
					&& std::wcslen(last_dot_in_fname) > file_mask.size() - mask_dot_pos)
				continue;
		}

		more = true;
		return true;
	}
}

DirList::DirList(const std::wstring &dir_path) noexcept : FileEnum(add_trailing_slash(dir_path) + L'*') {}

std::wstring get_temp_path()
{
#if 0
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD len = WINPORT_GetTempPath(static_cast<DWORD>(buf.size()), buf.data());
  CHECK(len <= buf.size());
  CHECK_SYS(len);
  return std::wstring(buf.data(), len);
#else
	return std::wstring(L"/tmp/");
#endif
}

TempFile::TempFile()
{
	Buffer<wchar_t> buf(MAX_PATH);
	std::wstring temp_path = get_temp_path();
	CHECK_SYS(WINPORT_GetTempFileName(temp_path.c_str(), L"", 0, buf.data()));
	path.assign(buf.data());
}

TempFile::~TempFile()
{
	WINPORT_DeleteFile(path.c_str());
}

std::wstring format_file_time(const FILETIME &file_time)
{
	FILETIME local_ft;
	CHECK_SYS(WINPORT_FileTimeToLocalFileTime(&file_time, &local_ft));
	SYSTEMTIME st;
	CHECK_SYS(WINPORT_FileTimeToSystemTime(&local_ft, &st));
	wchar_t DateSeparator;
	wchar_t TimeSeparator;
	wchar_t DecimalSeparator;
	wchar_t tempbuff[64];
	int iDateFormat;
	int n;

	//  Buffer<wchar_t> buf(1024);
	//  CHECK_SYS(GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, buf.data(),
	//  static_cast<int>(buf.size()))); std::wstring date_str = buf.data();
	//  CHECK_SYS(GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, nullptr, buf.data(),
	//  static_cast<int>(buf.size()))); std::wstring time_str = buf.data(); return date_str + L' ' + time_str;

	DateSeparator = Far::g_fsf.GetDateSeparator();
	TimeSeparator = Far::g_fsf.GetTimeSeparator();
	DecimalSeparator = Far::g_fsf.GetDecimalSeparator();

	iDateFormat = Far::g_fsf.GetDateFormat();

	switch (iDateFormat) {
		case 0:
			n = swprintf(tempbuff, 16, L"%02d%c%02d%c%04d", st.wMonth, DateSeparator, st.wDay, DateSeparator,
					st.wYear);
			break;
		case 1:
			n = swprintf(tempbuff, 16, L"%02d%c%02d%c%04d", st.wDay, DateSeparator, st.wMonth, DateSeparator,
					st.wYear);
			break;
		default:
			n = swprintf(tempbuff, 16, L"%04d%c%02d%c%02d", st.wYear, DateSeparator, st.wMonth, DateSeparator,
					st.wDay);
			break;
	}

	tempbuff[n++] = 32;

	if (st.wMilliseconds) {
		n += swprintf(tempbuff + n, 16, L"%02d%c%02d%c%02d%c%03d", st.wHour, TimeSeparator, st.wMinute, TimeSeparator,
					st.wSecond, DecimalSeparator, st.wMilliseconds);
	}
	else {
		n += swprintf(tempbuff + n, 16, L"%02d%c%02d%c%02d", st.wHour, TimeSeparator, st.wMinute, TimeSeparator,
					st.wSecond);
	}

	return std::wstring(tempbuff, n);
}

std::wstring upcase(const std::wstring &str)
{
	Buffer<wchar_t> up_str(str.size());
	std::wmemcpy(up_str.data(), str.data(), str.size());
	WINPORT_CharUpperBuff(up_str.data(), static_cast<DWORD>(up_str.size()));
	return std::wstring(up_str.data(), up_str.size());
}

std::wstring create_guid()
{
	GUID guid;
	CoCreateGuid(&guid);
	wchar_t guid_str[50];
	StringFromGUID2(&guid, guid_str, ARRAYSIZE(guid_str));
	return guid_str;
}

void enable_lfh()
{
	//  ULONG heap_info = 2;
	//  HeapSetInformation(reinterpret_cast<HANDLE>(_get_heap_handle()), HeapCompatibilityInformation,
	//  &heap_info, sizeof(heap_info));
}

std::wstring search_path(const std::wstring &file_name)
{
	fprintf(stderr, "(!!!!!!!!!!!!) search_path() do nothing\n");
	// TODO: From far2l
#if 0
  Buffer<wchar_t> path(MAX_PATH);
  wchar_t* name_ptr;
  DWORD size = SearchPathW(nullptr, file_name.c_str(), nullptr, static_cast<DWORD>(path.size()), path.data(), &name_ptr);
  if (size > path.size()) {
	path.resize(size);
	size = SearchPathW(nullptr, file_name.c_str(), nullptr, static_cast<DWORD>(path.size()), path.data(), &name_ptr);
  }
  CHECK_SYS(size);
  CHECK(size < path.size());
  return std::wstring(path.data(), size);
#else
	return std::wstring(L"Not today");
#endif
	// WINPORT_
}

std::pair<DWORD, DWORD> get_posix_and_nt_attributes(DWORD const RawAttributes)
{
	// some programs store posix attributes in high 16 bits.
	// p7zip - stores additional 0x8000 flag marker.
	// macos - stores additional 0x4000 flag marker.
	// info-zip - no additional marker.

	if (RawAttributes & 0xF0000000)
		return {RawAttributes >> 16, RawAttributes & 0x3FFF};

	return {0, RawAttributes};
}

