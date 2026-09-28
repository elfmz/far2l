#include "Common.h"
#include "ImageView.h"
#include "lng.h"
#include "Settings.h"
#include "ToolExec.h"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>

class ImageViewAtFull : public ImageView
{
	WinportGraphicsInfo _drag_wgi{};
	HANDLE _dlg{NULL};
	COORD _drag_prev_pos{}, _drag_pending{};
	bool _dragging{false};

protected:
	virtual void DenoteInfoAndPan(const std::string &info, const std::string &pan)
	{
		const int visible_box_dlgid = CurFileSelected() ? 1 : 0;
		const int invisible_box_dlgid = CurFileSelected() ? 0 : 1;
		const int hint_text_dlgid = 3;
		const int pan_text_dlgid = 4;
		const int info_text_dlgid = 5;

		ConsoleRepaintsDeferScope crds(NULL);
		std::wstring ws_title = CurFileSelected() ? L"* " : L"  ";
		StrMB2Wide(CurFile(), ws_title, true);
		FarDialogItemData dd_title = { ws_title.size(), (wchar_t*)ws_title.c_str() };

		// update pan and info lengthes before title, so title will paint over previous one
		// but texts  - after title, so text it will get drawn after title, and due to that - will remain visible
		const auto &ws_pan = StrMB2Wide(pan);
		FarDialogItem di{};
		if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, pan_text_dlgid, (LONG_PTR)&di)) {
			di.X2 = di.X1 + (ws_pan.empty() ? 0 : ws_pan.size() - 1);
			g_far.SendDlgMessage(_dlg, DM_SETDLGITEMSHORT, pan_text_dlgid, (LONG_PTR)&di);
		}
		const auto &ws_info = StrMB2Wide(info);
		if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, info_text_dlgid, (LONG_PTR)&di)) {
			di.X1 = di.X2 - (ws_info.empty() ? 0 : ws_info.size() - 1);
			g_far.SendDlgMessage(_dlg, DM_SETDLGITEMSHORT, info_text_dlgid, (LONG_PTR)&di);
		}

		g_far.SendDlgMessage(_dlg, DM_SHOWITEM, invisible_box_dlgid, 0);
		g_far.SendDlgMessage(_dlg, DM_SHOWITEM, visible_box_dlgid, 1);

		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 0, (LONG_PTR)&dd_title);
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 1, (LONG_PTR)&dd_title);

		if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, visible_box_dlgid, (LONG_PTR)&di)) {
			int X1 = di.X1, X2 = di.X2;
			const int hint_length = g_far.SendDlgMessage(_dlg, DM_GETTEXTPTR, hint_text_dlgid, 0);
			if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, hint_text_dlgid, (LONG_PTR)&di)) {
				di.X1 = std::max(X1, int(X1 + X2 + 1 - hint_length) / 2);
				di.X2 = std::min(X2, int(di.X1 + hint_length - 1));
				g_far.SendDlgMessage(_dlg, DM_SETDLGITEMSHORT, hint_text_dlgid, (LONG_PTR)&di);
			}
		}

		FarDialogItemData dd_pan = { ws_pan.size(), (wchar_t*)ws_pan.c_str() };
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, pan_text_dlgid, (LONG_PTR)&dd_pan);

		FarDialogItemData dd_info = { ws_info.size(), (wchar_t*)ws_info.c_str() };
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, info_text_dlgid, (LONG_PTR)&dd_info);

		ImageView::DenoteInfoAndPan(info, pan);
	}

public:
	bool may_select{false};
	bool full_size{false};
	bool _first_draw{true};
	using ImageView::CurFile;

	ImageViewAtFull(size_t initial_file, const std::vector<std::pair<std::string, bool> > &all_files)
		: ImageView(initial_file, all_files)
	{
	}

	ImageOpResult Setup(SMALL_RECT &rc, HANDLE dlg)
	{
		_dlg = dlg;
		_first_draw = true;
		return ImageView::Setup(rc);
	}

	void DraggingMove(COORD pos)
	{
		if (!_dragging) {
			_dragging = true;
			_drag_pending = COORD{};
			if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(_drag_wgi), &_drag_wgi)) {
				fprintf(stderr, "%s: GetConsoleImageCaps failed\n", __FUNCTION__);
				_drag_wgi.PixPerCell = COORD{}; // essentially disable dragging
			}
		} else {
			_drag_pending.X+= SHORT((_drag_prev_pos.X - pos.X) * _drag_wgi.PixPerCell.X);
			_drag_pending.Y+= SHORT((_drag_prev_pos.Y - pos.Y) * _drag_wgi.PixPerCell.Y);
		}
		_drag_prev_pos = pos;
	}

	void DraggingApplyMoves()
	{
		if (_drag_pending.X != 0 || _drag_pending.Y != 0) {
			COORD actual = ShiftByPixels(_drag_pending);
			if (_dragging) {
				_drag_pending.X-= actual.X;
				_drag_pending.Y-= actual.Y;
			} else {
				_drag_pending = COORD{};
			}
		}
	}

	void DraggingFinish()
	{
		if (_dragging) {
			_dragging = false;
			DraggingApplyMoves();
		}
	}

	bool ShowExifInfo();
	void ShowGpsInfo();
};

static LONG_PTR WINAPI ImageDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_MOUSEEVENT:
		{
			ImageViewAtFull *iv = (ImageViewAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const MOUSE_EVENT_RECORD *me = (const MOUSE_EVENT_RECORD *)Param2;
			if ( (me->dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0 && (me->dwEventFlags & MOUSE_MOVED)  == 0) {
				if ((me->dwControlKeyState & (SHIFT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) {
					iv->Rotate(-90);
				} else {
					iv->Rotate(90);
				}

			} else if ((me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
				iv->DraggingMove(me->dwMousePosition);
				if (!WINPORT(WaitConsoleInput)(NULL, 0)) { // avoid movements 'accumulation'
					iv->DraggingApplyMoves();
				}
			} else {
				iv->DraggingFinish();
			}
		}
		return TRUE;

		case DN_INITDIALOG:
		{
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);
			SMALL_RECT rc;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &rc, 0);

			ImageViewAtFull *iv = (ImageViewAtFull *)Param2;
			if (!iv->full_size) {
				RectReduce(rc);
			}
			const auto result = iv->Setup(rc, hDlg);
			if (result == ImageOpResult::OK) {
				g_far.SendDlgMessage(hDlg, DM_SETMOUSEEVENTNOTIFY, 1, 0);
			} else {
				const auto exit_code = (result == ImageOpResult::CANCELLED) ? EXITED_DUE_CANCELLED : EXITED_DUE_ERROR;
				g_far.SendDlgMessage(hDlg, DM_CLOSE, exit_code, 0);
			}
		}
		return TRUE;

		case DN_KEY:
		{
			ImageViewAtFull *iv = (ImageViewAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const int delta = ((((int)Param2) & KEY_SHIFT) != 0) ? 1 : 10;
			const int key = (int)(Param2 & ~KEY_SHIFT);
			PurgeAccumulatedInputEvents(); // avoid navigation etc keypresses 'accumulation'
			switch (key) {
				case 'a': case 'A': case KEY_MULTIPLY: case '*':
					g_settings.SetDefaultScale(Settings::LESSOREQUAL_SCREEN);
					iv->Reset(true);
					break;
				case 'q': case 'Q': case KEY_DEL: case KEY_NUMDEL:
					g_settings.SetDefaultScale(Settings::EQUAL_SCREEN);
					iv->Reset(true);
					break;
				case 'z': case 'Z': case KEY_DIVIDE: case '/':
					g_settings.SetDefaultScale(Settings::EQUAL_IMAGE);
					iv->Reset(true);
					break;
				case KEY_CLEAR: case '=': iv->Reset(false); break;
				case KEY_ADD: case '+': case KEY_MSWHEEL_UP: iv->Scale(delta); break;
				case KEY_SUBTRACT: case '-': case KEY_MSWHEEL_DOWN: iv->Scale(-delta); break;
				case KEY_NUMPAD6: case KEY_RIGHT: iv->Shift(delta, 0); break;
				case KEY_NUMPAD4: case KEY_LEFT: iv->Shift(-delta, 0); break;
				case KEY_NUMPAD2: case KEY_DOWN: iv->Shift(0, delta); break;
				case KEY_NUMPAD8: case KEY_UP: iv->Shift(0, -delta); break;
				case KEY_NUMPAD9: iv->Shift(delta, -delta); break;
				case KEY_NUMPAD1: iv->Shift(-delta, delta); break;
				case KEY_NUMPAD3: iv->Shift(delta, delta); break;
				case KEY_NUMPAD7: iv->Shift(-delta, -delta); break;
				case KEY_TAB: iv->Rotate( (delta == 1) ? -90 : 90); break;
				case KEY_INS: case KEY_NUMPAD0:
					if (iv->may_select)
						iv->ToggleSelection();
					break;
				case KEY_SPACE:
					if (iv->may_select)
						iv->Select();
					break;
				case KEY_BS:
					if (iv->may_select)
						iv->Deselect();
					break;
				case KEY_HOME: iv->Home(); break;
				case KEY_PGDN: iv->Iterate(true); break;
				case KEY_PGUP: iv->Iterate(false); break;
				case KEY_ENTER: case KEY_NUMENTER:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ENTER, 0);
					break;
				case KEY_ESC: case KEY_F10:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ESCAPE, 0);
					break;
				case KEY_F7: case 'h': case 'H': iv->MirrorH(); break;
				case KEY_F8: case 'v': case 'V': iv->MirrorV(); break;
				case KEY_F5: case 'f': case 'F':
					iv->full_size = !iv->full_size;
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
					break;
				case KEY_F9:
					WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
					g_settings.ConfigurationDialog();
					iv->ForceShow();
					break;
				case KEY_F1:
					WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
					g_far.ShowHelp(g_far.ModuleName, L"Contents", FHELP_USECONTENTS);
					iv->ForceShow();
					break;
				case KEY_F4:
					iv->RunProcessingCommand();
					break;
				case 'i': case 'I': case KEY_F3:
					if (iv->ShowExifInfo()) {
						g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
					}
					break;
				case 'g': case 'G': case KEY_ALTF8:
					iv->ShowGpsInfo();
					break;
				case 't': case 'T': case KEY_CTRLF10:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_GOTO_CURFILE, 0);
					break;
			}
		}
		return TRUE;

		case DN_CLOSE:
			WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
			break;
		case DN_ENTERIDLE:
		{

			// WezTerm erases the kitty protocol image if text (even spaces) is displayed over it.
			// As a result, when opening ImageViewer, the background erases the image.
			// DN_ENTERIDLE is the first event that ensures that the dialog has been fully rendered.
			// Calling ForceShow() at this point draws the image
			// over the now-definitely-drawn background, and it remains visible.
			//
			// See #3201 and #3209 for details.

			ImageViewAtFull *iv = (ImageViewAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			if (iv && iv->_first_draw) {
				iv->_first_draw = false;
				iv->ForceShow();
			}
		}
		break;

		case DN_RESIZECONSOLE:
			g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
			break;

		case DN_DRAGGED:
			return FALSE;
	}

	return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static EXITED_DUE ShowImageAtFullInternal(size_t initial_file, std::vector<std::pair<std::string, bool>> &all_files, std::unordered_set<std::string> *selection, bool silent_exit_on_error, std::string *goto_file = nullptr)
{
	ImageViewAtFull iv(initial_file, all_files);
	if (selection) {
		iv.may_select = true;
	}

	for (;;) {
		SMALL_RECT rc;
		g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &rc, 0);

		std::wstring hint;
		hint+= L' ';
		if (all_files.size() > 1) {
			hint+= g_settings.Msg(M_HINT_NAVIGATE);
			hint+= L" | ";
		}
		hint+= g_settings.Msg(M_HINT_PAN);
		hint+= L" | ";
		if (selection) {
			hint+= g_settings.Msg(M_HINT_SELECTION);
			hint+= L" | ";
		}
		hint+= g_settings.Msg(M_HINT_OTHER);
		hint+= L' ';

		FarDialogItem DlgItems[] = {
			{ DI_SINGLEBOX, 0, 0, rc.Right, rc.Bottom, FALSE, {}, DIF_SHOWAMPERSAND, 0, L"???", 0 },
			{ DI_DOUBLEBOX, 0, 0, rc.Right, rc.Bottom, FALSE, {}, DIF_HIDDEN | DIF_SHOWAMPERSAND, 0, L"???", 0 },
			{ DI_USERCONTROL, 1, 1, rc.Right - 1, rc.Bottom - 1, 0, {COL_DIALOGBOX}, 0, 0, L"", 0},
			{ DI_TEXT, 0, rc.Bottom, rc.Right, rc.Bottom, 0, {}, DIF_CENTERTEXT | DIF_SHOWAMPERSAND, 0, hint.c_str(), 0},
			{ DI_TEXT, rc.Left + 1, rc.Top, rc.Left + 1, rc.Top, 0, {}, DIF_SHOWAMPERSAND, 0, L"", 0},
			{ DI_TEXT, rc.Right - 1, rc.Top, rc.Right - 1, rc.Top, 0, {}, DIF_SHOWAMPERSAND, 0, L"", 0},
		};

		HANDLE dlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, rc.Right, rc.Bottom,
							 L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ImageDlgProc, (LONG_PTR)&iv);

		auto exit_code = EXITED_DUE_ERROR;
		if (dlg != INVALID_HANDLE_VALUE) {
			exit_code = (EXITED_DUE)g_far.DialogRun(dlg);
			g_far.DialogFree(dlg);
		}

		switch (exit_code) {
			case EXITED_DUE_RESIZE:
				continue;
			case EXITED_DUE_ENTER:
				if (selection) {
					*selection = iv.GetSelection();
				}
				break;
			case EXITED_DUE_GOTO_CURFILE:
				if (goto_file) {
					*goto_file = iv.CurFile();
				}
				break;
			case EXITED_DUE_ERROR:
				if (!silent_exit_on_error) {
					std::wstring ws_cur_file = L"\"" + StrMB2Wide(all_files[initial_file].first) + L"\"";
					std::wstring werr_str = StrMB2Wide(iv.ErrorString());
					ShowError({g_settings.Msg(M_FAILED_TO_LOAD_IMAGE), ws_cur_file, werr_str});
				}
				break;
			case EXITED_DUE_CANCELLED:
			case EXITED_DUE_ESCAPE:
				break;
		}
		return exit_code;
	}
}

EXITED_DUE ShowImageAtFull(size_t initial_file, std::vector<std::pair<std::string, bool>> &all_files, std::unordered_set<std::string> &selection, bool silent_exit_on_error, std::string *goto_file)
{
	return ShowImageAtFullInternal(initial_file, all_files, &selection, silent_exit_on_error, goto_file);
}

EXITED_DUE ShowImageAtFull(const std::string &file, bool silent_exit_on_error, std::string *goto_file)
{
	std::vector<std::pair<std::string, bool>> all_files{{file, false}};
	return ShowImageAtFullInternal(0, all_files, nullptr, silent_exit_on_error, goto_file);
}

namespace
{
	void ReplaceAll(std::string& str, std::string_view from, std::string_view to)
	{
		if (from.empty()) {
			return;
		}

		for (std::size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += to.length()) {
			str.replace(pos, from.length(), to);
		}
	}

	struct ScopedImageHider
	{
		ImageView* viewer;
		explicit ScopedImageHider(ImageView* v) : viewer(v)
		{
			WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
		}
		ScopedImageHider(const ScopedImageHider&) = delete;
		ScopedImageHider& operator=(const ScopedImageHider&) = delete;
		~ScopedImageHider()
		{
			if (viewer) {
				viewer->ForceShow();
			}
		}
		void dismiss() {
			viewer = nullptr;
		}
	};

	enum ExifDlgItem
	{
		EXIF_DOUBLEBOX_IDX,
		EXIF_MEMO_IDX,
	};

	void SetExifDlgItemPositions(HANDLE hDlg, int dlg_width, int dlg_height)
	{
		SMALL_RECT doublebox_rect  = {0, 0, (SHORT)(dlg_width - 1), (SHORT)(dlg_height - 1)};
		SMALL_RECT memo_rect       = {1, 1, (SHORT)(dlg_width - 2), (SHORT)(dlg_height - 2)};
		g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, EXIF_DOUBLEBOX_IDX, reinterpret_cast<LONG_PTR>(&doublebox_rect));
		g_far.SendDlgMessage(hDlg, DM_SETITEMPOSITION, EXIF_MEMO_IDX,      reinterpret_cast<LONG_PTR>(&memo_rect));
	}

	LONG_PTR WINAPI ExifDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
	{
		switch (Msg) {
			case DN_INITDIALOG: {
				SMALL_RECT dlg_rect{};
				if (g_far.SendDlgMessage(hDlg, DM_GETDLGRECT, 0, reinterpret_cast<LONG_PTR>(&dlg_rect))) {
					SetExifDlgItemPositions(hDlg, dlg_rect.Right - dlg_rect.Left + 1, dlg_rect.Bottom - dlg_rect.Top + 1);
				}
				return TRUE;
			}
			case DN_DRAGGED: {
				return FALSE;
			}
			case DN_RESIZECONSOLE: {
				const COORD *console_size = reinterpret_cast<const COORD *>(Param2);
				g_far.SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(console_size));
				SetExifDlgItemPositions(hDlg, console_size->X, console_size->Y);
				if (bool *resized = reinterpret_cast<bool *>(g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0))) {
					*resized = true;
				}
				return TRUE;
			}
			default: {
				return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
			}
		}
	}
}

bool ImageViewAtFull::ShowExifInfo()
{
	ScopedImageHider guard(this);

	ToolExec exiftool(CancelFlag());
	exiftool.AddArguments("exiftool", "-charset", "UTF8", "-g", "--", CurFile());
	if (!exiftool.Run(CurFile(), CurFileSizeStr(), "exiftool", "Reading EXIF metadata...")) {
		return false;
	}
	if (exiftool.ExecError() != 0) {
		return false;
	}

	const std::wstring exiftool_output = StrMB2Wide(exiftool.FetchStdout());
	if (exiftool_output.empty()) {
		ShowError({g_settings.Msg(M_NO_EXIF_OR_UNSUPPORTED_FORMAT)});
		return false;
	}

	SMALL_RECT far_rect{};
	if (!g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &far_rect, 0)) {
		return false;
	}

	std::wstring dlg_title = g_settings.Msg(M_TITLE);
	dlg_title += g_settings.Msg(M_MEDIA_METADATA);

	FarDialogItem items[2]{};
	items[EXIF_DOUBLEBOX_IDX].Type    = DI_DOUBLEBOX;
	items[EXIF_DOUBLEBOX_IDX].PtrData = dlg_title.c_str();
	items[EXIF_MEMO_IDX].Type         = DI_MEMOEDIT;
	items[EXIF_MEMO_IDX].Flags        = DIF_READONLY | DIF_FOCUS;
	items[EXIF_MEMO_IDX].PtrData      = exiftool_output.c_str();

	bool resized = false;
	HANDLE hdlg = g_far.DialogInit(g_far.ModuleNumber, far_rect.Left, far_rect.Top, far_rect.Right, far_rect.Bottom, nullptr, items, ARRAYSIZE(items), 0, 0, ExifDlgProc, reinterpret_cast<LONG_PTR>(&resized));
	if (hdlg != INVALID_HANDLE_VALUE) {
		g_far.DialogRun(hdlg);
		g_far.DialogFree(hdlg);
	}
	if (resized) {
		guard.dismiss();
	}
	return resized;
}

void ImageViewAtFull::ShowGpsInfo()
{
	ScopedImageHider guard(this);

	ToolExec exiftool(CancelFlag());
	exiftool.AddArguments("exiftool", "-n", "-T", "-GPSLatitude", "-GPSLongitude", "--", CurFile());

	if (!exiftool.Run(CurFile(), CurFileSizeStr(), "exiftool", "Reading GPS metadata...")) {
		return;
	}
	if (exiftool.ExecError() != 0) {
		return;
	}

	double latitude = 0.0, longitude = 0.0;
	std::istringstream stream(exiftool.FetchStdout());
	stream.imbue(std::locale::classic());

	if (!(stream >> latitude >> longitude)) {
		ShowError({g_settings.Msg(M_NO_GPS_METADATA_FOUND)});
		return;
	}

	auto format_coords = [](double val) {
		std::ostringstream oss;
		oss.imbue(std::locale::classic());
		oss << std::fixed << std::setprecision(6) << val;
		return oss.str();
	};

	const std::string lat     = format_coords(latitude);
	const std::string lon     = format_coords(longitude);
	const std::string abs_lat = format_coords(std::abs(latitude));
	const std::string abs_lon = format_coords(std::abs(longitude));
	const std::string lat_dir = (latitude >= 0) ? "N" : "S";
	const std::string lon_dir = (longitude >= 0) ? "E" : "W";

	struct MapProvider
	{
		std::wstring name;
		std::string url_template;
	};

	static const std::vector<MapProvider> providers = {
		{ L"2gis",          "https://2gis.ru/geo/{lon},{lat}" },
		{ L"Apple Maps",    "https://maps.apple.com/?q=loc:{lat},{lon}"},
		{ L"Bing Maps",     "https://www.bing.com/maps?where1={lat},{lon}&lvl=15" },
		{ L"GeoHack",       "https://geohack.toolforge.org/geohack.php?params={abs_lat}_{lat_dir}_{abs_lon}_{lon_dir}_" },
		{ L"Google Maps",   "https://www.google.com/maps/search/?api=1&query={lat},{lon}"},
		{ L"OpenStreetMap", "https://www.openstreetmap.org/?mlat={lat}&mlon={lon}"},
		{ L"Organic Maps",  "https://omaps.app/{lat},{lon}"},
		{ L"Wikimapia",     "https://wikimapia.org/#lat={lat}&lon={lon}&z=11&m=w"},
		{ L"Yandex Maps",   "https://yandex.com/maps?whatshere%5Bpoint%5D={lon},{lat}"},
	};
	static int s_last_provider_idx = 0;

	auto build_url = [&](int idx) {
		std::string url = providers[static_cast<size_t>(idx)].url_template;
		const std::pair<std::string_view, std::string_view> replacements[] = {
			{"{lat}", lat},
			{"{lon}", lon},
			{"{abs_lat}", abs_lat},
			{"{abs_lon}", abs_lon},
			{"{lat_dir}", lat_dir},
			{"{lon_dir}", lon_dir}
		};
		for (const auto& [gps_tag, gps_val] : replacements) {
			ReplaceAll(url, gps_tag, gps_val);
		}
		return StrMB2Wide(url);
	};

	std::vector<FarMenuItem> menu_items(providers.size());
	for (size_t i = 0; i < providers.size(); ++i) {
		menu_items[i].Text = providers[i].name.c_str();
	}

	const std::wstring ws_coords = StrMB2Wide(abs_lat) + L"\u00B0" + StrMB2Wide(lat_dir) + L", "
			+ StrMB2Wide(abs_lon) + L"\u00B0" + StrMB2Wide(lon_dir);
	wchar_t menu_title[256];
	swprintf(menu_title, ARRAYSIZE(menu_title), g_settings.Msg(M_OPEN_GPS_COORDINATES_IN), ws_coords.c_str());

	constexpr int BREAK_KEYS[] = { MAKELONG('C', PKF_CONTROL), 0 };

	int active_menu_idx = s_last_provider_idx;
	for (;;) {
		int break_code = -1;
		menu_items[active_menu_idx].Selected = 1;
		const int selected_menu_idx = g_far.Menu(g_far.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_CHANGECONSOLETITLE, menu_title,
									  L"Enter Ctrl+C", nullptr, BREAK_KEYS, &break_code, menu_items.data(), menu_items.size());
		menu_items[active_menu_idx].Selected = 0;

		if (selected_menu_idx < 0) {
			return; // Esc/F10
		}

		active_menu_idx = selected_menu_idx;
		const std::wstring url = build_url(active_menu_idx);

		if (break_code == 0) { // Ctrl+C
			// Copy URL of currently highlighted item, keep menu open.
			g_fsf.CopyToClipboard(url.c_str());
			continue;
		}

		s_last_provider_idx = active_menu_idx;
		const std::wstring url_quoted = L"'" + url + L"'";
		g_fsf.Execute(url_quoted.c_str(), EF_OPEN | EF_NOWAIT | EF_HIDEOUT | EF_NOCMDPRINT);
		return;
	}
}