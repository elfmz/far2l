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

#define HINT_STRING "[Navigate: PGUP PGDN HOME | Pan: TAB CURSORS NUMPAD + - = | Select: SPACE | Deselect: BS | Toggle: INS | ENTER | ESC]"

#define EXITED_DUE_ERROR      -1
#define EXITED_DUE_ENTER      42
#define EXITED_DUE_ESCAPE     24

static PluginStartupInfo g_far;
static FarStandardFunctions g_fsf;

class IVInfoMessage
{
private:
	HANDLE _h_scr;
	std::wstring _title;
	std::wstring _text1;
	std::wstring _text2;

public:
	void Show(std::wstring const &text2, std::wstring const &text1 = L"")
	{
		WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
		if (_h_scr == nullptr)
			_h_scr = g_far.SaveScreen(0,0,-1,-1);
		if (!text1.empty()) {
			_text1 = text1;
			if (_text1.back() != L'\n')
				_text1 += L'\n';
		}
		_text2 = text2;
		std::wstring tmp = _title + L'\n' + _text1 + _text2;
		g_far.Message(g_far.ModuleNumber, FMSG_ALLINONE, nullptr,
			(const wchar_t * const *) tmp.c_str(),
			0, 0);
	}
	void Close()
	{
		if (_h_scr) {
			g_far.RestoreScreen(_h_scr);
			_h_scr = nullptr;
		}
	}
	IVInfoMessage(const std::wstring &text1 = L"", const std::wstring &title = L"ImageViewer")
		: _h_scr(nullptr), _title(title), _text1(text1), _text2(L"")
		{
			if (_text1.back() != L'\n')
				_text1 += L'\n';
		}
	~IVInfoMessage() { Close(); }
};

class ImageViewer
{
	HANDLE _dlg{};
	std::string _initial_file, _cur_file, _render_file, _tmp_file;
	std::set<std::string> _selection, _all_files;
	COORD _pos{}, _size{};
	int _dx{0}, _dy{0};
	int _scale{100};
	int _rotate{0};
	int _orig_w{0}, _orig_h{0}; // info about image size for title
	std::string _err_str;

	void ErrorMessage()
	{
		std::wstring ws_cur_file = L"\"" + StrMB2Wide(_cur_file) + L"\"";
		std::wstring werr_str = StrMB2Wide(_err_str);
		const wchar_t *MsgItems[]={L"Image Viewer", L"Failed to load image file:", ws_cur_file.c_str(), werr_str.c_str(), L"Ok"};
		g_far.Message(g_far.ModuleNumber, FMSG_WARNING|FMSG_ERRORTYPE, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
	}

	bool IterateFile(bool forward)
	{
		if (_all_files.empty()) {
			DIR *d = opendir(".");
			if (d) {
				for (;;) {
					auto *de = readdir(d);
					if (!de) break;
					if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
						_all_files.insert(de->d_name);
					}
				}
				closedir(d);
			}
		}
		auto it = _all_files.find(_cur_file);
		if (it == _all_files.end()) {
			return false;
		}
		if (forward) {
			++it;
			if (it == _all_files.end()) {
				it = _all_files.begin();
			}
		} else {
			if (it == _all_files.begin()) {
				it = _all_files.end();
			}
			--it;
		}
		_cur_file = *it;
		_dx = _dy = 0;
		_scale = 100;
		_rotate = 0;
		return true;
	}

	bool IsVideoFile() const
	{
		const char *video_extensions[] = {".mp4", ".mpg", ".avi", ".flv", ".mov", ".wmv", ".mkv"};
		for (const auto &video_ext : video_extensions) {
			if (StrEndsBy(_cur_file, video_ext)) {
				return true;
			}
		}
		return false;
	}

	bool InspectFileFormat()
	{
		_render_file = _cur_file;

		struct stat st {};
		if (stat(_cur_file.c_str(), &st) == -1) {
			return false;
		}

		if (!S_ISREG(st.st_mode) || st.st_size == 0) {
			return false;
		}

		if (!IsVideoFile()) {
			return true;
		}

		IVInfoMessage ivmessage(
			L"Processing video file ("
			+ ( st.st_size < 1024 ? std::to_wstring(st.st_size) + L" b"
				: st.st_size < 1024*1024 ? std::to_wstring(st.st_size / 1024) + L" K"
				: st.st_size < 1024*1024*1024 ? std::to_wstring(st.st_size / 1024 / 1024) + L" M"
				: std::to_wstring(st.st_size / 1024 / 1024 / 1024) + L" G" )
			+ L"):\n\""
			+ StrMB2Wide(_cur_file) + L"\""
			);

		_orig_w = _orig_h = 0; // clear info about image size for title

		ivmessage.Show(L"Video file: get count of frames...");
		DenoteState("Transforming...");
		std::string cmd = StrPrintf(
			"ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 -- '%s'",
			_cur_file.c_str());

		std::string frames_count;
		if (!POpen(frames_count, cmd.c_str())) {
			_err_str = "ERROR: ffprobe failed";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}
		fprintf(stderr, "\n--- ImageViewer: frames_count=%s from %s\n", frames_count.c_str(), cmd.c_str());

		unsigned int frames_count_i = atoi(frames_count.c_str());
		unsigned int frames_interval = frames_count_i / 6;
		if (frames_interval < 5) frames_interval = 5;

		if (_tmp_file.empty()) {
			wchar_t preview_tmp[MAX_PATH + 32]{};
			g_fsf.MkTemp(preview_tmp, MAX_PATH, L"far2l-img");
			wcscat(preview_tmp, L".jpg");
			_tmp_file = StrWide2MB(preview_tmp);
		}

		unlink(_tmp_file.c_str());

		ivmessage.Show(L"Video file has " + std::to_wstring(frames_count_i) + L" frames.\nObtaining 6 frames to preview picture...");
		cmd = StrPrintf("ffmpeg -i '%s' -vf \"select='not(mod(n,%d))',scale=200:-1,tile=3x2\" '%s'",
			_cur_file.c_str(), frames_interval, _tmp_file.c_str());

		int r = system(cmd.c_str());
		fprintf(stderr, "\n--- ImageViewer: r=%d from %s\n", r, cmd.c_str());

		if (stat(_tmp_file.c_str(), &st) == -1 || st.st_size == 0) {
			unlink(_tmp_file.c_str());
			return false;
		}

		_render_file = _tmp_file;
		return true;
	}

	bool RenderImage(bool bmess = false)
	{
		fprintf(stderr, "\n--- ImageViewer: '%s' ---\n", _render_file.c_str());

		_orig_w = _orig_h = 0; // clear info about image size for title

		if (_render_file.empty()) {
			_err_str = "ERROR: bad file";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		fprintf(stderr, "Target cell grid _pos=%dx%d _size=%dx%d\n", _pos.X, _pos.Y, _size.X, _size.Y);
		if (_pos.X < 0 || _pos.Y < 0 || _size.X <= 0 || _size.Y <= 0) {
			_err_str = "ERROR: bad grid";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		// 1. Получаем соотношение сторон ячейки терминала
		WinportGraphicsInfo wgi{};

		if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(wgi), &wgi) || (wgi.Caps & WP_IMGCAP_RGBA) == 0) {
			_err_str = "ERROR: GetConsoleImageCaps failed";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}
		int canvas_w = int(_size.X) * wgi.PixPerCell.X;
		int canvas_h = int(_size.Y) * wgi.PixPerCell.Y;

		IVInfoMessage ivmessage(L"Processing file: \"" + StrMB2Wide(_render_file) + L"\"");

		DenoteState("Analyzing...");
		// 2. Получаем оригинальные размеры картинки
		if (bmess)
			ivmessage.Show(L"Obtain picture size via ImageMagick 'identify'...");
		std::string cmd = "identify -format \"%w %h\" -- \"";
		cmd += _render_file;
		cmd += "\"";

		std::string dims_str;
		if (!POpen(dims_str, cmd.c_str())) {
			_err_str = "ERROR: ImageMagick 'identify' failed";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		int orig_w = 0, orig_h = 0;
		if (sscanf(dims_str.c_str(), "%d %d", &orig_w, &orig_h) != 2 || orig_w <= 0 || orig_h <= 0) {
			_err_str = "ERROR: Failed to parse original dimensions. Got: '" + dims_str + "'";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		fprintf(stderr, "Image pixels _size, original: %dx%d canvas: %dx%d\n", orig_w, orig_h, canvas_w, canvas_h);

		// update info about image size for title
		_orig_w = orig_w;
		_orig_h = orig_h;

		DenoteState("Rendering...");
		// 3. Формируем команду для imagemagick: ресайз, центрирование и добавление полей.
		if (bmess)
			ivmessage.Show(L"Executing ImageMagick 'convert'...");
		int resize_w = canvas_w, resize_h = canvas_h;
		if (_scale != 100) {
			resize_w = long(resize_w) * long(_scale) / 100;
			resize_h = long(resize_h) * long(_scale) / 100;
		}
		cmd.clear();
		if (resize_w > 8000 || resize_h > 8000) {
			cmd += "timeout 3 "; // workaround for stuck on too huge images
		}
		cmd += "convert -- \"";
		cmd += _render_file;
		cmd += "\" -background black -gravity Center";

		if (_rotate != 0) {
			cmd += " -rotate " + std::to_string(_rotate);
		}

		if (_dx != 0 || _dy != 0) {
			int rdx = long(orig_w) * long(_dx) / 100;
			int rdy = long(orig_h) * long(_dy) / 100; // orig_h
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

		FILE* fp = popen(cmd.c_str(), "r");
		if (!fp) {
			_err_str = "ERROR: ImageMagick start failed";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		const size_t pixels_count = size_t(canvas_w) * canvas_h;
		std::vector<uint8_t> final_pixel_data(pixels_count * 4);
		size_t n_read = fread(final_pixel_data.data(), final_pixel_data.size(), 1, fp);
		if (pclose(fp) != 0) {
			_err_str = "ERROR: ImageMagick 'convert' failed";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		if (n_read != 1) {
			_err_str = "ERROR: Failed to read final pixel data from ImageMagick";
			fprintf(stderr, "%s.\n", _err_str.c_str());
			return false;
		}

		DWORD flags = WP_IMG_RGB; // use RGBA only if image really has non-opaque pixels
		for (size_t i = 0; i < pixels_count; ++i) {
			if (final_pixel_data[i * 4 + 3] != 0xff) {
				flags = WP_IMG_RGBA;
				break;
			}
		}
		if (flags == WP_IMG_RGB) {
			std::vector<uint8_t> rgb_pixel_data(pixels_count * 3);
			for (size_t i = 0; i < pixels_count; ++i) {
				rgb_pixel_data[i * 3] = final_pixel_data[i * 4];
				rgb_pixel_data[i * 3 + 1] = final_pixel_data[i * 4 + 1];
				rgb_pixel_data[i * 3 + 2] = final_pixel_data[i * 4 + 2];
			}
			final_pixel_data.swap(rgb_pixel_data);
		}

		if (bmess)
			ivmessage.Close();

		// 4. Создаем ConsoleImage с готовым битмапом.
		fprintf(stderr, "--- Image processing finished, flags=%u ---\n\n", flags);
		return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, flags, _pos, canvas_w, canvas_h, final_pixel_data.data()) != FALSE;
	}

	void SetTitleAndStatus(const std::string &title, const std::string &status)
	{
		std::wstring ws_title = StrMB2Wide(title);
		std::wstring ws_status = StrMB2Wide(status);

		FarDialogItemData dd_title = { ws_title.size(), (wchar_t*)ws_title.c_str() };
		FarDialogItemData dd_status = { ws_status.size(), (wchar_t*)ws_status.c_str() };

		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 0, (LONG_PTR)&dd_title);
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 1, (LONG_PTR)&dd_status);
	}

	void DenoteState(const char *stage = NULL)
	{
		std::string title = (_selection.find(_cur_file) != _selection.end()) ? "* " : "  ";
		title+= _cur_file;
		if (stage) {
			title+= " [";
			title+= stage;
			title+= ']';
		}
		else if (_orig_w > 0 && _orig_h > 0)
			title+= " (" + std::to_string(_orig_w) + 'x' + std::to_string(_orig_h) + ')';

		std::string status = HINT_STRING;

		char prefix[32];

		if (_dx != 0 || _dy != 0) {
			snprintf(prefix, ARRAYSIZE(prefix), "%s%d:%s%d ", (_dx > 0) ? "+" : "", _dx, (_dy > 0) ? "+" : "", _dy);
			status.insert(0, prefix);
		}

		if (_scale != 100) {
			snprintf(prefix, ARRAYSIZE(prefix), "%d%% ", _scale);
			status.insert(0, prefix);
		}

		if (_rotate != 0) {
			snprintf(prefix, ARRAYSIZE(prefix), "%d° ", _rotate);
			status.insert(0, prefix);
		}

		SetTitleAndStatus(title, status);
	}

public:
	ImageViewer(const std::string &initial_file, const std::set<std::string> &selection)
		:
		_initial_file(initial_file),
		_cur_file(initial_file),
		_selection(selection),
		_all_files(selection)
	{
	}

	~ImageViewer()
	{
		if (!_tmp_file.empty()) {
			unlink(_tmp_file.c_str());
		}
	}


	const std::set<std::string> &GetSelection() const
	{
		return _selection;
	}

	bool Init(HANDLE dlg, SMALL_RECT &rc)
	{
		_dlg = dlg;
		_pos.X = 1;
		_pos.Y = 1;
		_size.X = rc.Right > 1 ? rc.Right - 1 : 1;
		_size.Y = rc.Bottom > 1 ? rc.Bottom - 1 : 1;

		_err_str.clear();
		if (!InspectFileFormat() || !RenderImage(true)) {
			ErrorMessage();
			return false;
		}
		DenoteState();

		return true;
	}

	void Home()
	{
		_cur_file = _initial_file;
		if (InspectFileFormat() && RenderImage()) {
			DenoteState();
		}
	}

	bool Iterate(bool forward)
	{
		for (size_t i = 0;; ++i) { // silently skip bad files
			if (!IterateFile(forward) || i > _all_files.size()) {
				_cur_file.clear();
				return false; // bail out on logic error or infinite loop
			}
			if (InspectFileFormat() && RenderImage()) {
				DenoteState();
				return true;
			}
			_selection.erase(_cur_file); // remove non-loadable files from _selection
		}
	}

	void Scale(int change)
	{
		if (change > 0) {
			if (_scale < 100) _scale+= change;
			else if (_scale < 200) _scale+= change * 5;
			else if (_scale < 400) _scale+= change * 10;
		} else if (change < 0) {
			if (_scale > 200) _scale+= change * 10;
			else if (_scale > 100) _scale+= change * 5;
			else if (_scale > 10) _scale+= change;
		}
		if (_scale < 10) {
			_scale = 10;
		}
		RenderImage();
		DenoteState();
	}

	void Rotate(int change)
	{
		_rotate+= (change > 0) ? 90 : -90;
		if (_rotate == 360 || _rotate == -360) {
			_rotate = 0;
		}
		RenderImage();
		DenoteState();
	}

	void Shift(int horizontal, int vertical)
	{
		if (horizontal != 0) {
			_dx+= horizontal;
			if (_dx >= 100) _dx-= 100;
			if (_dx <= -100) _dx+= 100;
		}
		if (vertical != 0) {
			_dy+= vertical;
			if (_dy >= 100) _dy-= 100;
			if (_dy <= -100) _dy+= 100;
		}
		RenderImage();
		DenoteState();
	}

	void Reset()
	{
		_dx = _dy = 0;
		_scale = 100;
		_rotate = 0;

		RenderImage();
		DenoteState();
	}

	void Select()
	{
		if (_selection.insert(_cur_file).second) {
			DenoteState();
		}
	}

	void Deselect()
	{
		if (_selection.erase(_cur_file)) {
			DenoteState();
		}
	}

	void Toggle()
	{
		if (!_selection.erase(_cur_file)) {
			_selection.insert(_cur_file);
		}
		DenoteState();
	}
};




static LONG_PTR WINAPI ViewerDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch(Msg) {
		case DN_INITDIALOG:
		{
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);

			SMALL_RECT Rect;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

			ImageViewer *iv = (ImageViewer *)Param2;

			if (!iv->Init(hDlg, Rect)) {
				g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ERROR, 0);
			}
			return TRUE;
		}

		case DN_KEY:
		{
			ImageViewer *iv = (ImageViewer *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const int delta = ((((int)Param2) & KEY_SHIFT) != 0) ? 1 : 10;
			switch ((int)(Param2 & ~KEY_SHIFT)) {
				case KEY_CLEAR: case KEY_MULTIPLY: case '=': case '*': iv->Reset(); break;
				case KEY_NUMPAD6: case KEY_RIGHT: iv->Shift(delta, 0); break;
				case KEY_NUMPAD4: case KEY_LEFT: iv->Shift(-delta, 0); break;
				case KEY_NUMPAD2: case KEY_DOWN: iv->Shift(0, delta); break;
				case KEY_NUMPAD8: case KEY_UP: iv->Shift(0, -delta); break;
				case KEY_NUMPAD9: iv->Shift(delta, -delta); break;
				case KEY_NUMPAD1: iv->Shift(-delta, delta); break;
				case KEY_NUMPAD3: iv->Shift(delta, delta); break;
				case KEY_NUMPAD7: iv->Shift(-delta, -delta); break;
				case KEY_ADD: case '+': iv->Scale(delta); break;
				case KEY_SUBTRACT: case '-': iv->Scale(-delta); break;
				case KEY_TAB: iv->Rotate( (delta == 1) ? -90 : 90); break;
				case KEY_INS: iv->Toggle(); break;
				case KEY_SPACE: iv->Select(); break;
				case KEY_BS: iv->Deselect(); break;
				case KEY_HOME: iv->Home(); break;
				case KEY_PGDN: iv->Iterate(true); break;
				case KEY_PGUP: iv->Iterate(false); break;
				case KEY_ENTER: case KEY_NUMENTER:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ENTER, 0);
					break;
				case KEY_ESC: case KEY_F10:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ESCAPE, 0);
					break;
			}
			return TRUE;
		}

		case DN_CLOSE:
			WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
			break;
	}

	return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool ShowImage(const std::string &initial_file, std::set<std::string> &selection)
{
	ImageViewer iv(initial_file, selection);

	SMALL_RECT Rect;
	g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

	FarDialogItem DlgItems[] = {
		{ DI_DOUBLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, 0, 0, L"???", 0 },
		{ DI_TEXT, 0, Rect.Bottom, Rect.Right, Rect.Bottom, 0, {}, DIF_CENTERTEXT, 0, L"", 0},
		{ DI_USERCONTROL, 1, 1, Rect.Right - 1, Rect.Bottom - 1, 0, {}, 0, 0, L"", 0}, //
	};

	HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, Rect.Right, Rect.Bottom,
							 L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ViewerDlgProc, (LONG_PTR)&iv);

	if (hDlg == INVALID_HANDLE_VALUE) {
		return false;
	}

	int exit_code = g_far.DialogRun(hDlg);
	g_far.DialogFree(hDlg);

	if (exit_code != EXITED_DUE_ENTER) {
		return false;
	}
	selection = iv.GetSelection();
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
			std::set<std::string> _selection = GetSelectedItems();
			if (ShowImage(fn_sel.first, _selection)) {
				std::vector<std::string> all_items = GetAllItems();
				g_far.Control(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, 0);
				for (size_t i = 0; i < all_items.size(); ++i) {
					BOOL selected = _selection.find(all_items[i]) != _selection.end();
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
