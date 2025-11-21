#include "Common.h"
#include "ImageViewer.h"
#include "ExecAsync.h"

#define HINT_STRING "[Navigate: PGUP PGDN HOME | Pan: TAB CURSORS NUMPAD DEL + - * / = | Select: SPACE | Deselect: BS | Toggle: INS | ENTER | ESC]"

// how long msec wait before showing progress message window
#define COMMAND_TIMEOUT_BEFORE_MESSAGE 300
// how long msec wait between checking for skip keypress while command is running
#define COMMAND_TIMEOUT_CHECK_PRESS 100
// how long msec wait after gracefull kill before doing kill -9
#define COMMAND_TIMEOUT_HARD_KILL 300

// keep following settings across plugin invokations
DefaultScale g_def_scale{DS_EQUAL_SCREEN};
static std::set<std::wstring> s_warned_tools;
static std::atomic<int> s_in_progress_dialog{0};

class ToolExec : public ExecAsync
{
	std::atomic<bool> _exited{false};

public:
	void ErrorDialog(const char *pkg, int err)
	{
		std::wstring ws_tool;
		const auto &args = GetArguments();
		if (!args.empty()) {
			ws_tool = StrMB2Wide(args.front());
		}
		if (s_warned_tools.insert(ws_tool).second) {
			const auto &ws_pkg = MB2Wide(pkg);
			const wchar_t *MsgItems[] = { PLUGIN_TITLE,
				L"Failed to run tool:", ws_tool.c_str(),
				L"Please install package:", ws_pkg.c_str(),
				L"Ok"
			};
			errno = err;
			g_far.Message(g_far.ModuleNumber, FMSG_WARNING | FMSG_ERRORTYPE, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		}
	}

	void InfoDialog(const char *pkg)
	{
		std::wstring tmp;
		tmp = PLUGIN_TITLE L" - operation details\n";
		tmp+= L"Package: ";
		tmp+= MB2Wide(pkg);
		tmp+= L'\n';
		tmp+= L"Command:";
		for (const auto &a : GetArguments()) {
			tmp+= L" \"";
			StrMB2Wide(a, tmp, true);
			tmp+= L'"';
		}
		g_far.Message(g_far.ModuleNumber, FMSG_MB_OK | FMSG_ALLINONE, nullptr, (const wchar_t * const *) tmp.c_str(), 0, 0);
	}

	void ProgressDialog(const std::string &file, const std::string &size_str, const char *pkg, const std::string &info)
	{
		WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);

		wchar_t buf[0x100]{};
		swprintf(buf, ARRAYSIZE(buf) - 1, PLUGIN_TITLE L"\nFile of %s:\n", size_str.c_str());
		std::wstring tmp = buf;
		StrMB2Wide(file, tmp, true);
		tmp+= L'\n';
		StrMB2Wide(info, tmp, true);
		tmp+= L'\n';
		tmp+= L"\n&Skip";
		tmp+= L"\n&Info";
		++s_in_progress_dialog;
		while (!_exited && g_far.Message(g_far.ModuleNumber,
				FMSG_ALLINONE, nullptr, (const wchar_t * const *) tmp.c_str(), 0, 2) == 1) {
			InfoDialog(pkg);
		}
		--s_in_progress_dialog;
	}

	static VOID sCallback(VOID *Context)
	{
		// callbacks called withing UI thread, so there can be no race condition at dialog creation/s_in_progress_dialog counter update
		if (s_in_progress_dialog != 0) { // inject ESCAPE keypress that will close dialog
			DWORD dw;
			INPUT_RECORD ir{KEY_EVENT, {}};
			ir.Event.KeyEvent.bKeyDown = 1;
			ir.Event.KeyEvent.wRepeatCount = 1;
			ir.Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
			ir.Event.KeyEvent.wVirtualScanCode = 0;
			ir.Event.KeyEvent.uChar.UnicodeChar = 0;
			ir.Event.KeyEvent.dwControlKeyState = 0;
			WINPORT(WriteConsoleInput)(NULL, &ir, 1, &dw);
			ir.Event.KeyEvent.bKeyDown = 0;
			WINPORT(WriteConsoleInput)(NULL, &ir, 1, &dw);
		}
	}

	// return false in case tool run dismissed by user, otherwise always return true
	bool FN_PRINTF_ARGS(5) Run(const std::string &file, const std::string &size_str, const char *pkg, const char *info_fmt, ...)
	{
		if (Start() && !Wait(COMMAND_TIMEOUT_BEFORE_MESSAGE)) {
			va_list args;
			va_start(args, info_fmt);
			const std::string &info = StrPrintfV(info_fmt, args);
			va_end(args);

			ProgressDialog(file, size_str, pkg, info);
			if (!_exited) {
				KillSoftly();
				if (!Wait(COMMAND_TIMEOUT_HARD_KILL)) {
					KillHardly();
					Wait();
				}
			} else {
				Wait();
			}
			// purge injected escape that could remain or anything else user could press
			PurgeAccumulatedKeyPresses();
		}
		if (ExecError() != 0) {
			ErrorDialog(pkg, ExecError());
		}
		return true;
	}

	virtual void *ThreadProc()
	{
		void *out = ExecAsync::ThreadProc();
		_exited = true;
		DWORD dw;
		INPUT_RECORD ir{CALLBACK_EVENT, {}};
		ir.Event.CallbackEvent.Function = sCallback;
		WINPORT(WriteConsoleInput)(NULL, &ir, 1, &dw);
		return out;
	}
};

////////////////// ImageViewer

void ImageViewer::RotatePixelData(bool clockwise)
{
	std::vector<char> new_pixel_data(_pixel_data.size());
	for (int y = 0; y < _pixel_data_h; ++y) {
		for (int x = 0; x < _pixel_data_w; ++x) {
			const size_t dst_ofs = size_t(x) * _pixel_data_h + (clockwise ? _pixel_data_h - 1 - y : y);
			const size_t src_ofs = size_t(y) * _pixel_data_w + (clockwise ? x : _pixel_data_w - 1 - x);
			for (unsigned i = 0; i < _pixel_size; ++i) {
				new_pixel_data[dst_ofs * _pixel_size + i ] = _pixel_data[src_ofs * _pixel_size + i];
			}
		}
	}
	std::swap(_pixel_data_w, _pixel_data_h);
	_pixel_data.swap(new_pixel_data);
}

unsigned int ImageViewer::EnsureRotated()
{
	int rotated_angle = 0;
	for (;_rotated < _rotate; ++_rotated) {
		RotatePixelData(true);
		rotated_angle++;
	}
	for (;_rotated > _rotate; --_rotated) {
		RotatePixelData(false);
		rotated_angle--;
	}
	if (_rotated == 4 || _rotated == -4) {
		_rotated = _rotate = 0;
	}
	while (rotated_angle < 0) {
		rotated_angle+= 4;
	}
	return rotated_angle;
}

void ImageViewer::ErrorMessage()
{
	std::wstring ws_cur_file = L"\"" + StrMB2Wide(_cur_file) + L"\"";
	std::wstring werr_str = StrMB2Wide(_err_str);
	const wchar_t *MsgItems[] = { PLUGIN_TITLE,
		L"Failed to load image file:",
		ws_cur_file.c_str(),
		werr_str.c_str(),
		L"Ok"
	};
	g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
}

bool ImageViewer::IterateFile(bool forward)
{
	auto it = std::find(_all_files.begin(), _all_files.end(), _cur_file);
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
	JustReset();
	return true;
}

bool ImageViewer::IsVideoFile() const
{
	const char *video_extensions[] = {
				".3g2",  ".3gp",  ".asf",  ".avchd", ".avi",
				".divx", ".enc",  ".flv",  ".ifo",   ".m1v",  ".m2ts",
				".m2v",  ".m4p",  ".m4v",  ".mkv",   ".mov",  ".mp2",
				".mp4",  ".mpe",  ".mpeg", ".mpg",   ".mpv",  ".mts",
				".ogm",  ".qt",   ".ra",   ".ram",   ".rmvb", ".swf",
				".ts",   ".vob",  ".vob",  ".webm",  ".wm",   ".wmv" };
	for (const auto &video_ext : video_extensions) {
		if (StrEndsBy(_cur_file, video_ext)) {
			return true;
		}
	}
	return false;
}

bool ImageViewer::IdentifyImage()
{
	DenoteState("Analyzing...");
	_orig_w = _orig_h = 0; // clear info about image size for title

	ToolExec identify;
	identify.AddArguments("identify", "-format", "%w %h", "--", _render_file);
	if (!identify.Run(_cur_file, _file_size_str, "imagemagick", "Obtaining picture size...")) {
		return false;
	}

	const std::string &dims_str = identify.FetchStdout();
	if (sscanf(dims_str.c_str(), "%d %d", &_orig_w, &_orig_h) != 2 || _orig_w <= 0 || _orig_w <= 0) {
		_orig_w = _orig_h = 0; // clear info about image size for title
		std::string stderr_str = identify.FetchStderr();
		StrTrim(stderr_str, " \t\r\n");
		_err_str = StrPrintf("Failed to parse original dimensions. Got: '%s' '%s'", dims_str.c_str(), stderr_str.c_str());
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		_selection.erase(_cur_file); // remove non-loadable files from _selection
		return false;
	}

	return true;
}

bool ImageViewer::PrepareImage()
{
	_render_file = _cur_file;
	_pixel_data.clear();

	struct stat st {};
	if (stat(_cur_file.c_str(), &st) == -1 || !S_ISREG(st.st_mode) || st.st_size == 0) {
		_selection.erase(_cur_file); // remove non-loadable files from _selection
		return false;
	}

	StrWide2MB(FileSizeString(st.st_size), _file_size_str);

	if (!IsVideoFile()) {
		return IdentifyImage();
	}

	DenoteState("Transforming...");

	ToolExec ffprobe;
	ffprobe.AddArguments("ffprobe", "-v", "error", "-select_streams", "v:0", "-count_packets",
		"-show_entries", "stream=nb_read_packets", "-of", "csv=p=0", "--",  _cur_file);

	if (!ffprobe.Run(_cur_file, _file_size_str, "ffmpeg", "Obtaining video frames count...")) {
		return false;
	}
	const auto &frames_count = ffprobe.FetchStdout();

	fprintf(stderr, "\n--- ImageViewer: frames_count=%s\n", frames_count.c_str());

	unsigned int frames_count_i = atoi(frames_count.c_str());
	unsigned int frames_interval = frames_count_i / 6;
	if (frames_interval < 5) frames_interval = 5;

	if (_tmp_file.empty()) {
		wchar_t preview_tmp[MAX_PATH + 1]{};
		g_fsf.MkTemp(preview_tmp, MAX_PATH, L"far2l-img");
		_tmp_file = StrWide2MB(preview_tmp);
		_tmp_file+= ".jpg";
	}

	unlink(_tmp_file.c_str());

	ToolExec ffmpeg;
	ffmpeg.DontCare();
	ffmpeg.AddArguments("ffmpeg", "-i", _cur_file,
		"-vf", StrPrintf("select='not(mod(n,%d))',scale=200:-1,tile=3x2", frames_interval), _tmp_file);
	if (!ffmpeg.Run(_cur_file, _file_size_str, "ffmpeg",
			"Obtaining 6 video frames of %d for preview...", frames_count_i)) {
		return false;
	}

	if (stat(_tmp_file.c_str(), &st) == -1 || st.st_size == 0) {
		unlink(_tmp_file.c_str());
		_selection.erase(_cur_file); // remove non-loadable files from _selection
		return false;
	}

	_render_file = _tmp_file;
	return IdentifyImage();
}

bool ImageViewer::ConvertImage()
{
	int resize_w = _orig_w, resize_h = _orig_h;

	ToolExec convert;

	convert.AddArguments("convert", "--", _render_file);
	if (fabs(_scale - 1) > 0.01) {
		resize_w = double(resize_w) * _scale;
		resize_h = double(resize_h) * _scale;
		convert.AddArguments("-resize", std::to_string(int(_scale * 100)) + "%");
	}
	convert.AddArguments("-print", "%w %h:", "-depth", "8", "rgb:-");

	fprintf(stderr, "Image dimensions: original=%dx%d wanted=%dx%d [scale=%f]\n",
		_orig_w, _orig_h, resize_w, resize_h, _scale);

	if (!convert.Run(_cur_file, _file_size_str, "imagemagick", "Convering picture...")) {
		return false;
	}
	convert.FetchStdout(_pixel_data);
	size_t print_end = 0;
	while (print_end < _pixel_data.size() && print_end < 32 && _pixel_data[print_end] != ':') {
		++print_end;
	}
	if (print_end < _pixel_data.size()) {
		_pixel_data[print_end] = 0;
		fprintf(stderr, "Obtained dimensions: %s\n", _pixel_data.data());
		sscanf(_pixel_data.data(), "%d %d", &resize_w, &resize_h);
		_pixel_data.erase(_pixel_data.begin(), _pixel_data.begin() + print_end + 1);
	}

	const size_t pixels_count = size_t(resize_w) * resize_h;
	if (_pixel_data.size() != pixels_count * _pixel_size) {
		_err_str = "ImageMagick 'convert' failed";
		fprintf(stderr, "ERROR: %s. _pixel_data.size()=%lu while expected=%lu (%u * %u * %d)\n",
			_err_str.c_str(), (unsigned long)_pixel_data.size(), (unsigned long)pixels_count * 4, resize_w, resize_h, _pixel_size);
		_selection.erase(_cur_file); // remove non-loadable files from _selection
		_pixel_data.clear();
		return false;
	}

	_pixel_data_scale = _scale;
	_pixel_data_w = resize_w;
	_pixel_data_h = resize_h;
	_rotated = 0;
	return true;
}

static int ShiftPercentsToPixels(int &percents, int width, int limit)
{
	int pixels = long(width) * percents / 100;
	if (abs(pixels) > limit) {
		pixels = (percents < 0) ? -limit : limit;
		percents = long(pixels) * 100 / width;
		percents+= (percents > 0) ? 1 : -1;
	}
	return pixels;
}

void ImageViewer::Blit(int cpy_w, int cpy_h,
		char *dst, int dst_left, int dst_top, int dst_width,
		const char *src, int src_left, int src_top, int src_width)
{
	for (int y = 0; y < cpy_h; ++y) {
		memcpy(
			&dst[((dst_top + y) * dst_width + dst_left) * _pixel_size],
			&src[((src_top + y) * src_width + src_left) * _pixel_size],
			cpy_w * _pixel_size);
	}
}

bool ImageViewer::RenderImage()
{
	fprintf(stderr, "\n--- ImageViewer: '%s' ---\n", _render_file.c_str());

	if (_render_file.empty()) {
		_err_str = "bad file";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}

	fprintf(stderr, "Target cell grid _pos=%dx%d _size=%dx%d\n", _pos.X, _pos.Y, _size.X, _size.Y);

	if (_pos.X < 0 || _pos.Y < 0 || _size.X <= 0 || _size.Y <= 0) {
		_err_str = "bad grid";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}

	WinportGraphicsInfo wgi{};

	if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(wgi), &wgi)) {
		_err_str = "GetConsoleImageCaps failed";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}
	if ((wgi.Caps & WP_IMGCAP_RGBA) == 0) {
		_err_str = "backend doesn't support graphical output";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}
	int canvas_w = int(_size.X) * wgi.PixPerCell.X;
	int canvas_h = int(_size.Y) * wgi.PixPerCell.Y;

	if (_scale <= 0) {
		auto rotated_orig_w = _orig_w;
		auto rotated_orig_h = _orig_h;
		if ((_rotate % 2) != 0) {
			std::swap(rotated_orig_w, rotated_orig_h);
		}

		_scale_max = ceil(std::max(double(4 * canvas_w) / rotated_orig_w, double(4 * canvas_h) / rotated_orig_h));
		if (g_def_scale == DS_EQUAL_IMAGE) {
			_scale = 1.0;
		} else if (g_def_scale == DS_EQUAL_SCREEN || canvas_w < rotated_orig_w || canvas_h < rotated_orig_h) {
			_scale = std::min(double(canvas_w) / double(rotated_orig_w), double(canvas_h) / double(rotated_orig_h));
		} else {
			_scale = 1.0;
		}
	}

	DenoteState("Rendering...");

	const bool do_convert = (_pixel_data.empty() || fabs(_scale -_pixel_data_scale) > 0.01);
	if (do_convert) {
		if (!ConvertImage()) {
			return false;
		}
	}

	unsigned int rotated_angle = EnsureRotated();

	auto viewport_w = canvas_w;
	auto viewport_h = canvas_h;
	SMALL_RECT area = {_pos.X, _pos.Y, 0, 0};

	if (viewport_w > _pixel_data_w) {
		auto margin = (viewport_w - _pixel_data_w) / 2;
		area.Left+= margin / wgi.PixPerCell.X;  // offset by cells count
		area.Right = margin % wgi.PixPerCell.X; // extra pixel offset due to WP_IMG_PIXEL_OFFSET
		viewport_w = _pixel_data_w;
	}

	if (viewport_h > _pixel_data_h) {
		auto margin = (viewport_h - _pixel_data_h) / 2;
		area.Top+= margin / wgi.PixPerCell.Y;    // offset by cells count
		area.Bottom = margin % wgi.PixPerCell.Y; // extra pixel offset due to WP_IMG_PIXEL_OFFSET
		viewport_h = _pixel_data_h;
	}

	int src_left = (_pixel_data_w > viewport_w) ? (_pixel_data_w - viewport_w) / 2 : 0;
	int src_top = (_pixel_data_h > viewport_h) ? (_pixel_data_h - viewport_h) / 2 : 0;

	if (_dx != 0 && _pixel_data_w > viewport_w) {
		src_left+= ShiftPercentsToPixels(_dx, _pixel_data_w, (_pixel_data_w - viewport_w) / 2);
	} else {
		_dx = 0;
	}

	if (_dy != 0 && _pixel_data_h > viewport_h) {
		src_top+= ShiftPercentsToPixels(_dy, _pixel_data_h, (_pixel_data_h - viewport_h) / 2);
	} else {
		_dy = 0;
	}

	if (!do_convert && _prev_left == src_left && _prev_top == src_top && rotated_angle == 0) {
		fprintf(stderr, "--- Nothing to do\n");
		return true;
	}

	bool out = true;

	if (rotated_angle != 0 && !do_convert && (wgi.Caps & WP_IMGCAP_ROTATE) != 0
			&& _pixel_data_w <= std::min(canvas_w, canvas_h)
			&& _pixel_data_h <= std::min(canvas_w, canvas_h)) {
		if (WINPORT(RotateConsoleImage)(NULL, WINPORT_IMAGE_ID, &area, rotated_angle)) {
			rotated_angle = 0; // no need to rotate anything else
		}
	}

	if (!do_convert && rotated_angle == 0 && (wgi.Caps & WP_IMGCAP_SCROLL) != 0
			&& abs(_prev_left - src_left) < viewport_w && abs(_prev_top - src_top) < viewport_h) {

		if (_prev_left != src_left && out) {
			DWORD ins_w = abs(_prev_left - src_left);
			_send_data.resize( size_t(ins_w) * viewport_h * _pixel_size);
			if (_prev_left > src_left) {
				Blit(ins_w, viewport_h,
					_send_data.data(), 0, 0, ins_w,
					_pixel_data.data(), src_left, _prev_top, _pixel_data_w);
				fprintf(stderr, "--- Sending to left edge [%d %d %d %d]\n",
					src_left, _prev_top, src_left + ins_w, _prev_top + viewport_h);
				out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB | WP_IMG_PIXEL_OFFSET
					| WP_IMG_SCROLL_AT_LEFT, &area, ins_w, viewport_h, _send_data.data()) != FALSE;
			} else {
				Blit(ins_w, viewport_h,
					_send_data.data(), 0, 0, ins_w,
					_pixel_data.data(), src_left + viewport_w - ins_w, _prev_top, _pixel_data_w);
				fprintf(stderr, "--- Sending to right edge [%d %d %d %d]\n",
					src_left + viewport_w - ins_w, _prev_top, src_left + viewport_w, _prev_top + viewport_h);
				out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB | WP_IMG_PIXEL_OFFSET
					| WP_IMG_SCROLL_AT_RIGHT, &area, ins_w, viewport_h, _send_data.data()) != FALSE;
			}
		}
		if (_prev_top != src_top && out) {
			DWORD ins_h = abs(_prev_top - src_top);
			_send_data.resize( size_t(ins_h) * viewport_w * _pixel_size);
			if (_prev_top > src_top) {
				Blit(viewport_w, ins_h,
					_send_data.data(), 0, 0, viewport_w,
					_pixel_data.data(), src_left, src_top, _pixel_data_w);
				fprintf(stderr, "--- Sending to top edge [%d %d %d %d]\n",
					src_left, src_top, src_left + viewport_w, src_top + ins_h);
				out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB | WP_IMG_PIXEL_OFFSET
					| WP_IMG_SCROLL_AT_TOP, &area, viewport_w, ins_h, _send_data.data()) != FALSE;
			} else {
				Blit(viewport_w, ins_h,
					_send_data.data(), 0, 0, viewport_w,
					_pixel_data.data(), src_left, src_top + viewport_h - ins_h, _pixel_data_w);
				fprintf(stderr, "--- Sending to bottom edge [%d %d %d %d]\n",
					src_left, src_top + viewport_h - ins_h, src_left + viewport_w, src_top + viewport_h);
				out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB | WP_IMG_PIXEL_OFFSET
					| WP_IMG_SCROLL_AT_BOTTOM, &area, viewport_w, ins_h, _send_data.data()) != FALSE;
			}
		}
	} else {
		_send_data.resize(viewport_w * viewport_h * _pixel_size);
		memset(_send_data.data(), 0, _send_data.size());
		const int cpy_w = std::min(viewport_w, _pixel_data_w);
		const int cpy_h = std::min(viewport_h, _pixel_data_h);

		Blit(cpy_w, cpy_h,
			_send_data.data(), 0, 0, viewport_w,
			_pixel_data.data(), src_left, src_top, _pixel_data_w);

		fprintf(stderr, "--- Sending to full viewport [%d %d %d %d]\n",
			src_left, src_top, src_left + viewport_w, src_top + viewport_h);
		out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB | WP_IMG_PIXEL_OFFSET,
			&area, viewport_w, viewport_h, _send_data.data()) != FALSE;
	}
	if (out) {
		_prev_left = src_left;
		_prev_top = src_top;
	}
	return out;
}

void ImageViewer::SetTitleAndStatus(const std::string &title, const std::string &status)
{
	std::wstring ws_title = (_selection.find(_cur_file) != _selection.end()) ? L"* " : L"  ";
	StrMB2Wide(title, ws_title, true);
	FarDialogItemData dd_title = { ws_title.size(), (wchar_t*)ws_title.c_str() };

	if (_dlg != INVALID_HANDLE_VALUE) {
		if (_selection.find(_cur_file) == _selection.end()) {
			g_far.SendDlgMessage(_dlg, DM_SETTEXT, 0, (LONG_PTR)&dd_title);
			g_far.SendDlgMessage(_dlg, DM_SHOWITEM, 0, 1);
			g_far.SendDlgMessage(_dlg, DM_SHOWITEM, 1, 0);
		} else {
			g_far.SendDlgMessage(_dlg, DM_SETTEXT, 1, (LONG_PTR)&dd_title);
			g_far.SendDlgMessage(_dlg, DM_SHOWITEM, 0, 0);
			g_far.SendDlgMessage(_dlg, DM_SHOWITEM, 1, 1);
		}

		// update status after title, so it will get redrawn after too, and due to that - will remain visible
		std::wstring ws_status = StrMB2Wide(status);
		FarDialogItemData dd_status = { ws_status.size(), (wchar_t*)ws_status.c_str() };
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 3, (LONG_PTR)&dd_status);
	}
}

void ImageViewer::DenoteState(const char *stage)
{
	std::string title = _cur_file;
	if (stage) {
		title+= " [";
		title+= stage;
		title+= ']';
	} else {
		std::string title2;
		if (_orig_w > 0 && _orig_h > 0)
			title2 = std::to_string(_orig_w) + 'x' + std::to_string(_orig_h);
		if (!_file_size_str.empty())
			title2+= (title2.empty() ? "" : ", ") + _file_size_str;
		if (!title2.empty())
			title+= " (" + title2 + ')';
	}

	std::string status = HINT_STRING;

	char prefix[32];

	if (_dx != 0 || _dy != 0) {
		snprintf(prefix, ARRAYSIZE(prefix), "%s%d:%s%d ", (_dx > 0) ? "+" : "", _dx, (_dy > 0) ? "+" : "", _dy);
		status.insert(0, prefix);
	}

	if (fabs(_scale - 1) > 0.01) {
		snprintf(prefix, ARRAYSIZE(prefix), "%d%% ", int(_scale * 100));
		status.insert(0, prefix);
	}

	if (_rotate != 0) {
		snprintf(prefix, ARRAYSIZE(prefix), "%d° ", _rotate * 90);
		status.insert(0, prefix);
	}

	SetTitleAndStatus(title, status);
}

void ImageViewer::JustReset()
{
	_dx = _dy = 0;
	_scale = -1;
	_rotate = 0;
}

///////////////////// ImageViewer PUBLICs

ImageViewer::ImageViewer(const std::string &initial_file, const std::vector<std::string> &all_files, const std::set<std::string> &selection)
	:
	_initial_file(initial_file),
	_cur_file(initial_file),
	_selection(selection),
	_all_files(all_files)
{
	if (std::find(_all_files.begin(), _all_files.end(), initial_file) == _all_files.end())
		_all_files.push_back(initial_file);
}

ImageViewer::~ImageViewer()
{
	WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
	if (!_tmp_file.empty()) {
		unlink(_tmp_file.c_str());
	}
}


bool ImageViewer::Setup(SMALL_RECT &rc, HANDLE dlg)
{
	_dlg = dlg;
	_pos.X = 1;
	_pos.Y = 1;
	_size.X = rc.Right > 1 ? rc.Right - 1 : 1;
	_size.Y = rc.Bottom > 1 ? rc.Bottom - 1 : 1;

	_pixel_data.clear();
	JustReset();

	_err_str.clear();
	if (!PrepareImage() || !RenderImage()) {
		if (dlg != INVALID_HANDLE_VALUE) { // show error dialog only if not quick-view mode
			ErrorMessage();
		}
		return false;
	}
	DenoteState();

	return true;
}

void ImageViewer::Home()
{
	_cur_file = _initial_file;
	if (PrepareImage() && RenderImage()) {
		DenoteState();
	}
}

bool ImageViewer::Iterate(bool forward)
{
	for (size_t i = 0;; ++i) { // silently skip bad files
		if (!IterateFile(forward) || i > _all_files.size()) {
			_cur_file.clear();
			return false; // bail out on logic error or infinite loop
		}
		if (PrepareImage() && RenderImage()) {
			DenoteState();
			return true;
		}
	}
}

void ImageViewer::Scale(int change)
{
	double ds = 0;
	if (change > 0) {
		if (_scale < 1) ds = change;
		else if (_scale < 2) ds = change * 5;
		else if (_scale < _scale_max) ds = change * 10;
	} else if (change < 0) {
		if (_scale > 2) ds = change * 10;
		else if (_scale > 1) ds = change * 5;
		else if (_scale > 0.1) ds = change;
	} else {
		return;
	}
	ds/= 100.0;
	fprintf(stderr, "Scale: %f + %f\n", _scale, ds);
	_scale+= ds;
	if (_scale < 0.1) {
		_scale = 0.1;
	} else if (_scale > _scale_max) {
		_scale = _scale_max;
	} else if (fabs(_scale - 1.0) < 0.01) {
		_scale = 1.0;
	}

	RenderImage();
	DenoteState();
}

void ImageViewer::Rotate(int change)
{
	_rotate+= (change > 0) ? 1 : -1;
	RenderImage();
	DenoteState();
}

void ImageViewer::Shift(int horizontal, int vertical)
{
	if (horizontal != 0) {
		_dx+= horizontal;
		if (_dx > 100) _dx = 100;
		if (_dx < -100) _dx = -100;
	}
	if (vertical != 0) {
		_dy+= vertical;
		if (_dy > 100) _dy = 100;
		if (_dy < -100) _dy = -100;
	}
	RenderImage();
	DenoteState();
}

void ImageViewer::Reset()
{
	JustReset();
	RenderImage();
	DenoteState();
}

void ImageViewer::Select()
{
	if (_selection.insert(_cur_file).second) {
		DenoteState();
	}
}

void ImageViewer::Deselect()
{
	if (_selection.erase(_cur_file)) {
		DenoteState();
	}
}

void ImageViewer::ToggleSelection()
{
	if (!_selection.erase(_cur_file)) {
		_selection.insert(_cur_file);
	}
	DenoteState();
}

