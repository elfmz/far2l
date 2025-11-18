#include "headers.hpp"

#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "guids.hpp"
#include "msg.hpp"

namespace Far
{

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void init(const PluginStartupInfo *psi)
{
	g_far = *psi;
	g_fsf = *psi->FSF;
}

extern std::wstring g_plugin_path;

std::wstring get_plugin_module_path()
{
	return search_and_replace(extract_file_path(g_far.ModuleName), L"\\", L"/");
}

const wchar_t *msg_ptr(int id)
{
	return g_far.GetMsg(g_far.ModuleNumber, id);
}

std::wstring get_msg(int id)
{
	return g_far.GetMsg(g_far.ModuleNumber, id);
}

unsigned get_optimal_msg_width()
{
	SMALL_RECT console_rect;
	if (adv_control(ACTL_GETFARRECT, 0, &console_rect)) {
		unsigned con_width = console_rect.Right - console_rect.Left + 1;
		if (con_width >= 80)
			return con_width - 20;
	}
	return 60;
}

intptr_t message(const GUID &id, const std::wstring &msg, int button_cnt, FARMESSAGEFLAGS flags)
{
	return g_far.Message(g_far.ModuleNumber, flags | FMSG_ALLINONE, {},
			reinterpret_cast<const wchar_t *const *>(msg.c_str()), 0, button_cnt);
}

unsigned MenuItems::add(const std::wstring &item)
{
	push_back(item);
	return static_cast<unsigned>(size() - 1);
}

intptr_t menu(const GUID &id, const std::wstring &title, const MenuItems &items, const wchar_t *help)
{
	std::vector<FarMenuItem> menu_items;
	menu_items.reserve(items.size());
	FarMenuItem mi;
	for (unsigned i = 0; i < items.size(); i++) {
		mi = {};
		mi.Text = items[i].c_str();
		mi.Selected = 0;
		mi.Checked = 0;
		mi.Separator = 0;
		menu_items.push_back(mi);
	}

	static const int BreakKeys[] = {VK_ESCAPE, 0};
	//  intptr_t BreakCode = (intptr_t)-1;
	int BreakCode = -1;

	auto ret = g_far.Menu(g_far.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, title.c_str(), nullptr, help,
			BreakKeys, &BreakCode, menu_items.data(), static_cast<int>(menu_items.size()));

	if (BreakCode >= 0)
		ret = (intptr_t)-1;

	return ret;
}

#if 0
struct FarMenuItem
{
	const wchar_t *Text;
	int  Selected;
	int  Checked;
	int  Separator;
};

enum MENUITEMFLAGS
{
	MIF_NONE   = 0,
	MIF_SELECTED   = 0x00010000UL,
	MIF_CHECKED    = 0x00020000UL,
	MIF_SEPARATOR  = 0x00040000UL,
	MIF_DISABLE    = 0x00080000UL,
	MIF_GRAYED     = 0x00100000UL,
	MIF_HIDDEN     = 0x00200000UL,
#ifdef FAR_USE_INTERNALS
	MIF_SUBMENU    = 0x00400000UL,
#endif // END FAR_USE_INTERNALS
};

struct FarMenuItemEx
{
	DWORD Flags;
	const wchar_t *Text;
	DWORD AccelKey;
	DWORD Reserved;
	DWORD_PTR UserData;
};

enum FARMENUFLAGS
{
	FMENU_SHOWAMPERSAND        = 0x00000001,
	FMENU_WRAPMODE             = 0x00000002,
	FMENU_AUTOHIGHLIGHT        = 0x00000004,
	FMENU_REVERSEAUTOHIGHLIGHT = 0x00000008,
#ifdef FAR_USE_INTERNALS
	FMENU_SHOWNOBOX            = 0x00000010,
#endif // END FAR_USE_INTERNALS
	FMENU_USEEXT               = 0x00000020,
	FMENU_CHANGECONSOLETITLE   = 0x00000040,
};

typedef int (WINAPI *FARAPIMENU)(
	INT_PTR             PluginNumber,
	int                 X,
	int                 Y,
	int                 MaxHeight,
	DWORD               Flags,
	const wchar_t      *Title,
	const wchar_t      *Bottom,
	const wchar_t      *HelpTopic,
	const int          *BreakKeys,
	int                *BreakCode,
	const struct FarMenuItem *Item,
	int                 ItemsNumber
);

#endif

std::wstring get_progress_bar_str(unsigned width, UInt64 completed, UInt64 total)
{
	constexpr wchar_t c_pb_black = 9608;	// '\x2588' █
	constexpr wchar_t c_pb_white = 9617;	// '\x2591' ░
	unsigned len1;
	if (total == 0)
		len1 = 0;
	else
		len1 = static_cast<unsigned>(
				static_cast<double>(completed) * static_cast<double>(width) / static_cast<double>(total));
	if (len1 > width)
		len1 = width;
	unsigned len2 = width - len1;
	std::wstring result;
	result.append(len1, c_pb_black);
	result.append(len2, c_pb_white);
	return result;
}

/***

typedef uint32_t PROGRESSTATE;

static const PROGRESSTATE
	PGS_NOPROGRESS   =0x0,
	PGS_INDETERMINATE=0x1,
	PGS_NORMAL       =0x2,
	PGS_ERROR        =0x4,
	PGS_PAUSED       =0x8;


enum TASKBARPROGRESSTATE
{
	TBPS_NOPROGRESS   =0x0,
	TBPS_INDETERMINATE=0x1,
	TBPS_NORMAL       =0x2,
	TBPS_ERROR        =0x4,
	TBPS_PAUSED       =0x8,
};

**/

void set_progress_state(PROGRESSTATE state)
{
	///  g_far.AdvControl(g_far.ModuleNumber, ACTL_SETPROGRESSSTATE, (void *)(intptr_t)state, nullptr);
}

void set_progress_value(UInt64 completed, UInt64 total)
{
///	PROGRESSVALUE pv;
	//  pv.StructSize = sizeof(ProgressValue);
	///  pv.Completed = completed;
	///  pv.Total = total;
	///  g_far.AdvControl(g_far.ModuleNumber, ACTL_SETPROGRESSVALUE, &pv, 0 );
}

void progress_notify()
{
	///  g_far.AdvControl(g_far.ModuleNumber, ACTL_PROGRESSNOTIFY, 0, nullptr);
}

void call_user_apc(void *param)
{
	g_far.AdvControl(g_far.ModuleNumber, ACTL_SYNCHRO, param, 0);
}

bool post_macro(const std::wstring &macro)
{
#if 0
  MacroSendMacroText mcmd = { sizeof(MacroSendMacroText), KMFLAGS_ENABLEOUTPUT };
  mcmd.SequenceText = macro.c_str();
  return g_far.MacroControl({}, MCTL_SENDSTRING, MSSC_POST, &mcmd) != 0;
#endif
	return false;
}

void quit()
{
	g_far.AdvControl(g_far.ModuleNumber, ACTL_QUIT, 0, nullptr);
}

HANDLE save_screen()
{
	return g_far.SaveScreen(0, 0, -1, -1);
}

void restore_screen(HANDLE h_scr)
{
	g_far.RestoreScreen(h_scr);
}

void flush_screen()
{
	g_far.Text(0, 0, {}, {});	 // flush buffer hack
	g_far.AdvControl(g_far.ModuleNumber, ACTL_REDRAWALL, 0, nullptr);
}

intptr_t viewer(const std::wstring &file_name, const std::wstring &title, uint32_t flags)
{
	return g_far.Viewer(file_name.c_str(), title.c_str(), 0, 0, -1, -1, flags, 0);
}

intptr_t editor(const std::wstring &file_name, const std::wstring &title, uint32_t flags)
{
	return g_far.Editor(file_name.c_str(), title.c_str(), 0, 0, -1, -1, flags, 1, 1, 0);
}

void update_panel(HANDLE h_panel, const bool keep_selection, const bool reset_pos)
{
	g_far.Control(h_panel, FCTL_UPDATEPANEL, keep_selection ? 1 : 0, 0);
	PanelRedrawInfo ri = {0, 0};
	g_far.Control(h_panel, FCTL_REDRAWPANEL, 0, reset_pos ? (LONG_PTR)&ri : 0);
}

void set_view_mode(HANDLE h_panel, unsigned view_mode)
{
	g_far.Control(h_panel, FCTL_SETVIEWMODE, view_mode, 0);
}

void set_sort_mode(HANDLE h_panel, unsigned sort_mode)
{
	g_far.Control(h_panel, FCTL_SETSORTMODE, sort_mode, 0);
}

void set_reverse_sort(HANDLE h_panel, bool reverse_sort)
{
	g_far.Control(h_panel, FCTL_SETSORTORDER, reverse_sort ? 1 : 0, 0);
}

void set_directories_first(HANDLE h_panel, bool first)
{
	g_far.Control(h_panel, FCTL_SETDIRECTORIESFIRST, first ? 1 : 0, 0);
}

bool get_panel_info(HANDLE h_panel, PanelInfo &panel_info)
{
	return g_far.Control(h_panel, FCTL_GETPANELINFO, 0, (LONG_PTR)&panel_info) == TRUE;
}

bool is_real_file_panel(const PanelInfo &panel_info)
{
	return panel_info.PanelType == PTYPE_FILEPANEL && (panel_info.Flags & PFLAGS_REALNAMES);
}

std::wstring get_panel_dir(HANDLE h_panel)
{
	size_t buf_size = 1024;
	wchar_t buff[1024];

	size_t size = g_far.Control(h_panel, FCTL_GETPANELDIR, buf_size, (LONG_PTR)buff);
	if (size > buf_size) {
		return std::wstring(L"", 0);
	}
	return std::wstring(buff, size - 1);
}

void get_panel_item(HANDLE h_panel, uint32_t command, size_t index, Buffer<unsigned char> &buf)
{
	size_t size = g_far.Control(h_panel, command, index, (LONG_PTR)buf.data());

	if (size > buf.size()) {
		buf.resize(size);
		size = g_far.Control(h_panel, command, index, (LONG_PTR)buf.data());
		CHECK(size == buf.size());
	}
}

static PanelItem get_panel_item(HANDLE h_panel, FILE_CONTROL_COMMANDS command, size_t index)
{
	Buffer<unsigned char> buf(0x1000);
	get_panel_item(h_panel, command, index, buf);
	const PluginPanelItem *panel_item = reinterpret_cast<const PluginPanelItem *>(buf.data());
	PanelItem pi;
	pi.file_attributes = panel_item->FindData.dwFileAttributes;
	pi.creation_time = panel_item->FindData.ftCreationTime;
	pi.last_access_time = panel_item->FindData.ftLastAccessTime;
	pi.last_write_time = panel_item->FindData.ftLastWriteTime;
	pi.file_size = panel_item->FindData.nFileSize;
	pi.pack_size = panel_item->FindData.nPhysicalSize;
	pi.file_name = panel_item->FindData.lpwszFileName;
	//  if (panel_item->AlternateFileName)
	//    pi.alt_file_name = panel_item->AlternateFileName;
	pi.user_data = (void *)panel_item->UserData;
	return pi;
}

PanelItem get_current_panel_item(HANDLE h_panel)
{
	return get_panel_item(h_panel, FCTL_GETCURRENTPANELITEM, 0);
}

PanelItem get_panel_item(HANDLE h_panel, size_t index)
{
	return get_panel_item(h_panel, FCTL_GETPANELITEM, index);
}

PanelItem get_selected_panel_item(HANDLE h_panel, size_t index)
{
	return get_panel_item(h_panel, FCTL_GETSELECTEDPANELITEM, index);
}

void error_dlg(const std::wstring &title, const Error &e)
{
	std::wostringstream st;
	st << title << L'\n';

	bool show_file_line = false;
	if (e.code != E_MESSAGE && e.code != S_FALSE) {
		show_file_line = true;
		std::wstring sys_msg = get_system_message(e.code, get_lang_id());
		if (!sys_msg.empty())
			st << word_wrap(sys_msg, get_optimal_msg_width()) << L'\n';
	}

	for (const auto &s : e.objects)
		st << s << L'\n';

	if (!e.messages.empty() && (!e.objects.empty() || !e.warnings.empty()))
		st << Far::get_msg(MSG_KPID_ERRORFLAGS) << L":\n";
	for (const auto &s : e.messages)
		st << word_wrap(s, get_optimal_msg_width()) << L'\n';

	if (!e.warnings.empty())
		st << Far::get_msg(MSG_KPID_WARNINGFLAGS) << L":\n";
	for (const auto &s : e.warnings)
		st << word_wrap(s, get_optimal_msg_width()) << L'\n';

	if (show_file_line)
		st << extract_file_name(widen(e.file)) << L':' << e.line;

	message(c_error_dialog_guid, st.str(), 0, FMSG_WARNING | FMSG_MB_OK);
}

void info_dlg(const GUID &id, const std::wstring &title, const std::wstring &msg)
{
	message(id, title + L'\n' + msg, 0, FMSG_MB_OK);
}

bool input_dlg(const GUID &id, const std::wstring &title, const std::wstring &msg, std::wstring &text,
		INPUTBOXFLAGS flags)
{
	Buffer<wchar_t> buf(1024);

	if (g_far.InputBox(title.c_str(), msg.c_str(), nullptr, text.c_str(), buf.data(),
				static_cast<int>(buf.size()), nullptr, flags)) {
		text.assign(buf.data());
		return true;
	}
	return false;
}

size_t Dialog::get_label_len(const std::wstring &str, uint32_t flags)
{
	if (flags & DIF_SHOWAMPERSAND)
		return str.size();
	else {
		unsigned cnt = 0;
		for (unsigned i = 0; i < str.size(); i++) {
			if (str[i] != '&')
				cnt++;
		}
		return cnt;
	}
}

unsigned Dialog::new_value(const std::wstring &text)
{
	values.push_back(text);
	return static_cast<unsigned>(values.size());
}

const wchar_t *Dialog::get_value(unsigned idx) const
{
	return values[idx - 1].c_str();
}

void Dialog::frame(const std::wstring &text)
{
	if (text.size() > client_xs)
		client_xs = text.size();
	DialogItem di;
	di.type = DI_DOUBLEBOX;
	di.x1 = c_x_frame - 2;
	di.y1 = c_y_frame - 1;
	di.x2 = c_x_frame + client_xs + 1;
	di.y2 = c_y_frame + client_ys;
	di.text_idx = new_value(text);
	new_item(di);
}

void Dialog::calc_frame_size()
{
	client_ys = y - c_y_frame;
	DialogItem &di = items.front();	   // dialog frame
	di.x2 = c_x_frame + client_xs + 1;
	di.y2 = c_y_frame + client_ys;
}

unsigned Dialog::new_item(const DialogItem &di)
{
	items.push_back(di);
	return static_cast<unsigned>(items.size()) - 1;
}

intptr_t WINAPI Dialog::internal_dialog_proc(HANDLE h_dlg, int msg, int param1, void *param2)
{
	Dialog *dlg = reinterpret_cast<Dialog *>(g_far.SendDlgMessage(h_dlg, DM_GETDLGDATA, 0, {}));
	dlg->h_dlg = h_dlg;
	FAR_ERROR_HANDLER_BEGIN
	if (!dlg->events_enabled)
		return dlg->default_dialog_proc(msg, param1, param2);
	else
		return dlg->dialog_proc(msg, param1, param2);
	FAR_ERROR_HANDLER_END(return 0, return 0, false)
}

intptr_t Dialog::default_dialog_proc(intptr_t msg, intptr_t param1, void *param2)
{
	return g_far.DefDlgProc(h_dlg, msg, param1, (LONG_PTR)param2);
}

intptr_t Dialog::send_message(intptr_t msg, intptr_t param1, void *param2)
{
	return g_far.SendDlgMessage(h_dlg, msg, param1, (LONG_PTR)param2);
}

Dialog::Dialog(const std::wstring &title, const GUID *guid, unsigned width, const wchar_t *help,
		uint32_t flags)
	: client_xs(width),
	  x(c_x_frame),
	  y(c_y_frame),
	  help(help),
	  m_flags(flags),
	  guid(guid),
	  events_enabled(true)
{
	(void)this->guid;
	frame(title);
}

void Dialog::new_line()
{
	x = c_x_frame;
	++y;
}

void Dialog::reset_line()
{
	x = c_x_frame;
}

void Dialog::spacer(size_t size)
{
	x += size;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
}

void Dialog::pad(size_t pos)
{
	if (pos > x - c_x_frame)
		spacer(pos - (x - c_x_frame));
}

unsigned Dialog::separator()
{
	return separator(std::wstring());
}

unsigned Dialog::separator(const std::wstring &text)
{
	DialogItem di;
	di.type = DI_TEXT;
	di.y1 = y;
	di.y2 = y;
	di.flags = DIF_SEPARATOR;
	if (!text.empty()) {
		di.flags |= DIF_CENTERTEXT;
		di.text_idx = new_value(text);
	}
	return new_item(di);
}

unsigned Dialog::label(const std::wstring &text, size_t boxsize, uint32_t flags)
{
	DialogItem di;
	di.type = DI_TEXT;
	di.x1 = x;
	di.y1 = y;
	if ((intptr_t)boxsize == AUTO_SIZE)
		x += get_label_len(text, flags);
	else
		x += boxsize;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.x2 = x - 1;
	di.y2 = y;
	di.flags = flags;
	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned Dialog::edit_box(const std::wstring &text, size_t boxsize, uint32_t flags)
{
	DialogItem di;
	di.type = DI_EDIT;
	di.x1 = x;
	di.y1 = y;
	if ((intptr_t)boxsize == AUTO_SIZE)
		x = c_x_frame + client_xs;
	else
		x += boxsize;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.x2 = x - 1 - (flags & DIF_HISTORY ? 1 : 0);
	di.y2 = y;
	di.flags = flags;
	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned Dialog::history_edit_box(const std::wstring &text, const std::wstring &history_name, size_t boxsize,
		uint32_t flags)
{
	unsigned idx = edit_box(text, boxsize, flags | DIF_HISTORY);
	items[idx].history_idx = new_value(history_name);
	return idx;
}

unsigned Dialog::mask_edit_box(const std::wstring &text, const std::wstring &mask, size_t boxsize, uint32_t flags)
{
	unsigned idx = fix_edit_box(text, boxsize, flags | DIF_MASKEDIT);
	items[idx].mask_idx = new_value(mask);
	return idx;
}

unsigned Dialog::fix_edit_box(const std::wstring &text, size_t boxsize, uint32_t flags)
{
	DialogItem di;
	di.type = DI_FIXEDIT;
	di.x1 = x;
	di.y1 = y;
	if ((intptr_t)boxsize == AUTO_SIZE)
		x += static_cast<unsigned>(text.size());
	else
		x += boxsize;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.x2 = x - 1;
	di.y2 = y;
	di.flags = flags;
	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned Dialog::pwd_edit_box(const std::wstring &text, size_t boxsize, uint32_t flags)
{
	DialogItem di;
	di.type = DI_PSWEDIT;
	di.x1 = x;
	di.y1 = y;
	if ((intptr_t)boxsize == AUTO_SIZE)
		x = c_x_frame + client_xs;
	else
		x += boxsize;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.x2 = x - 1;
	di.y2 = y;
	di.flags = flags;
	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned Dialog::button(const std::wstring &text, uint32_t flags)
{
	DialogItem di;
	di.type = DI_BUTTON;
	di.x1 = x;
	di.y1 = y;
	x += get_label_len(text, flags) + 4;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.y2 = y;
	di.flags = flags;

	if (flags & DIF_DEFAULT)
		di.def_button = TRUE;

	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned Dialog::check_box(const std::wstring &text, int value, uint32_t flags)
{
	DialogItem di;
	di.type = DI_CHECKBOX;
	di.x1 = x;
	di.y1 = y;
	x += get_label_len(text, flags) + 4;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.y2 = y;
	di.flags = flags;
	di.selected = value;
	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned Dialog::radio_button(const std::wstring &text, bool value, uint32_t flags)
{
	DialogItem di;
	di.type = DI_RADIOBUTTON;
	di.x1 = x;
	di.y1 = y;
	x += get_label_len(text, flags) + 4;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.y2 = y;
	di.flags = flags;
	di.selected = value ? 1 : 0;
	di.text_idx = new_value(text);
	return new_item(di);
}

unsigned
Dialog::combo_box(const std::vector<std::wstring> &list_items, size_t sel_idx, size_t boxsize, uint32_t flags)
{
	DialogItem di;
	di.type = DI_COMBOBOX;
	di.x1 = x;
	di.y1 = y;
	if ((intptr_t)boxsize == AUTO_SIZE) {
		if (flags & DIF_DROPDOWNLIST) {
			unsigned max_len = 1;
			for (unsigned i = 0; i < list_items.size(); i++) {
				if (max_len < list_items[i].size())
					max_len = static_cast<unsigned>(list_items[i].size());
			}
			x += max_len + 5;
		} else
			x = c_x_frame + client_xs;
	} else
		x += boxsize;
	if (x - c_x_frame > client_xs)
		client_xs = x - c_x_frame;
	di.x2 = x - 1 - 1;	  // -1 for down arrow
	di.y2 = y;
	di.flags = flags;
	for (unsigned i = 0; i < list_items.size(); i++) {
		if (di.list_idx)
			new_value(list_items[i]);
		else
			di.list_idx = new_value(list_items[i]);
	}
	di.list_size = list_items.size();
	di.list_pos = sel_idx;
	return new_item(di);
}

intptr_t Dialog::show()
{
	calc_frame_size();

	unsigned list_cnt = 0;
	size_t list_item_cnt = 0;
	for (size_t i = 0; i < items.size(); i++) {
		if (items[i].list_idx) {
			list_cnt++;
			list_item_cnt += items[i].list_size;
		}
	}
	Buffer<FarList> far_lists(list_cnt);
	far_lists.clear();
	Buffer<FarListItem> far_list_items(list_item_cnt);
	far_list_items.clear();

	Buffer<FarDialogItem> dlg_items(items.size());
	dlg_items.clear();
	unsigned fl_idx = 0;
	unsigned fli_idx = 0;

	for (unsigned i = 0; i < items.size(); i++) {
		FarDialogItem *dlg_item = dlg_items.data() + i;

		dlg_item->Type = items[i].type;
		dlg_item->X1 = items[i].x1;
		dlg_item->Y1 = items[i].y1;
		dlg_item->X2 = items[i].x2;
		dlg_item->Y2 = items[i].y2;
		dlg_item->Flags = items[i].flags;
		dlg_item->Param.Selected = items[i].selected;
		dlg_item->DefaultButton = items[i].def_button;

		if (items[i].history_idx)
			dlg_item->Param.History = get_value(items[i].history_idx);

		if (items[i].mask_idx)
			dlg_item->Param.Mask = get_value(items[i].mask_idx);

		if (items[i].text_idx)
			dlg_item->PtrData = get_value(items[i].text_idx);

		if (items[i].list_idx) {
			FarList *fl = far_lists.data() + fl_idx;
			//      fl->StructSize = sizeof(FarList);
			fl->Items = far_list_items.data() + fli_idx;
			fl->ItemsNumber = items[i].list_size;
			for (unsigned j = 0; j < items[i].list_size; j++) {
				FarListItem *fli = far_list_items.data() + fli_idx;
				if (j == items[i].list_pos)
					fli->Flags = LIF_SELECTED;
				fli->Text = get_value(items[i].list_idx + j);
				fli_idx++;
			}
			fl_idx++;
			dlg_item->Param.ListItems = fl;
		}
	}

	intptr_t res = -1;
	HANDLE dlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, client_xs + 2 * c_x_frame,
			client_ys + 2 * c_y_frame, help, dlg_items.data(), static_cast<unsigned>(dlg_items.size()), 0,
			m_flags, (FARWINDOWPROC)internal_dialog_proc, (LONG_PTR)this);
	if (dlg != INVALID_HANDLE_VALUE) {
		res = g_far.DialogRun(dlg);
		g_far.DialogFree(dlg);
	}
	return res;
}

std::wstring Dialog::get_text(unsigned ctrl_id) const
{
	uint32_t textlength = g_far.SendDlgMessage(h_dlg, DM_GETTEXTLENGTH, ctrl_id, 0);

	Buffer<wchar_t> buf(textlength + 1);
	g_far.SendDlgMessage(h_dlg, DM_GETTEXTPTR, ctrl_id, (LONG_PTR)buf.data());
	return std::wstring(buf.data(), textlength);
}

size_t Dialog::get_text(unsigned ctrl_id, wchar_t *buf, size_t bufsize)
{
	if (!buf)
		return 0;
	size_t textlength = g_far.SendDlgMessage(h_dlg, DM_GETTEXTLENGTH, ctrl_id, 0);
	if (bufsize < textlength + 1 || !textlength) {
		*buf = 0;
		return 0;
	}

	g_far.SendDlgMessage(h_dlg, DM_GETTEXTPTR, ctrl_id, (LONG_PTR)buf);
	return textlength;
}

void Dialog::set_text(unsigned ctrl_id, const std::wstring &text)
{
	g_far.SendDlgMessage(h_dlg, DM_SETTEXTPTR, ctrl_id, (LONG_PTR)(text.c_str()));
}

void Dialog::set_text(unsigned ctrl_id, const wchar_t *text)
{
	g_far.SendDlgMessage(h_dlg, DM_SETTEXTPTR, ctrl_id, (LONG_PTR)text );
}

void Dialog::set_text_silent(unsigned ctrl_id, const std::wstring &text)
{
	g_far.SendDlgMessage(h_dlg, DM_SETTEXTPTRSILENT, ctrl_id, (LONG_PTR)(text.c_str()));
}

void Dialog::set_text_silent(unsigned ctrl_id, const wchar_t *text)
{
	g_far.SendDlgMessage(h_dlg, DM_SETTEXTPTRSILENT, ctrl_id, (LONG_PTR)text );
}

bool Dialog::get_check(unsigned ctrl_id) const
{
	return g_far.SendDlgMessage(h_dlg, DM_GETCHECK, ctrl_id, {}) == BSTATE_CHECKED;
}

void Dialog::set_check(unsigned ctrl_id, bool check)
{
	g_far.SendDlgMessage(h_dlg, DM_SETCHECK, ctrl_id, (LONG_PTR)(check ? BSTATE_CHECKED : BSTATE_UNCHECKED));
}

TriState Dialog::get_check3(unsigned ctrl_id) const
{
	INT_PTR value = g_far.SendDlgMessage(h_dlg, DM_GETCHECK, ctrl_id, {});
	return value == BSTATE_3STATE ? triUndef : value == BSTATE_CHECKED ? triTrue : triFalse;
}

void Dialog::set_check3(unsigned ctrl_id, TriState check)
{
	g_far.SendDlgMessage(h_dlg, DM_SETCHECK, ctrl_id,
			(LONG_PTR)(check == triUndef	   ? BSTATE_3STATE
							: check == triTrue ? BSTATE_CHECKED
											   : BSTATE_UNCHECKED));
}

unsigned Dialog::get_list_pos(unsigned ctrl_id) const
{
	return static_cast<unsigned>(g_far.SendDlgMessage(h_dlg, DM_LISTGETCURPOS, ctrl_id, {}));
}

void Dialog::set_list_pos(unsigned ctrl_id, uintptr_t pos)
{
	FarListPos list_pos;
	list_pos.SelectPos = pos;
	list_pos.TopPos = -1;
	g_far.SendDlgMessage(h_dlg, DM_LISTSETCURPOS, ctrl_id, (LONG_PTR)&list_pos);
}

void Dialog::set_focus(unsigned ctrl_id)
{
	g_far.SendDlgMessage(h_dlg, DM_SETFOCUS, ctrl_id, {});
}

void Dialog::enable(unsigned ctrl_id, bool enable)
{
	g_far.SendDlgMessage(h_dlg, DM_ENABLE, ctrl_id, (LONG_PTR)(static_cast<size_t>(enable ? TRUE : FALSE)));
}

void Dialog::set_visible(unsigned ctrl_id, bool visible)
{
	g_far.SendDlgMessage(h_dlg, DM_SHOWITEM, ctrl_id, (LONG_PTR)(static_cast<size_t>(visible ? 1 : 0)));
}

Regex::Regex() : h_regex(INVALID_HANDLE_VALUE)
{
	CHECK(g_far.RegExpControl({}, RECTL_CREATE, (LONG_PTR)&h_regex));
}

Regex::~Regex()
{
	if (h_regex != INVALID_HANDLE_VALUE)
		g_far.RegExpControl(h_regex, RECTL_FREE, 0);
}

size_t Regex::search(const std::wstring &expr, const std::wstring &text)
{
	CHECK(g_far.RegExpControl(h_regex, RECTL_COMPILE, (LONG_PTR)((L"/" + expr + L"/").c_str())));
	CHECK(g_far.RegExpControl(h_regex, RECTL_OPTIMIZE, 0));
	RegExpSearch regex_search{};
	regex_search.Text = text.c_str();
	regex_search.Position = 0;
	regex_search.Length = static_cast<int>(text.size());
	RegExpMatch regex_match;
	regex_search.Match = &regex_match;
	regex_search.Count = 1;
	if (g_far.RegExpControl(h_regex, RECTL_SEARCHEX, (LONG_PTR)&regex_search))
		return regex_search.Match[0].start;
	else
		return -1;
}

Selection::Selection(HANDLE h_plugin) : h_plugin(h_plugin)
{
	g_far.Control(h_plugin, FCTL_BEGINSELECTION, 0, 0);
}

Selection::~Selection()
{
	g_far.Control(h_plugin, FCTL_ENDSELECTION, 0, 0);
}

void Selection::select(unsigned idx, bool value)
{
	g_far.Control(h_plugin, FCTL_SETSELECTION, idx, (LONG_PTR)(static_cast<size_t>(value ? TRUE : FALSE)));
}

FileFilter::FileFilter() : h_filter(INVALID_HANDLE_VALUE) {}

FileFilter::~FileFilter()
{
	clean();
}

void FileFilter::clean()
{
	if (h_filter != INVALID_HANDLE_VALUE) {
		g_far.FileFilterControl(h_filter, FFCTL_FREEFILEFILTER, 0, {});
		h_filter = INVALID_HANDLE_VALUE;
	}
}

bool FileFilter::create(HANDLE h_panel, int type)
{
	clean();
	return g_far.FileFilterControl(h_panel, FFCTL_CREATEFILEFILTER, type, (LONG_PTR)&h_filter) != FALSE;
}

bool FileFilter::menu()
{
	return g_far.FileFilterControl(h_filter, FFCTL_OPENFILTERSMENU, 0, {}) != FALSE;
}

void FileFilter::start()
{
	g_far.FileFilterControl(h_filter, FFCTL_STARTINGTOFILTER, 0, {});
}

bool FileFilter::match(const PluginPanelItem &panel_item)
{
	return g_far.FileFilterControl(h_filter, FFCTL_ISFILEINFILTER, 0, (LONG_PTR)(&panel_item)) != FALSE;
}

std::wstring get_absolute_path(const std::wstring &rel_path)
{
	Buffer<wchar_t> buf(MAX_PATH);
	size_t len = g_fsf.ConvertPath(CPM_FULL, rel_path.c_str(), buf.data(), static_cast<int>(buf.size()));
	if (len > buf.size()) {
		buf.resize(len);
		len = g_fsf.ConvertPath(CPM_FULL, rel_path.c_str(), buf.data(), static_cast<int>(buf.size()));
	}
	return buf.data();
}

INT_PTR control(HANDLE h_panel, FILE_CONTROL_COMMANDS command, int param1, void *param2)
{
	return g_far.Control(h_panel, command, param1, (LONG_PTR)param2);
}

INT_PTR adv_control(ADVANCED_CONTROL_COMMANDS command, int param1, void *param2)
{
	return g_far.AdvControl(g_far.ModuleNumber, command, (void *)(size_t)param1, param2);
}

//INT_PTR adv_control_async(ADVANCED_CONTROL_COMMANDS command, int param1, void *param2)
//{
//	return g_far.AdvControlAsync(g_far.ModuleNumber, command, (void *)(size_t)param1, param2);
//}

bool match_masks(const std::wstring &file_name, const std::wstring &masks)
{
	return g_fsf.ProcessName(masks.c_str(), const_cast<wchar_t *>(file_name.c_str()), 0, PN_CMPNAMELIST) != 0;
}

bool get_color(PaletteColors color_id, UInt64 &color)
{
	return g_far.AdvControl(g_far.ModuleNumber, ACTL_GETCOLOR, (void *)(size_t)color_id, &color) != 0;
}

void panel_go_to_part(HANDLE h_panel, const int pidx)
{
	//	PanelRedrawInfo panel_ri = { sizeof(PanelRedrawInfo) };
	PanelRedrawInfo panel_ri = {0, 0};
	if (pidx >= 0) {
		g_far.Control(h_panel, FCTL_UPDATEPANEL, 0, 0);
		//		PanelInfo panel_info{ sizeof(PanelInfo) };
		PanelInfo panel_info;
		if (g_far.Control(h_panel, FCTL_GETPANELINFO, 0, (LONG_PTR)&panel_info)) {
			Buffer<unsigned char> buf(512);
			//			for (size_t i = 0; i < panel_info.ItemsNumber; ++i) {
			for (int i = 0; i < panel_info.ItemsNumber; ++i) {
				get_panel_item(h_panel, FCTL_GETPANELITEM, i, buf);
				const PluginPanelItem *panel_item = reinterpret_cast<const PluginPanelItem *>(buf.data());
				if (isdigit(panel_item->FindData.lpwszFileName[0])
						&& pidx == _wtoi(panel_item->FindData.lpwszFileName)) {
					panel_ri.CurrentItem = i;
					break;
				}
			}
		}
	}
	g_far.Control(h_panel, FCTL_REDRAWPANEL, 0, (LONG_PTR)&panel_ri);
}

bool panel_go_to_dir(HANDLE h_panel, const std::wstring &dir)
{
	//  FarPanelDirectory fpd{};
	//  fpd.StructSize = sizeof(FarPanelDirectory);
	//  fpd.Name = dir.c_str();
	return g_far.Control(h_panel, FCTL_SETPANELDIR, 0, (LONG_PTR)dir.c_str()) != 0;
}

// set current file on panel to file_path
bool panel_go_to_file(HANDLE h_panel, const std::wstring &file_path)
{
	std::wstring dir = extract_file_path(file_path);

	// we don't want to call FCTL_SETPANELDIRECTORY unless needed to not lose previous selection
	//	if (upcase(Far::get_panel_dir(h_panel)) != upcase(extract_file_path(file_path))) {
	if (Far::get_panel_dir(h_panel) != extract_file_path(file_path)) {
		if (!g_far.Control(h_panel, FCTL_SETPANELDIR, 0, (LONG_PTR)dir.c_str())) {
			return false;
		}
	} else {
		g_far.Control(h_panel, FCTL_UPDATEPANEL, 1, 0);
	}

	//  PanelInfo panel_info = {sizeof(PanelInfo)};
	PanelInfo panel_info;
	if (!g_far.Control(h_panel, FCTL_GETPANELINFO, 0, (LONG_PTR)&panel_info)) {
		return false;
	}

	std::wstring file_name = extract_file_name(file_path);
	//  PanelRedrawInfo panel_ri = { sizeof(PanelRedrawInfo) };
	PanelRedrawInfo panel_ri;
	Buffer<unsigned char> buf(0x1000);

	int i;
	for (i = 0; i < panel_info.ItemsNumber; i++) {
		get_panel_item(h_panel, FCTL_GETPANELITEM, i, buf);
		const PluginPanelItem *panel_item = reinterpret_cast<const PluginPanelItem *>(buf.data());
		if (file_name == panel_item->FindData.lpwszFileName) {
			panel_ri.CurrentItem = i;
			break;
		}
	}

	if (i == panel_info.ItemsNumber) {
		return false;
	}

	if (!g_far.Control(h_panel, FCTL_REDRAWPANEL, 0, (LONG_PTR)&panel_ri)) {
		return false;
	}

	return true;
}

/**
struct PanelRedrawInfo
{
	int CurrentItem;
	int TopPanelItem;
};

enum PANELINFOTYPE
{
	PTYPE_FILEPANEL,
	PTYPE_TREEPANEL,
	PTYPE_QVIEWPANEL,
	PTYPE_INFOPANEL
};

struct PanelInfo
{
	int PanelType;
	int Plugin;
	RECT PanelRect;
	int ItemsNumber;
	int SelectedItemsNumber;
	int CurrentItem;
	int TopPanelItem;
	int Visible;
	int Focus;
	int ViewMode;
	int SortMode;
	DWORD Flags;
	DWORD Reserved;
};

**/

bool panel_set_file(HANDLE h_panel, const std::wstring &file_name)
{
	//  PanelInfo panel_info = {sizeof(PanelInfo)};
	PanelInfo panel_info;
	if (!g_far.Control(h_panel, FCTL_GETPANELINFO, 0, (LONG_PTR)&panel_info)) {
		return false;
	}
	//  PanelRedrawInfo panel_ri = { sizeof(PanelRedrawInfo) };
	PanelRedrawInfo panel_ri;
	Buffer<unsigned char> buf(0x1000);

	int i;
	for (i = 0; i < panel_info.ItemsNumber; i++) {
		get_panel_item(h_panel, FCTL_GETPANELITEM, i, buf);
		const PluginPanelItem *panel_item = reinterpret_cast<const PluginPanelItem *>(buf.data());
		if (file_name == panel_item->FindData.lpwszFileName) {
			panel_ri.CurrentItem = i;
			break;
		}
	}

	if (i == panel_info.ItemsNumber) {
		return false;
	}

	if (!g_far.Control(h_panel, FCTL_REDRAWPANEL, 0, (LONG_PTR)&panel_ri)) {
		return false;
	}

	return true;
}

DWORD get_lang_id()
{
	return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
}

void close_panel(HANDLE h_panel, const std::wstring &dir)
{
	g_far.Control(h_panel, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)(dir.c_str()));
}

void open_help(const std::wstring &topic)
{
	g_far.ShowHelp(search_and_replace(g_far.ModuleName, L"\\", L"/").c_str(), topic.c_str(), FHELP_SELFHELP);
}

INT_PTR Settings::control(uint32_t command, void *param)
{
	//	return g_far.SettingsControl(handle, command, 0, param);
	return 1;
}

void Settings::clean()
{
	if (_settings_kfh) {
		_settings_kfh->Save();
	}

	_settings_kfh.reset();
	dir_id = 0;
}

Settings::Settings()
{
	//	_settings_kfh.reset();
	_settings_kfh = nullptr;
	settingsIni = InMyConfig("plugins/arclite/arclite.ini");
	cSectionName = "Settings";
}

Settings::~Settings()
{
	clean();
}

bool Settings::create(bool app_settings)
{
	clean();

	_settings_kfh.reset(new KeyFileHelper( settingsIni ));

	return true;
}

bool Settings::have_dir(const std::wstring &path)
{
	cSectionName = StrWide2MB(path);

	return _settings_kfh->HasSection(StrWide2MB(path));
}

bool Settings::set_dir(const std::wstring &path)
{
	cSectionName = StrWide2MB(path);

	return true;
}

bool Settings::list_dir(std::vector<std::wstring> &result)
{
	std::vector<std::string> lst = _settings_kfh->EnumSectionsAt(cSectionName, false);

	for (unsigned i = 0; i < lst.size(); i++) {
		std::wstring p;
		StrMB2Wide(lst[i], p);
		result.push_back(p);
	}

	return true;
}

bool Settings::set(const wchar_t *name, UInt64 value)
{
	_settings_kfh->SetULL(cSectionName, StrWide2MB(name), value);

	return true;
}

bool Settings::set(const wchar_t *name, int value)
{
	_settings_kfh->SetInt(cSectionName, StrWide2MB(name), value);

	return true;
}

bool Settings::set(const wchar_t *name, unsigned int value)
{
	_settings_kfh->SetUInt(cSectionName, StrWide2MB(name), value);

	return true;
}

bool Settings::set(const wchar_t *name, const std::wstring &value)
{
	_settings_kfh->SetString(cSectionName, StrWide2MB(name), value.c_str());
	return true;
}

bool Settings::set(const wchar_t *name, const void *value, size_t value_size)
{
	_settings_kfh->SetBytes(cSectionName, StrWide2MB(name), (const unsigned char *)value, value_size);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::get(const wchar_t *name, UInt64 &value)
{
	if (!_settings_kfh->HasKey(cSectionName, StrWide2MB(name)))
		return false;

	value = _settings_kfh->GetULL(cSectionName, StrWide2MB(name));
	return true;
}

bool Settings::get(const wchar_t *name, int &value)
{
	if (!_settings_kfh->HasKey(cSectionName, StrWide2MB(name)))
		return false;

	value = _settings_kfh->GetInt(cSectionName, StrWide2MB(name));
	return true;
}

bool Settings::get(const wchar_t *name, unsigned int &value)
{
	if (!_settings_kfh->HasKey(cSectionName, StrWide2MB(name)))
		return false;

	value = _settings_kfh->GetUInt(cSectionName, StrWide2MB(name));
	return true;
}

bool Settings::get(const wchar_t *name, std::wstring &value)
{
	if (!_settings_kfh->HasKey(cSectionName, StrWide2MB(name)))
		return false;

	StrMB2Wide(_settings_kfh->GetString(cSectionName, StrWide2MB(name)), value);
	return true;
}

bool Settings::get(const wchar_t *name, ByteVector &value)
{
	if (!_settings_kfh->HasKey(cSectionName, StrWide2MB(name)))
		return false;

	return _settings_kfh->GetBytes(value, cSectionName, StrWide2MB(name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::get(const wchar_t *dirname, const wchar_t *name, UInt64 &value)
{
	if (!_settings_kfh->HasKey(StrWide2MB(dirname), StrWide2MB(name)))
		return false;

	value = _settings_kfh->GetULL(StrWide2MB(dirname), StrWide2MB(name));

	return true;
}

bool Settings::get(const wchar_t *dirname, const wchar_t *name, std::wstring &value)
{
	if (!_settings_kfh->HasKey(StrWide2MB(dirname), StrWide2MB(name)))
		return false;

	StrMB2Wide(_settings_kfh->GetString(StrWide2MB(dirname), StrWide2MB(name)), value);
	return true;
}

bool Settings::get(const wchar_t *dirname, const wchar_t *name, ByteVector &value)
{
	if (!_settings_kfh->HasKey(StrWide2MB(dirname), StrWide2MB(name)))
		return false;

	return _settings_kfh->GetBytes(value, StrWide2MB(dirname), StrWide2MB(name));
}

bool Settings::del(const wchar_t *name)
{
	_settings_kfh->RemoveKey(cSectionName, StrWide2MB(name));
	return true;
}

bool Settings::del_dir(const wchar_t *name)
{
	_settings_kfh->RemoveSection(StrWide2MB(name));
	return true;
}

// const ArclitePrivateInfo* get_system_functions() {
//   return static_cast<const ArclitePrivateInfo*>(g_far.Private);
// }

};	  // namespace Far
