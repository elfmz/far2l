#include "headers.hpp"

#include "msg.hpp"
#include "version.hpp"
#include "guids.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"
#include "options.hpp"
#include "cmdline.hpp"
#include "sfx.hpp"
#include <Threaded.h>
#include <algorithm>

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <sys/types.h>
#else
#include <sys/sysmacros.h>	  // major / minor
#endif

struct ArchiveType
{
	unsigned name_id;
	const ArcType &value;
};

const ArchiveType c_archive_types[] = {
	{MSG_COMPRESSION_ARCHIVE_7Z, c_7z },
	{MSG_COMPRESSION_ARCHIVE_ZIP, c_zip},
};

struct CompressionLevel
{
	unsigned name_id;
	unsigned value;
	//	uint32_t flags;
};

const CompressionLevel c_levels[] = {
	{MSG_COMPRESSION_LEVEL_STORE,   0},
	{MSG_COMPRESSION_LEVEL_FASTEST, 1},
	{MSG_COMPRESSION_LEVEL_FAST,	3},
	{MSG_COMPRESSION_LEVEL_NORMAL,  5},
	{MSG_COMPRESSION_LEVEL_MAXIMUM, 7},
	{MSG_COMPRESSION_LEVEL_ULTRA,	9},
};

enum
{
	CMF_7ZIP = 1,
	CMF_ZIP = 2
};

struct CompressionMethod
{
	unsigned name_id;
	const wchar_t *value;
	uint32_t flags;
};

const CompressionMethod c_methods[] = {
	{MSG_COMPRESSION_METHOD_DEFAULT,	c_method_default,	CMF_7ZIP | CMF_ZIP},
	{MSG_COMPRESSION_METHOD_LZMA,		c_method_lzma,		CMF_7ZIP | CMF_ZIP},
	{MSG_COMPRESSION_METHOD_LZMA2,		c_method_lzma2,		CMF_7ZIP },
	{MSG_COMPRESSION_METHOD_PPMD,		c_method_ppmd,		CMF_7ZIP | CMF_ZIP},
	{MSG_COMPRESSION_METHOD_DEFLATE,	c_method_deflate,	CMF_7ZIP | CMF_ZIP},
	{MSG_COMPRESSION_METHOD_DEFLATE64,	c_method_deflate64,	CMF_ZIP	},
	{MSG_COMPRESSION_METHOD_BZIP2,		c_method_bzip2,		CMF_7ZIP | CMF_ZIP},
	{MSG_COMPRESSION_METHOD_DELTA,		c_method_delta,		CMF_7ZIP },
	{MSG_COMPRESSION_METHOD_BCJ,		c_method_bcj,		CMF_7ZIP },
	{MSG_COMPRESSION_METHOD_BCJ2,		c_method_bcj2,		CMF_7ZIP },
};

const CompressionMethod c_tar_methods[] = {
	{MSG_COMPRESSION_METHOD_DEFAULT,	c_method_default,	1},
	{MSG_COMPRESSION_TAR_METHOD_GNU,	c_tar_method_gnu,	1},
	{MSG_COMPRESSION_TAR_METHOD_PAX,	c_tar_method_pax,	1},
	{MSG_COMPRESSION_TAR_METHOD_POSIX,	c_tar_method_posix,	1},
};

enum TBPFLAG
{
	TBPF_NOPROGRESS = 0,
	TBPF_INDETERMINATE = 0x1,
	TBPF_NORMAL = 0x2,
	TBPF_ERROR = 0x4,
	TBPF_PAUSED = 0x8
} TBPFLAG;

std::wstring get_error_dlg_title()
{
	return Far::get_msg(MSG_PLUGIN_NAME);
}

static inline void QueryPerformanceCounter(UInt64 *x)
{
	*x = WINPORT(GetTickCount)();
}

static inline void QueryPerformanceFrequency(UInt64 *x)
{
	//	*x = 1000000;
	*x = 1000;
}

ProgressMonitor::ProgressMonitor(const std::wstring &progress_title, bool progress_known, bool lazy, int priority)
	:   h_scr(nullptr),
		paused(false),
		low_priority(false),
		priority_changed(false),
		sleep_disabled(false),
		initial_priority(priority),
		original_priority(-1),
		confirm_esc(false),
		progress_title(progress_title),
		progress_known(progress_known),
		percent_done(0)
{
	//  fprintf(stderr, "==== ProgressMonitor::ProgressMonitor( )\n");
	//  fprintf(stderr, "==== TREAD [%ld] \n", pthread_self());

	QueryPerformanceCounter(&time_cnt);
	QueryPerformanceFrequency(&time_freq);

	tID = pthread_self();

	time_total = 0;
	if (lazy)
		time_update = time_total + time_freq / c_first_delay_div;
	else
		time_update = time_total;

	//	CHECK(get_app_option(FSSF_CONFIRMATIONS, c_esc_confirmation_option, confirm_esc));
	confirm_esc = g_options.confirm_esc_interrupt_operation;
	original_priority = _GetPriorityClass(_GetCurrentProcess());
}

ProgressMonitor::~ProgressMonitor()
{
	clean();
}

void ProgressMonitor::display()
{
	do_update_ui();
	std::wstring title;

	if (progress_known) {
		title += L"{" + int_to_str(percent_done) + L"%} ";
	}
	if (paused) {
		title += L"[" + Far::get_msg(MSG_PROGRESS_PAUSED) + L"] ";
	} else if (low_priority) {
		title += L"[" + Far::get_msg(MSG_PROGRESS_LOW_PRIORITY) + L"] ";
	}
	title += progress_title;

	if (progress_known) {
		Far::set_progress_state(TBPF_NORMAL);
		Far::set_progress_value(percent_done, 100);
	} else {
		Far::set_progress_state(TBPF_INDETERMINATE);
	}

	Far::message(c_progress_dialog_guid, title + L'\n' + progress_text, 0, FMSG_LEFTALIGN);
	WINPORT(SetConsoleTitle)(NULL, title.c_str());
}

void ProgressMonitor::update_ui(bool force)
{
	update_time();

	if (!priority_changed && time_elapsed() > 2000) {
		if (initial_priority != -1) {
			_SetPriorityClass(_GetCurrentProcess(), _map_priority(initial_priority));
			priority_changed = true;
		}
	}

	if (!sleep_disabled && time_elapsed() > 30000) {
		sleep_disabled = true;
	}

	if ((time_total >= time_update) || force) {

//		        fprintf(stderr, "time_total = %lu, time_update = %lu %ld\n", time_total, time_update,
//		        pthread_self());

		time_update = time_total + time_freq / c_update_delay_div;
		if (h_scr == nullptr) {
			h_scr = Far::save_screen();
			con_title = get_console_title();
		}

		INPUT_RECORD rec;
		DWORD read_cnt;

		while (true) {

			if (!paused) {
				WINPORT_PeekConsoleInput(NULL, &rec, 1, &read_cnt);
				if (read_cnt == 0) {
					break;
				}
			}

			WINPORT_ReadConsoleInput(NULL, &rec, 1, &read_cnt);
			if (rec.EventType == KEY_EVENT) {
				const KEY_EVENT_RECORD &key_event = rec.Event.KeyEvent;
				const WORD c_vk_b = 0x42;
				const WORD c_vk_p = 0x50;

				//                fprintf(stderr, "key_event.wVirtualKeyCode = %u\n",
				//                key_event.wVirtualKeyCode);

				if (is_single_key(key_event)) {
					if (key_event.wVirtualKeyCode == VK_ESCAPE) {
						handle_esc();
					} else if (key_event.wVirtualKeyCode == c_vk_b) {
						low_priority = !low_priority;
						_SetPriorityClass(_GetCurrentProcess(), low_priority ? _IDLE_PRIORITY_CLASS : original_priority);
						priority_changed = true;
						if (paused)
							display();
					} else if (key_event.wVirtualKeyCode == c_vk_p) {
						paused = !paused;
						if (paused) {
							update_time();
							display();
						} else
							discard_time();
					}
				}
			}
		}

		display();
	}
}

bool ProgressMonitor::is_single_key(const KEY_EVENT_RECORD &key_event)
{
	return key_event.bKeyDown
			&& (key_event.dwControlKeyState
					   & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED
							   | SHIFT_PRESSED))
			== 0;
}

void ProgressMonitor::handle_esc()
{
	if (!confirm_esc)
		FAIL(E_ABORT);
	ProgressSuspend ps(*this);
	if (Far::message(c_interrupt_dialog_guid,
				Far::get_msg(MSG_PLUGIN_NAME) + L"\n" + Far::get_msg(MSG_PROGRESS_INTERRUPT), 0,
				FMSG_MB_YESNO)
			== 0)
		FAIL(E_ABORT);
}

void ProgressMonitor::clean()
{
	if (h_scr) {
		Far::restore_screen(h_scr);
		WINPORT_SetConsoleTitle(NULL, con_title.c_str());
		Far::set_progress_state(TBPF_NOPROGRESS);
		h_scr = nullptr;
	}

	if (priority_changed) {
		_SetPriorityClass(_GetCurrentProcess(), original_priority);
	}

	if (sleep_disabled) {

	}

	if (time_elapsed() >= 5000) {
		Far::g_fsf.DisplayNotification(L"Arclite - operation complete", progress_title.c_str());
	}
}

void ProgressMonitor::update_time()
{
	UInt64 time_curr;
	QueryPerformanceCounter(&time_curr);
	time_total += time_curr - time_cnt;
	time_cnt = time_curr;
}

void ProgressMonitor::discard_time()
{
	QueryPerformanceCounter(&time_cnt);
}

UInt64 ProgressMonitor::time_elapsed()
{
	update_time();
	return time_total;
}

UInt64 ProgressMonitor::ticks_per_sec()
{
	return time_freq;
}

static const wchar_t **get_suffixes(int start)
{
	static const int msg_ids[1 + 5 + 5] = {MSG_LANG, -1, MSG_SUFFIX_SIZE_KB, MSG_SUFFIX_SIZE_MB,
			MSG_SUFFIX_SIZE_GB, MSG_SUFFIX_SIZE_TB, MSG_SUFFIX_SPEED_B, MSG_SUFFIX_SPEED_KB,
			MSG_SUFFIX_SPEED_MB, MSG_SUFFIX_SPEED_GB, MSG_SUFFIX_SPEED_TB};
	static std::wstring msg_texts[1 + 5 + 5];
	static const wchar_t *suffixes[1 + 5 + 5];

	if (Far::get_msg(msg_ids[0]) != msg_texts[0]) {
		for (int i = 0; i < 1 + 5 + 5; ++i) {
			auto msg_id = msg_ids[i];
			suffixes[i] = (msg_texts[i] = msg_id >= 0 ? Far::get_msg(msg_id) : std::wstring()).c_str();
		}
	}

	return suffixes + start;
}

const wchar_t **get_size_suffixes()
{
	return get_suffixes(1 + 0);
}
const wchar_t **get_speed_suffixes()
{
	return get_suffixes(1 + 5);
}

class PasswordDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 60
	};

	std::wstring arc_path;
	std::wstring &password;

	int password_ctrl_id{};
	int ok_ctrl_id{};
	int cancel_ctrl_id{};

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
			password = get_text(password_ctrl_id);
		}
		return default_dialog_proc(msg, param1, param2);
	}

public:
	PasswordDialog(std::wstring &password, const std::wstring &arc_path)
		: Far::Dialog(Far::get_msg(MSG_PASSWORD_TITLE), &c_password_dialog_guid, c_client_xs),
		  arc_path(arc_path),
		  password(password)
	{}

	bool show()
	{
		label(fit_str(arc_path, c_client_xs), c_client_xs, DIF_SHOWAMPERSAND);
		new_line();
		label(Far::get_msg(MSG_PASSWORD_PASSWORD));
		password_ctrl_id = pwd_edit_box(password);
		new_line();
		separator();
		new_line();

		ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
		cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
		new_line();

		intptr_t item = Far::Dialog::show();

		return (item != -1) && (item != cancel_ctrl_id);
	}
};

bool password_dialog(std::wstring &password, const std::wstring &arc_path)
{
	return PasswordDialog(password, arc_path).show();
}

class OverwriteDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 60
	};

	std::wstring file_path;
	OverwriteFileInfo src_file_info;
	OverwriteFileInfo dst_file_info;
	OverwriteDialogKind kind;
	OverwriteOptions &options;

	int all_ctrl_id{};
	int overwrite_ctrl_id{};
	int skip_ctrl_id{};
	int rename_ctrl_id{};
	int append_ctrl_id{};
	int cancel_ctrl_id{};

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if (msg == DN_CLOSE && param1 >= 0 && param1 != cancel_ctrl_id) {
			options.all = get_check(all_ctrl_id);
			if (param1 == overwrite_ctrl_id)
				options.action = oaOverwrite;
			else if (param1 == skip_ctrl_id)
				options.action = oaSkip;
			else if (kind == odkExtract && param1 == rename_ctrl_id)
				options.action = oaRename;
			else if (kind == odkExtract && param1 == append_ctrl_id)
				options.action = oaAppend;
			else
				FAIL(E_ABORT);
		}
		return default_dialog_proc(msg, param1, param2);
	}

public:
	OverwriteDialog(const std::wstring &file_path, const OverwriteFileInfo &src_file_info,
			const OverwriteFileInfo &dst_file_info, OverwriteDialogKind kind, OverwriteOptions &options)
		: Far::Dialog(Far::get_msg(MSG_OVERWRITE_DLG_TITLE), &c_overwrite_dialog_guid, c_client_xs, nullptr,
				  FDLG_WARNING),
		  file_path(file_path),
		  src_file_info(src_file_info),
		  dst_file_info(dst_file_info),
		  kind(kind),
		  options(options)
	{}

	bool show()
	{
		label(fit_str(file_path, c_client_xs), c_client_xs, DIF_SHOWAMPERSAND);
		new_line();
		label(Far::get_msg(MSG_OVERWRITE_DLG_QUESTION));
		new_line();
		separator();
		new_line();

		std::wstring src_label = Far::get_msg(MSG_OVERWRITE_DLG_SOURCE);
		std::wstring dst_label = Far::get_msg(MSG_OVERWRITE_DLG_DESTINATION);
		uintptr_t label_pad = std::max(src_label.size(), dst_label.size()) + 1;
		std::wstring src_size = uint_to_str(src_file_info.size);
		std::wstring dst_size = uint_to_str(dst_file_info.size);
		uintptr_t size_pad = label_pad + std::max(src_size.size(), dst_size.size()) + 1;

		label(src_label);
		pad(label_pad);
		if (!src_file_info.is_dir) {
			label(src_size);
			pad(size_pad);
		}
		label(format_file_time(src_file_info.mtime));
		if (WINPORT_CompareFileTime(&src_file_info.mtime, &dst_file_info.mtime) > 0) {
			spacer(1);
			label(Far::get_msg(MSG_OVERWRITE_DLG_NEWER));
		}
		new_line();

		label(dst_label);
		pad(label_pad);
		if (!dst_file_info.is_dir) {
			label(dst_size);
			pad(size_pad);
		}
		label(format_file_time(dst_file_info.mtime));
		if (WINPORT_CompareFileTime(&src_file_info.mtime, &dst_file_info.mtime) < 0) {
			spacer(1);
			label(Far::get_msg(MSG_OVERWRITE_DLG_NEWER));
		}
		new_line();

		separator();
		new_line();
		all_ctrl_id = check_box(Far::get_msg(MSG_OVERWRITE_DLG_ALL), false);
		new_line();
		separator();
		new_line();
		overwrite_ctrl_id = def_button(Far::get_msg(MSG_OVERWRITE_DLG_OVERWRITE), DIF_CENTERGROUP);
		spacer(1);
		skip_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_SKIP), DIF_CENTERGROUP);
		spacer(1);
		if (kind == odkExtract) {
			rename_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_RENAME), DIF_CENTERGROUP);
			spacer(1);
			append_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_APPEND), DIF_CENTERGROUP);
			spacer(1);
		}
		cancel_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_CANCEL), DIF_CENTERGROUP);
		new_line();

		intptr_t item = Far::Dialog::show();

		return item >= 0 && item != cancel_ctrl_id;
	}
};

bool overwrite_dialog(const std::wstring &file_path, const OverwriteFileInfo &src_file_info,
		const OverwriteFileInfo &dst_file_info, OverwriteDialogKind kind, OverwriteOptions &options)
{
	return OverwriteDialog(file_path, src_file_info, dst_file_info, kind, options).show();
}

struct ExportProfile
{
	std::wstring name;
	ExportOptions options;
};

typedef std::vector<ExportProfile> ExportProfiles;

//static const wchar_t kPosixTypes[16 + 1] = L"0pc3d5b7-9lBsDEF";
#define ATTR_CHAR(a, n, c) (((a) & (1 << (n))) ? c : L'-')

static void format_posix_attrib(wchar_t *attr, uint32_t val)
{
	attr[0] = 32;
	for (int i = 6; i >= 0; i -= 3) {
		attr[7 - i] = ATTR_CHAR(val, i + 2, L'r');
		attr[8 - i] = ATTR_CHAR(val, i + 1, L'w');
		attr[9 - i] = ATTR_CHAR(val, i + 0, L'x');
	}
	if ((val & 0x800) != 0)
		attr[3] = ((val & (1 << 6)) ? L's' : L'S');
	if ((val & 0x400) != 0)
		attr[6] = ((val & (1 << 3)) ? L's' : L'S');
	if ((val & 0x200) != 0)
		attr[9] = ((val & (1 << 0)) ? L't' : L'T');

	attr[10] = 0;
}

static constexpr unsigned kNumWinAtrribFlags = 21;
static const wchar_t g_WinAttribChars[kNumWinAtrribFlags + 1] = L"RHS8DAdNTsLCOIEV.X.PU";

/* FILE_ATTRIBUTE_
 0 READONLY
 1 HIDDEN
 2 SYSTEM
 3 (Volume label - obsolete)
 4 DIRECTORY
 5 ARCHIVE
 6 DEVICE
 7 NORMAL
 8 TEMPORARY
 9 SPARSE_FILE
10 REPARSE_POINT
11 COMPRESSED
12 OFFLINE
13 NOT_CONTENT_INDEXED (I - Win10 attrib/Explorer)
14 ENCRYPTED
15 INTEGRITY_STREAM (V - ReFS Win8/Win2012)
16 VIRTUAL (reserved) -----------------------------------------------------------
17 NO_SCRUB_DATA (X - ReFS Win8/Win2012 attrib)
18 RECALL_ON_OPEN or EA
19 PINNED
20 UNPINNED ---------------------------------------------------------------------
21 STRICTLY_SEQUENTIAL
22 RECALL_ON_DATA_ACCESS
*/

// #define FILE_ATTRIBUTE_INTEGRITY_STREAM     0x00008000
// #define FILE_ATTRIBUTE_VIRTUAL              0x00010000 // 65536
// #define FILE_ATTRIBUTE_NO_SCRUB_DATA        0x00020000
// #define FILE_ATTRIBUTE_EA                   0x00040000
// #define FILE_ATTRIBUTE_PINNED               0x00080000
// #define FILE_ATTRIBUTE_UNPINNED             0x00100000

static void
format_winattrib(wchar_t *attr, uint32_t val, uint32_t mask = 0xFFFFFFFF, unsigned nf = kNumWinAtrribFlags)
{
	for (unsigned i = 0; i < nf; i++) {
		unsigned f = (1U << i);
		if (val & f) {
			attr[i * 2 + 0] = '+';
			attr[i * 2 + 1] = g_WinAttribChars[i];
		} else if (!(mask & f)) {
			attr[i * 2 + 0] = '-';
			attr[i * 2 + 1] = g_WinAttribChars[i];
		} else {
			attr[i * 2 + 0] = '.';
			attr[i * 2 + 1] = '.';
		}
	}
	attr[nf * 2] = 0;
}

class ExportAttrDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 32
	};

	uint32_t &_attrlist;
	uint32_t &_attrmask;
	uint32_t attrlist;
	uint32_t attrmask;
	uint32_t num;

	int attr_ctrl_id;

	int ok_ctrl_id;
	int cancel_ctrl_id;
	int clear_ctrl_id;
	int reset_ctrl_id;

	void set_control_state() {}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		switch (msg) {

			case DN_BTNCLICK: {
				if (param1 == clear_ctrl_id) {
					DisableEvents de(*this);
					attrlist = 0;
					attrmask = 0;
					for (size_t i = 0; i < kNumWinAtrribFlags; i++)
						set_check3(attr_ctrl_id + i, triFalse);
				} else if (param1 == reset_ctrl_id) {
					DisableEvents de(*this);
					attrlist = 0;
					attrmask = 0xFFFFFFFF;
					for (size_t i = 0; i < kNumWinAtrribFlags; i++)
						set_check3(attr_ctrl_id + i, triUndef);
				} else if (param1 >= attr_ctrl_id && param1 < (attr_ctrl_id + kNumWinAtrribFlags)) {
					unsigned n = (unsigned)(param1 - attr_ctrl_id);
					unsigned f = (1U << n);
					TriState state = get_check3(param1);
					if (state == triTrue) {
						attrlist |= f;
						attrmask |= f;
					} else if (state == triFalse) {
						attrlist &= (~f);
						attrmask &= (~f);
					} else {
						attrlist &= (~f);
						attrmask |= f;
					}
				}
			} break;
		}

		return default_dialog_proc(msg, param1, param2);
	}

public:
	ExportAttrDialog(uint32_t &_attrlist, uint32_t &_attrmask, uint32_t num)
		: Far::Dialog(Far::get_msg(MSG_EXPORT_ATTR_DLG_TITLE), &c_export_options_dialog_guid, c_client_xs,
				  L"ExportAttrOptions"),
		  _attrlist(_attrlist),
		  _attrmask(_attrmask),
		  attrlist(_attrlist),
		  attrmask(_attrmask),
		  num(num)
	{}

	bool show()
	{
		for (size_t i = 0; i < kNumWinAtrribFlags; i++) {
			unsigned attrflag = (1U << i);
			TriState state;

			if (attrflag & attrlist)
				state = triTrue;
			else if (attrflag & attrmask)
				state = triUndef;
			else
				state = triFalse;

			int id = check_box3(Far::get_msg(MSG_EXPORT_ATTR_DLG_ATTR00 + i), state,
					i < num ? 0 : DIF_DISABLE);
			if (!i)
				attr_ctrl_id = id;
			new_line();
		}

		clear_ctrl_id = button(Far::get_msg(MSG_BUTTON_CLEAR), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
		reset_ctrl_id = button(Far::get_msg(MSG_BUTTON_RESET), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
		new_line();
		separator();
		new_line();

		ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
		cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
		new_line();

		intptr_t item = Far::Dialog::show();

		if (item != -1 && item != cancel_ctrl_id) {
			_attrlist = attrlist;
			_attrmask = attrmask;
			return true;
		}

		return false;
	}
};

bool export_attr_dialog(uint32_t &attrlist, uint32_t &attrmask, uint32_t num)
{
	return ExportAttrDialog(attrlist, attrmask, num).show();
}

bool is_valid_unsigned_word(const wchar_t *name, size_t len)
{
	static const wchar_t wcu16max[6] = L"65535";
	if (len == 0 || len > 5) {
		return false;
	}

	for (size_t i = 0; i < len; i++) {
		wchar_t ch = name[i];
		if (ch < L'0' || ch > L'9')
			return false;
	}

	if (len == 5) {
		for (size_t i = 0; i < 5; i++) {
			if (name[i] > wcu16max[i])
				return false;
			if (name[i] < wcu16max[i])
				break;
		}
	}

	return true;
}

bool is_valid_userid_or_groupid(const wchar_t *name, size_t len)
{
	static const wchar_t wcu32max[11] = L"4294967295";
	if (len == 0 || len > 10) {
		return false;
	}

	for (size_t i = 0; i < len; i++) {
		wchar_t ch = name[i];
		if (ch < L'0' || ch > L'9')
			return false;
	}

	if (len == 10) {
		for (size_t i = 0; i < 10; i++) {
			if (name[i] > wcu32max[i])
				return false;
			if (name[i] < wcu32max[i])
				break;
		}
	}

	return true;
}

bool is_valid_username_or_groupname(const wchar_t *name, size_t len)
{
	if (len == 0 || len > 32) {
		return false;
	}

	if (name[0] >= L'0' && name[0] <= L'9') {
		return false;
	}

	for (size_t i = 0; i < len; i++) {
		wchar_t ch = name[i];

		if (!((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9')
					|| ch == L'-' || ch == L'_' || ch == L'.')) {
			return false;
		}
	}

	return true;
}

class ExportOptionsDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 69
	};

	ExportOptions *_options;
	ExportOptions m_options;
	ArcType arc_type;
	int arc_mth;
	ExportProfiles profiles;

	int profile_ctrl_id;
	wchar_t strSDMask[16], strSTMask[16];

	int export_creation_time_ctrl_id;
	int custom_creation_time_ctrl_id;
	int current_creation_time_ctrl_id;
	int ftCreationDate_ctrl_id;
	int ftCreationTime_ctrl_id;
	int ftCreationClear_ctrl_id;

	int export_last_access_time_ctrl_id;
	int custom_last_access_time_ctrl_id;
	int current_last_access_time_ctrl_id;
	int ftLastAccessDate_ctrl_id;
	int ftLastAccessTime_ctrl_id;
	int ftLastAccessClear_ctrl_id;

	int export_last_write_time_ctrl_id;
	int custom_last_write_time_ctrl_id;
	int current_last_write_time_ctrl_id;
	int ftLastWriteDate_ctrl_id;
	int ftLastWriteTime_ctrl_id;
	int ftLastWriteClear_ctrl_id;
	bool bLastWriteTime = true;
	bool bLastAccessTime = true;
	bool bCreationTime = true;
	bool ftClearBtState[3] = {0, 0, 0};

	int export_user_name_ctrl_id;
	int custom_user_name_ctrl_id;
	int Owner_ctrl_id;
	int export_group_name_ctrl_id;
	int custom_group_name_ctrl_id;
	int Group_ctrl_id;
	bool bUsersGroups = true;

	int export_user_id_ctrl_id;
	int custom_user_id_ctrl_id;
	int UnixOwner_ctrl_id;
	int export_group_id_ctrl_id;
	int custom_group_id_ctrl_id;
	int UnixGroup_ctrl_id;
	bool bUsersGroupsIDS = true;

	int export_unix_device_ctrl_id;
	int custom_unix_device_ctrl_id;
	int UnixDeviceMajor_ctrl_id;
	int UnixDeviceMinor_ctrl_id;
	bool bUnixDevice = true;

	int export_unix_mode_ctrl_id;
	int custom_unix_mode_ctrl_id;
	int UnixMode_ctrl_id;
	int label_UnixMode_ctrl_id;
	bool bUnixMode = true;

	int export_file_attributes_ctrl_id;
	int custom_file_attributes_ctrl_id;
//	int but_file_attributes_ctrl_id;
	int label_file_attributes_ctrl_id;
//	int file_attributes_ctrl_id;
	bool bFileAttributes = true;

	int export_file_descriptions_ctrl_id;
	bool bFileDescriptions = true;

	int iDateFormat;
	wchar_t DateSeparator;
	wchar_t TimeSeparator;
	wchar_t DecimalSeparator;

	int ok_ctrl_id;
	int cancel_ctrl_id;
	int reset_ctrl_id;

	void write_controls(const ExportOptions &options)
	{
		wchar_t strName[64];
		DisableEvents de(*this);
		m_options = options;

		SetDateTime(ftCreationDate_ctrl_id, m_options.ftCreationTime, 2, 1);
		SetDateTime(ftLastAccessDate_ctrl_id, m_options.ftLastAccessTime, 2, 1);
		SetDateTime(ftLastWriteDate_ctrl_id, m_options.ftLastWriteTime, 2, 1);

		set_check3(export_creation_time_ctrl_id, m_options.export_creation_time);
		set_check(custom_creation_time_ctrl_id, m_options.custom_creation_time);
		set_check(current_creation_time_ctrl_id, m_options.current_creation_time);
		enable(custom_creation_time_ctrl_id, m_options.export_creation_time == triTrue && bCreationTime);
		bool bExportCustom =
				(m_options.export_creation_time == triTrue && m_options.custom_creation_time && bCreationTime);
		bool bCustomEdit = (!m_options.current_creation_time && bExportCustom);
		enable(current_creation_time_ctrl_id, bExportCustom);
		enable(ftCreationDate_ctrl_id, bCustomEdit);
		enable(ftCreationTime_ctrl_id, bCustomEdit);
		enable(ftCreationClear_ctrl_id, bCustomEdit);

		set_check3(export_last_access_time_ctrl_id, m_options.export_last_access_time);
		set_check(custom_last_access_time_ctrl_id, m_options.custom_last_access_time);
		set_check(current_last_access_time_ctrl_id, m_options.current_last_access_time);
		enable(custom_last_access_time_ctrl_id, m_options.export_last_access_time == triTrue && bLastAccessTime);
		bExportCustom =
				(m_options.export_last_access_time == triTrue && m_options.custom_last_access_time && bLastAccessTime);
		bCustomEdit = (!m_options.current_last_access_time && bExportCustom);
		enable(current_last_access_time_ctrl_id, bExportCustom);
		enable(ftLastAccessDate_ctrl_id, bCustomEdit);
		enable(ftLastAccessTime_ctrl_id, bCustomEdit);
		enable(ftLastAccessClear_ctrl_id, bCustomEdit);

		set_check3(export_last_write_time_ctrl_id, m_options.export_last_write_time);
		set_check(custom_last_write_time_ctrl_id, m_options.custom_last_write_time);
		set_check(current_last_write_time_ctrl_id, m_options.current_last_write_time);
		enable(custom_last_write_time_ctrl_id, m_options.export_last_write_time == triTrue && bLastWriteTime);
		bExportCustom =
				(m_options.export_last_write_time == triTrue && m_options.custom_last_write_time && bLastWriteTime);
		bCustomEdit = (!m_options.current_last_write_time && bExportCustom);
		enable(current_last_write_time_ctrl_id, bExportCustom);
		enable(ftLastWriteDate_ctrl_id, bCustomEdit);
		enable(ftLastWriteTime_ctrl_id, bCustomEdit);
		enable(ftLastWriteClear_ctrl_id, bCustomEdit);

		swprintf(strName, 32, L"%ls",
				is_valid_username_or_groupname(m_options.Owner.c_str(), m_options.Owner.length())
						? m_options.Owner.c_str()
						: L"user");
		set_text_silent(Owner_ctrl_id, strName);
		set_check(export_user_name_ctrl_id, m_options.export_user_name);
		set_check(custom_user_name_ctrl_id, m_options.custom_user_name);
		bExportCustom = (m_options.export_user_name && m_options.custom_user_name && bUsersGroups);
		enable(custom_user_name_ctrl_id, m_options.export_user_name && bUsersGroups);
		enable(Owner_ctrl_id, bExportCustom);

		swprintf(strName, 32, L"%ls",
				is_valid_username_or_groupname(m_options.Group.c_str(), m_options.Group.length())
						? m_options.Group.c_str()
						: L"group");
		set_text_silent(Group_ctrl_id, strName);
		set_check(export_group_name_ctrl_id, m_options.export_group_name);
		set_check(custom_group_name_ctrl_id, m_options.custom_group_name);
		bExportCustom = (m_options.export_group_name && m_options.custom_group_name && bUsersGroups);
		enable(custom_group_name_ctrl_id, m_options.export_group_name && bUsersGroups);
		enable(Group_ctrl_id, bExportCustom);

		swprintf(strName, 32, L"%u", (uint32_t)m_options.UnixOwner);
		set_text_silent(UnixOwner_ctrl_id, strName);
		set_check(export_user_id_ctrl_id, m_options.export_user_id);
		set_check(custom_user_id_ctrl_id, m_options.custom_user_id);
		bExportCustom = (m_options.export_user_id && m_options.custom_user_id && bUsersGroupsIDS);
		enable(custom_user_id_ctrl_id, m_options.export_user_id && bUsersGroupsIDS);
		enable(UnixOwner_ctrl_id, bExportCustom);

		swprintf(strName, 32, L"%u", (uint32_t)m_options.UnixGroup);
		set_text_silent(UnixGroup_ctrl_id, strName);
		set_check(export_group_id_ctrl_id, m_options.export_group_id);
		set_check(custom_group_id_ctrl_id, m_options.custom_group_id);
		bExportCustom = (m_options.export_group_id && m_options.custom_group_id && bUsersGroupsIDS);
		enable(custom_group_id_ctrl_id, m_options.export_group_id && bUsersGroupsIDS);
		enable(UnixGroup_ctrl_id, bExportCustom);

		swprintf(strName, 5, L"%04o", m_options.UnixNode & 0xFFF);
		set_text_silent(UnixMode_ctrl_id, strName);
		format_posix_attrib(strName, m_options.UnixNode & 0xFFF);
		set_text(label_UnixMode_ctrl_id, strName);
		set_check(export_unix_mode_ctrl_id, m_options.export_unix_mode);
		set_check(custom_unix_mode_ctrl_id, m_options.custom_unix_mode);
		bExportCustom = (m_options.export_unix_mode && m_options.custom_unix_mode && bUnixMode);
		enable(custom_unix_mode_ctrl_id, m_options.export_unix_mode && bUnixMode);
		enable(custom_unix_mode_ctrl_id + 1, bExportCustom);
		enable(custom_unix_mode_ctrl_id + 2, bExportCustom);
		enable(custom_unix_mode_ctrl_id + 3, bExportCustom);

		swprintf(strName, 16, L"%u", ((uint32_t)major(m_options.UnixDevice)) & 0xFFFF);
		set_text_silent(UnixDeviceMajor_ctrl_id, strName);
		swprintf(strName, 16, L"%u", ((uint32_t)minor(m_options.UnixDevice)) & 0xFFFF);
		set_text_silent(UnixDeviceMinor_ctrl_id, strName);
		set_check(export_unix_device_ctrl_id, m_options.export_unix_device);
		set_check(custom_unix_device_ctrl_id, m_options.custom_unix_device);
		bExportCustom = (m_options.export_unix_device && m_options.custom_unix_device && bUnixDevice);
		enable(custom_unix_device_ctrl_id, m_options.export_unix_device && bUnixDevice);
		enable(custom_unix_device_ctrl_id + 1, bExportCustom);
		enable(custom_unix_device_ctrl_id + 2, bExportCustom);
		enable(custom_unix_device_ctrl_id + 3, bExportCustom);
		enable(custom_unix_device_ctrl_id + 4, bExportCustom);

		format_winattrib(strName, m_options.dwFileAttributes, m_options.dwExportAttributesMask,
				m_options.export_unix_mode ? 16 : kNumWinAtrribFlags);
		set_text(label_file_attributes_ctrl_id, strName);
		set_check(export_file_attributes_ctrl_id, m_options.export_file_attributes);
		set_check(custom_file_attributes_ctrl_id, m_options.custom_file_attributes);
		bExportCustom =
				(m_options.export_file_attributes && m_options.custom_file_attributes && bFileAttributes);
		enable(custom_file_attributes_ctrl_id, m_options.export_file_attributes && bFileAttributes);
		enable(label_file_attributes_ctrl_id, bExportCustom);

		set_check(export_file_descriptions_ctrl_id, m_options.export_file_descriptions);
	}

	void FillDateTime(const FILETIME &ft, wchar_t *strDate, wchar_t *strTime)
	{

		SYSTEMTIME st;
		FILETIME ct;

		if (!ft.dwHighDateTime) {
			strDate[0] = strTime[0] = 0;
			return;
		}

		WINPORT(FileTimeToLocalFileTime)(&ft, &ct);
		WINPORT(FileTimeToSystemTime)(&ct, &st);
		swprintf(strTime, 16, L"%02d%c%02d%c%02d%c%03d", st.wHour, TimeSeparator, st.wMinute, TimeSeparator,
				st.wSecond, DecimalSeparator, st.wMilliseconds);

		switch (iDateFormat) {
			case 0:
				swprintf(strDate, 16, L"%02d%c%02d%c%04d", st.wMonth, DateSeparator, st.wDay, DateSeparator,
						st.wYear);
				break;
			case 1:
				swprintf(strDate, 16, L"%02d%c%02d%c%04d", st.wDay, DateSeparator, st.wMonth, DateSeparator,
						st.wYear);
				break;
			default:
				swprintf(strDate, 16, L"%04d%c%02d%c%02d", st.wYear, DateSeparator, st.wMonth, DateSeparator,
						st.wDay);
				break;
		}
	}

	void SetDateTime(int id, FILETIME &ft, int sm = 0, bool focus = true)
	{

		wchar_t strDate[32], strTime[32];

		if (sm == 0) {
			ft = {0, 0};
		} else if (sm == 1) {
			WINPORT(GetSystemTimeAsFileTime)(&ft);
		}

		FillDateTime(ft, strDate, strTime);
		set_text_silent(id + 0, strDate);
		set_text_silent(id + 1, strTime);

		//		if (focus)
		//			set_focus(id);
	}

	void GetDateTime(int id, FILETIME &ft)
	{

		wchar_t strDate[32], strTime[32];
		get_text(id + 0, strDate, 32);
		get_text(id + 1, strTime, 32);

		int iDay, iMonth, iYear;
		int iHour, iMinute, iSecond, iMilliseconds;

		swscanf(strTime, strSTMask, &iHour, &iMinute, &iSecond, &iMilliseconds);

		switch (iDateFormat) {
			case 0:
				swscanf(strDate, strSDMask, &iMonth, &iDay, &iYear);
//				fprintf(stderr, "GetDateTime %d ID] w d y %d %d %d  \n", id, iMonth, iDay, iYear);
				break;
			case 1:
				swscanf(strDate, strSDMask, &iDay, &iMonth, &iYear);
//				fprintf(stderr, "GetDateTime %d ID] d m y %d %d %d  \n", id, iDay, iMonth, iYear);
				break;
			default:
				swscanf(strDate, strSDMask, &iYear, &iMonth, &iDay);
//				fprintf(stderr, "GetDateTime %d ID] y m d %d %d %d  \n", id, iYear, iMonth, iDay);
				break;
		}

		SYSTEMTIME st = {(WORD)iYear, (WORD)iMonth, 0, (WORD)iDay, (WORD)iHour, (WORD)iMinute, (WORD)iSecond,
				(WORD)iMilliseconds};
		FILETIME lft;

		if (WINPORT(SystemTimeToFileTime)(&st, &lft)) {
			WINPORT(LocalFileTimeToFileTime)(&lft, &ft);
		}
	}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		wchar_t strName[64];

		if (msg == DN_EDITCHANGE && param1 == profile_ctrl_id) {
			unsigned profile_idx = get_list_pos(profile_ctrl_id);
			if (profile_idx != (unsigned)-1 && profile_idx < profiles.size()) {
				write_controls(profiles[profile_idx].options);
			}
		}

		switch (msg) {

			case DN_BTNCLICK: {

				if (param1 == reset_ctrl_id) {
					ExportOptions def_options;
					write_controls(def_options);
					break;
				}


				if (param1 == export_creation_time_ctrl_id) {
					DisableEvents de(*this);
					m_options.export_creation_time = get_check3(export_creation_time_ctrl_id);
					bool bEnable = (m_options.export_creation_time == triTrue);
					enable(custom_creation_time_ctrl_id, bEnable);
					bool bCustom = get_check(custom_creation_time_ctrl_id);
					enable(current_creation_time_ctrl_id, bEnable && bCustom);
					bool bCurrent = get_check(current_creation_time_ctrl_id);
					enable(ftCreationDate_ctrl_id, bEnable && bCustom && !bCurrent);
					enable(ftCreationTime_ctrl_id, bEnable && bCustom && !bCurrent);
					enable(ftCreationClear_ctrl_id, bEnable && bCustom && !bCurrent);
					break;
				}
				if (param1 == export_last_access_time_ctrl_id) {
					DisableEvents de(*this);
					m_options.export_last_access_time = get_check3(export_last_access_time_ctrl_id);
					bool bEnable = (m_options.export_last_access_time == triTrue);
					enable(custom_last_access_time_ctrl_id, bEnable);
					bool bCustom = get_check(custom_last_access_time_ctrl_id);
					enable(current_last_access_time_ctrl_id, bEnable && bCustom);
					bool bCurrent = get_check(current_last_access_time_ctrl_id);
					enable(ftLastAccessDate_ctrl_id, bEnable && bCustom && !bCurrent);
					enable(ftLastAccessTime_ctrl_id, bEnable && bCustom && !bCurrent);
					enable(ftLastAccessClear_ctrl_id, bEnable && bCustom && !bCurrent);
					break;
				}
				if (param1 == export_last_write_time_ctrl_id) {
					DisableEvents de(*this);
					m_options.export_last_write_time = get_check3(export_last_write_time_ctrl_id);
					bool bEnable = (m_options.export_last_write_time == triTrue);
					enable(custom_last_write_time_ctrl_id, bEnable);
					bool bCustom = get_check(custom_last_write_time_ctrl_id);
					enable(current_last_write_time_ctrl_id, bEnable && bCustom);
					bool bCurrent = get_check(current_last_write_time_ctrl_id);
					enable(ftLastWriteDate_ctrl_id, bEnable && bCustom && !bCurrent);
					enable(ftLastWriteTime_ctrl_id, bEnable && bCustom && !bCurrent);
					enable(ftLastWriteClear_ctrl_id, bEnable && bCustom && !bCurrent);
					break;
				}

				if (param1 == export_user_name_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_user_name_ctrl_id);
					m_options.export_user_name = bEnable;
					enable(custom_user_name_ctrl_id, bEnable);
					bool bCustom = get_check(custom_user_name_ctrl_id);
					enable(custom_user_name_ctrl_id + 1, bEnable & bCustom);
					break;
				}
				if (param1 == export_group_name_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_group_name_ctrl_id);
					m_options.export_group_name = bEnable;
					enable(custom_group_name_ctrl_id, bEnable);
					bool bCustom = get_check(custom_group_name_ctrl_id);
					enable(custom_group_name_ctrl_id + 1, bEnable & bCustom);
					break;
				}
				if (param1 == export_user_id_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_user_id_ctrl_id);
					m_options.export_user_id = bEnable;
					enable(custom_user_id_ctrl_id, bEnable);
					bool bCustom = get_check(custom_user_id_ctrl_id);
					enable(custom_user_id_ctrl_id + 1, bEnable & bCustom);
					break;
				}
				if (param1 == export_group_id_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_group_id_ctrl_id);
					m_options.export_group_id = bEnable;
					enable(custom_group_id_ctrl_id, bEnable);
					bool bCustom = get_check(custom_group_id_ctrl_id);
					enable(custom_group_id_ctrl_id + 1, bEnable & bCustom);
					break;
				}
				if (param1 == export_unix_mode_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_unix_mode_ctrl_id);
					m_options.export_unix_mode = bEnable;
					enable(custom_unix_mode_ctrl_id, bEnable);
					bool bCustom = get_check(custom_unix_mode_ctrl_id);
					enable(custom_unix_mode_ctrl_id + 1, bEnable & bCustom);
					enable(custom_unix_mode_ctrl_id + 2, bEnable & bCustom);
					enable(custom_unix_mode_ctrl_id + 3, bEnable & bCustom);
					break;
				}
				if (param1 == export_unix_device_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_unix_device_ctrl_id);
					m_options.export_unix_device = bEnable;
					enable(custom_unix_device_ctrl_id, bEnable);
					bool bCustom = get_check(custom_unix_device_ctrl_id);
					enable(custom_unix_device_ctrl_id + 1, bEnable & bCustom);
					enable(custom_unix_device_ctrl_id + 2, bEnable & bCustom);
					enable(custom_unix_device_ctrl_id + 3, bEnable & bCustom);
					enable(custom_unix_device_ctrl_id + 4, bEnable & bCustom);
					break;
				}
				if (param1 == export_file_attributes_ctrl_id) {
					DisableEvents de(*this);
					bool bEnable = get_check(export_file_attributes_ctrl_id);
					m_options.export_file_attributes = bEnable;
					enable(custom_file_attributes_ctrl_id, bEnable);
					bool bCustom = get_check(custom_file_attributes_ctrl_id);
					enable(label_file_attributes_ctrl_id, bEnable & bCustom);
					break;
				}
				if (param1 == export_file_descriptions_ctrl_id) {
					m_options.export_file_descriptions = get_check(export_file_descriptions_ctrl_id);
					break;
				}

				if (param1 == custom_creation_time_ctrl_id) {
					DisableEvents de(*this);
					bool bCustom = get_check(custom_creation_time_ctrl_id);
					bool bCustomEdit = (!get_check(current_creation_time_ctrl_id) && bCustom);
					m_options.custom_creation_time = bCustom;
					enable(current_creation_time_ctrl_id, bCustom);
					enable(ftCreationDate_ctrl_id, bCustomEdit);
					enable(ftCreationTime_ctrl_id, bCustomEdit);
					enable(ftCreationClear_ctrl_id, bCustomEdit);
					break;
				}
				if (param1 == current_creation_time_ctrl_id) {
					DisableEvents de(*this);
					bool bCurrent = get_check(current_creation_time_ctrl_id);
					m_options.current_creation_time = bCurrent;
					enable(ftCreationDate_ctrl_id, !bCurrent);
					enable(ftCreationTime_ctrl_id, !bCurrent);
					enable(ftCreationClear_ctrl_id, !bCurrent);
					break;
				}

				if (param1 == ftCreationClear_ctrl_id) {
					DisableEvents de(*this);
					SetDateTime(ftCreationDate_ctrl_id, m_options.ftCreationTime, ftClearBtState[0] ^= 1, 1);
					break;
				}

				if (param1 == ftLastAccessClear_ctrl_id) {
					DisableEvents de(*this);
					SetDateTime(ftLastAccessDate_ctrl_id, m_options.ftLastAccessTime, ftClearBtState[1] ^= 1,
							1);
					break;
				}
				if (param1 == ftLastWriteClear_ctrl_id) {
					DisableEvents de(*this);
					SetDateTime(ftLastWriteDate_ctrl_id, m_options.ftLastWriteTime, ftClearBtState[2] ^= 1,
							1);
					break;
				}

				if (param1 == custom_last_access_time_ctrl_id) {
					DisableEvents de(*this);
					bool bCustom = get_check(custom_last_access_time_ctrl_id);
					bool bCustomEdit = (!get_check(current_last_access_time_ctrl_id) && bCustom);
					m_options.custom_last_access_time = bCustom;
					enable(current_last_access_time_ctrl_id, bCustom);
					enable(ftLastAccessDate_ctrl_id, bCustomEdit);
					enable(ftLastAccessTime_ctrl_id, bCustomEdit);
					enable(ftLastAccessClear_ctrl_id, bCustomEdit);
					break;
				}
				if (param1 == current_last_access_time_ctrl_id) {
					DisableEvents de(*this);
					bool bCurrent = get_check(current_last_access_time_ctrl_id);
					m_options.current_last_access_time = bCurrent;
					enable(ftLastAccessDate_ctrl_id, !bCurrent);
					enable(ftLastAccessTime_ctrl_id, !bCurrent);
					enable(ftLastAccessClear_ctrl_id, !bCurrent);
					break;
				}
				if (param1 == custom_last_write_time_ctrl_id) {
					DisableEvents de(*this);
					bool bCustom = get_check(custom_last_write_time_ctrl_id);
					bool bCustomEdit = (!get_check(current_last_write_time_ctrl_id) && bCustom);
					m_options.custom_last_write_time = bCustom;
					enable(current_last_write_time_ctrl_id, bCustom);
					enable(ftLastWriteDate_ctrl_id, bCustomEdit);
					enable(ftLastWriteTime_ctrl_id, bCustomEdit);
					enable(ftLastWriteClear_ctrl_id, bCustomEdit);
					break;
				}
				if (param1 == current_last_write_time_ctrl_id) {
					DisableEvents de(*this);
					bool bCurrent = get_check(current_last_write_time_ctrl_id);
					m_options.current_last_write_time = bCurrent;
					enable(ftLastWriteDate_ctrl_id, !bCurrent);
					enable(ftLastWriteTime_ctrl_id, !bCurrent);
					enable(ftLastWriteClear_ctrl_id, !bCurrent);
					break;
				}

				if (param1 == custom_user_name_ctrl_id) {
					bool bCustom = get_check(custom_user_name_ctrl_id);
					m_options.custom_user_name = bCustom;
					enable(custom_user_name_ctrl_id + 1, bCustom);
					break;
				}
				if (param1 == custom_group_name_ctrl_id) {
					bool bCustom = get_check(custom_group_name_ctrl_id);
					m_options.custom_group_name = bCustom;
					enable(custom_group_name_ctrl_id + 1, bCustom);
					break;
				}
				if (param1 == custom_user_id_ctrl_id) {
					bool bCustom = get_check(custom_user_id_ctrl_id);
					m_options.custom_user_id = bCustom;
					enable(custom_user_id_ctrl_id + 1, bCustom);
					break;
				}
				if (param1 == custom_group_id_ctrl_id) {
					bool bCustom = get_check(custom_group_id_ctrl_id);
					m_options.custom_group_id = bCustom;
					enable(custom_group_id_ctrl_id + 1, bCustom);
					break;
				}
				if (param1 == custom_unix_mode_ctrl_id) {
					DisableEvents de(*this);
					bool bCustom = get_check(custom_unix_mode_ctrl_id);
					m_options.custom_unix_mode = bCustom;
					enable(custom_unix_mode_ctrl_id + 1, bCustom);
					enable(custom_unix_mode_ctrl_id + 2, bCustom);
					enable(custom_unix_mode_ctrl_id + 3, bCustom);
					break;
				}
				if (param1 == custom_unix_device_ctrl_id) {
					DisableEvents de(*this);
					bool bCustom = get_check(custom_unix_device_ctrl_id);
					m_options.custom_unix_device = bCustom;
					enable(custom_unix_device_ctrl_id + 1, bCustom);
					enable(custom_unix_device_ctrl_id + 2, bCustom);
					enable(custom_unix_device_ctrl_id + 3, bCustom);
					enable(custom_unix_device_ctrl_id + 4, bCustom);
					break;
				}

				if (param1 == custom_file_attributes_ctrl_id) {
					m_options.custom_file_attributes = get_check(custom_file_attributes_ctrl_id);
					if (m_options.custom_file_attributes) {
						if (!export_attr_dialog(m_options.dwFileAttributes, m_options.dwExportAttributesMask,
									m_options.export_unix_mode ? 16 : kNumWinAtrribFlags)) {
							m_options.custom_file_attributes = false;
							set_check(custom_file_attributes_ctrl_id, false);
						} else {	// refresh attrlist label
							DisableEvents de(*this);
							format_winattrib(strName, m_options.dwFileAttributes,
									m_options.dwExportAttributesMask,
									m_options.export_unix_mode ? 16 : kNumWinAtrribFlags);
							set_text(label_file_attributes_ctrl_id, strName);
							enable(label_file_attributes_ctrl_id, true);
						}
					} else
						enable(label_file_attributes_ctrl_id, false);
				}
			} break;

			case DN_EDITCHANGE: {

				if (param1 == ftCreationDate_ctrl_id || param1 == ftCreationTime_ctrl_id) {
					GetDateTime(ftCreationDate_ctrl_id, m_options.ftCreationTime);
					break;
				}
				if (param1 == ftLastAccessDate_ctrl_id || param1 == ftLastAccessTime_ctrl_id) {
					GetDateTime(ftLastAccessDate_ctrl_id, m_options.ftLastAccessTime);
					break;
				}
				if (param1 == ftLastWriteDate_ctrl_id || param1 == ftLastWriteTime_ctrl_id) {
					GetDateTime(ftLastWriteDate_ctrl_id, m_options.ftLastWriteTime);
					break;
				}

				if (param1 == Owner_ctrl_id) {
					size_t len = get_text(Owner_ctrl_id, strName, 64);
					if (!len) {
						m_options.Owner = L"";
						break;
					}
					if (!is_valid_username_or_groupname(strName, len)) {
						set_text_silent(Owner_ctrl_id, m_options.Owner.c_str());
					} else
						m_options.Owner = strName;
					break;
				}

				if (param1 == Group_ctrl_id) {
					size_t len = get_text(Group_ctrl_id, strName, 64);
					if (!len) {
						m_options.Group = L"";
						break;
					}
					if (!is_valid_username_or_groupname(strName, len)) {
						set_text_silent(Group_ctrl_id, m_options.Group.c_str());
					} else
						m_options.Group = strName;
					break;
				}

				if (param1 == UnixOwner_ctrl_id) {
					size_t len = get_text(UnixOwner_ctrl_id, strName, 64);
					if (!len) {
						m_options.UnixOwner = 0;
						break;
					}
					if (!is_valid_userid_or_groupid(strName, len)) {
						swprintf(strName, 16, L"%u", (uint32_t)m_options.UnixOwner);
						set_text_silent(UnixOwner_ctrl_id, strName);
					} else
						m_options.UnixOwner = wcstoul(strName, NULL, 10);
					break;
				}

				if (param1 == UnixGroup_ctrl_id) {
					size_t len = get_text(UnixGroup_ctrl_id, strName, 64);
					if (!len) {
						m_options.UnixGroup = 0;
						break;
					}
					if (!is_valid_userid_or_groupid(strName, len)) {
						swprintf(strName, 16, L"%u", (uint32_t)m_options.UnixGroup);
						set_text_silent(UnixGroup_ctrl_id, strName);
					} else
						m_options.UnixGroup = wcstoul(strName, NULL, 10);
					break;
				}

				if (param1 == UnixMode_ctrl_id) {
					size_t len = get_text(UnixMode_ctrl_id, strName, 64);
					if (!len) {
						m_options.UnixNode = 0;
					} else {
						if (len > 4 || strName[0] < L'0' || strName[0] > L'7' || strName[1] < L'0'
								|| strName[1] > L'7' || strName[2] < L'0' || strName[2] > L'7'
								|| strName[3] < L'0' || strName[3] > L'7') {
							swprintf(strName, 5, L"%04o", m_options.UnixNode & 0xFFF);
							set_text_silent(UnixMode_ctrl_id, strName);
						} else {
							m_options.UnixNode = wcstoul(strName, NULL, 8) & 0xFFF;
						}
					}

					format_posix_attrib(strName, m_options.UnixNode & 0xFFF);
					set_text(label_UnixMode_ctrl_id, strName);
					break;
				}

				if (param1 == UnixDeviceMajor_ctrl_id) {
					size_t len = get_text(UnixDeviceMajor_ctrl_id, strName, 64);
					if (!len) {
						uint32_t mn = (uint32_t)minor(m_options.UnixDevice) & 0xFFFF;
						m_options.UnixDevice = makedev(0, mn);
						break;
					}
					if (!is_valid_unsigned_word(strName, len)) {
						swprintf(strName, 16, L"%u", ((uint32_t)major(m_options.UnixDevice)) & 0xFFFF);
						set_text_silent(UnixDeviceMajor_ctrl_id, strName);
					} else {
						uint32_t mj = wcstoul(strName, NULL, 10);
						uint32_t mn = (uint32_t)minor(m_options.UnixDevice) & 0xFFFF;
						m_options.UnixDevice = makedev(mj, mn);
					}
					break;
				}

				if (param1 == UnixDeviceMinor_ctrl_id) {
					size_t len = get_text(UnixDeviceMinor_ctrl_id, strName, 64);
					if (!len) {
						uint32_t mj = (uint32_t)major(m_options.UnixDevice) & 0xFFFF;
						m_options.UnixDevice = makedev(mj, 0);
						break;
					}
					if (!is_valid_unsigned_word(strName, len)) {
						swprintf(strName, 16, L"%u", ((uint32_t)minor(m_options.UnixDevice)) & 0xFFFF);
						set_text_silent(UnixDeviceMinor_ctrl_id, strName);
					} else {
						uint32_t mn = wcstoul(strName, NULL, 10);
						uint32_t mj = (uint32_t)major(m_options.UnixDevice) & 0xFFFF;
						m_options.UnixDevice = makedev(mj, mn);
					}
					break;
				}

			} break;
		}

		if (msg == DN_EDITCHANGE || msg == DN_BTNCLICK) {
			unsigned profile_idx = static_cast<unsigned>(profiles.size());

			for (unsigned i = 0; i < profiles.size(); i++) {
				if (m_options == profiles[i].options) {
					profile_idx = i;
					break;
				}
			}

			if (profile_idx != get_list_pos(profile_ctrl_id)) {
				DisableEvents de(*this);
				set_list_pos(profile_ctrl_id, profile_idx);
			}
		}

		return default_dialog_proc(msg, param1, param2);
	}

public:
	ExportOptionsDialog(ExportOptions *_options, const UpdateProfiles &update_profiles, ArcType &arc_type,
			int arc_mth)
		: Far::Dialog(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_TITLE), &c_export_options_dialog_guid, c_client_xs,
				  L"ExportOptions"),
		  _options(_options),
		  m_options(*_options),
		  arc_type(arc_type),
		  arc_mth(arc_mth)
	{
		std::for_each(update_profiles.cbegin(), update_profiles.cend(),
				[&](const UpdateProfile &update_profile) {
					if (update_profile.options.use_export_settings) {
						ExportProfile export_profile;
						export_profile.name = update_profile.name;
						export_profile.options = update_profile.options.export_options;
						profiles.push_back(export_profile);
					}
				});
	}

	bool show()
	{
		if (arc_type == c_7z) {
			bUsersGroups = false;
			bUsersGroupsIDS = false;
			bUnixDevice = false;
			bFileDescriptions = false;
		} else if (arc_type == c_zip) {
			bUsersGroups = false;
			bUsersGroupsIDS = false;
			bUnixDevice = false;
			bFileAttributes = false;
		} else if (arc_type == c_tar) {
			if (!arc_mth) {
				bLastAccessTime = false;
				bCreationTime = false;
			}
			bFileAttributes = false;
			bFileDescriptions = false;
		} else if (arc_type == c_wim) {
			bUsersGroups = false;
			bUsersGroupsIDS = false;
			bUnixDevice = false;
		} else if (arc_type == c_xz || arc_type == c_gzip || arc_type == c_bzip2) {

			if (arc_type != c_gzip)
				bLastWriteTime = false;

			bLastAccessTime = false;
			bCreationTime = false;
			bUsersGroups = false;
			bUsersGroupsIDS = false;
			bUnixDevice = false;
			bUnixMode = false;
			bFileAttributes = false;
			bFileDescriptions = false;
		}

		//========================================================================================================================================

		label(Far::get_msg(MSG_SFX_OPTIONS_DLG_PROFILE));
		std::vector<std::wstring> profile_names;
		profile_names.reserve(profiles.size() + 1);
		for (unsigned i = 0; i < profiles.size(); i++) {
			profile_names.push_back(profiles[i].name);
		}
		profile_names.emplace_back();
		profile_ctrl_id = combo_box(profile_names, profiles.size(), 30, DIF_DROPDOWNLIST);
		new_line();
		separator();
		new_line();

		size_t label_len = 0;
		std::vector<std::wstring> labels;
		labels.reserve(11);
		//		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FILEATTR));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_CREATION_TIME));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_LAST_ACCESS_TIME));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_LAST_WRITE_TIME));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_OWNER));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_GROUP));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_OWNER_ID));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_GROUP_ID));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_UNIX_MODE));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_UNIX_DEVICE));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FILE_ATTR));
		labels.emplace_back(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FILE_DESC));

		for (unsigned i = 0; i < labels.size(); i++)
			if (label_len < labels[i].size())
				label_len = labels[i].size();
		label_len += 5;
		std::vector<std::wstring>::const_iterator label_text = labels.cbegin();

		DateSeparator = Far::g_fsf.GetDateSeparator();
		TimeSeparator = Far::g_fsf.GetTimeSeparator();
		DecimalSeparator = Far::g_fsf.GetDecimalSeparator();
		iDateFormat = Far::g_fsf.GetDateFormat();

		swprintf(strSTMask, 16, L"%%u%c%%u%c%%u%c%%u", TimeSeparator, TimeSeparator, DecimalSeparator);
		swprintf(strSDMask, 16, L"%%u%c%%u%c%%u", DateSeparator, DateSeparator);

		wchar_t strDMask[16], strTMask[16], strDTTitle[32];
		swprintf(strTMask, 16, L"99%c99%c99%c99N", TimeSeparator, TimeSeparator, DecimalSeparator);

		switch (iDateFormat) {
			case 0:
				swprintf(strDMask, 16, L"99%c99%c9999N", DateSeparator, DateSeparator);
				swprintf(strDTTitle, 32, Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FTIMEFORMAT1).c_str(),
						DateSeparator, DateSeparator, TimeSeparator, TimeSeparator, DecimalSeparator);
				break;
			case 1:
				swprintf(strDMask, 16, L"99%c99%c9999N", DateSeparator, DateSeparator);
				swprintf(strDTTitle, 32, Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FTIMEFORMAT2).c_str(),
						DateSeparator, DateSeparator, TimeSeparator, TimeSeparator, DecimalSeparator);
				break;
			default:
				swprintf(strDMask, 16, L"N9999%c99%c99", DateSeparator, DateSeparator);
				swprintf(strDTTitle, 16, Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FTIMEFORMAT3).c_str(),
						DateSeparator, DateSeparator, TimeSeparator, TimeSeparator, DecimalSeparator);
				break;
		}
		label(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_FILEATTR));
		pad(label_len + 4);
		label(strDTTitle);
		new_line();

		ftClearBtState[0] = ftClearBtState[1] = ftClearBtState[2] = 0;

		export_creation_time_ctrl_id =
				check_box3(*label_text++, m_options.export_creation_time, bCreationTime ? 0 : DIF_DISABLE);
		pad(label_len);
		custom_creation_time_ctrl_id = check_box(L"", m_options.custom_creation_time,
				bCreationTime && m_options.export_creation_time == triTrue ? 0 : DIF_DISABLE);
		{
			bool bExportCustom =
					(m_options.export_creation_time == triTrue && m_options.custom_creation_time && bCreationTime);
			bool bCustomEdit = (!m_options.current_creation_time && bExportCustom && bCreationTime);
			wchar_t strDate[32], strTime[32];
			FillDateTime(m_options.ftCreationTime, strDate, strTime);
			ftCreationDate_ctrl_id = mask_edit_box(strDate, strDMask, 11, bCustomEdit ? 0 : DIF_DISABLE);
			spacer(1);
			ftCreationTime_ctrl_id = mask_edit_box(strTime, strTMask, 12, bCustomEdit ? 0 : DIF_DISABLE);
			ftCreationClear_ctrl_id =
					button(L"<", bCustomEdit ? DIF_BTNNOCLOSE : DIF_DISABLE | DIF_BTNNOCLOSE);
			spacer(1);
			current_creation_time_ctrl_id = check_box(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_CURTIME),
					m_options.current_creation_time, bExportCustom ? 0 : DIF_DISABLE);
		}
		new_line();

		export_last_access_time_ctrl_id = check_box3(*label_text++, m_options.export_last_access_time,
				bLastAccessTime ? 0 : DIF_DISABLE);
		pad(label_len);
		custom_last_access_time_ctrl_id = check_box(L"", m_options.custom_last_access_time,
				bLastAccessTime && m_options.export_last_access_time == triTrue ? 0 : DIF_DISABLE);
		{
			bool bExportCustom = (m_options.export_last_access_time == triTrue && m_options.custom_last_access_time
					&& bLastAccessTime);
			bool bCustomEdit = (!m_options.current_last_access_time && bExportCustom && bLastAccessTime);
			wchar_t strDate[32], strTime[32];
			FillDateTime(m_options.ftLastAccessTime, strDate, strTime);
			ftLastAccessDate_ctrl_id = mask_edit_box(strDate, strDMask, 11, bCustomEdit ? 0 : DIF_DISABLE);
			spacer(1);
			ftLastAccessTime_ctrl_id = mask_edit_box(strTime, strTMask, 12, bCustomEdit ? 0 : DIF_DISABLE);
			ftLastAccessClear_ctrl_id =
					button(L"<", bCustomEdit ? DIF_BTNNOCLOSE : DIF_DISABLE | DIF_BTNNOCLOSE);
			spacer(1);
			current_last_access_time_ctrl_id = check_box(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_CURTIME),
					m_options.current_last_access_time, bExportCustom ? 0 : DIF_DISABLE);
		}
		new_line();

		export_last_write_time_ctrl_id =
				check_box3(*label_text++, m_options.export_last_write_time, bLastWriteTime ? 0 : DIF_DISABLE);
		pad(label_len);
		custom_last_write_time_ctrl_id = check_box(L"", m_options.custom_last_write_time,
				bLastWriteTime && m_options.export_last_write_time == triTrue ? 0 : DIF_DISABLE);
		{
			bool bExportCustom =
					(m_options.export_last_write_time == triTrue && m_options.custom_last_write_time && bLastWriteTime);
			bool bCustomEdit = (!m_options.current_last_write_time && bExportCustom && bLastWriteTime);
			wchar_t strDate[32], strTime[32];
			FillDateTime(m_options.ftLastWriteTime, strDate, strTime);
			ftLastWriteDate_ctrl_id = mask_edit_box(strDate, strDMask, 11, bCustomEdit ? 0 : DIF_DISABLE);
			spacer(1);
			ftLastWriteTime_ctrl_id = mask_edit_box(strTime, strTMask, 12, bCustomEdit ? 0 : DIF_DISABLE);
			ftLastWriteClear_ctrl_id =
					button(L"<", bCustomEdit ? DIF_BTNNOCLOSE : DIF_DISABLE | DIF_BTNNOCLOSE);
			spacer(1);
			current_last_write_time_ctrl_id = check_box(Far::get_msg(MSG_EXPORT_OPTIONS_DLG_CURTIME),
					m_options.current_last_write_time, bExportCustom ? 0 : DIF_DISABLE);
		}
		new_line();
		separator();
		new_line();

		{
			wchar_t strName[40];
			swprintf(strName, 32, L"%ls",
					is_valid_username_or_groupname(m_options.Owner.c_str(), m_options.Owner.length())
							? m_options.Owner.c_str()
							: L"user");
			export_user_name_ctrl_id =
					check_box(*label_text++, m_options.export_user_name, bUsersGroups ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_user_name_ctrl_id = check_box(L"", m_options.custom_user_name,
					bUsersGroups && m_options.export_user_name ? 0 : DIF_DISABLE);
			bool bExportCustom = (m_options.export_user_name && m_options.custom_user_name && bUsersGroups);
			Owner_ctrl_id = edit_box(strName, 24,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);	 // 32
			new_line();

			swprintf(strName, 32, L"%ls",
					is_valid_username_or_groupname(m_options.Group.c_str(), m_options.Group.length())
							? m_options.Group.c_str()
							: L"group");
			export_group_name_ctrl_id =
					check_box(*label_text++, m_options.export_group_name, bUsersGroups ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_group_name_ctrl_id = check_box(L"", m_options.custom_group_name,
					bUsersGroups && m_options.export_group_name ? 0 : DIF_DISABLE);
			bExportCustom = (m_options.export_group_name && m_options.custom_group_name && bUsersGroups);
			Group_ctrl_id = edit_box(strName, 24,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);	 // 32
			new_line();
		}

		{
			wchar_t strName[40];
			swprintf(strName, 32, L"%u", (uint32_t)m_options.UnixOwner);

			export_user_id_ctrl_id =
					check_box(*label_text++, m_options.export_user_id, bUsersGroupsIDS ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_user_id_ctrl_id = check_box(L"", m_options.custom_user_id,
					bUsersGroupsIDS && m_options.export_user_id ? 0 : DIF_DISABLE);
			bool bExportCustom = (m_options.export_user_id && m_options.custom_user_id && bUsersGroupsIDS);
			UnixOwner_ctrl_id = edit_box(strName, 11,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);
			new_line();

			swprintf(strName, 32, L"%u", (uint32_t)m_options.UnixGroup);
			export_group_id_ctrl_id =
					check_box(*label_text++, m_options.export_group_id, bUsersGroupsIDS ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_group_id_ctrl_id = check_box(L"", m_options.custom_group_id,
					bUsersGroupsIDS && m_options.export_group_id ? 0 : DIF_DISABLE);
			bExportCustom = (m_options.export_group_id && m_options.custom_group_id && bUsersGroupsIDS);
			UnixGroup_ctrl_id = edit_box(strName, 11,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);
			new_line();
		}

		{
			wchar_t strName[32];
			swprintf(strName, 5, L"%04o", m_options.UnixNode & 0xFFF);

			export_unix_mode_ctrl_id =
					check_box(*label_text++, m_options.export_unix_mode, bUnixMode ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_unix_mode_ctrl_id = check_box(L"", m_options.custom_unix_mode,
					bUnixMode && m_options.export_unix_mode ? 0 : DIF_DISABLE);
			bool bExportCustom = (m_options.export_unix_mode && m_options.custom_unix_mode && bUnixMode);

			label(L"Octal SUGO:", AUTO_SIZE, bExportCustom ? 0 : DIF_DISABLE);
			spacer(1);
			UnixMode_ctrl_id = mask_edit_box(strName, L"9999", 4,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);
			format_posix_attrib(strName, m_options.UnixNode & 0xFFF);
			label_UnixMode_ctrl_id = label(strName, 10, bExportCustom ? 0 : DIF_DISABLE);
			new_line();
		}

		{
			wchar_t strName[32];
			swprintf(strName, 16, L"%u", ((uint32_t)major(m_options.UnixDevice)) & 0xFFFF);
			export_unix_device_ctrl_id =
					check_box(*label_text++, m_options.export_unix_device, bUnixDevice ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_unix_device_ctrl_id = check_box(L"", m_options.custom_unix_device,
					bUnixDevice && m_options.export_unix_device ? 0 : DIF_DISABLE);
			bool bExportCustom =
					(m_options.export_unix_device && m_options.custom_unix_device && bUnixDevice);
			label(L"Major:", AUTO_SIZE, bExportCustom ? 0 : DIF_DISABLE);
			UnixDeviceMajor_ctrl_id = edit_box(strName, 10,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);
			spacer(1);
			label(L"Minor:", AUTO_SIZE, bExportCustom ? 0 : DIF_DISABLE);
			swprintf(strName, 16, L"%u", ((uint32_t)minor(m_options.UnixDevice)) & 0xFFFF);
			UnixDeviceMinor_ctrl_id = edit_box(strName, 10,
					bExportCustom ? DIF_SELECTONENTRY : DIF_SELECTONENTRY | DIF_DISABLE);
			new_line();
		}
		{
			wchar_t strName[64];
			format_winattrib(strName, m_options.dwFileAttributes, m_options.dwExportAttributesMask,
					m_options.export_unix_mode ? 16 : kNumWinAtrribFlags);

			export_file_attributes_ctrl_id = check_box(*label_text++, m_options.export_file_attributes,
					bFileAttributes ? 0 : DIF_DISABLE);
			pad(label_len);
			custom_file_attributes_ctrl_id = check_box(L"", m_options.custom_file_attributes,
					bFileAttributes && m_options.export_file_attributes ? 0 : DIF_DISABLE);
			label_file_attributes_ctrl_id = label(strName, kNumWinAtrribFlags * 2,
					(bFileAttributes && m_options.export_file_attributes && m_options.custom_file_attributes)
							? 0
							: DIF_DISABLE);
			new_line();
		}

		export_file_descriptions_ctrl_id = check_box(*label_text++, m_options.export_file_descriptions,
				bFileDescriptions ? 0 : DIF_DISABLE);
		new_line();

		separator();
		new_line();

		ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
		cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
		reset_ctrl_id = button(Far::get_msg(MSG_BUTTON_RESET), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
		new_line();

		intptr_t item = Far::Dialog::show();
		if (item != -1 && item != cancel_ctrl_id) {
			*_options = m_options;
			return true;
		}

		return false;
	}
};

bool export_options_dialog(ExportOptions *_export_options, const UpdateProfiles &update_profiles,
		ArcType &arc_type, int arc_mth)
{
	return ExportOptionsDialog(_export_options, update_profiles, arc_type, arc_mth).show();
}

class ExtractDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 60
	};

	ExtractOptions &m_options;

	int dst_dir_ctrl_id{};
	int ignore_errors_ctrl_id{};
	int extract_accessr_ctrl_id{};
	int extract_ownersg_ctrl_id{};
	int extract_ownersg_cbox_ctrl_id{};
	int extract_attr_ctrl_id{};
	int oa_ask_ctrl_id{};
	int oa_overwrite_ctrl_id{};
	int oa_skip_ctrl_id{};
	int oa_rename_ctrl_id{};
	int oa_append_ctrl_id{};
	int move_files_ctrl_id{};
	int password_ctrl_id{};
	int separate_dir_ctrl_id{};
	int delete_archive_ctrl_id{};
	int open_dir_ctrl_id{};
	int ok_ctrl_id{};
	int cancel_ctrl_id{};
	int save_params_ctrl_id{};
	int enable_filter_ctrl_id{};
	int dup_hardlnks_ctrl_id{};
	int special_files_ctrl_id{};
	int double_cache_ctrl_id{};

	void read_controls(ExtractOptions &options)
	{
		auto dst = search_and_replace(strip(get_text(dst_dir_ctrl_id)), L"\"", std::wstring());
		bool expand_env = add_trailing_slash(options.dst_dir) != add_trailing_slash(dst);
		options.dst_dir = expand_env ? expand_env_vars(dst) : dst;
		options.ignore_errors = get_check(ignore_errors_ctrl_id);

		options.extract_access_rights = get_check(extract_accessr_ctrl_id);

		bool bExOwnGr = get_check(extract_ownersg_ctrl_id);
		if (bExOwnGr) {
			unsigned sel = get_list_pos(extract_ownersg_cbox_ctrl_id);
			options.extract_owners_groups = sel + 1;
		}
		else
			options.extract_owners_groups = 0;

		options.extract_attributes = get_check(extract_attr_ctrl_id);
		options.duplicate_hardlinks = get_check(dup_hardlnks_ctrl_id);
		options.restore_special_files = get_check(special_files_ctrl_id);

		if (get_check(oa_ask_ctrl_id))
			options.overwrite = oaAsk;
		else if (get_check(oa_overwrite_ctrl_id))
			options.overwrite = oaOverwrite;
		else if (get_check(oa_skip_ctrl_id))
			options.overwrite = oaSkip;
		else if (get_check(oa_rename_ctrl_id))
			options.overwrite = oaRename;
		else if (get_check(oa_append_ctrl_id))
			options.overwrite = oaAppend;
		else
			options.overwrite = oaAsk;
		if (options.move_files != triUndef)
			options.move_files = get_check3(move_files_ctrl_id);
		options.password = get_text(password_ctrl_id);
		options.separate_dir = get_check3(separate_dir_ctrl_id);
		options.double_buffering = get_check3(double_cache_ctrl_id);
		options.delete_archive = get_check(delete_archive_ctrl_id);
		if (options.open_dir != triUndef)
			options.open_dir = get_check3(open_dir_ctrl_id);
	}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
			read_controls(m_options);
		} else if (msg == DN_BTNCLICK && param1 == delete_archive_ctrl_id) {
			enable(move_files_ctrl_id,
					m_options.move_files != triUndef && !get_check(delete_archive_ctrl_id));
		} else if (msg == DN_BTNCLICK && param1 == extract_ownersg_ctrl_id) {
			enable(extract_ownersg_cbox_ctrl_id, get_check(extract_ownersg_ctrl_id));
		} else if (msg == DN_BTNCLICK && param1 == save_params_ctrl_id) {
			ExtractOptions options;
			read_controls(options);

			g_options.extract_ignore_errors = options.ignore_errors;
			g_options.extract_access_rights = options.extract_access_rights;
			g_options.extract_owners_groups = options.extract_owners_groups;
			g_options.extract_attributes = options.extract_attributes;
			g_options.extract_duplicate_hardlinks = options.duplicate_hardlinks;
			g_options.extract_restore_special_files = options.restore_special_files;
			g_options.extract_overwrite = options.overwrite;
			g_options.extract_separate_dir = options.separate_dir;

			if (options.open_dir != triUndef)
				g_options.extract_open_dir = options.open_dir == triTrue;
			g_options.save();
			Far::info_dlg(c_extract_params_saved_dialog_guid, Far::get_msg(MSG_EXTRACT_DLG_TITLE),
					Far::get_msg(MSG_EXTRACT_DLG_PARAMS_SAVED));
			set_focus(ok_ctrl_id);
		} else if (msg == DN_BTNCLICK && param1 == enable_filter_ctrl_id) {
			if (param2) {
				m_options.filter.reset(new Far::FileFilter());
				if (!m_options.filter->create(PANEL_NONE, FFT_CUSTOM) || !m_options.filter->menu()) {
					m_options.filter.reset();
					DisableEvents de(*this);
					set_check(enable_filter_ctrl_id, false);
				}
			} else {
				m_options.filter.reset();
			}
			//			set_control_state();
		}

		return default_dialog_proc(msg, param1, param2);
	}

public:
	ExtractDialog(ExtractOptions &options)
		: Far::Dialog(Far::get_msg(MSG_EXTRACT_DLG_TITLE), &c_extract_dialog_guid, c_client_xs, L"Extract"),
		  m_options(options)
	{}

	bool show()
	{
		label(Far::get_msg(MSG_EXTRACT_DLG_DST_DIR));
		new_line();
		dst_dir_ctrl_id = history_edit_box(add_trailing_slash(m_options.dst_dir), L"arclite.extract_dir",
				c_client_xs, DIF_EDITPATH);
		new_line();
		separator();
		new_line();

		ignore_errors_ctrl_id =
				check_box(Far::get_msg(MSG_EXTRACT_DLG_IGNORE_ERRORS), m_options.ignore_errors);
		new_line();
		extract_accessr_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_ACCESS_RIGHTS),
				m_options.extract_access_rights);
		new_line();

		extract_ownersg_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_OWNER_GROUPS_BY),
				m_options.extract_owners_groups > 0);

		{
			std::vector<std::wstring> exowngrpby = {Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_OWNER_GROUPS_BY_NAMES),
					Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_OWNER_GROUPS_BY_IDS),
					Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_OWNER_GROUPS_BY_NAMES_IDS)};

			unsigned exowngrby_sel = (m_options.extract_owners_groups > 0) ? (m_options.extract_owners_groups - 1) : 0;
			if (exowngrby_sel > 2) 
				exowngrby_sel = 2;

			extract_ownersg_cbox_ctrl_id = combo_box(exowngrpby, exowngrby_sel, AUTO_SIZE, DIF_DROPDOWNLIST | 
									(m_options.extract_owners_groups ? 0 : DIF_DISABLE) );
		}

		new_line();
		extract_attr_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_EX_ATTRIBUTES),
				m_options.extract_attributes, DIF_DISABLE);
		new_line();

		dup_hardlnks_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_DUP_HARDLINKS),
				m_options.duplicate_hardlinks);

		new_line();

		special_files_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_RESTORE_SPECIAL_FILES),
				m_options.restore_special_files);

		new_line();

		label(Far::get_msg(MSG_EXTRACT_DLG_OA));
		new_line();
		spacer(2);
		oa_ask_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OA_ASK), m_options.overwrite == oaAsk);
		spacer(2);
		oa_overwrite_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OA_OVERWRITE),
				m_options.overwrite == oaOverwrite || m_options.overwrite == oaOverwriteCase);
		spacer(2);
		oa_skip_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OA_SKIP), m_options.overwrite == oaSkip);
		new_line();
		spacer(2);
		oa_rename_ctrl_id =
				radio_button(Far::get_msg(MSG_EXTRACT_DLG_OA_RENAME), m_options.overwrite == oaRename);
		spacer(2);
		oa_append_ctrl_id =
				radio_button(Far::get_msg(MSG_EXTRACT_DLG_OA_APPEND), m_options.overwrite == oaAppend);
		new_line();

		move_files_ctrl_id = check_box3(Far::get_msg(MSG_EXTRACT_DLG_MOVE_FILES), m_options.move_files,
				m_options.move_files == triUndef ? DIF_DISABLE : 0);
		new_line();
		delete_archive_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_DELETE_ARCHIVE),
				m_options.delete_archive, m_options.disable_delete_archive ? DIF_DISABLE : 0);
		new_line();
		separate_dir_ctrl_id = check_box3(Far::get_msg(MSG_EXTRACT_DLG_SEPARATE_DIR), m_options.separate_dir);
		new_line();
		open_dir_ctrl_id = check_box3(Far::get_msg(MSG_EXTRACT_DLG_OPEN_DIR), m_options.open_dir,
				m_options.open_dir == triUndef ? DIF_DISABLE : 0);
		new_line();

		double_cache_ctrl_id = check_box3(Far::get_msg(MSG_EXTRACT_DLG_EXTRACT_DOUBLE_CACHE),
				m_options.double_buffering);

		new_line();

		label(Far::get_msg(MSG_EXTRACT_DLG_PASSWORD));
		password_ctrl_id = pwd_edit_box(m_options.password, 20);

		spacer(2);
		enable_filter_ctrl_id =
				check_box(Far::get_msg(MSG_UPDATE_DLG_ENABLE_FILTER), m_options.filter != nullptr);
		new_line();

		separator();
		new_line();
		ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
		cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
		save_params_ctrl_id =
				button(Far::get_msg(MSG_EXTRACT_DLG_SAVE_PARAMS), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
		new_line();

		intptr_t item = Far::Dialog::show();

		return (item != -1) && (item != cancel_ctrl_id);
	}
};

bool extract_dialog(ExtractOptions &options)
{
	return ExtractDialog(options).show();
}

void retry_or_ignore_error(const Error &error, bool &ignore, bool &ignore_errors, ErrorLog &error_log,
		ProgressMonitor &progress, bool can_retry, bool can_ignore)
{
	if (error.code == E_ABORT)
		throw error;
	ignore = ignore_errors;
	if (!ignore) {
		std::wostringstream st;
		st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
		if (error.code != E_MESSAGE) {
			std::wstring sys_msg = get_system_message(error.code, Far::get_lang_id());
			if (!sys_msg.empty())
				st << word_wrap(sys_msg, Far::get_optimal_msg_width()) << L'\n';
		}
		for (const auto &msg : error.messages) {
			st << word_wrap(msg, Far::get_optimal_msg_width()) << L'\n';
		}
		st << extract_file_name(widen(error.file)) << L':' << error.line << L'\n';
		unsigned button_cnt = 0;
		unsigned retry_id = 0, ignore_id = 0, ignore_all_id = 0;
		if (can_retry) {
			st << Far::get_msg(MSG_BUTTON_RETRY) << L'\n';
			retry_id = button_cnt;
			button_cnt++;
		}
		if (can_ignore) {
			st << Far::get_msg(MSG_BUTTON_IGNORE) << L'\n';
			ignore_id = button_cnt;
			button_cnt++;
			st << Far::get_msg(MSG_BUTTON_IGNORE_ALL) << L'\n';
			ignore_all_id = button_cnt;
			button_cnt++;
		}
		st << Far::get_msg(MSG_BUTTON_CANCEL);
		button_cnt++;
		ProgressSuspend ps(progress);
		auto id = Far::message(c_retry_ignore_dialog_guid, st.str(), button_cnt, FMSG_WARNING);
		if (can_retry && (unsigned)id == retry_id)
			return;
		if (can_ignore && (unsigned)id == ignore_id) {
			ignore = true;
		} else if (can_ignore && (unsigned)id == ignore_all_id) {
			ignore = true;
			ignore_errors = true;
		} else
			FAIL(E_ABORT);
	}
	if (ignore)
		error_log.push_back(error);
}

void show_error_log(const ErrorLog &error_log)
{
	std::wstring msg;
	msg += Far::get_msg(MSG_LOG_TITLE) + L'\n';
	msg += Far::get_msg(MSG_LOG_INFO) + L'\n';
	msg += Far::get_msg(MSG_LOG_CLOSE) + L'\n';
	msg += Far::get_msg(MSG_LOG_SHOW) + L'\n';
	if (Far::message(c_error_log_dialog_guid, msg, 2, FMSG_WARNING) != 1)
		return;

	TempFile temp_file;
	File file(temp_file.get_path(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

	const wchar_t sig = 0xFEFF;
	file.write(&sig, sizeof(sig));
	std::wstring line;
	for (const auto &error : error_log) {
		line.clear();
		if (error.code != E_MESSAGE && error.code != S_FALSE) {
			std::wstring sys_msg = get_system_message(error.code, Far::get_lang_id());
			if (!sys_msg.empty())
				line.append(sys_msg).append(1, L'\n');
		}
		for (const auto &o : error.objects)
			line.append(o).append(1, L'\n');
		std::wstring indent;
		if (!error.messages.empty() && (!error.objects.empty() || !error.warnings.empty())) {
			line.append(Far::get_msg(MSG_KPID_ERRORFLAGS)).append(L":\n");
			indent = L"  ";
		}
		for (const auto &e : error.messages)
			line.append(indent).append(e).append(1, L'\n');
		if (!error.warnings.empty())
			line.append(Far::get_msg(MSG_KPID_WARNINGFLAGS)).append(L":\n");
		for (const auto &w : error.warnings)
			line.append(L"  ").append(w).append(1, L'\n');
		line.append(1, L'\n');
		file.write(line.data(), static_cast<unsigned>(line.size()) * sizeof(wchar_t));
	}
	file.close();

	Far::viewer(temp_file.get_path(), Far::get_msg(MSG_LOG_TITLE), VF_DISABLEHISTORY | VF_ENABLE_F6);
}

bool operator==(const SfxOptions &o1, const SfxOptions &o2)
{
	if (o1.name != o2.name || o1.replace_icon != o2.replace_icon || o1.replace_version != o2.replace_version
			|| o1.append_install_config != o2.append_install_config)
		return false;
	if (o1.replace_icon) {
		if (o1.icon_path != o2.icon_path)
			return false;
	}
	if (o1.replace_version) {
		if (o1.ver_info.version != o2.ver_info.version || o1.ver_info.comments != o2.ver_info.comments
				|| o1.ver_info.company_name != o2.ver_info.company_name
				|| o1.ver_info.file_description != o2.ver_info.file_description
				|| o1.ver_info.legal_copyright != o2.ver_info.legal_copyright
				|| o1.ver_info.product_name != o2.ver_info.product_name)
			return false;
	}
	if (o1.append_install_config) {
		if (o1.install_config.title != o2.install_config.title
				|| o1.install_config.begin_prompt != o2.install_config.begin_prompt
				|| o1.install_config.progress != o2.install_config.progress
				|| o1.install_config.run_program != o2.install_config.run_program
				|| o1.install_config.directory != o2.install_config.directory
				|| o1.install_config.execute_file != o2.install_config.execute_file
				|| o1.install_config.execute_parameters != o2.install_config.execute_parameters)
			return false;
	}
	return true;
}

bool operator==(const ExportOptions &o1, const ExportOptions &o2)
{
	if (o1.export_creation_time != o2.export_creation_time)
		return false;
	if (o1.export_creation_time) {
		if (o1.custom_creation_time != o2.custom_creation_time)
			return false;
		if (o1.custom_creation_time) {
			if (o1.current_creation_time != o2.current_creation_time)
				return false;
			if (!o1.current_creation_time
					&& o1.ftCreationTime.dwHighDateTime != o2.ftCreationTime.dwHighDateTime
					&& o1.ftCreationTime.dwLowDateTime != o2.ftCreationTime.dwLowDateTime)
				return false;
		}
	}

	if (o1.export_last_access_time != o2.export_last_access_time)
		return false;
	if (o1.export_last_access_time) {
		if (o1.custom_last_access_time != o2.custom_last_access_time)
			return false;
		if (o1.custom_last_access_time) {
			if (o1.current_last_access_time != o2.current_last_access_time)
				return false;
			if (!o1.current_last_access_time
					&& o1.ftLastAccessTime.dwHighDateTime != o2.ftLastAccessTime.dwHighDateTime
					&& o1.ftLastAccessTime.dwLowDateTime != o2.ftLastAccessTime.dwLowDateTime)
				return false;
		}
	}

	if (o1.export_last_write_time != o2.export_last_write_time)
		return false;
	if (o1.export_last_write_time) {
		if (o1.custom_last_write_time != o2.custom_last_write_time)
			return false;
		if (o1.custom_last_write_time) {
			if (o1.current_last_write_time != o2.current_last_write_time)
				return false;
			if (!o1.current_last_write_time
					&& o1.ftLastWriteTime.dwHighDateTime != o2.ftLastWriteTime.dwHighDateTime
					&& o1.ftLastWriteTime.dwLowDateTime != o2.ftLastWriteTime.dwLowDateTime)
				return false;
		}
	}

	if (o1.export_user_id != o2.export_user_id)
		return false;
	if (o1.export_user_id) {
		if (o1.custom_user_id != o2.custom_user_id)
			return false;
		if (o1.custom_user_id) {
			if (o1.UnixOwner != o2.UnixOwner)
				return false;
		}
	}

	if (o1.export_group_id != o2.export_group_id)
		return false;
	if (o1.export_group_id) {
		if (o1.custom_group_id != o2.custom_group_id)
			return false;
		if (o1.custom_group_id) {
			if (o1.UnixGroup != o2.UnixGroup)
				return false;
		}
	}

	if (o1.export_user_name != o2.export_user_name)
		return false;
	if (o1.export_user_name) {
		if (o1.custom_user_name != o2.custom_user_name)
			return false;
		if (o1.custom_user_name) {
			if (o1.Owner != o2.Owner)
				return false;
		}
	}

	if (o1.export_group_name != o2.export_group_name)
		return false;
	if (o1.export_group_name) {
		if (o1.custom_group_name != o2.custom_group_name)
			return false;
		if (o1.custom_group_name) {
			if (o1.Group != o2.Group)
				return false;
		}
	}

	if (o1.export_unix_device != o2.export_unix_device)
		return false;
	if (o1.export_unix_device) {
		if (o1.custom_unix_device != o2.custom_unix_device)
			return false;
		if (o1.custom_unix_device) {
			if (o1.UnixDevice != o2.UnixDevice)
				return false;
		}
	}

	if (o1.export_unix_mode != o2.export_unix_mode)
		return false;
	if (o1.export_unix_mode) {
		if (o1.custom_unix_mode != o2.custom_unix_mode)
			return false;
		if (o1.custom_unix_mode) {
			if (o1.UnixNode != o2.UnixNode)
				return false;
		}
	}

	if (o1.export_file_attributes != o2.export_file_attributes)
		return false;
	if (o1.export_file_attributes) {
		if (o1.dwExportAttributesMask != o2.dwExportAttributesMask)
			return false;
		if (o1.custom_file_attributes != o2.custom_file_attributes)
			return false;
		if (o1.custom_file_attributes) {
			if (o1.dwFileAttributes != o2.dwFileAttributes)
				return false;
		}
	}

	if (o1.export_file_descriptions != o2.export_file_descriptions)
		return false;

	return true;
}

bool operator==(const ProfileOptions &o1, const ProfileOptions &o2)
{
	if (o1.arc_type != o2.arc_type || o1.level != o2.level) {
		return false;
	}

	if (o1.use_export_settings != o2.use_export_settings)
		return false;

	if (!(o1.export_options == o2.export_options))
		return false;

	bool is_7z = o1.arc_type == c_7z;
	bool is_zip = o1.arc_type == c_zip;
	bool is_tar = o1.arc_type == c_tar;
	bool is_compressed = o1.level != 0;

	if (is_7z || is_zip) {
		if (is_compressed) {
			if (o1.method != o2.method) {
				return false;
			}
		}
		if (o1.solid != o2.solid) {
			return false;
		}
	}

	if (o1.process_priority != o2.process_priority)
		return false;

	if (o1.multithreading != o2.multithreading)
		return false;

	if (o1.multithreading && o1.threads_num != o2.threads_num)
		return false;

	if (is_tar) {
		if (o1.method != o2.method)
			return false;
		if (o1.repack != o2.repack)
			return false;
		if (o1.repack)
			if (o1.repack_arc_type != o2.repack_arc_type)
				return false;
	}

	if (o1.advanced != o2.advanced) {
		return false;
	}

	if (is_7z || is_zip) {
		if (o1.encrypt != o2.encrypt) {
			return false;
		}
		bool is_encrypted = o1.encrypt;
		if (is_encrypted) {
			if (o1.password != o2.password) {
				return false;
			}
			if (is_7z) {
				if (o1.encrypt_header != o2.encrypt_header) {
					return false;
				}
			}
		}
	}

	bool is_sfx = o1.create_sfx;

	if (is_7z) {
		if (o1.create_sfx != o2.create_sfx) {
			return false;
		}
		if (is_sfx) {
			if (!(o1.sfx_options == o2.sfx_options)) {
				return false;
			}
		}
	}

	if (!is_7z || !is_sfx) {
		if (o1.enable_volumes != o2.enable_volumes) {
			return false;
		}
		bool is_multi_volume = o1.enable_volumes;
		if (is_multi_volume) {
			if (o1.volume_size != o2.volume_size) {
				return false;
			}
		}
	}

	if (o1.move_files != o2.move_files || o1.ignore_errors != o2.ignore_errors ||
		o1.skip_symlinks != o2.skip_symlinks || o1.dereference_symlinks != o2.dereference_symlinks ||
		o1.skip_hardlinks != o2.skip_hardlinks || o1.duplicate_hardlinks != o2.duplicate_hardlinks) {
		return false;
	}

	if (!o1.skip_symlinks && !o1.dereference_symlinks && o1.symlink_fix_path_mode != o2.symlink_fix_path_mode)
		return false;

	return true;
}

/// gnu, pax, posix.

///  LZMA, LZMA2, PPMd, BZip2, Deflate, Delta, BCJ, BCJ2, Copy.
///  Deflate, Deflate64, BZip2, LZMA, PPMd.

static bool is_SWFu(const std::wstring &fname)
{
	bool unpacked_swf = false;
	File file;
	file.open_nt(Far::get_absolute_path(fname), FILE_READ_DATA,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
	if (file.is_open()) {
		char bf[8];
		size_t nr = 0;
		file.read_nt(bf, sizeof(bf), nr);
		unpacked_swf =
				nr == sizeof(bf) && bf[2] == 'S' && bf[1] == 'W' && bf[0] == 'F' && (unsigned)bf[3] < 20;
	}
	return unpacked_swf;
}

class UpdateDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 68
	};

	bool new_arc;
	bool multifile;
	std::wstring default_arc_name;
	std::vector<ArcType> main_formats;
	std::vector<ArcType> other_formats;
	std::vector<ArcType> repack_formats;
	UpdateOptions &m_options;
	UpdateProfiles &profiles;

	int profile_ctrl_id{};
	int save_profile_ctrl_id{};
	int delete_profile_ctrl_id{};
	int arc_path_ctrl_id{};
	int arc_path_eval_ctrl_id{};
	int append_ext_ctrl_id{};
	int main_formats_ctrl_id{};
	int other_formats_ctrl_id{};
	int other_repack_ctrl_id{};
	int level_ctrl_id{};
	int method_ctrl_id{};
	int solid_ctrl_id{};

	int priority_ctrl_id{};
	int multithreading_ctrl_id{};
	int thread_num_ctrl_id{};

	int advanced_ctrl_id{};
	int encrypt_ctrl_id{};
	int encrypt_header_ctrl_id{};
	int show_password_ctrl_id{};
	int password_ctrl_id{};
	int password_verify_ctrl_id{};
	int password_visible_ctrl_id{};
	int create_sfx_ctrl_id{};
	int sfx_options_ctrl_id{};
	int symlinks_ctrl_id{};
	int symlinks_paths_ctrl_id{};
	int hardlinks_ctrl_id{};
	int move_files_ctrl_id{};
	int open_shared_ctrl_id{};
	int ignore_errors_ctrl_id{};
	int enable_volumes_ctrl_id{};
	int volume_size_ctrl_id{};
	int oa_ask_ctrl_id{};
	int oa_overwrite_ctrl_id{};
	int oa_skip_ctrl_id{};
	int enable_filter_ctrl_id{};
	int ok_ctrl_id{};
	int cancel_ctrl_id{};
	int save_params_ctrl_id{};
	int use_export_settings_ctrl_id{};
	int exp_options_ctrl_id{};

	int _BestThreadsCount;
	std::wstring old_ext;
	ArcType arc_type;
	unsigned level;

	std::wstring get_default_ext() const
	{
		std::wstring ext;
		bool create_sfx = get_check(create_sfx_ctrl_id);
		bool enable_volumes = get_check(enable_volumes_ctrl_id);

		if (ArcAPI::formats().count(arc_type))
			ext = ArcAPI::formats().at(arc_type).default_extension();

		if (create_sfx && arc_type == c_7z)
			ext += c_sfx_ext;
		else if (enable_volumes)
			ext += c_volume_ext;

//		fprintf(stderr, " +++ get_default_ext() = \"%ls\"\n", ext.c_str());
		return ext;
	}

	bool change_extension()
	{
		assert(new_arc);
		std::wstring new_ext = get_default_ext();

		if (old_ext.empty() || new_ext.empty()) {
			return false;
		}

		std::wstring arc_path = get_text(arc_path_ctrl_id);
		std::wstring file_name = extract_file_name(strip(unquote(strip(arc_path))));

		/// expand_env_vars

		size_t pos = file_name.find_last_of(L'.');
		if (pos == std::wstring::npos || pos == 0)
			return false;

		std::wstring ext = file_name.substr(pos);
		if (StrCmpI(ext.c_str(), c_sfx_ext) == 0 || StrCmpI(ext.c_str(), c_volume_ext) == 0) {
			pos = file_name.find_last_of(L'.', pos - 1);
			if (pos != std::wstring::npos && pos != 0)
				ext = file_name.substr(pos);
		}

		if (StrCmpI(old_ext.c_str(), ext.c_str()) != 0)
			return false;

		pos = arc_path.find_last_of(ext) - (ext.size() - 1);
		arc_path.replace(pos, ext.size(), new_ext);
		set_text(arc_path_ctrl_id, arc_path);

		old_ext = std::move(new_ext);
		return true;
	}

	bool arc_levels(char mode)
	{
		ArcType _arc_type = arc_type;
		if (arc_type == c_tar && m_options.repack) {
			_arc_type = m_options.repack_arc_type;
		}

		if (!ArcAPI::formats().count(_arc_type))
			return false;

		auto search = ArcAPI::formats().at(_arc_type).name;
		if (search.empty())
			return false;
		search += L"=";
		size_t pos = 0;
		while (pos + search.size() <= m_options.levels.size()) {
			if (m_options.levels.substr(pos, search.size()) == search) {
				if (mode == 'g') {
					level = (unsigned)str_to_int(m_options.levels.substr(pos + search.size()));
					return true;
				}
				auto ndel = search.size();
				while (pos + ndel < m_options.levels.size()
						&& std::wcschr(L"0123456789", m_options.levels[pos + ndel]))
					++ndel;
				if (pos + ndel < m_options.levels.size() && m_options.levels[pos + ndel] == L';')
					++ndel;
				if (pos > 0 && pos + ndel >= m_options.levels.size() && m_options.levels[pos - 1] == L';') {
					--pos;
					++ndel;
				}
				m_options.levels.erase(pos, ndel);
				break;
			}
			pos = m_options.levels.find(L';', pos + 1);
			if (pos == std::wstring::npos)
				break;
			++pos;
		}
		if (mode == 'g')
			return false;
		m_options.levels =
				search + int_to_str((int)level) + (m_options.levels.empty() ? L"" : L";") + m_options.levels;
		return true;
	}

	void update_level_list()
	{
		unsigned level_sel = get_list_pos(level_ctrl_id);
		unsigned new_level_sel = -1, def_level_sel = -1;

		bool bRepack = get_check(other_repack_ctrl_id);

		for (unsigned i = 0; i < ARRAYSIZE(c_levels); ++i) {
			bool skip = c_levels[i].value == 0 && arc_type == c_bzip2;
			if (arc_type == c_tar && bRepack) {
				skip = c_levels[i].value == 0 && m_options.repack_arc_type == c_bzip2;
			} else
				skip = skip || (c_levels[i].value != 0 && (arc_type == c_wim || arc_type == c_tar));

			FarListGetItem flgi{};
			//      flgi.StructSize = sizeof(FarListGetItem);
			flgi.ItemIndex = i;
			CHECK(send_message(DM_LISTGETITEM, level_ctrl_id, &flgi));
			if ((skip && (flgi.Item.Flags & LIF_DISABLE) == 0)
					|| (!skip && (flgi.Item.Flags & LIF_DISABLE) != 0)) {
				FarListUpdate flu{};
				//        flu.StructSize = sizeof(FarListUpdate);
				flu.Index = i;
				flu.Item.Flags = skip ? LIF_DISABLE : 0;
				flu.Item.Text = Far::msg_ptr(c_levels[i].name_id);
				send_message(DM_LISTUPDATE, level_ctrl_id, &flu);
			}
			if (!skip && (def_level_sel == (unsigned)-1 || c_levels[i].value == 5))	   // normal or first
				def_level_sel = i;
			if (c_levels[i].value == level && !skip)	// if current level enabled
				new_level_sel = i;
		}
		if (new_level_sel == (unsigned)-1)
			new_level_sel = def_level_sel != (unsigned)-1 ? def_level_sel : level_sel;
		if (new_level_sel != level_sel)
			set_list_pos(level_ctrl_id, new_level_sel);
		level = c_levels[new_level_sel].value;
		arc_levels('s');
	}

	void update_method_list(bool bSetDefaultMedthod = false)
	{
		uint32_t method_sel = 0, def_method_sel = 0xFFFFFFFF;

		{
			FarListUpdate flu{};
			flu.Item.Flags = 0;
			flu.Index = 0;
			bool bStop = false;

			if (arc_type == c_wim) {
				flu.Item.Text = L"---";
				bStop = true;
			} else if (arc_type == c_bzip2) {
				flu.Item.Text = L"BWT";
				bStop = true;
			} else if (arc_type == c_gzip) {
				flu.Item.Text = L"Deflate";
				bStop = true;
			} else if (arc_type == c_xz) {
				flu.Item.Text = L"LZMA";
				bStop = true;
			}

			if (bStop) {
				send_message(DM_LISTUPDATE, method_ctrl_id, &flu);
				set_list_pos(method_ctrl_id, 0);
				return;
			}
		}

		if (bSetDefaultMedthod) {
			if (arc_type == c_7z)
				method_sel = 0;
			else if (arc_type == c_zip)
				method_sel = 0;
			else
				method_sel = 0;
		} else {
			method_sel = get_list_pos(method_ctrl_id);
		}

		uint32_t fff = 0xFFFFFFFF;
		const CompressionMethod *clist;
		uint32_t cn, ilbs;

		if (arc_type == c_tar) {
			clist = c_tar_methods;
			cn = ARRAYSIZE(c_tar_methods);
			ilbs = cn;
		} else {
			clist = c_methods;
			cn = ARRAYSIZE(c_methods);
			ilbs = cn + ArcAPI::Count7zCodecs();
		}

		if (arc_type == c_7z)
			fff = CMF_7ZIP;
		else if (arc_type == c_zip)
			fff = CMF_ZIP;

		FarListItem listitems[128];
		FarList farlist = {(int)ilbs, listitems};

		for (size_t i = 0; i < cn; i++) {
			bool bEnabled = clist[i].flags & fff;
			listitems[i].Flags = bEnabled ? 0 : LIF_DISABLE;

			if (bEnabled) {
				if (method_sel == i && def_method_sel != method_sel) {
					listitems[i].Flags |= LIF_SELECTED;
					def_method_sel = method_sel;
				} else if (def_method_sel == 0xFFFFFFFF) {
					def_method_sel = i;
				}
			}

			listitems[i].Text = clist[i].value;
			listitems[i].Reserved[0] = (DWORD)i;
		}

		if (arc_type != c_tar) {
			const auto &codecs = ArcAPI::codecs();

			static std::unordered_set<std::wstring> static_method_names;
			static bool smn_initialized = false;
			if (!smn_initialized) {
				for (size_t sm_i = 0; sm_i < ARRAYSIZE(c_methods); sm_i++) {
					static_method_names.insert(std::wstring(c_methods[sm_i].value));
				}
				smn_initialized = true;
			}

			int codec_items_added = 0;

			for (size_t codec_index = 0; codec_index < ArcAPI::Count7zCodecs(); codec_index++) {
				const std::wstring &codec_name = codecs[codec_index].Name;
				if (static_method_names.find(codec_name) != static_method_names.end()) {
					// fprintf(stderr, "SKIP duplicate codec: %ls\n", codec_name.c_str());
					continue;
				}

				size_t list_index = cn + codec_items_added;
				if (list_index >= ARRAYSIZE(listitems)) {
					fprintf(stderr, "ERROR: Too many items for FarListItem array! Skipping codec '%ls'.\n", codec_name.c_str());
					break;
				}

				listitems[list_index].Flags = 0;
				if (method_sel == cn + codec_index && def_method_sel != method_sel) {
					listitems[list_index].Flags |= LIF_SELECTED;
					def_method_sel = method_sel;
				} else if (def_method_sel == 0xFFFFFFFF) {
					def_method_sel = cn + codec_index;
				}

				listitems[list_index].Text = codec_name.c_str();
				listitems[list_index].Reserved[0] = (DWORD)(cn + codec_index);
				codec_items_added++;
			}

			farlist.ItemsNumber = cn + codec_items_added;
		}

		send_message(DM_LISTSET, method_ctrl_id, &farlist);

		if (def_method_sel != method_sel) {
			set_list_pos(method_ctrl_id, def_method_sel);
		}
	}

	void set_control_state(bool bSetDefaultMedthod = false)
	{
		DisableEvents de(*this);
		bool is_7z = arc_type == c_7z;
		bool is_zip = arc_type == c_zip;
		bool is_tar = arc_type == c_tar;

		update_level_list();
		update_method_list(bSetDefaultMedthod);

		bool is_compressed = get_list_pos(level_ctrl_id) != 0;
		for (int i = method_ctrl_id - 1; i <= method_ctrl_id; i++) {
			enable(i, ((is_7z || is_zip) && is_compressed) || is_tar);
		}
		enable(solid_ctrl_id, is_7z && is_compressed);

		enable(encrypt_ctrl_id, is_7z || is_zip);
		bool encrypt = get_check(encrypt_ctrl_id);
		for (int i = encrypt_ctrl_id + 1; i <= password_visible_ctrl_id; i++) {
			enable(i, encrypt && (is_7z || is_zip));
		}

		enable(encrypt_header_ctrl_id, is_7z && encrypt);
		bool show_password = get_check(show_password_ctrl_id);
		for (int i = password_ctrl_id - 1; i <= password_verify_ctrl_id; i++) {
			set_visible(i, !show_password);
		}
		for (int i = password_visible_ctrl_id - 1; i <= password_visible_ctrl_id; i++) {
			set_visible(i, show_password);
		}

		bool multithreading = get_check(multithreading_ctrl_id);
		enable(thread_num_ctrl_id, multithreading);

		if (new_arc) {
			change_extension();
			bool create_sfx = get_check(create_sfx_ctrl_id);
			bool enable_volumes = get_check(enable_volumes_ctrl_id);
			if (create_sfx && enable_volumes)
				enable_volumes = false;
			enable(create_sfx_ctrl_id, is_7z && !enable_volumes);
			for (int i = create_sfx_ctrl_id + 1; i <= sfx_options_ctrl_id; i++) {
				enable(i, is_7z && create_sfx && !enable_volumes);
			}
			enable(enable_volumes_ctrl_id, !is_7z || !create_sfx);
			for (int i = enable_volumes_ctrl_id + 1; i <= volume_size_ctrl_id; i++) {
				enable(i, enable_volumes && (!is_7z || !create_sfx));
			}
			bool other_format = get_check(other_formats_ctrl_id);
			enable(other_formats_ctrl_id + 1, other_format);

			bool bRepack_format = (is_tar && !repack_formats.empty());
			bool bRepack_check = get_check(other_repack_ctrl_id);
			enable(other_repack_ctrl_id, bRepack_format);
			enable(other_repack_ctrl_id + 1, bRepack_format && bRepack_check);

			unsigned profile_idx = get_list_pos(profile_ctrl_id);
			enable(delete_profile_ctrl_id, profile_idx != (unsigned)-1 && profile_idx < profiles.size());
		}

		unsigned pos = get_list_pos(symlinks_ctrl_id);
		bool bStoreSymlinks = (pos == 0);
		//		bool bStoreSymlinks = false;

		enable(symlinks_paths_ctrl_id, bStoreSymlinks);
		enable(symlinks_paths_ctrl_id + 1, bStoreSymlinks);

		bool bUseExport = get_check(use_export_settings_ctrl_id);
		enable(exp_options_ctrl_id, bUseExport);

		enable(move_files_ctrl_id, !get_check(enable_filter_ctrl_id));
	}

	std::wstring eval_arc_path()
	{
		std::wstring arc_path = expand_env_vars(
				search_and_replace(expand_macros(strip(get_text(arc_path_ctrl_id))), L"\"", std::wstring()));

		if (arc_path.empty() || arc_path.back() == L'/')
			arc_path += default_arc_name + get_default_ext();

		if (get_check(append_ext_ctrl_id)) {
			std::wstring ext = get_default_ext();
			if (ext.size() > arc_path.size()
					|| upcase(arc_path.substr(arc_path.size() - ext.size())) != upcase(ext))
				arc_path += ext;
		}

		return Far::get_absolute_path(arc_path);
	}

	void read_controls(UpdateOptions &options)
	{
		if (new_arc) {
			options.levels = m_options.levels;
			options.append_ext = get_check(append_ext_ctrl_id);

			for (unsigned i = 0; i < main_formats.size(); i++) {
				if (get_check(main_formats_ctrl_id + i)) {
					options.arc_type = c_archive_types[i].value;
					break;
				}
			}
			if (!other_formats.empty() && get_check(other_formats_ctrl_id)) {
				options.arc_type = other_formats[get_list_pos(other_formats_ctrl_id + 1)];
			}
			if (options.arc_type.empty()) {
				FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_ARC_TYPE));
			}
		} else {
			options.arc_type = arc_type;
		}
		bool is_7z = options.arc_type == c_7z;
		bool is_zip = options.arc_type == c_zip;
		bool is_tar = options.arc_type == c_tar;

		options.level = (unsigned)-1;
		unsigned level_sel = get_list_pos(level_ctrl_id);
		if (level_sel < ARRAYSIZE(c_levels))
			options.level = c_levels[level_sel].value;
		if (options.level == (unsigned)-1) {
			FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_LEVEL));
		}
		bool is_compressed = options.level != 0;

		if (is_tar) {
			options.method.clear();
			unsigned method_sel = get_list_pos(method_ctrl_id);
			if (method_sel < ARRAYSIZE(c_tar_methods))
				options.method = c_tar_methods[method_sel].value;
			else
				options.method = c_tar_methods[0].value;
		} else if (is_compressed) {
			if (is_7z || is_zip) {
				options.method.clear();
				unsigned method_sel = get_list_pos(method_ctrl_id);
				const auto &codecs = ArcAPI::codecs();
				if (method_sel < ARRAYSIZE(c_methods) + codecs.size())
					options.method = method_sel < ARRAYSIZE(c_methods)
							? c_methods[method_sel].value
							: codecs[method_sel - ARRAYSIZE(c_methods)].Name;
				if (options.method.empty()) {
					FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_METHOD));
				}
			}
		} else
			options.method = c_method_copy;

		if (is_7z && is_compressed)
			options.solid = get_check(solid_ctrl_id);

		options.process_priority = get_list_pos(priority_ctrl_id);
		options.multithreading = get_check(multithreading_ctrl_id);
		if (options.multithreading) {
			options.threads_num = parse_size_string(get_text(thread_num_ctrl_id));
			if (!options.threads_num)
				options.threads_num = _BestThreadsCount;
		} else
			options.threads_num = 0;

		options.advanced = get_text(advanced_ctrl_id);

		if (is_7z || is_zip)
			options.encrypt = get_check(encrypt_ctrl_id);
		if (options.encrypt) {
			options.show_password = get_check(show_password_ctrl_id);
			if (options.show_password) {
				options.password = get_text(password_visible_ctrl_id);
			} else {
				options.password = get_text(password_ctrl_id);
				std::wstring password_verify = get_text(password_verify_ctrl_id);
				if (options.password != password_verify) {
					FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_PASSWORDS_DONT_MATCH));
				}
			}
			if (options.password.empty()) {
				FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_PASSWORD_IS_EMPTY));
			}
			if (is_7z)
				options.encrypt_header = get_check3(encrypt_header_ctrl_id);
		}

		if (new_arc) {
			options.create_sfx = is_7z && get_check(create_sfx_ctrl_id);
			if (options.create_sfx) {
				options.sfx_options = m_options.sfx_options;
				uintptr_t sfx_id = ArcAPI::sfx().find_by_name(options.sfx_options.name);
				if (sfx_id >= ArcAPI::sfx().size())
					FAIL_MSG(Far::get_msg(MSG_SFX_OPTIONS_DLG_WRONG_MODULE));
				if (options.method == c_method_ppmd && !ArcAPI::sfx()[sfx_id].all_codecs())
					FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_SFX_NO_PPMD));
				if (options.encrypt && !ArcAPI::sfx()[sfx_id].all_codecs())
					FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_SFX_NO_ENCRYPT));
			}

			options.enable_volumes = get_check(enable_volumes_ctrl_id);
			if (options.enable_volumes) {
				options.volume_size = get_text(volume_size_ctrl_id);
				if (parse_size_string(options.volume_size) < c_min_volume_size) {
					FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_VOLUME_SIZE));
				}
			}

			if (is_tar) {
				options.repack = get_check(other_repack_ctrl_id);

				if (!repack_formats.empty() && options.repack) {
					unsigned repack_arc_sel = get_list_pos(other_repack_ctrl_id + 1);
					if (repack_arc_sel < repack_formats.size())
						options.repack_arc_type = repack_formats[repack_arc_sel];
					else
						options.repack_arc_type = repack_formats[0];
				}
			}
		}

		options.move_files = get_check(enable_filter_ctrl_id) ? false : get_check(move_files_ctrl_id);
		options.open_shared = get_check(open_shared_ctrl_id);
		options.ignore_errors = get_check(ignore_errors_ctrl_id);

		if (!new_arc) {
			if (get_check(oa_ask_ctrl_id))
				options.overwrite = oaAsk;
			else if (get_check(oa_overwrite_ctrl_id))
				options.overwrite = oaOverwrite;
			else if (get_check(oa_skip_ctrl_id))
				options.overwrite = oaSkip;
			else
				options.overwrite = oaAsk;
		}

		{
			unsigned symlinks_sel = get_list_pos(symlinks_ctrl_id);

			if (symlinks_sel == 2) {
				options.skip_symlinks = true;
				options.dereference_symlinks = false;
			} else if (symlinks_sel == 1) {
				options.skip_symlinks = false;
				options.dereference_symlinks = true;
			} else {
				options.skip_symlinks = false;
				options.dereference_symlinks = false;
			}
		}

		{
			unsigned hardlinks_sel = get_list_pos(hardlinks_ctrl_id);

			if (hardlinks_sel == 2) {
				options.skip_hardlinks = true;
				options.duplicate_hardlinks = false;
			} else if (hardlinks_sel == 1) {
				options.skip_hardlinks = false;
				options.duplicate_hardlinks = true;
			} else {
				options.skip_hardlinks = false;
				options.duplicate_hardlinks = false;
			}
		}

		options.symlink_fix_path_mode = get_list_pos(symlinks_paths_ctrl_id + 1);
		options.use_export_settings = get_check(use_export_settings_ctrl_id);
		if (options.use_export_settings)
			options.export_options = m_options.export_options;
	}

	void write_controls(const ProfileOptions &options)
	{
		DisableEvents de(*this);
		if (new_arc) {
			arc_type = options.arc_type;
			for (unsigned i = 0; i < main_formats.size(); i++) {
				if (options.arc_type == main_formats[i]) {
					set_check(main_formats_ctrl_id + i);
					break;
				}
			}
			for (unsigned i = 0; i < other_formats.size(); i++) {
				if (options.arc_type == other_formats[i]) {
					set_check(other_formats_ctrl_id);
					set_list_pos(other_formats_ctrl_id + 1, i);
					break;
				}
			}
		}

		level = options.level;
		unsigned level_sel = 0;
		for (unsigned i = 0; i < ARRAYSIZE(c_levels); i++) {
			if (options.level == c_levels[i].value) {
				level_sel = i;
				break;
			}
		}
		set_list_pos(level_ctrl_id, level_sel);

		{
			unsigned symlinks_sel = 0;
			if (options.skip_symlinks)
				symlinks_sel = 2;
			else if (options.dereference_symlinks)
				symlinks_sel = 1;

			set_list_pos(symlinks_ctrl_id, symlinks_sel);
		}

		set_list_pos(symlinks_paths_ctrl_id + 1,
				options.symlink_fix_path_mode >= 3 ? options.symlink_fix_path_mode : 0);

		{
			unsigned hardlinks_sel = 0;
			if (options.skip_hardlinks)
				hardlinks_sel = 2;
			else if (options.duplicate_hardlinks)
				hardlinks_sel = 1;

			set_list_pos(hardlinks_ctrl_id, hardlinks_sel);
		}

		std::wstring method =
				options.method.empty() && options.arc_type == c_7z ? c_methods[0].value : options.method;
		unsigned method_sel = 0;
		const auto &codecs = ArcAPI::codecs();
		if (options.arc_type == c_tar) {
			for (unsigned i = 0; i < ARRAYSIZE(c_tar_methods); ++i) {
				std::wstring method_name = c_tar_methods[i].value;
				if (method == method_name) {
					method_sel = i;
					break;
				}
			}
		} else {
			for (unsigned i = 0; i < ARRAYSIZE(c_methods) + codecs.size(); ++i) {
				std::wstring method_name =
						i < ARRAYSIZE(c_methods) ? c_methods[i].value : codecs[i - ARRAYSIZE(c_methods)].Name;
				if (method == method_name) {
					method_sel = i;
					break;
				}
			}
		}

		set_list_pos(method_ctrl_id, method_sel);
		set_check(solid_ctrl_id, options.solid);

		set_list_pos(priority_ctrl_id, options.process_priority < 5 ? options.process_priority : 2);
		set_check(multithreading_ctrl_id, options.multithreading);

		{
			uint32_t n = options.threads_num > 0 ? options.threads_num : (uint32_t)_BestThreadsCount;
			wchar_t str[64];
			swprintf(str, 64, L"%u", n);
			set_text(thread_num_ctrl_id, str);
		}

		set_text(advanced_ctrl_id, options.advanced);

		set_check(encrypt_ctrl_id, options.encrypt);
		set_check3(encrypt_header_ctrl_id, options.encrypt_header);
		set_text(password_ctrl_id, options.password);
		set_text(password_verify_ctrl_id, options.password);
		set_text(password_visible_ctrl_id, options.password);

		if (new_arc) {
			set_check(create_sfx_ctrl_id, options.create_sfx);
			m_options.sfx_options = options.sfx_options;

			set_check(enable_volumes_ctrl_id, options.enable_volumes);
			set_text(volume_size_ctrl_id, options.volume_size);
		}

		set_check(move_files_ctrl_id, options.move_files);
		set_check(ignore_errors_ctrl_id, options.ignore_errors);

		set_check(use_export_settings_ctrl_id, options.use_export_settings);
	}

	void populate_profile_list()
	{
		DisableEvents de(*this);
		std::vector<FarListItem> fl_items;
		fl_items.reserve(profiles.size() + 1);
		FarListItem fl_item{};
		for (unsigned i = 0; i < profiles.size(); i++) {
			fl_item.Text = profiles[i].name.c_str();
			fl_items.push_back(fl_item);
		}
		fl_item.Text = L"";
		fl_items.push_back(fl_item);
		FarList fl;
		//    fl.StructSize = sizeof(FarList);
		fl.ItemsNumber = profiles.size() + 1;
		fl.Items = fl_items.data();
		send_message(DM_LISTSET, profile_ctrl_id, &fl);
	}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		switch (msg) {
			case DN_EDITCHANGE:
			case DN_BTNCLICK:
				if (new_arc && !repack_formats.empty()) {
					if (param1 == other_repack_ctrl_id) {
						m_options.repack = get_check(other_repack_ctrl_id);
						if (m_options.repack) {
						}
						arc_levels('g');
						set_control_state();
						break;
					} else if (param1 == other_repack_ctrl_id + 1) {
						m_options.repack = get_check(other_repack_ctrl_id);
						unsigned repack_arc_sel = get_list_pos(other_repack_ctrl_id + 1);
						if (repack_arc_sel < repack_formats.size())
							m_options.repack_arc_type = repack_formats[repack_arc_sel];
						else
							m_options.repack_arc_type = repack_formats[0];
						arc_levels('g');
						set_control_state();
						break;
					}
				}

				if (param1 == priority_ctrl_id) {
					m_options.process_priority = get_list_pos(priority_ctrl_id);
				} else if (param1 == multithreading_ctrl_id) {
					m_options.multithreading = get_check(multithreading_ctrl_id);

					if (m_options.multithreading) {
						m_options.threads_num = _BestThreadsCount;
					}
					{
						wchar_t str[64];
						swprintf(str, 64, L"%u", m_options.threads_num);
						set_text(thread_num_ctrl_id, str);
					}
					enable(thread_num_ctrl_id, m_options.multithreading);
				} else if (param1 == symlinks_ctrl_id || param1 == symlinks_paths_ctrl_id + 1) {
					set_control_state();
				} else if (param1 == use_export_settings_ctrl_id) {
					set_control_state();
				}

				break;
		}

		if (msg == DN_CLOSE && param1 >= 0 && param1 != cancel_ctrl_id) {
			read_controls(m_options);
			if (new_arc)
				m_options.arc_path = eval_arc_path();
		} else if (msg == DN_INITDIALOG) {
			set_control_state();
			set_focus(arc_path_ctrl_id);
		} else if (msg == DN_EDITCHANGE && param1 == profile_ctrl_id) {
			unsigned profile_idx = get_list_pos(profile_ctrl_id);
			if (profile_idx != (unsigned)-1 && profile_idx < profiles.size()) {
				write_controls(profiles[profile_idx].options);
				set_control_state();
			}
		} else if (new_arc && msg == DN_BTNCLICK && !main_formats.empty() && param1 >= main_formats_ctrl_id
				&& param1 < main_formats_ctrl_id + static_cast<int>(main_formats.size())) {

			if (param2) {
				ArcType prev_arc_type = arc_type;
				arc_type = main_formats[param1 - main_formats_ctrl_id];
				arc_levels('g');
#if 0
				if (arc_type != prev_arc_type) {
					if (prev_arc_type != c_7z && prev_arc_type != c_zip) {
						if (arc_type == c_7z) {
						}
						else if (arc_type == c_zip) {

						}
						else {
						}
					}
					else {
						unsigned method_sel = get_list_pos(method_ctrl_id);
						if (method_sel == 3 && arc_type == c_7z) {
						} else if (method_sel == 1 && arc_type == c_zip) {
						}
					}
				}
#endif
				set_control_state(true);
			}
		} else if (new_arc && msg == DN_BTNCLICK && !other_formats.empty()
				&& param1 == other_formats_ctrl_id) {
			if (param2) {
				arc_type = other_formats[get_list_pos(other_formats_ctrl_id + 1)];
				arc_levels('g');
				set_control_state();
			}
		} else if (new_arc && msg == DN_EDITCHANGE && !other_formats.empty()
				&& param1 == other_formats_ctrl_id + 1) {
			arc_type = other_formats[get_list_pos(other_formats_ctrl_id + 1)];
			arc_levels('g');
			set_control_state();
		} else if (msg == DN_EDITCHANGE && param1 == level_ctrl_id) {
			unsigned level_sel = get_list_pos(level_ctrl_id);
			if (level_sel < ARRAYSIZE(c_levels))
				level = c_levels[level_sel].value;
			set_control_state();
		} else if (msg == DN_BTNCLICK && param1 == encrypt_ctrl_id) {
			set_control_state();
		} else if (new_arc && msg == DN_BTNCLICK && param1 == create_sfx_ctrl_id) {
			set_control_state();
		} else if (new_arc && msg == DN_BTNCLICK && param1 == enable_volumes_ctrl_id) {
			set_control_state();
		} else if (msg == DN_BTNCLICK && param1 == show_password_ctrl_id) {
			set_control_state();
			if (!param2) {
				set_text(password_ctrl_id, get_text(password_visible_ctrl_id));
				set_text(password_verify_ctrl_id, get_text(password_visible_ctrl_id));
			} else {
				set_text(password_visible_ctrl_id, get_text(password_ctrl_id));
			}
		} else if (new_arc && msg == DN_BTNCLICK && param1 == save_profile_ctrl_id) {
			std::wstring name;
			UpdateOptions options;
			read_controls(options);
			unsigned profile_idx = profiles.find_by_options(options);
			if (profile_idx < profiles.size())
				name = profiles[profile_idx].name;

			if (Far::input_dlg(c_save_profile_dialog_guid, Far::get_msg(MSG_PLUGIN_NAME),
						Far::get_msg(MSG_UPDATE_DLG_INPUT_PROFILE_NAME), name)) {
				DisableEvents de(*this);
				profiles.update(name, options);
				profiles.save();
				populate_profile_list();
				set_list_pos(profile_ctrl_id, profiles.find_by_name(name));
				set_control_state();
			}

		} else if (new_arc && msg == DN_BTNCLICK && param1 == delete_profile_ctrl_id) {
			unsigned profile_idx = get_list_pos(profile_ctrl_id);
			if (profile_idx != (unsigned)-1 && profile_idx < profiles.size()) {
				if (Far::message(c_delete_profile_dialog_guid,
							Far::get_msg(MSG_PLUGIN_NAME) + L'\n'
									+ Far::get_msg(MSG_UPDATE_DLG_CONFIRM_PROFILE_DELETE),
							0, FMSG_MB_YESNO)
						== 0) {
					DisableEvents de(*this);
					profiles.erase(profiles.begin() + profile_idx);
					profiles.save();
					populate_profile_list();
					set_list_pos(profile_ctrl_id, static_cast<unsigned>(profiles.size()));
					set_control_state();
				}
			}
		} else if (new_arc && msg == DN_BTNCLICK && param1 == arc_path_eval_ctrl_id) {
			Far::info_dlg(c_arc_path_eval_dialog_guid, std::wstring(),
					word_wrap(eval_arc_path(), Far::get_optimal_msg_width()));
			set_focus(arc_path_ctrl_id);
		} else if (msg == DN_BTNCLICK && param1 == enable_filter_ctrl_id) {
			if (param2) {
				m_options.filter.reset(new Far::FileFilter());
				if (!m_options.filter->create(PANEL_NONE, FFT_CUSTOM) || !m_options.filter->menu()) {
					m_options.filter.reset();
					DisableEvents de(*this);
					set_check(enable_filter_ctrl_id, false);
				}
			} else
				m_options.filter.reset();
			set_control_state();
		} else if (msg == DN_BTNCLICK && param1 == sfx_options_ctrl_id) {
			sfx_options_dialog(m_options.sfx_options, profiles);
			set_control_state();

		} else if (msg == DN_BTNCLICK && param1 == exp_options_ctrl_id) {
			int mth = get_list_pos(method_ctrl_id);
			export_options_dialog(&m_options.export_options, profiles, arc_type, mth);
			set_control_state();

		} else if (msg == DN_BTNCLICK && param1 == save_params_ctrl_id) {
			UpdateOptions options;
			read_controls(options);
			if (new_arc) {
				g_options.update_arc_format_name = ArcAPI::formats().at(options.arc_type).name;
				g_options.update_arc_repack_format_name = ArcAPI::formats().at(options.repack_arc_type).name;
				g_options.update_repack = options.repack;
				g_options.update_create_sfx = options.create_sfx;
				g_options.update_sfx_options = options.sfx_options;
				g_options.update_enable_volumes = options.enable_volumes;
				g_options.update_volume_size = options.volume_size;
				g_options.update_encrypt_header = options.encrypt_header;
				g_options.update_password = options.password;
				g_options.update_append_ext = options.append_ext;
				g_options.update_move_files = options.move_files;

			} else {
				g_options.update_overwrite = options.overwrite;
			}

			g_options.update_export_options = options.export_options;
			g_options.update_use_export_settings = options.use_export_settings;
			g_options.update_level = options.level;
			g_options.update_levels = options.levels;
			g_options.update_method = options.method;
			g_options.update_multithreading = options.multithreading;
			g_options.update_process_priority = options.process_priority;
			g_options.update_threads_num = options.threads_num;
			g_options.update_solid = options.solid;
			g_options.update_advanced = options.advanced;
			g_options.update_encrypt = options.encrypt;
			g_options.update_show_password = options.show_password;
			g_options.update_ignore_errors = options.ignore_errors;
			g_options.update_skip_symlinks = options.skip_symlinks;
			g_options.update_symlink_fix_path_mode = options.symlink_fix_path_mode;
			g_options.update_dereference_symlinks = options.dereference_symlinks;
			g_options.update_skip_hardlinks = options.skip_hardlinks;
			g_options.update_duplicate_hardlinks = options.duplicate_hardlinks;

			g_options.save();
			Far::info_dlg(c_update_params_saved_dialog_guid, Far::get_msg(MSG_UPDATE_DLG_TITLE),
					Far::get_msg(MSG_UPDATE_DLG_PARAMS_SAVED));
			set_focus(ok_ctrl_id);
		}

		if (new_arc && (msg == DN_EDITCHANGE || msg == DN_BTNCLICK)) {

			unsigned profile_idx = static_cast<unsigned>(profiles.size());
			UpdateOptions options;
			bool valid_options = true;

			try {
				read_controls(options);
			} catch (const Error &) {
				valid_options = false;
			}

			if (valid_options) {
				for (unsigned i = 0; i < profiles.size(); i++) {
					if (options == profiles[i].options) {
						profile_idx = i;
						break;
					}
				}
			}

//			if (profiles.size() != profile_idx)
//				fprintf(stderr, "profile_idx found in = %u\n", profile_idx);
//			else
//				fprintf(stderr, "profile_idx not found\n");

			if (profile_idx != get_list_pos(profile_ctrl_id)) {
				DisableEvents de(*this);
				set_list_pos(profile_ctrl_id, profile_idx);
				set_control_state();
			}
		}

		return default_dialog_proc(msg, param1, param2);
	}

public:
	UpdateDialog(bool new_arc, bool multifile, UpdateOptions &options, UpdateProfiles &profiles)
		: Far::Dialog(Far::get_msg(new_arc ? MSG_UPDATE_DLG_TITLE_CREATE : MSG_UPDATE_DLG_TITLE),
				  &c_update_dialog_guid, c_client_xs, L"Update"),
		  new_arc(new_arc),
		  multifile(multifile),
		  default_arc_name(options.arc_path),
		  m_options(options),
		  profiles(profiles),
		  arc_type(options.arc_type),
		  level(options.level)
	{
		_BestThreadsCount = BestThreadsCount();
	}

	bool show()
	{
		if (new_arc) {
			if (ArcAPI::formats().count(m_options.arc_type))
				old_ext = ArcAPI::formats().at(m_options.arc_type).default_extension();

			std::vector<std::wstring> profile_names;
			profile_names.reserve(profiles.size() + 1);
			unsigned profile_idx = static_cast<unsigned>(profiles.size());
			std::for_each(profiles.begin(), profiles.end(), [&](const UpdateProfile &profile) {
				profile_names.push_back(profile.name);
				if (profile.options == m_options)
					profile_idx = static_cast<unsigned>(profile_names.size()) - 1;
			});
			profile_names.emplace_back();
			label(Far::get_msg(MSG_UPDATE_DLG_PROFILE));
			profile_ctrl_id = combo_box(profile_names, profile_idx, 30, DIF_DROPDOWNLIST);
			spacer(1);
			save_profile_ctrl_id = button(Far::get_msg(MSG_UPDATE_DLG_SAVE_PROFILE), DIF_BTNNOCLOSE);
			spacer(1);
			delete_profile_ctrl_id = button(Far::get_msg(MSG_UPDATE_DLG_DELETE_PROFILE), DIF_BTNNOCLOSE);
			new_line();
			separator();
			new_line();

			const auto label_text = Far::get_msg(MSG_UPDATE_DLG_ARC_PATH);
			spacer(get_label_len(label_text, 0) + 1);
			arc_path_eval_ctrl_id = button(Far::get_msg(MSG_UPDATE_DLG_ARC_PATH_EVAL), DIF_BTNNOCLOSE);
			spacer(1);
			append_ext_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_APPEND_EXT), m_options.append_ext);
			reset_line();
			label(label_text);
			new_line();
			arc_path_ctrl_id = history_edit_box(m_options.arc_path + old_ext, L"arclite.arc_path",
					c_client_xs, DIF_EDITPATH);
			new_line();
			separator();
			new_line();

			label(Far::get_msg(MSG_UPDATE_DLG_ARC_TYPE));
			spacer(1);

			const ArcFormats &arc_formats = ArcAPI::formats();
			for (unsigned i = 0; i < ARRAYSIZE(c_archive_types); i++) {
				const auto arc_iter = arc_formats.find(c_archive_types[i].value);
				if (arc_iter != arc_formats.end() && arc_iter->second.updatable) {
					bool first = main_formats.size() == 0;
					if (!first)
						spacer(1);
					int ctrl_id = radio_button(Far::get_msg(c_archive_types[i].name_id),
							m_options.arc_type == c_archive_types[i].value, first ? DIF_GROUP : 0);
					if (first)
						main_formats_ctrl_id = ctrl_id;
					main_formats.push_back(c_archive_types[i].value);
				}
			}

			for (const auto &f : arc_formats) {
				ArcType key = f.first;
				ArcFormat value = f.second;

				if (value.updatable) {

					if (!multifile || !ArcAPI::is_single_file_format(key)) {
						if (!multifile && key == c_SWFc && !is_SWFu(m_options.arc_path))
							continue;
						if (std::find(main_formats.begin(), main_formats.end(), key) == main_formats.end()) {
							other_formats.push_back(key);
						}
					}
					if (key != c_tar && key != c_wim)
						repack_formats.push_back(key);
				}
			}

			std::sort(other_formats.begin(), other_formats.end(), [&](const ArcType &a, const ArcType &b) {
				const ArcFormat &a_format = arc_formats.at(a);
				const ArcFormat &b_format = arc_formats.at(b);
				if (a_format.lib_index != b_format.lib_index)
					return a_format.lib_index < b_format.lib_index;
				else
					return StrCmpI(a_format.name.c_str(), b_format.name.c_str()) < 0;
			});

			std::sort(repack_formats.begin(), repack_formats.end(), [&](const ArcType &a, const ArcType &b) {
				const ArcFormat &a_format = arc_formats.at(a);
				const ArcFormat &b_format = arc_formats.at(b);
				if (a_format.lib_index != b_format.lib_index)
					return a_format.lib_index < b_format.lib_index;
				else
					return StrCmpI(a_format.name.c_str(), b_format.name.c_str()) < 0;
			});

			std::vector<std::wstring> repack_format_names;
			std::vector<std::wstring> other_format_names;
			other_format_names.reserve(other_formats.size());
			unsigned other_format_index = 0;
			unsigned repack_format_index = 0;
			bool found = false;
			for (const auto &t : other_formats) {
				if (!found && t == m_options.arc_type) {
					other_format_index = static_cast<unsigned>(other_format_names.size());
					found = true;
				}
				other_format_names.push_back(arc_formats.at(t).name);
			}

			bool found2 = false;
			for (const auto &t : repack_formats) {
				if (!found2 && t == m_options.repack_arc_type) {
					repack_format_index = static_cast<unsigned>(repack_format_names.size());
					found2 = true;
				}
				repack_format_names.push_back(arc_formats.at(t).name);
			}

			if (!other_formats.empty()) {
				if (!main_formats.empty())
					spacer(1);
				other_formats_ctrl_id = radio_button(Far::get_msg(MSG_UPDATE_DLG_ARC_TYPE_OTHER), found);
				combo_box(other_format_names, other_format_index, AUTO_SIZE, DIF_DROPDOWNLIST);
			}

			if (!repack_formats.empty()) {
				label(L" + ");
				other_repack_ctrl_id = check_box(L"", m_options.repack);
				combo_box(repack_format_names, repack_format_index, AUTO_SIZE, DIF_DROPDOWNLIST);
			}

			new_line();
		}

		label(Far::get_msg(MSG_UPDATE_DLG_LEVEL));
		std::vector<std::wstring> level_names;
		unsigned level_sel = 0;
		for (unsigned i = 0; i < ARRAYSIZE(c_levels); i++) {
			level_names.emplace_back(Far::get_msg(c_levels[i].name_id));
			if (m_options.level == c_levels[i].value)
				level_sel = i;
		}
		level_ctrl_id = combo_box(level_names, level_sel, AUTO_SIZE, DIF_DROPDOWNLIST);
		spacer(2);

		label(Far::get_msg(MSG_UPDATE_DLG_METHOD));
		std::wstring method = m_options.method.empty() && m_options.arc_type == c_7z
				? c_methods[0].value
				: m_options.method;

		std::vector<std::wstring> method_names;
		unsigned method_sel = 0;

		if (m_options.arc_type == c_tar) {
			method_names.reserve(ARRAYSIZE(c_tar_methods));
			for (unsigned i = 0; i < ARRAYSIZE(c_tar_methods); ++i) {
				std::wstring method_name = c_tar_methods[i].value;
				if (method == method_name)
					method_sel = i;
				method_names.push_back(method_name);
			}
		} else {
			const unsigned sizeMethods = ARRAYSIZE(c_methods) + ArcAPI::Count7zCodecs();
			method_names.reserve(sizeMethods);
			const auto &codecs = ArcAPI::codecs();
			for (unsigned i = 0; i < sizeMethods; ++i) {
				std::wstring method_name =
						i < ARRAYSIZE(c_methods) ? c_methods[i].value : codecs[i - ARRAYSIZE(c_methods)].Name;
				if (method == method_name)
					method_sel = i;
				method_names.push_back(method_name);
			}
		}

		method_ctrl_id = combo_box(method_names, method_sel, AUTO_SIZE, DIF_DROPDOWNLIST);
		spacer(2);

		solid_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_SOLID), m_options.solid);
		new_line();

#if 1
		std::vector<std::wstring> prior_names = {Far::get_msg(MSG_UPDATE_DLG_PROCESS_PRIORITY_LOW),
				Far::get_msg(MSG_UPDATE_DLG_PROCESS_PRIORITY_BELOW_NORMAL),
				Far::get_msg(MSG_UPDATE_DLG_PROCESS_PRIORITY_NORMAL),
				Far::get_msg(MSG_UPDATE_DLG_PROCESS_PRIORITY_ABOVE_NORMAL),
				Far::get_msg(MSG_UPDATE_DLG_PROCESS_PRIORITY_HIGH)};
#else
		std::vector<std::wstring> prior_names = {L"low", L"below normal", L"normal", L"above normal",
				L"high"};
#endif
		label(Far::get_msg(MSG_UPDATE_DLG_PROCESS_PRIORITY));
		priority_ctrl_id = combo_box(prior_names,
				m_options.process_priority < 5 ? m_options.process_priority : 0, AUTO_SIZE, DIF_DROPDOWNLIST);

		spacer(2);
		multithreading_ctrl_id =
				check_box(Far::get_msg(MSG_UPDATE_DLG_MULTITHREADING), m_options.multithreading);
		spacer(2);

		/// BestThreadsCount()

		label(L"N:");
		{
			uint32_t n = m_options.threads_num > 0 ? m_options.threads_num : (uint32_t)BestThreadsCount();
			wchar_t str[64];
			swprintf(str, 64, L"%u", n);
			thread_num_ctrl_id = history_edit_box(str, L"arclite.thread_num", 6);
		}

		new_line();

		label(Far::get_msg(MSG_UPDATE_DLG_ADVANCED));
		advanced_ctrl_id = history_edit_box(m_options.advanced, L"arclite.advanced");
		new_line();

		separator();
		new_line();

		encrypt_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_ENCRYPT), m_options.encrypt);
		spacer(2);
		encrypt_header_ctrl_id =
				check_box3(Far::get_msg(MSG_UPDATE_DLG_ENCRYPT_HEADER), m_options.encrypt_header);
		spacer(2);
		show_password_ctrl_id =
				check_box(Far::get_msg(MSG_UPDATE_DLG_SHOW_PASSWORD), m_options.show_password);
		new_line();
		label(Far::get_msg(MSG_UPDATE_DLG_PASSWORD));
		password_ctrl_id = pwd_edit_box(m_options.password, 20);
		spacer(2);
		label(Far::get_msg(MSG_UPDATE_DLG_PASSWORD2));
		password_verify_ctrl_id = pwd_edit_box(m_options.password, 20);
		reset_line();
		label(Far::get_msg(MSG_UPDATE_DLG_PASSWORD));
		password_visible_ctrl_id = edit_box(m_options.password, 20);
		new_line();
		separator();
		new_line();

		if (new_arc) {
			create_sfx_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_CREATE_SFX), m_options.create_sfx);
			spacer(2);
			sfx_options_ctrl_id = button(Far::get_msg(MSG_UPDATE_DLG_SFX_OPTIONS), DIF_BTNNOCLOSE);
			new_line();
			separator();
			new_line();

			enable_volumes_ctrl_id =
					check_box(Far::get_msg(MSG_UPDATE_DLG_ENABLE_VOLUMES), m_options.enable_volumes);
			spacer(2);
			label(Far::get_msg(MSG_UPDATE_DLG_VOLUME_SIZE));
			volume_size_ctrl_id = history_edit_box(m_options.volume_size, L"arclite.volume_size", 20);
			new_line();
			separator();
			new_line();
		}

		if (!new_arc) {
			label(Far::get_msg(MSG_UPDATE_DLG_OA));
			new_line();
			spacer(2);
			oa_ask_ctrl_id = radio_button(Far::get_msg(MSG_UPDATE_DLG_OA_ASK), m_options.overwrite == oaAsk);
			spacer(2);
			oa_overwrite_ctrl_id = radio_button(Far::get_msg(MSG_UPDATE_DLG_OA_OVERWRITE),
					m_options.overwrite == oaOverwrite || m_options.overwrite == oaOverwriteCase);
			spacer(2);
			oa_skip_ctrl_id =
					radio_button(Far::get_msg(MSG_UPDATE_DLG_OA_SKIP), m_options.overwrite == oaSkip);
			new_line();
			separator();
			new_line();
		}

		label(Far::get_msg(MSG_UPDATE_DLG_SYMLINKS));
		{
			std::vector<std::wstring> symlink_actions = {Far::get_msg(MSG_UPDATE_DLG_SYMCOMBO_ARCHIVE_SYMLINKS),
					Far::get_msg(MSG_UPDATE_DLG_SYMCOMBO_ARCHIVE_FILES),
					Far::get_msg(MSG_UPDATE_DLG_SYMCOMBO_SKIP_SYMLINKS)};

			unsigned symlinks_sel = 0;
			if (m_options.skip_symlinks)
				symlinks_sel = 2;
			else if (m_options.dereference_symlinks)
				symlinks_sel = 1;

			symlinks_ctrl_id = combo_box(symlink_actions, symlinks_sel, AUTO_SIZE, DIF_DROPDOWNLIST);
		}

		spacer(2);

		symlinks_paths_ctrl_id = label(Far::get_msg(MSG_UPDATE_DLG_SYMLINK_PATHS));
		{
			std::vector<std::wstring> symlink_paths = {Far::get_msg(MSG_UPDATE_DLG_SYMLINK_PATHS_AS_IS),
					Far::get_msg(MSG_UPDATE_DLG_SYMLINK_PATHS_ABSOLUTE),
					Far::get_msg(MSG_UPDATE_DLG_SYMLINK_PATHS_RELATIVE)};

			if (m_options.symlink_fix_path_mode >= 3)
				m_options.symlink_fix_path_mode = 0;

			combo_box(symlink_paths, m_options.symlink_fix_path_mode, AUTO_SIZE, DIF_DROPDOWNLIST);
		}

		new_line();
		label(Far::get_msg(MSG_UPDATE_DLG_HARDLINKS));
		{
			std::vector<std::wstring> hardlinks_actions = {Far::get_msg(MSG_UPDATE_DLG_HARDCOMBO_ARCHIVE_HARDLINKS),
					Far::get_msg(MSG_UPDATE_DLG_HARDCOMBO_SEP_FILES),
					Far::get_msg(MSG_UPDATE_DLG_HARDCOMBO_SKIP_HARDLINKS)};

			unsigned hardlinks_sel = 0;
			if (m_options.skip_hardlinks)
				hardlinks_sel = 2;
			else if (m_options.duplicate_hardlinks)
				hardlinks_sel = 1;

			hardlinks_ctrl_id = combo_box(hardlinks_actions, hardlinks_sel, AUTO_SIZE, DIF_DROPDOWNLIST);
		}
		spacer(2);

		use_export_settings_ctrl_id =
				check_box(Far::get_msg(MSG_UPDATE_DLG_USE_EXPORT), m_options.use_export_settings);

		new_line();
		separator();
		new_line();

		move_files_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_MOVE_FILES), m_options.move_files);
		spacer(2);
		ignore_errors_ctrl_id =
				check_box(Far::get_msg(MSG_UPDATE_DLG_IGNORE_ERRORS), m_options.ignore_errors);
		new_line();
		open_shared_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_OPEN_SHARED), m_options.open_shared);
		spacer(2);
		enable_filter_ctrl_id =
				check_box(Far::get_msg(MSG_UPDATE_DLG_ENABLE_FILTER), m_options.filter != nullptr);
		new_line();

		separator();
		new_line();
		ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
		cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
		save_params_ctrl_id =
				button(Far::get_msg(MSG_UPDATE_DLG_SAVE_PARAMS), DIF_CENTERGROUP | DIF_BTNNOCLOSE);
		exp_options_ctrl_id = button(Far::get_msg(MSG_UPDATE_DLG_EXPORT_SETTINGS),
				m_options.use_export_settings
						? (DIF_BTNNOCLOSE | DIF_CENTERGROUP)
						: (DIF_BTNNOCLOSE | DIF_DISABLE | DIF_CENTERGROUP));

		new_line();

		intptr_t item = Far::Dialog::show();
		return (item != -1) && (item != cancel_ctrl_id);
	}
};

bool update_dialog(bool new_arc, bool multifile, UpdateOptions &options, UpdateProfiles &profiles)
{
	return UpdateDialog(new_arc, multifile, options, profiles).show();
}

class MultiSelectDialog : public Far::Dialog
{
private:
	bool read_only{};
	std::wstring items_str;
	std::wstring &selected_str;

	std::vector<std::wstring> m_items;
	int first_item_ctrl_id{};

	int ok_ctrl_id{};
	int cancel_ctrl_id{};

	std::vector<size_t> estimate_column_widths(const std::vector<std::wstring> &dialog_items)
	{
		SMALL_RECT console_rect;
		double window_ratio;

		if (Far::adv_control(ACTL_GETFARRECT, 0, &console_rect)) {
			window_ratio = static_cast<double>(console_rect.Right - console_rect.Left + 1)
					/ (console_rect.Bottom - console_rect.Top + 1);
		} else {
			window_ratio = 80 / 25;
		}
		double window_ratio_diff = std::numeric_limits<double>::max();
		std::vector<size_t> prev_col_widths;
		for (unsigned num_cols = 1; num_cols <= dialog_items.size(); ++num_cols) {
			std::vector<size_t> col_widths(num_cols, 0);
			for (size_t i = 0; i < dialog_items.size(); ++i) {
				size_t col_index = i % num_cols;
				if (col_widths[col_index] < dialog_items[i].size())
					col_widths[col_index] = dialog_items[i].size();
			}
			size_t width = accumulate(col_widths.cbegin(), col_widths.cend(), size_t(0));
			width += num_cols * 4 + (num_cols - 1);
			size_t height = dialog_items.size() / num_cols + (dialog_items.size() % num_cols ? 1 : 0);
			double ratio = static_cast<double>(width) / static_cast<double>(height);
			double diff = fabs(ratio - window_ratio);
			if (diff > window_ratio_diff)
				break;
			window_ratio_diff = diff;
			prev_col_widths = std::move(col_widths);
		}
		return prev_col_widths;
	}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if (!read_only && (msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
			selected_str.clear();
			for (unsigned i = 0; i < m_items.size(); ++i) {
				if (get_check(first_item_ctrl_id + i)) {
					if (!selected_str.empty())
						selected_str += L',';
					selected_str += m_items[i];
				}
			}
		} else if (read_only && msg == DN_CTLCOLORDLGITEM) {
			FarDialogItem dlg_item;
			if (send_message(DM_GETDLGITEMSHORT, param1, &dlg_item) && dlg_item.Type == DI_CHECKBOX) {

				UInt64 color;
				if (Far::get_color(COL_DIALOGTEXT, color)) {
					FarListColors *item_colors = static_cast<FarListColors *>(param2);
					CHECK(item_colors->ColorCount == 4);
					item_colors->Colors[0] = color;
				}
			}
		}
		return default_dialog_proc(msg, param1, param2);
	}

public:
	MultiSelectDialog(const std::wstring &title, const std::wstring &items_str, std::wstring &selected_str)
		: Far::Dialog(title, &c_multi_select_dialog_guid, 1), items_str(items_str), selected_str(selected_str)
	{}

	bool show()
	{
		struct ItemCompare
		{
			bool operator()(const std::wstring &a, const std::wstring &b) const
			{
				return upcase(a) < upcase(b);
			}
		};

		std::set<std::wstring, ItemCompare> selected_items;
		if (!read_only) {
			std::list<std::wstring> split_selected_str = split(selected_str, L',');
			selected_items.insert(split_selected_str.cbegin(), split_selected_str.cend());
		}

		std::list<std::wstring> split_items_str = split(items_str, L',');
		m_items.assign(split_items_str.cbegin(), split_items_str.cend());
		std::sort(m_items.begin(), m_items.end(), ItemCompare());
		if (m_items.empty())
			return false;
		std::vector<size_t> col_widths = estimate_column_widths(m_items);
		first_item_ctrl_id = -1;
		for (unsigned i = 0; i < m_items.size(); ++i) {
			unsigned col_index = i % col_widths.size();
			unsigned ctrl_id = check_box(m_items[i], read_only ? true : selected_items.count(m_items[i]) != 0,
					read_only ? DIF_DISABLE : 0);
			if (first_item_ctrl_id == -1)
				first_item_ctrl_id = ctrl_id;
			if (col_index != col_widths.size() - 1) {
				spacer(col_widths[col_index] - m_items[i].size() + 1);
			} else {
				new_line();
			}
		}
		if (m_items.size() % col_widths.size())
			new_line();

		if (read_only) {
			Far::Dialog::show();
			return true;
		} else {
			separator();
			new_line();
			ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
			cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
			new_line();

			intptr_t item = Far::Dialog::show();
			return (item != -1) && (item != cancel_ctrl_id);
		}
	}
};

class FormatLibraryInfoDialog : public Far::Dialog
{
private:
	std::map<intptr_t, size_t> format_btn_map;
	std::map<intptr_t, size_t> mask_btn_map;

	std::wstring get_masks(size_t lib_index)
	{
		std::wstring masks;
		for (auto format_iter = ArcAPI::formats().cbegin(); format_iter != ArcAPI::formats().cend();
				++format_iter) {
			const ArcFormat &format = format_iter->second;
			if ((size_t)format.lib_index == lib_index) {
				std::for_each(format.extension_list.cbegin(), format.extension_list.cend(),
						[&](const std::wstring &ext) {
							masks += L"*" + ext + L",";
						});
			}
		}
		if (!masks.empty())
			masks.erase(masks.size() - 1);
		return masks;
	}

	std::wstring get_formats(size_t lib_index)
	{
		std::wstring formats;
		for (auto format_iter = ArcAPI::formats().cbegin(); format_iter != ArcAPI::formats().cend();
				++format_iter) {
			const ArcFormat &format = format_iter->second;
			if ((size_t)format.lib_index == lib_index) {
				if (!formats.empty())
					formats += L',';
				formats += format.name;
			}
		}
		return formats;
	}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if (msg == DN_INITDIALOG) {
			FarDialogItem dlg_item;
			for (unsigned ctrl_id = 0; send_message(DM_GETDLGITEMSHORT, ctrl_id, &dlg_item); ctrl_id++) {
				if (dlg_item.Type == DI_EDIT) {
					EditorSetPosition esp = {sizeof(EditorSetPosition)};
					send_message(DM_SETEDITPOSITION, ctrl_id, &esp);
				}
			}
		} else if (msg == DN_CTLCOLORDLGITEM) {
			FarDialogItem dlg_item;
			if (send_message(DM_GETDLGITEMSHORT, param1, &dlg_item) && dlg_item.Type == DI_EDIT) {
				UInt64 color;
				if (Far::get_color(COL_DIALOGTEXT, color)) {
					uint64_t *ItemColor = reinterpret_cast<uint64_t *>(param2);
					ItemColor[0] = color;
					ItemColor[2] = color;
				}
			}
		} else if (msg == DN_BTNCLICK) {
			std::wstring unused;
			auto lib_iter = format_btn_map.find(param1);
			if (lib_iter != format_btn_map.end()) {
				MultiSelectDialog(Far::get_msg(MSG_SETTINGS_DLG_LIB_FORMATS), get_formats(lib_iter->second),
						unused)
						.show();
			} else {
				lib_iter = mask_btn_map.find(param1);
				if (lib_iter != mask_btn_map.end()) {
					MultiSelectDialog(Far::get_msg(MSG_SETTINGS_DLG_LIB_MASKS), get_masks(lib_iter->second),
							unused)
							.show();
				}
			}
		}
		return default_dialog_proc(msg, param1, param2);
	}

public:
	FormatLibraryInfoDialog()
		: Far::Dialog(Far::get_msg(MSG_SETTINGS_DLG_LIB_INFO), &c_format_library_info_dialog_guid)
	{}

	void show()
	{
		const ArcLibs &libs = ArcAPI::libs();
		if (libs.empty()) {
			label(Far::get_msg(MSG_SETTINGS_DLG_LIB_NOT_FOUND));
			new_line();
		} else {
			size_t width = 0;
			for (size_t lib_index = 0; lib_index < libs.size(); ++lib_index) {
				if (width < libs[lib_index].module_path.size())
					width = libs[lib_index].module_path.size();
			}
			width += 1;
			if (width > Far::get_optimal_msg_width())
				width = Far::get_optimal_msg_width();
			for (size_t lib_index = 0; lib_index < libs.size(); ++lib_index) {
				edit_box(libs[lib_index].module_path, width, DIF_READONLY);
				new_line();
				label(Far::get_msg(MSG_SETTINGS_DLG_LIB_VERSION) + L' '
						+ int_to_str(HIWORD(libs[lib_index].version >> 32)) + L'.'
						+ int_to_str(LOWORD(libs[lib_index].version >> 32)) + L'.'
						+ int_to_str(HIWORD(libs[lib_index].version & 0xFFFFFFFF)) + L'.'
						+ int_to_str(LOWORD(libs[lib_index].version & 0xFFFFFFFF)));
				spacer(1);
				format_btn_map[button(Far::get_msg(MSG_SETTINGS_DLG_LIB_FORMATS), DIF_BTNNOCLOSE)] =
						lib_index;
				spacer(1);
				mask_btn_map[button(Far::get_msg(MSG_SETTINGS_DLG_LIB_MASKS), DIF_BTNNOCLOSE)] = lib_index;
				new_line();
			}
		}

		Far::Dialog::show();
	}
};

static size_t llen(const std::wstring &label, int add)
{
	size_t len = label.length() + add;
	if (label.find(L'&') != std::wstring::npos)
		--len;
	return len;
}

class SettingsDialog : public Far::Dialog
{
private:
	enum
	{
		c_client_xs = 60
	};

	PluginSettings &settings;

	int plugin_enabled_ctrl_id{};
	int handle_create_ctrl_id{};
	int handle_commands_ctrl_id{};
	int own_panel_view_mode_ctrl_id{};
	int oemCP_ctrl_id{};
	int ansiCP_ctrl_id{};
	int oemCPl_ctrl_id{};
	int ansiCPl_ctrl_id{};
	int patchCPs_ctrl_id{};
	int include_masks_ctrl_id{};
	int use_include_masks_ctrl_id{};
	int edit_include_masks_ctrl_id{};
	int include_masks_label_ctrl_id{};
	int use_exclude_masks_ctrl_id{};
	int edit_exclude_masks_ctrl_id{};
	int pgdn_masks_ctrl_id{};
	int exclude_masks_ctrl_id{};
	int exclude_masks_label_ctrl_id{};
	int generate_masks_ctrl_id{};
	int default_masks_ctrl_id{};
	int use_enabled_formats_ctrl_id{};
	int edit_enabled_formats_ctrl_id{};
	int enabled_formats_label_ctrl_id{};
	int enabled_formats_ctrl_id{};
	int use_disabled_formats_ctrl_id{};
	int edit_disabled_formats_ctrl_id{};
	int disabled_formats_ctrl_id{};
	int disabled_formats_label_ctrl_id{};
	int pgdn_formats_ctrl_id{};

	int lib_info_ctrl_id{};
	int ok_ctrl_id{};
	int cancel_ctrl_id{};
	int reload_ctrl_id{};
	int edit_path_ctrl_id{};

	bool _7zlib_status = false;

	void set_control_state(void)
	{
		bool bPluginEnabled = get_check(plugin_enabled_ctrl_id);
		DisableEvents de(*this);
		enable(handle_create_ctrl_id, bPluginEnabled);
		enable(handle_commands_ctrl_id, bPluginEnabled);
		enable(own_panel_view_mode_ctrl_id, bPluginEnabled);
		enable(oemCP_ctrl_id, bPluginEnabled);
		enable(ansiCP_ctrl_id, bPluginEnabled);
		enable(patchCPs_ctrl_id, bPluginEnabled);
		enable(use_include_masks_ctrl_id, bPluginEnabled);
		enable(use_exclude_masks_ctrl_id, bPluginEnabled);
		enable(pgdn_masks_ctrl_id, bPluginEnabled);
		enable(generate_masks_ctrl_id, bPluginEnabled);
		enable(default_masks_ctrl_id, bPluginEnabled);
		enable(use_enabled_formats_ctrl_id, bPluginEnabled);
		enable(use_disabled_formats_ctrl_id, bPluginEnabled);
		enable(pgdn_formats_ctrl_id, bPluginEnabled);
		enable(oemCPl_ctrl_id, bPluginEnabled);
		enable(ansiCPl_ctrl_id, bPluginEnabled);
		enable(include_masks_label_ctrl_id, bPluginEnabled);
		enable(exclude_masks_label_ctrl_id, bPluginEnabled);
		enable(enabled_formats_label_ctrl_id , bPluginEnabled);
		enable(disabled_formats_label_ctrl_id , bPluginEnabled);
		enable(include_masks_ctrl_id, settings.use_include_masks && bPluginEnabled);
		enable(edit_include_masks_ctrl_id, settings.use_include_masks && !settings.include_masks.empty() && bPluginEnabled);
		enable(exclude_masks_ctrl_id, settings.use_exclude_masks && bPluginEnabled);
		enable(edit_exclude_masks_ctrl_id, settings.use_exclude_masks && !settings.exclude_masks.empty() && bPluginEnabled);
		enable(enabled_formats_ctrl_id, settings.use_enabled_formats && bPluginEnabled);
		enable(edit_enabled_formats_ctrl_id, settings.use_enabled_formats && bPluginEnabled);
		enable(disabled_formats_ctrl_id, settings.use_disabled_formats && bPluginEnabled);
		enable(edit_disabled_formats_ctrl_id, settings.use_disabled_formats && bPluginEnabled);
	}

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
			settings.plugin_enabled = get_check(plugin_enabled_ctrl_id);
			settings.handle_create = get_check(handle_create_ctrl_id);
			settings.handle_commands = get_check(handle_commands_ctrl_id);
			settings.own_panel_view_mode = get_check(own_panel_view_mode_ctrl_id);
			settings.oemCP = (UINT)str_to_uint(strip(get_text(oemCP_ctrl_id)));
			settings.ansiCP = (UINT)str_to_uint(strip(get_text(ansiCP_ctrl_id)));
			settings.patchCP = get_check(patchCPs_ctrl_id);
			settings.use_include_masks = get_check(use_include_masks_ctrl_id);
			settings.include_masks = get_text(include_masks_ctrl_id);
			settings.use_exclude_masks = get_check(use_exclude_masks_ctrl_id);
			settings.exclude_masks = get_text(exclude_masks_ctrl_id);
			settings.pgdn_masks = get_check(pgdn_masks_ctrl_id);
			settings.use_enabled_formats = get_check(use_enabled_formats_ctrl_id);
			settings.enabled_formats = get_text(enabled_formats_ctrl_id);
			settings.use_disabled_formats = get_check(use_disabled_formats_ctrl_id);
			settings.disabled_formats = get_text(disabled_formats_ctrl_id);
			settings.pgdn_formats = get_check(pgdn_formats_ctrl_id);
			settings.preferred_7zip_path = add_trailing_slash(get_text(edit_path_ctrl_id));
		} else if (msg == DN_INITDIALOG) {
			set_control_state();
		} else if (msg == DN_BTNCLICK && param1 == plugin_enabled_ctrl_id) {
			set_control_state();
		} else if (msg == DN_BTNCLICK && param1 == use_include_masks_ctrl_id) {
			enable(include_masks_ctrl_id, param2 != nullptr);
			enable(edit_include_masks_ctrl_id, param2 != nullptr);
		} else if (msg == DN_BTNCLICK && param1 == edit_include_masks_ctrl_id) {
			std::wstring include_masks = get_text(include_masks_ctrl_id);
			if (MultiSelectDialog(Far::get_msg(MSG_SETTINGS_DLG_USE_INCLUDE_MASKS), include_masks,
						include_masks)
							.show()) {
				set_text(include_masks_ctrl_id, include_masks);
				set_focus(include_masks_ctrl_id);
			}
		} else if (msg == DN_EDITCHANGE && param1 == include_masks_ctrl_id) {
			enable(edit_include_masks_ctrl_id,
					get_check(use_include_masks_ctrl_id) && !get_text(include_masks_ctrl_id).empty());
		} else if (msg == DN_BTNCLICK && param1 == use_exclude_masks_ctrl_id) {
			enable(exclude_masks_ctrl_id, param2 != nullptr);
			enable(edit_exclude_masks_ctrl_id, param2 != nullptr);
		} else if (msg == DN_BTNCLICK && param1 == edit_exclude_masks_ctrl_id) {
			std::wstring exclude_masks = get_text(exclude_masks_ctrl_id);
			if (MultiSelectDialog(Far::get_msg(MSG_SETTINGS_DLG_USE_EXCLUDE_MASKS), exclude_masks,
						exclude_masks)
							.show()) {
				set_text(exclude_masks_ctrl_id, exclude_masks);
				set_focus(exclude_masks_ctrl_id);
			}
		} else if (msg == DN_EDITCHANGE && param1 == exclude_masks_ctrl_id) {
			enable(edit_exclude_masks_ctrl_id,
					get_check(use_exclude_masks_ctrl_id) && !get_text(exclude_masks_ctrl_id).empty());
		} else if (msg == DN_BTNCLICK && param1 == generate_masks_ctrl_id) {
			generate_masks();
		} else if (msg == DN_BTNCLICK && param1 == default_masks_ctrl_id) {
			default_masks();
		} else if (msg == DN_BTNCLICK && param1 == use_enabled_formats_ctrl_id) {
			enable(enabled_formats_ctrl_id, param2 != nullptr);
			enable(edit_enabled_formats_ctrl_id, param2 != nullptr);
		} else if (msg == DN_BTNCLICK && param1 == edit_enabled_formats_ctrl_id) {
			std::wstring enabled_formats = get_text(enabled_formats_ctrl_id);
			if (MultiSelectDialog(Far::get_msg(MSG_SETTINGS_DLG_USE_ENABLED_FORMATS), get_available_formats(),
						enabled_formats)
							.show()) {
				set_text(enabled_formats_ctrl_id, enabled_formats);
				set_focus(enabled_formats_ctrl_id);
			}
		} else if (msg == DN_BTNCLICK && param1 == use_disabled_formats_ctrl_id) {
			enable(disabled_formats_ctrl_id, param2 != nullptr);
			enable(edit_disabled_formats_ctrl_id, param2 != nullptr);
		} else if (msg == DN_BTNCLICK && param1 == edit_disabled_formats_ctrl_id) {
			std::wstring disabled_formats = get_text(disabled_formats_ctrl_id);
			if (MultiSelectDialog(Far::get_msg(MSG_SETTINGS_DLG_USE_DISABLED_FORMATS),
						get_available_formats(), disabled_formats)
							.show()) {
				set_text(disabled_formats_ctrl_id, disabled_formats);
				set_focus(disabled_formats_ctrl_id);
			}
		} else if (msg == DN_BTNCLICK && param1 == lib_info_ctrl_id) {
			FormatLibraryInfoDialog().show();
		}
		else if (msg == DN_BTNCLICK && param1 == reload_ctrl_id) {
			g_options.preferred_7zip_path = settings.preferred_7zip_path = add_trailing_slash(get_text(edit_path_ctrl_id));
			ArcAPI::reload();
			if (g_options.patchCP) {
				Patch7zCP::SetCP(static_cast<UINT>(g_options.oemCP), static_cast<UINT>(g_options.ansiCP), true);
			}
			_7zlib_status = !ArcAPI::libs().empty();
		}
		else if (msg == DN_CTLCOLORDLGITEM && param1 == lib_info_ctrl_id) {
			FarDialogItem dlg_item;
			if (send_message(DM_GETDLGITEMSHORT, param1, &dlg_item)) {
				UInt64 color[4];
				if (!_7zlib_status) {
					Far::get_color(COL_WARNDIALOGDEFAULTBUTTON, color[0]);
					Far::get_color(COL_WARNDIALOGSELECTEDDEFAULTBUTTON, color[1]);
					Far::get_color(COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON, color[2]);
					Far::get_color(COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON, color[3]);
				}
				else {
					Far::get_color(COL_DIALOGDEFAULTBUTTON, color[0]);
					Far::get_color(COL_DIALOGSELECTEDDEFAULTBUTTON, color[1]);
					Far::get_color(COL_DIALOGHIGHLIGHTDEFAULTBUTTON, color[2]);
					Far::get_color(COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON, color[3]);
				}

				uint64_t *ItemColor = reinterpret_cast<uint64_t *>(param2);
				if (dlg_item.Focus) {
					ItemColor[0] = color[1];
					ItemColor[1] = color[3];
				}
				else {
					ItemColor[0] = color[0];
					ItemColor[1] = color[2];
				}
			}

		}
		return default_dialog_proc(msg, param1, param2);
	}

	void generate_masks()
	{
		const ArcFormats &arc_formats = ArcAPI::formats();
		std::wstring masks;
		std::for_each(arc_formats.begin(), arc_formats.end(),
				[&](const std::pair<const ArcType, ArcFormat> &arc_type_format) {
					std::for_each(arc_type_format.second.extension_list.cbegin(),
							arc_type_format.second.extension_list.cend(), [&](const std::wstring &ext) {
								masks += L"*" + ext + L",";
							});
				});
		if (!masks.empty())
			masks.erase(masks.size() - 1);
		set_text(include_masks_ctrl_id, masks);
	}

	void default_masks() { set_text(include_masks_ctrl_id, Options().include_masks); }

	std::wstring get_available_formats()
	{
		const ArcFormats &arc_formats = ArcAPI::formats();
		std::vector<std::wstring> format_list;
		format_list.reserve(arc_formats.size());
		std::for_each(arc_formats.begin(), arc_formats.end(),
				[&](const std::pair<const ArcType, ArcFormat> &arc_type_format) {
					format_list.push_back(arc_type_format.second.name);
				});
		std::sort(format_list.begin(), format_list.end(),
				[](const std::wstring &a, const std::wstring &b) -> bool {
					return upcase(a) < upcase(b);
				});
		std::wstring formats;
		std::for_each(format_list.begin(), format_list.end(), [&](const std::wstring &format_name) {
			if (!formats.empty())
				formats += L',';
			formats += format_name;
		});
		return formats;
	}

public:
	SettingsDialog(PluginSettings &settings)
		: Far::Dialog(Far::get_msg(MSG_PLUGIN_NAME), &c_settings_dialog_guid, c_client_xs, L"Config"),
		  settings(settings)
	{
		_7zlib_status = !ArcAPI::libs().empty();
	}

	bool show()
	{
		std::wstring box0 = Far::get_msg(MSG_SETTINGS_DLG_PLUGIN_ENABLED);
		std::wstring box1 = Far::get_msg(MSG_SETTINGS_DLG_HANDLE_CREATE);
		std::wstring box2 = Far::get_msg(MSG_SETTINGS_DLG_HANDLE_COMMANDS);
		std::wstring box3 = Far::get_msg(MSG_SETTINGS_DLG_OWN_PANEL_VIEW_MODE);
		plugin_enabled_ctrl_id = check_box(box0, settings.plugin_enabled);
		new_line();
		handle_create_ctrl_id = check_box(box1, settings.handle_create);
		new_line();
		handle_commands_ctrl_id = check_box(box2, settings.handle_commands);
		new_line();
		own_panel_view_mode_ctrl_id = check_box(box3, settings.own_panel_view_mode);
		new_line();
		separator();
		new_line();
		std::wstring label1 = Far::get_msg(MSG_SETTINGS_DLG_OEM_CODEPAGE);
		std::wstring label2 = Far::get_msg(MSG_SETTINGS_DLG_ANSI_CODEPAGE);
		std::wstring label3 = Far::get_msg(MSG_SETTINGS_DLG_PATCH_CODEPAGE);
		oemCPl_ctrl_id = label(label1);
		std::wstring tmp1 = settings.oemCP ? uint_to_str(settings.oemCP) : std::wstring();
		oemCP_ctrl_id = edit_box(tmp1, 5);
		auto total = llen(label1, 5) + llen(label2, 5) + llen(label3, 4);
		auto width =
				std::max(std::max(std::max(std::max(std::max(llen(box0, 4), llen(box1, 4)), llen(box2, 4)), llen(box3, 4)), total + 2),
				size_t(c_client_xs) );
		auto space = (width - total) / 2;
		pad(llen(label1, 5) + space);
		ansiCPl_ctrl_id = label(label2);
		std::wstring tmp2 = settings.ansiCP ? uint_to_str(settings.ansiCP) : std::wstring();
		ansiCP_ctrl_id = edit_box(tmp2, 5);

		pad(width - llen(label3, 4));
		patchCPs_ctrl_id = check_box(label3, settings.patchCP);
		new_line();
		separator();
		new_line();
		include_masks_label_ctrl_id = label(Far::get_msg(MSG_SETTINGS_DLG_USE_INCLUDE_MASKS));
		spacer(1);
		use_include_masks_ctrl_id =
				check_box(Far::get_msg(MSG_SETTINGS_DLG_ACTIVE), settings.use_include_masks);
		spacer(1);
		edit_include_masks_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_EDIT), DIF_BTNNOCLOSE);
		new_line();
		include_masks_ctrl_id = edit_box(settings.include_masks, width);
		new_line();
		exclude_masks_label_ctrl_id = label(Far::get_msg(MSG_SETTINGS_DLG_USE_EXCLUDE_MASKS));
		spacer(1);
		use_exclude_masks_ctrl_id =
				check_box(Far::get_msg(MSG_SETTINGS_DLG_ACTIVE), settings.use_exclude_masks);
		spacer(1);
		edit_exclude_masks_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_EDIT), DIF_BTNNOCLOSE);
		new_line();
		exclude_masks_ctrl_id = edit_box(settings.exclude_masks, width);
		new_line();
		pgdn_masks_ctrl_id = check_box(Far::get_msg(MSG_SETTINGS_DLG_PGDN_MASKS), settings.pgdn_masks);
		new_line();
		generate_masks_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_GENERATE_MASKS), DIF_BTNNOCLOSE);
		spacer(1);
		default_masks_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_DEFAULT_MASKS), DIF_BTNNOCLOSE);
		new_line();
		separator();
		new_line();

		enabled_formats_label_ctrl_id = label(Far::get_msg(MSG_SETTINGS_DLG_USE_ENABLED_FORMATS));
		spacer(1);
		use_enabled_formats_ctrl_id =
				check_box(Far::get_msg(MSG_SETTINGS_DLG_ACTIVE), settings.use_enabled_formats);
		spacer(1);
		edit_enabled_formats_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_EDIT), DIF_BTNNOCLOSE);
		new_line();
		enabled_formats_ctrl_id = edit_box(settings.enabled_formats, width);
		new_line();
		disabled_formats_label_ctrl_id = label(Far::get_msg(MSG_SETTINGS_DLG_USE_DISABLED_FORMATS));
		spacer(1);
		use_disabled_formats_ctrl_id =
				check_box(Far::get_msg(MSG_SETTINGS_DLG_ACTIVE), settings.use_disabled_formats);
		spacer(1);
		edit_disabled_formats_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_EDIT), DIF_BTNNOCLOSE);
		new_line();
		disabled_formats_ctrl_id = edit_box(settings.disabled_formats, width);
		new_line();
		pgdn_formats_ctrl_id = check_box(Far::get_msg(MSG_SETTINGS_DLG_PGDN_FORMATS), settings.pgdn_formats);
		new_line();
		separator();
		new_line();

		label(Far::get_msg(MSG_SETTINGS_DLG_7Z_PATH));
		new_line();
		edit_path_ctrl_id = edit_box(settings.preferred_7zip_path, width);
		new_line();

		lib_info_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_LIB_INFO), DIF_BTNNOCLOSE);
		spacer(2);

		{
			bool bEnableReload = false;
			PanelInfo panel_info1;
			PanelInfo panel_info2;
			if (Far::get_panel_info(PANEL_ACTIVE, panel_info1) &&
				Far::get_panel_info(PANEL_PASSIVE, panel_info2)) {
				if (Far::is_real_file_panel(panel_info1) && Far::is_real_file_panel(panel_info2) )
					bEnableReload = true;
			}
			reload_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_RELOAD), bEnableReload ? DIF_BTNNOCLOSE : (DIF_BTNNOCLOSE | DIF_DISABLE) );
		}

		new_line();
		separator();
		new_line();

		ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
		cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
		new_line();

		intptr_t item = Far::Dialog::show();

		return (item != -1) && (item != cancel_ctrl_id);
	}
};

bool settings_dialog(PluginSettings &settings)
{
	return SettingsDialog(settings).show();
}

class AttrDialog : public Far::Dialog
{
private:
	const AttrList &attr_list;

	intptr_t dialog_proc(intptr_t msg, intptr_t param1, void *param2) override
	{
		if (msg == DN_INITDIALOG) {
			FarDialogItem dlg_item;
			for (unsigned ctrl_id = 0; send_message(DM_GETDLGITEMSHORT, ctrl_id, &dlg_item); ctrl_id++) {
				if (dlg_item.Type == DI_EDIT) {
					EditorSetPosition esp = {sizeof(EditorSetPosition)};
					send_message(DM_SETEDITPOSITION, ctrl_id, &esp);
				}
			}
		} else if (msg == DN_CTLCOLORDLGITEM) {
			FarDialogItem dlg_item;
			if (send_message(DM_GETDLGITEMSHORT, param1, &dlg_item) && dlg_item.Type == DI_EDIT) {
				UInt64 color;
				if (Far::get_color(COL_DIALOGTEXT, color)) {
					uint64_t *ItemColor = reinterpret_cast<uint64_t *>(param2);
					ItemColor[0] = color;
					ItemColor[2] = color;
				}
			}
		}
		return default_dialog_proc(msg, param1, param2);
	}

public:
	AttrDialog(const AttrList &attr_list)
		: Far::Dialog(Far::get_msg(MSG_ATTR_DLG_TITLE), &c_attr_dialog_guid), attr_list(attr_list)
	{}

	void show()
	{
		unsigned max_name_len = 0;
		unsigned max_value_len = 0;
		std::for_each(attr_list.begin(), attr_list.end(), [&](const Attr &attr) {
			if (attr.name.size() > max_name_len)
				max_name_len = static_cast<unsigned>(attr.name.size());
			if (attr.value.size() > max_value_len)
				max_value_len = static_cast<unsigned>(attr.value.size());
		});
		max_value_len += 1;

		unsigned max_width = Far::get_optimal_msg_width();
		if (max_name_len > max_width / 2)
			max_name_len = max_width / 2;
		if (max_name_len + 1 + max_value_len > max_width)
			max_value_len = max_width - max_name_len - 1;

		set_width(max_name_len + 1 + max_value_len);

		std::for_each(attr_list.begin(), attr_list.end(), [&](const Attr &attr) {
			label(attr.name, max_name_len);
			spacer(1);
			edit_box(attr.value, max_value_len, DIF_READONLY);
			new_line();
		});

		Far::Dialog::show();
	}
};

void attr_dialog(const AttrList &attr_list)
{
	AttrDialog(attr_list).show();
}
