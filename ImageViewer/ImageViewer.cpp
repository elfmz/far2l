#include <farplug-wide.h>
#include <WinPort.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <farkeys.h>
#include <memory>
#include <cstdio>
#include <set>
#include <dirent.h>
#include <utils.h>
#include <math.h>

#define WINPORT_IMAGE_ID "image_viewer"

static PluginStartupInfo g_far;
static FarStandardFunctions g_fsf;

struct ViewerData
{
	std::string initial_file, cur_file, render_file, tmp_file;
	std::set<std::string> selection, all_files;
	COORD pos{}, size{};
	bool exited_by_enter{false};
	int dx = 0, dy = 0;
	int scale = 100;

	bool IterateFile(bool forward)
	{
		if (all_files.empty()) {
			DIR *d = opendir(".");
			if (d) {
				for (;;) {
					auto *de = readdir(d);
					if (!de) break;
					if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
						all_files.insert(de->d_name);
					}
				}
				closedir(d);
			}
		}
		auto it = all_files.find(cur_file);
		if (it == all_files.end()) {
			return false;
		}
		if (forward) {
			++it;
			if (it == all_files.end()) {
				it = all_files.begin();
			}
		} else {
			if (it == all_files.begin()) {
				it = all_files.end();
			}
			--it;
		}
		cur_file = *it;
		dx = dy = 0;
		scale = 100;
		return true;
	}
};

static bool InspectFileFormat(struct ViewerData *data)
{
	data->render_file = data->cur_file;

	struct stat st {};
	if (stat(data->cur_file.c_str(), &st) == -1) {
		return false;
	}

	if (!S_ISREG(st.st_mode) || st.st_size == 0) {
		return false;
	}

	if (!StrEndsBy(data->cur_file, ".mp4")
		&& !StrEndsBy(data->cur_file, ".mpg")
		&& !StrEndsBy(data->cur_file, ".avi")
		&& !StrEndsBy(data->cur_file, ".flv")
		&& !StrEndsBy(data->cur_file, ".mov")
		&& !StrEndsBy(data->cur_file, ".wmv")
		&& !StrEndsBy(data->cur_file, ".mkv"))
	{
		return true;
	}

	std::string cmd = StrPrintf(
		"ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 -- '%s'",
		data->cur_file.c_str());

	std::string frames_count;
	if (!POpen(frames_count, cmd.c_str())) {
		fprintf(stderr, "ERROR: ffprobe failed.\n");
		return false;
	}
	fprintf(stderr, "\n--- ImageViewer: frames_count=%s from %s\n", frames_count.c_str(), cmd.c_str());

	unsigned int frames_interval = atoi(frames_count.c_str()) / 6;
	if (frames_interval < 5) frames_interval = 5;

	if (data->tmp_file.empty()) {
		wchar_t preview_tmp[MAX_PATH + 32]{};
		g_fsf.MkTemp(preview_tmp, MAX_PATH, L"far2l-img");
		wcscat(preview_tmp, L".jpg");
		data->tmp_file = StrWide2MB(preview_tmp);
	}

	unlink(data->tmp_file.c_str());

	cmd = StrPrintf("ffmpeg -i '%s' -vf \"select='not(mod(n,%d))',scale=200:-1,tile=3x2\" '%s'",
		data->cur_file.c_str(), frames_interval, data->tmp_file.c_str());

	int r = system(cmd.c_str());
	fprintf(stderr, "\n--- ImageViewer: r=%d from %s\n", r, cmd.c_str());

	if (stat(data->tmp_file.c_str(), &st) == -1 || st.st_size == 0) {
		unlink(data->tmp_file.c_str());
		return false;
	}

	data->render_file = data->tmp_file;
	return true;
}

static bool LoadAndShowImage(struct ViewerData *data)
{
	fprintf(stderr, "\n--- ImageViewer: Starting image processing for '%s' ---\n", data->cur_file.c_str());

	if (data->render_file.empty()) {
		fprintf(stderr, "ERROR: bad file.\n");
		return false;
	}

	fprintf(stderr, "Target cell grid pos=%dx%d size=%dx%d\n", data->pos.X, data->pos.Y, data->size.X, data->size.Y);
	if (data->pos.X < 0 || data->pos.Y < 0 || data->size.X <= 0 || data->size.Y <= 0) {
		fprintf(stderr, "ERROR: bad grid.\n");
		return false;
	}


	// 1. Получаем оригинальные размеры картинки
	std::string cmd = "identify -format \"%w %h\" -- \"";
	cmd += data->render_file;
	cmd += "\"";

	std::string dims_str;
	if (!POpen(dims_str, cmd.c_str())) {
		fprintf(stderr, "ERROR: ImageMagick 'identify' failed.\n");
		return false;
	}

	int orig_w = 0, orig_h = 0;
	if (sscanf(dims_str.c_str(), "%d %d", &orig_w, &orig_h) != 2 || orig_w <= 0 || orig_h <= 0) {
		fprintf(stderr, "ERROR: Failed to parse original dimensions. Got: '%s'\n", dims_str.c_str());
		return false;
	}

	// 2. Получаем соотношение сторон ячейки терминала
	WinportGraphicsInfo wgi{};

	if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(wgi), &wgi) || !wgi.Caps) {
		fprintf(stderr, "ERROR: GetConsoleImageCaps failed\n");
		return false;
	}
	int canvas_w = int(data->size.X) * wgi.PixPerCell.X;
	int canvas_h = int(data->size.Y) * wgi.PixPerCell.Y;

	fprintf(stderr, "Image pixels size, original: %dx%d canvas: %dx%d\n", orig_w, orig_h, canvas_w, canvas_h);

	// 5. Формируем команду для imagemagick: ресайз, центрирование и добавление полей.
	int resize_w = canvas_w, resize_h = canvas_h;
	if (data->scale != 100) {
		resize_w = long(resize_w) * long(data->scale) / 100;
		resize_h = long(resize_h) * long(data->scale) / 100;
	}
	cmd.clear();
	if (resize_w > 8000 || resize_h > 8000) {
		cmd += "timeout 3 "; // workaround for stuck on too huge images
	}
	cmd += "convert -- \"";
	cmd += data->render_file;
	cmd += "\" -background black -gravity Center";

	if (data->dx != 0 || data->dy != 0) {
		int rdx = long(orig_w) * long(data->dx) / 100;
		int rdy = long(orig_h) * long(data->dy) / 100; // orig_h
		cmd += " -roll ";
		if (rdx >= 0) cmd+= "+";
		cmd+= std::to_string(rdx);
		if (rdy >= 0) cmd+= "+";
		cmd+= std::to_string(rdy);
	}

	cmd += " -resize " + std::to_string(resize_w) + "x" + std::to_string(resize_h);
	cmd += " -extent " + std::to_string(canvas_w) + "x" + std::to_string(canvas_h);
	cmd += " -depth 8 rgba:-";

	fprintf(stderr, "Executing ImageMagick: %s\n", cmd.c_str());

	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		fprintf(stderr, "ERROR: ImageMagick start failed.\n");
		return false;
	}

	std::vector<uint8_t> final_pixel_data(canvas_w * canvas_h * 4);
	size_t n_read = fread(final_pixel_data.data(), final_pixel_data.size(), 1, pipe);
	if (pclose(pipe) != 0) {
		fprintf(stderr, "ERROR: ImageMagick 'convert' failed.\n");
		return false;
	}

	if (n_read != 1) {
		fprintf(stderr, "ERROR: Failed to read final pixel data from ImageMagick.\n");
		return false;
	}

	// 6. Создаем ConsoleImage с готовым битмапом.
	fprintf(stderr, "--- Image processing finished, creating ConsoleImage ---\n\n");
	return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, 0, data->pos, canvas_w, canvas_h, final_pixel_data.data()) != FALSE;
}


static void UpdateDialogTitle(HANDLE hDlg, ViewerData* data)
{
	std::wstring ws_cur_file = StrMB2Wide(data->cur_file);
	if (data->selection.find(data->cur_file) != data->selection.end()) {
		ws_cur_file.insert(0, L"* "); 
	} else {
		ws_cur_file.insert(0, L"  "); 
	}

	std::wstring ws_hint = L"[Navigate: PGUP PGDN HOME | Pan: CURSORS NUMPAD + - = | Select: SPACE | Deselect: BS | Toggle: INS | ENTER | ESC]";

	FarDialogItemData dd_title = { ws_cur_file.size(), (wchar_t*)ws_cur_file.c_str() };
	FarDialogItemData dd_hint = { ws_hint.size(), (wchar_t*)ws_hint.c_str() };

	g_far.SendDlgMessage(hDlg, DM_SETTEXT, 0, (LONG_PTR)&dd_title);
	g_far.SendDlgMessage(hDlg, DM_SETTEXT, 2, (LONG_PTR)&dd_hint);

}


static LONG_PTR WINAPI ViewerDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	ViewerData* data = (ViewerData*)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

	switch(Msg) {
		case DN_INITDIALOG:
		{
			data = (ViewerData*)Param2;
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, (LONG_PTR)data);

			SMALL_RECT Rect;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);
			data->pos.X = 1;
			data->pos.Y = 1;
			data->size.X = Rect.Right > 1 ? Rect.Right - 1 : 1;
			data->size.Y = Rect.Bottom > 1 ? Rect.Bottom - 1 : 1;

			UpdateDialogTitle(hDlg, data);
			bool ok = InspectFileFormat(data) && LoadAndShowImage(data);
			if (!ok) {
				std::wstring ws_cur_file = StrMB2Wide(data->cur_file);
				const wchar_t *MsgItems[]={L"Image Viewer", L"Failed to load image file:", ws_cur_file.c_str()};
				g_far.Message(g_far.ModuleNumber, FMSG_WARNING|FMSG_ERRORTYPE, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
				g_far.SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
			}
			return TRUE;
		}

		case DN_KEY:
		{
			int Key = (int)Param2;
			if (Key == '=' || Key == '*' || Key == KEY_MULTIPLY || Key == KEY_CLEAR) {
				data->dx = data->dy = 0;
				data->scale = 100;
				LoadAndShowImage(data);

			} else if (Key == KEY_RIGHT || Key == KEY_NUMPAD6) {
				if (data->dx < 100) data->dx+= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_LEFT || Key == KEY_NUMPAD4) {
				if (data->dx > -100) data->dx-= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_DOWN || Key == KEY_NUMPAD2) {
				if (data->dy < 100) data->dy+= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_UP || Key == KEY_NUMPAD8) {
				if (data->dy > -100) data->dy-= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_NUMPAD9) {
				if (data->dx < 100) data->dx+= 10;
				if (data->dy > -100) data->dy-= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_NUMPAD1) {
				if (data->dx > -100) data->dx-= 10;
				if (data->dy < 100) data->dy+= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_NUMPAD3) {
				if (data->dx < 100) data->dx+= 10;
				if (data->dy < 100) data->dy+= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_NUMPAD7) {
				if (data->dx > -100) data->dx-= 10;
				if (data->dy > -100) data->dy-= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_ADD || Key == '+') {
				if (data->scale < 400) {
					if (data->scale < 200) data->scale+= 50;
					else data->scale+= 100;
				}
				LoadAndShowImage(data);

			}else if (Key == KEY_SUBTRACT || Key == '-') {
				if (data->scale > 1000) data->scale-= 100;
				else if (data->scale > 100) data->scale-= 50;
				else if (data->scale > 10) data->scale-= 10;
				LoadAndShowImage(data);

			} else if (Key == KEY_ESC || Key == KEY_F10 || Key == KEY_ENTER || Key == KEY_NUMENTER) {
				data->exited_by_enter = (Key == KEY_ENTER || Key == KEY_NUMENTER);
				g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0);

			} else if (Key == KEY_INS) {
				if (!data->selection.erase(data->cur_file)) {
					data->selection.insert(data->cur_file);
				}
				UpdateDialogTitle(hDlg, data);

			} else if (Key == KEY_SPACE) {
				data->selection.insert(data->cur_file);
				UpdateDialogTitle(hDlg, data);

			} else if (Key == KEY_BS) {
				data->selection.erase(data->cur_file);
				UpdateDialogTitle(hDlg, data);

			} else if (Key == KEY_HOME) {
				data->cur_file = data->initial_file;
				if (LoadAndShowImage(data)) {
					UpdateDialogTitle(hDlg, data);
				}

			} else if (Key == KEY_PGDN || Key == KEY_PGUP) {
				for (;;) { // silently skip bad files
					if (!data->IterateFile(Key == KEY_PGDN)) {
						data->cur_file.clear();
						g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0); // close dialog as reached the end
						break;
					}
					if (InspectFileFormat(data) && LoadAndShowImage(data)) {
						UpdateDialogTitle(hDlg, data);
						break;
					}
					data->selection.erase(data->cur_file); // remove non-loadable files from selection
				}
				return TRUE;
			}
			return TRUE;
		}
	}

	return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool ShowImage(const std::string &initial_file, std::set<std::string> &selection)
{
	ViewerData data;
	data.initial_file = initial_file;
	data.cur_file = initial_file;
	if (!selection.empty()) {
		data.all_files = selection;
		data.selection = selection;
	}

	SMALL_RECT Rect;
	g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

	FarDialogItem DlgItems[] = {
		{ DI_DOUBLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, 0, 0, L"???", 0 },
		{ DI_USERCONTROL, 1, 1, Rect.Right - 2, Rect.Bottom - 2, 0, {}, 0, 0, L"", 0},
		{ DI_TEXT, 0, Rect.Bottom, Rect.Right, Rect.Bottom, 0, {}, DIF_CENTERTEXT, 0, L"", 0},
	};

	HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, Rect.Right, Rect.Bottom,
							 L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ViewerDlgProc, (LONG_PTR)&data);

	if (hDlg == INVALID_HANDLE_VALUE) {
		return false;
	}

	g_far.DialogRun(hDlg);
	g_far.DialogFree(hDlg);

	WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);

	if (!data.tmp_file.empty()) {
		unlink(data.tmp_file.c_str());
	}

	if (!data.exited_by_enter) {
		return false;
	}
	selection.swap(data.selection);
	return true;
}

// --- Экспортируемые функции плагина ---

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	g_far = *Info;
	g_fsf = *Info->FSF;
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = 0;

	static const wchar_t *PluginMenuStrings[1];
	PluginMenuStrings[0] = L"Image Viewer";
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = 1;
}

static std::pair<std::string, bool> GetPanelItem(int cmd, int index)
{
	std::pair<std::string, bool> out;
	out.second = false;
	size_t sz = g_far.Control(PANEL_ACTIVE, cmd, index, 0);
	if (sz) {
		std::vector<char> buf(sz + 16);
		sz = g_far.Control(PANEL_ACTIVE, cmd, index, (LONG_PTR)buf.data());
		const PluginPanelItem *ppi = (const PluginPanelItem *)buf.data();
		if (ppi->FindData.lpwszFileName && *ppi->FindData.lpwszFileName) {
			out.first = Wide2MB(ppi->FindData.lpwszFileName);
			out.second = (ppi->Flags & PPIF_SELECTED) != 0;
		}
	}
	return out;
}

static std::set<std::string> GetSelectedItems()
{
	std::set<std::string> out;
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (pi.SelectedItemsNumber > 0 && pi.PanelType == PTYPE_FILEPANEL) {
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			const auto &fn_sel = GetPanelItem(FCTL_GETSELECTEDPANELITEM, i);
			if (!fn_sel.first.empty() && fn_sel.second) {
				out.insert(fn_sel.first);
			}
		}
	}
	return out;
}

static std::vector<std::string> GetAllItems()
{
	std::vector<std::string> out;
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	for (int i = 0; i < pi.ItemsNumber; ++i) {
		auto fn_sel = GetPanelItem(FCTL_GETPANELITEM, i);
		if (!fn_sel.first.empty()) {
			out.emplace_back(std::move(fn_sel.first));
		}
	}
	return out;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom == OPEN_PLUGINSMENU) {
		const auto &fn_sel = GetPanelItem(FCTL_GETCURRENTPANELITEM, 0);
		if (!fn_sel.first.empty()) {
			std::set<std::string> selection = GetSelectedItems();
			if (ShowImage(fn_sel.first, selection)) {
				std::vector<std::string> all_items = GetAllItems();
				g_far.Control(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, 0);
				for (size_t i = 0; i < all_items.size(); ++i) {
					BOOL selected = selection.find(all_items[i]) != selection.end();
					g_far.Control(PANEL_ACTIVE, FCTL_SETSELECTION, i, (LONG_PTR)selected);
				}
				g_far.Control(PANEL_ACTIVE,FCTL_ENDSELECTION, 0, 0);
				g_far.Control(PANEL_ACTIVE,FCTL_REDRAWPANEL, 0, 0);
			}
		}
	}

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin) {}
SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param) { return FALSE; }
