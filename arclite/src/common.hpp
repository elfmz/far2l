#pragma once

#define E_PENDING -2147483638

#ifndef ERROR_NOT_ENOUGH_MEMORY
#define ERROR_NOT_ENOUGH_MEMORY 0x8
#endif

#ifndef ERROR_INVALID_DATA
#define ERROR_INVALID_DATA 13L
#endif

#define STREAM_CTL_RESET		9001
#define STREAM_CTL_FINISH		9002
#define STREAM_CTL_GETFULLSIZE	9003

enum SETATTR_RET_CODES
{
	SETATTR_RET_UNKNOWN = -1,
	SETATTR_RET_ERROR   = 0,
	SETATTR_RET_OK,
	SETATTR_RET_SKIP,
	SETATTR_RET_SKIPALL,
};

typedef struct AnalyseData_s
{
	int StructSize;
	const wchar_t *lpwszFileName;
	const unsigned char *pBuffer;
	DWORD dwBufferSize;
	int OpMode;
} AnalyseData;

typedef ByteVector ArcType;
typedef std::list<ArcType> ArcTypes;

typedef std::list<Error> ErrorLog;

#define RETRY_OR_IGNORE_BEGIN                                                                                \
	bool error_ignored = false;                                                                              \
	while (true) {                                                                                           \
		try {

#define RETRY_OR_IGNORE_END(ignore_errors, error_log, progress)                                              \
	break;                                                                                                   \
	}                                                                                                        \
	catch (const Error &error)                                                                               \
	{                                                                                                        \
		retry_or_ignore_error(error, error_ignored, ignore_errors, error_log, progress, true, true);         \
		if (error_ignored)                                                                                   \
			break;                                                                                           \
	}                                                                                                        \
	}

#define RETRY_END(progress)                                                                                  \
	break;                                                                                                   \
	}                                                                                                        \
	catch (const Error &error)                                                                               \
	{                                                                                                        \
		bool ignore_errors = false;                                                                          \
		ErrorLog error_log;                                                                                  \
		retry_or_ignore_error(error, error_ignored, ignore_errors, error_log, progress, true, false);        \
	}                                                                                                        \
	}

#define IGNORE_END(ignore_errors, error_log, progress)                                                       \
	break;                                                                                                   \
	}                                                                                                        \
	catch (const Error &error)                                                                               \
	{                                                                                                        \
		retry_or_ignore_error(error, error_ignored, ignore_errors, error_log, progress, false, true);        \
		if (error_ignored)                                                                                   \
			break;                                                                                           \
	}                                                                                                        \
	}

class ProgressMonitor : private NonCopyable
{
private:
	HANDLE h_scr;
	std::wstring con_title;
	static const unsigned c_first_delay_div = 4;
	static const unsigned c_update_delay_div = 16;
	UInt64 time_cnt;
	UInt64 time_freq;
	UInt64 time_total;
	UInt64 time_update;
	bool paused;
	bool low_priority;
	bool priority_changed;
	bool sleep_disabled;
	int initial_priority;
	int original_priority;
	pthread_t tID;
	bool confirm_esc;
	void update_time();
	void discard_time();
	void display();

protected:
	std::wstring progress_title;
	std::wstring progress_text;
	bool progress_known;
	unsigned percent_done;
	virtual void do_update_ui() = 0;

protected:
	bool is_single_key(const KEY_EVENT_RECORD &key_event);
	void handle_esc();

public:
	ProgressMonitor(const std::wstring &progress_title, bool progress_known = true, bool lazy = true, int priority = -1);
	virtual ~ProgressMonitor();
	void update_ui(bool force = false);
	void clean();
	UInt64 time_elapsed();
	UInt64 ticks_per_sec();
	friend class ProgressSuspend;
};

class ProgressSuspend : private NonCopyable
{
private:
	ProgressMonitor &progress;

public:
	ProgressSuspend(ProgressMonitor &progress) : progress(progress) { progress.update_time(); }
	~ProgressSuspend() { progress.discard_time(); }
};

void retry_or_ignore_error(const Error &error, bool &ignore, bool &ignore_errors, ErrorLog &error_log,
		ProgressMonitor &progress, bool can_retry, bool can_ignore);

enum OverwriteAction
{
	oaAsk,
	oaOverwrite,
	oaSkip,
	oaRename,
	oaAppend,
	oaOverwriteCase
};

struct OpenOptions
{
	std::wstring arc_path;
	bool detect;
	bool open_ex;
	bool nochain;
	ArcTypes arc_types;
	std::wstring password;
	int *open_password_len;
	bool recursive_panel;
	char delete_on_close;
	OpenOptions();
};

struct ExtractOptions
{
	std::wstring dst_dir;
	bool ignore_errors;
	bool extract_access_rights;
	int extract_owners_groups;
	bool extract_attributes;
	bool duplicate_hardlinks;
	bool restore_special_files;
	OverwriteAction overwrite;
	TriState move_files;
	std::wstring password;
	TriState separate_dir;
	std::shared_ptr<Far::FileFilter> filter;
	bool delete_archive;
	bool disable_delete_archive;
	TriState double_buffering;
	TriState open_dir;
	ExtractOptions();
};

struct SfxVersionInfo
{
	std::wstring version;
	std::wstring comments;
	std::wstring company_name;
	std::wstring file_description;
	std::wstring legal_copyright;
	std::wstring product_name;
};

struct SfxInstallConfig
{
	std::wstring title;
	std::wstring begin_prompt;
	std::wstring progress;
	std::wstring run_program;
	std::wstring directory;
	std::wstring execute_file;
	std::wstring execute_parameters;
};

struct SfxOptions
{
	std::wstring name;
	bool replace_icon;
	std::wstring icon_path;
	bool replace_version;
	SfxVersionInfo ver_info;
	bool append_install_config;
	SfxInstallConfig install_config;
	SfxOptions();
};

struct ExportOptions
{
	TriState export_creation_time;
	bool custom_creation_time;
	bool current_creation_time;
    FILETIME ftCreationTime;
	TriState export_last_access_time;
	bool custom_last_access_time;
	bool current_last_access_time;
    FILETIME ftLastAccessTime;
	TriState export_last_write_time;
	bool custom_last_write_time;
	bool current_last_write_time;
    FILETIME ftLastWriteTime;

	bool export_user_id;
	bool custom_user_id;
    uid_t UnixOwner;
	bool export_group_id;
	bool custom_group_id;
    gid_t UnixGroup;

	bool export_user_name;
	bool custom_user_name;
	std::wstring Owner;
	bool export_group_name;
	bool custom_group_name;
	std::wstring Group;

	bool export_unix_device;
	bool custom_unix_device;
	uint64_t UnixDevice;
	bool export_unix_mode;
	bool custom_unix_mode;
	uint64_t UnixNode;

	bool export_file_attributes;
    DWORD dwExportAttributesMask;
	bool custom_file_attributes;
    DWORD dwFileAttributes;

	bool export_file_descriptions;

	ExportOptions();
};

//ExportOptions::ExportOptions()

struct ProfileOptions
{
	ArcType arc_type;
	ArcType repack_arc_type;
	unsigned level;
	std::wstring levels;	// format=level;...
	std::wstring method;
	bool multithreading;
	uint32_t process_priority;
	uint32_t threads_num;
	bool repack;
	bool solid;
	std::wstring password;
	bool encrypt;
	TriState encrypt_header;
	bool create_sfx;
	SfxOptions sfx_options;
	ExportOptions export_options;
	bool use_export_settings;
	bool enable_volumes;
	std::wstring volume_size;
	bool skip_symlinks;
	uint32_t  symlink_fix_path_mode;
	bool dereference_symlinks;
	bool skip_hardlinks;
	bool duplicate_hardlinks;
	bool move_files;
	bool ignore_errors;
	std::wstring advanced;
	ProfileOptions();
};

struct UpdateOptions : public ProfileOptions
{
	std::wstring arc_path;
	bool show_password;
	bool open_shared;
	OverwriteAction overwrite;
	std::shared_ptr<Far::FileFilter> filter;
	bool append_ext;
	UpdateOptions();
};

bool operator==(const ProfileOptions &o1, const ProfileOptions &o2);
bool operator==(const SfxOptions &o1, const SfxOptions &o2);
bool operator==(const ExportOptions &o1, const ExportOptions &o2);

struct UpdateProfile
{
	std::wstring name;
	ProfileOptions options;
};
struct UpdateProfiles : public std::vector<UpdateProfile>
{
	void load();
	void save() const;
	unsigned find_by_name(const std::wstring &name);
	unsigned find_by_options(const UpdateOptions &options);
	void sort_by_name();
	void update(const std::wstring &name, const UpdateOptions &options);
};

struct Attr
{
	std::wstring name;
	std::wstring value;
};
typedef std::list<Attr> AttrList;

unsigned calc_percent(UInt64 completed, UInt64 total);
UInt64 get_module_version(const std::wstring &file_path);
UInt64 parse_size_string(const std::wstring &str);
DWORD translate_seek_method(UInt32 seek_origin);
std::wstring expand_macros(const std::wstring &text);
std::wstring load_file(const std::wstring &file_name, unsigned *code_page = nullptr);
std::wstring auto_rename(const std::wstring &file_path);
