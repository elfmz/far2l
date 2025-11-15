#include <farplug-wide.h>
#include <WinPort.h>
#include <farkeys.h>
#include <farcolor.h>

#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <memory>
#include <cstdio>
#include <set>
#include <signal.h>
#include <dirent.h>
#include <utils.h>
#include <math.h>
#include <ExecAsync.h>

#define WINPORT_IMAGE_ID "image_viewer"

#define PLUGIN_TITLE L"Image Viewer"

#define HINT_STRING "[Navigate: PGUP PGDN HOME | Pan: TAB CURSORS NUMPAD DEL + - * / = | Select: SPACE | Deselect: BS | Toggle: INS | ENTER | ESC]"

// how long msec wait before showing progress message window
#define COMMAND_TIMEOUT_BEFORE_MESSAGE 300
// how long msec wait between checking for skip keypress while command is running
#define COMMAND_TIMEOUT_CHECK_PRESS 100
// how long msec wait after gracefull kill before doing kill -9
#define COMMAND_TIMEOUT_HARD_KILL 300

#define EXITED_DUE_ERROR      -1
#define EXITED_DUE_ENTER      42
#define EXITED_DUE_ESCAPE     24
#define EXITED_DUE_RESIZE     37

static PluginStartupInfo g_far;
static FarStandardFunctions g_fsf;

// keep following settings across plugin invokations
static enum DefaultScale {
	DS_EQUAL_SCREEN,
	DS_LESSOREQUAL_SCREEN,
	DS_EQUAL_IMAGE
} s_def_scale{DS_EQUAL_SCREEN};

static std::set<std::wstring> s_warned_tools;


class ImageViewerMessage
{
	HANDLE _h_scr{nullptr};

public:
	ImageViewerMessage(const std::string &file, const std::string &size_str, const std::string &info)
	{
		WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
		_h_scr = g_far.SaveScreen(0, 0, -1, -1);
		wchar_t buf[0x100]{};
		swprintf(buf, ARRAYSIZE(buf) - 1, PLUGIN_TITLE L"\nFile of %s:\n", size_str.c_str());
		std::wstring tmp = buf;
		StrMB2Wide(file, tmp, true);
		tmp+= L'\n';
		StrMB2Wide(info, tmp, true);
		tmp+= L'\n';
		tmp+= L"\n          <I> - additional details";
		tmp+= L"\n<PgDn>/<PgUp> - skip current file";
		g_far.Message(g_far.ModuleNumber, FMSG_ALLINONE, nullptr, (const wchar_t * const *) tmp.c_str(), 0, 0);
	}

	~ImageViewerMessage()
	{
		if (_h_scr) {
			g_far.RestoreScreen(_h_scr);
			_h_scr = nullptr;
		}
	}
};

enum ProcessingKeyPress {
	PKP_NONE,
	PKP_INFO,
	PKP_DISMISS,
};

static ProcessingKeyPress CheckForProcessingKeyPress()
{
	WORD KeyCodes[] = {'i', 'I', VK_ESCAPE, VK_NEXT, VK_PRIOR};
	DWORD index = WINPORT(CheckForKeyPress)(NULL, KeyCodes, ARRAYSIZE(KeyCodes),
		CFKP_KEEP_OTHER_EVENTS | CFKP_KEEP_UNMATCHED_KEY_EVENTS | CFKP_KEEP_MOUSE_EVENTS);
	switch (index) {
		case 0:
			return PKP_NONE;
		case 1: case 2:
			return PKP_INFO;
		default:
			return PKP_DISMISS;
	}
}

static void PurgeAccumulatedKeyPresses()
{
	// just purge all currently queued keypresses
	WINPORT(CheckForKeyPress)(NULL, NULL, 0, CFKP_KEEP_OTHER_EVENTS | CFKP_KEEP_MOUSE_EVENTS);
}

struct ToolExec : ExecAsync
{
	void ShowAdditionalInfo(const char *pkg)
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

	// return false in case tool run dismissed by user, otherwise always return true
	bool FN_PRINTF_ARGS(5) Run(const std::string &file, const std::string &size_str, const char *pkg, const char *info_fmt, ...)
	{
		if (Start() && !Wait(COMMAND_TIMEOUT_BEFORE_MESSAGE)) {
			va_list args;
			va_start(args, info_fmt);
			const std::string &info = StrPrintfV(info_fmt, args);
			va_end(args);

			ImageViewerMessage msg(file, size_str, info);
			do {
				switch (CheckForProcessingKeyPress()) {
					case PKP_DISMISS:
						KillSoftly();
						if (!Wait(COMMAND_TIMEOUT_HARD_KILL)) {
							KillHardly();
							Wait();
						}
						return false;

					case PKP_INFO:
						ShowAdditionalInfo(pkg);
						break;
					default: ;
				}
			} while (!Wait(COMMAND_TIMEOUT_CHECK_PRESS));
		}
		if (ExecError() != 0) {
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
				errno = ExecError();
				g_far.Message(g_far.ModuleNumber, FMSG_WARNING|FMSG_ERRORTYPE, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
			}
		}
		return true;
	}
};

class ImageViewer
{
	HANDLE _dlg{};
	std::string _initial_file, _cur_file, _render_file, _tmp_file, _file_size_str;
	std::set<std::string> _selection, _all_files;
	COORD _pos{}, _size{};
	int _dx{0}, _dy{0};
	double _scale{-1}, _scale_max{4};
	int _rotate{0}, _rotated{0};
	int _orig_w{0}, _orig_h{0}; // info about image size for title
	std::string _err_str;

	constexpr static unsigned int _pixel_size = 3; // for now hardcoded RGB

	std::vector<char> _pixel_data, _send_data;
	double _pixel_data_scale{0};
	int _pixel_data_w{0}, _pixel_data_h{0};
	int _prev_left{0}, _prev_top{0};

	void RotatePixelData(bool clockwise)
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

	unsigned int EnsureRotated()
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

	void ErrorMessage()
	{
		std::wstring ws_cur_file = L"\"" + StrMB2Wide(_cur_file) + L"\"";
		std::wstring werr_str = StrMB2Wide(_err_str);
		const wchar_t *MsgItems[] = { PLUGIN_TITLE,
			L"Failed to load image file:",
			ws_cur_file.c_str(),
			werr_str.c_str(),
			L"Ok"};
		g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
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
		JustReset();
		return true;
	}

	bool IsVideoFile() const
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

	bool IdentifyImage()
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

	bool PrepareImage()
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

	bool ConvertImage()
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

	void Blit(int cpy_w, int cpy_h,
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

	bool RenderImage(bool bmess = false)
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
			if (s_def_scale == DS_EQUAL_IMAGE) {
				_scale = 1.0;
			} else if (s_def_scale == DS_EQUAL_SCREEN || canvas_w < rotated_orig_w || canvas_h < rotated_orig_h) {
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
		COORD pos = _pos;

		if (viewport_w > _pixel_data_w) {
			pos.X+= (viewport_w - _pixel_data_w) / (2 * wgi.PixPerCell.X);
			viewport_w = _pixel_data_w;
		}
		if (viewport_h > _pixel_data_h) {
			pos.Y+= (viewport_h - _pixel_data_h) / (2 * wgi.PixPerCell.Y);
			viewport_h = _pixel_data_h;
		}

		int dst_left = (viewport_w > _pixel_data_w) ? (viewport_w - _pixel_data_w) / 2 : 0;
		int src_left = (_pixel_data_w > viewport_w) ? (_pixel_data_w - viewport_w) / 2 : 0;
		int dst_top = (viewport_h > _pixel_data_h) ? (viewport_h - _pixel_data_h) / 2 : 0;
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
			if (WINPORT(RotateConsoleImage)(NULL, WINPORT_IMAGE_ID, pos, rotated_angle)) {
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
					fprintf(stderr, "--- Sending to [%d %d] left edge [%d %d %d %d]\n",
						dst_left, dst_top, src_left, _prev_top, src_left + ins_w, _prev_top + viewport_h);
					out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB
						| WP_IMG_SCROLL_AT_LEFT, pos, ins_w, viewport_h, _send_data.data()) != FALSE;
				} else {
					Blit(ins_w, viewport_h,
						_send_data.data(), 0, 0, ins_w,
						_pixel_data.data(), src_left + viewport_w - ins_w, _prev_top, _pixel_data_w);
					fprintf(stderr, "--- Sending to [%d %d] right edge [%d %d %d %d]\n",
						dst_left, dst_top, src_left + viewport_w - ins_w, _prev_top, src_left + viewport_w, _prev_top + viewport_h);
					out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB
						| WP_IMG_SCROLL_AT_RIGHT, pos, ins_w, viewport_h, _send_data.data()) != FALSE;
				}
			}
			if (_prev_top != src_top && out) {
				DWORD ins_h = abs(_prev_top - src_top);
				_send_data.resize( size_t(ins_h) * viewport_w * _pixel_size);
				if (_prev_top > src_top) {
					Blit(viewport_w, ins_h,
						_send_data.data(), 0, 0, viewport_w,
						_pixel_data.data(), src_left, src_top, _pixel_data_w);
					fprintf(stderr, "--- Sending to [%d %d] top edge [%d %d %d %d]\n",
						dst_left, dst_top, src_left, src_top, src_left + viewport_w, src_top + ins_h);
					out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB
						| WP_IMG_SCROLL_AT_TOP, pos, viewport_w, ins_h, _send_data.data()) != FALSE;
				} else {
					Blit(viewport_w, ins_h,
						_send_data.data(), 0, 0, viewport_w,
						_pixel_data.data(), src_left, src_top + viewport_h - ins_h, _pixel_data_w);
					fprintf(stderr, "--- Sending to [%d %d] bottom edge [%d %d %d %d]\n",
						dst_left, dst_top, src_left, src_top + viewport_h - ins_h, src_left + viewport_w, src_top + viewport_h);
					out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB
						| WP_IMG_SCROLL_AT_BOTTOM, pos, viewport_w, ins_h, _send_data.data()) != FALSE;
				}
			}
		} else {
			_send_data.resize(viewport_w * viewport_h * _pixel_size);
			memset(_send_data.data(), 0, _send_data.size());
			const int cpy_w = std::min(viewport_w, _pixel_data_w);
			const int cpy_h = std::min(viewport_h, _pixel_data_h);

			Blit(cpy_w, cpy_h,
				_send_data.data(), dst_left, dst_top, viewport_w,
				_pixel_data.data(), src_left, src_top, _pixel_data_w);

			fprintf(stderr, "--- Sending to [%d %d] full viewport [%d %d %d %d]\n",
				dst_left, dst_top, src_left, src_top, src_left + viewport_w, src_top + viewport_h);
			out = WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, WP_IMG_RGB,
				pos, viewport_w, viewport_h, _send_data.data()) != FALSE;
		}
		if (out) {
			_prev_left = src_left;
			_prev_top = src_top;
		}
		return out;
	}

	void SetTitleAndStatus(const std::string &title, const std::string &status)
	{
		std::wstring ws_title = (_selection.find(_cur_file) != _selection.end()) ? L"* " : L"  ";
		StrMB2Wide(title, ws_title, true);
		FarDialogItemData dd_title = { ws_title.size(), (wchar_t*)ws_title.c_str() };

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

	void DenoteState(const char *stage = NULL)
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

	void JustReset()
	{
		_dx = _dy = 0;
		_scale = -1;
		_rotate = 0;
	}

public:
	ImageViewer(const std::string &initial_file, const std::set<std::string> &selection)
		:
		_initial_file(initial_file),
		_cur_file(initial_file),
		_selection(selection),
		_all_files(selection)
	{
		if (!_all_files.empty()) {
			_all_files.insert(initial_file);
		}
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

	bool Setup(HANDLE dlg, SMALL_RECT &rc)
	{
		_dlg = dlg;
		_pos.X = 1;
		_pos.Y = 1;
		_size.X = rc.Right > 1 ? rc.Right - 1 : 1;
		_size.Y = rc.Bottom > 1 ? rc.Bottom - 1 : 1;

		_pixel_data.clear();
		JustReset();

		_err_str.clear();
		if (!PrepareImage() || !RenderImage(true)) {
			ErrorMessage();
			return false;
		}
		DenoteState();

		return true;
	}

	void Home()
	{
		_cur_file = _initial_file;
		if (PrepareImage() && RenderImage()) {
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
			if (PrepareImage() && RenderImage()) {
				DenoteState();
				return true;
			}
		}
	}

	void Scale(int change)
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

	void Rotate(int change)
	{
		_rotate+= (change > 0) ? 1 : -1;
		RenderImage();
		DenoteState();
	}

	void Shift(int horizontal, int vertical)
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

	void Reset()
	{
		JustReset();
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

	void ToggleSelection()
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

			if (!iv->Setup(hDlg, Rect)) {
				g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ERROR, 0);
			}
			return TRUE;
		}

		case DN_KEY:
		{
			ImageViewer *iv = (ImageViewer *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const int delta = ((((int)Param2) & KEY_SHIFT) != 0) ? 1 : 10;
			const int key = (int)(Param2 & ~KEY_SHIFT);
			PurgeAccumulatedKeyPresses(); // avoid navigation etc keypresses 'accumulation'
			switch (key) {
				case 'a': case 'A': case KEY_MULTIPLY: case '*':
					s_def_scale = DS_LESSOREQUAL_SCREEN;
					iv->Reset();
					break;
				case 'q': case 'Q': case KEY_DEL: case KEY_NUMDEL:
					s_def_scale = DS_EQUAL_SCREEN;
					iv->Reset();
					break;
				case 'z': case 'Z': case KEY_DIVIDE: case '/':
					s_def_scale = DS_EQUAL_IMAGE;
					iv->Reset();
					break;
				case KEY_CLEAR: case '=': iv->Reset(); break;
				case KEY_ADD: case '+': iv->Scale(delta); break;
				case KEY_SUBTRACT: case '-': iv->Scale(-delta); break;
				case KEY_NUMPAD6: case KEY_RIGHT: iv->Shift(delta, 0); break;
				case KEY_NUMPAD4: case KEY_LEFT: iv->Shift(-delta, 0); break;
				case KEY_NUMPAD2: case KEY_DOWN: iv->Shift(0, delta); break;
				case KEY_NUMPAD8: case KEY_UP: iv->Shift(0, -delta); break;
				case KEY_NUMPAD9: iv->Shift(delta, -delta); break;
				case KEY_NUMPAD1: iv->Shift(-delta, delta); break;
				case KEY_NUMPAD3: iv->Shift(delta, delta); break;
				case KEY_NUMPAD7: iv->Shift(-delta, -delta); break;
				case KEY_TAB: iv->Rotate( (delta == 1) ? -90 : 90); break;
				case KEY_INS: case KEY_NUMPAD0: iv->ToggleSelection(); break;
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

		case DN_RESIZECONSOLE:
			g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
			break;
	}

	return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static bool ShowImage(const std::string &initial_file, std::set<std::string> &selection)
{
	ImageViewer iv(initial_file, selection);

	for (;;) {
		SMALL_RECT Rect;
		g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

		FarDialogItem DlgItems[] = {
			{ DI_SINGLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, 0, 0, L"???", 0 },
			{ DI_DOUBLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, DIF_HIDDEN, 0, L"???", 0 },
			{ DI_USERCONTROL, 1, 1, Rect.Right - 1, Rect.Bottom - 1, 0, {COL_DIALOGBOX}, 0, 0, L"", 0},
			{ DI_TEXT, 0, Rect.Bottom, Rect.Right, Rect.Bottom, 0, {}, DIF_CENTERTEXT, 0, L"", 0},
		};

		HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, Rect.Right, Rect.Bottom,
							 L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ViewerDlgProc, (LONG_PTR)&iv);

		if (hDlg == INVALID_HANDLE_VALUE) {
			return false;
		}

		int exit_code = g_far.DialogRun(hDlg);
		g_far.DialogFree(hDlg);

		if (exit_code != EXITED_DUE_RESIZE) {
			if (exit_code != EXITED_DUE_ENTER) {
				return false;
			}
			selection = iv.GetSelection();
			return true;
		}
	}
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
	PluginMenuStrings[0] = PLUGIN_TITLE;
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
