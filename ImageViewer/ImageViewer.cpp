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

#define WINPORT_IMAGE_ID "image_viewer"

static PluginStartupInfo g_far;
static FarStandardFunctions g_fsf;

struct ViewerData
{
	std::string cur_file;
	std::set<std::string> selection, all_files;
	COORD pos{}, size{};
	bool exited_by_enter{false};
};

static bool IterateFile(ViewerData &data, bool forward)
{
	if (data.all_files.empty()) {
		DIR *d = opendir(".");
		if (d) {
			for (;;) {
				auto *de = readdir(d);
				if (!de) break;
				if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
					data.all_files.insert(de->d_name);
				}
			}
			closedir(d);
		}
	}
	auto it = data.all_files.find(data.cur_file);
	if (it == data.all_files.end()) {
		return false;
	}
	if (forward) {
		++it;
		if (it == data.all_files.end()) {
			it = data.all_files.begin();
		}
	} else {
		if (it == data.all_files.begin()) {
			it = data.all_files.end();
		}
		--it;
	}
	data.cur_file = *it;
	return true;
}



static bool LoadAndShowImage(const char *path_name, COORD pos, COORD size)
{
	fprintf(stderr, "\n--- ImageViewer: Starting image processing for '%s' ---\n", path_name);
	fprintf(stderr, "Target cell grid pos=%dx%d size=%dx%d\n", pos.X, pos.Y, size.X, size.Y);
	if (pos.X < 0 || pos.Y < 0 || size.X <= 0 || size.Y <= 0) {
		fprintf(stderr, "ERROR: bad grid.\n");
		return false;
	}

	// 1. Получаем оригинальные размеры картинки
	std::string cmd = "identify -format \"%w %h\" \"";
	cmd += path_name;
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
	int canvas_w = int(size.X) * wgi.PixPerCell.X;
	int canvas_h = int(size.Y) * wgi.PixPerCell.Y;

	fprintf(stderr, "Image pixels size, original: %dx%d canvas: %dx%d\n", orig_w, orig_h, canvas_w, canvas_h);

	// 5. Формируем команду для imagemagick: ресайз, центрирование и добавление полей.
	cmd = "convert \"";
	cmd += path_name;
	cmd += "\" -background black -gravity center";
	cmd += " -resize " + std::to_string(canvas_w) + "x" + std::to_string(canvas_h);
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
	return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, 0, pos, canvas_w, canvas_h, final_pixel_data.data()) != FALSE;
}


static void UpdateDialogTitle(HANDLE hDlg, ViewerData* data)
{
	std::wstring ws_cur_file = StrMB2Wide(data->cur_file);
	if (data->selection.find(data->cur_file) != data->selection.end()) {
		ws_cur_file.insert(0, L"* "); 
	} else {
		ws_cur_file.insert(0, L"  "); 
	}
	FarDialogItemData dd = { ws_cur_file.size(), (wchar_t*)ws_cur_file.c_str() };
	g_far.SendDlgMessage(hDlg, DM_SETTEXT, 0, (LONG_PTR)&dd);
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
			bool ok = LoadAndShowImage(data->cur_file.c_str(), data->pos, data->size);
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
			if (Key == KEY_ESC || Key == KEY_F10 || Key == KEY_ENTER) {
				data->exited_by_enter = (Key == KEY_ENTER);
				g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0);

			} else if (Key == KEY_INS) {
				if (!data->selection.erase(data->cur_file)) {
					data->selection.insert(data->cur_file);
				}
				UpdateDialogTitle(hDlg, data);

			} else if (Key == KEY_SPACE || Key == KEY_RIGHT || Key == KEY_BS || Key == KEY_LEFT) {
				for (;;) { // silently skip bad files
					if (!IterateFile(*data, Key == KEY_SPACE || Key == KEY_RIGHT)) {
						data->cur_file.clear();
						g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0); // close dialog as reached the end
						break;
					}
					if (LoadAndShowImage(data->cur_file.c_str(), data->pos, data->size)) {
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
		{ DI_TEXT, 0, Rect.Bottom, Rect.Right, Rect.Bottom, 0, {}, DIF_CENTERTEXT, 0, L"[Navigate: RIGHT LEFT | (De)Select: INS | ENTER | ESC]", 0},
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
