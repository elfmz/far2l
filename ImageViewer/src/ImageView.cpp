#include "Common.h"
#include "ImageView.h"
#include "Settings.h"
#include "ToolExec.h"

// following constants used for incremental image display moderation
#define SETIMG_INITALLY_ASSUMED_SPEED    8192 // 8kb per ms = 8MB/second
#define SETIMG_DELAY_BASELINE_MSEC       256  // approx msec user may need to wait for cancellation
#define SETIMG_ESTIMATION_SIZE_THRESHOLD 0x10000 // minimal size of data that can be used for set image rate estimation

unsigned int ImageView::EnsureRotated()
{
	int rotated_angle = 0;
	for (;_rotated < _rotate; ++_rotated) {
		_ready_image.Rotate(_tmp_image, true);
		_ready_image.Swap(_tmp_image);
		rotated_angle++;
	}
	for (;_rotated > _rotate; --_rotated) {
		_ready_image.Rotate(_tmp_image, false);
		_ready_image.Swap(_tmp_image);
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

bool ImageView::IterateFile(bool forward)
{
	if (forward) {
		++_cur_file;
		if (_cur_file >= _all_files.size()) {
			_cur_file = 0;
		}
	} else if (_cur_file > 0) {
		--_cur_file;
	} else {
		_cur_file = _all_files.size() - 1;
	}
	return true;
}

bool ImageView::IsVideoFile() const
{
	return g_settings.MatchVideoFile(CurFile().c_str());
}

bool ImageView::PrepareImage()
{
	_render_file = CurFile();
	_orig_image.Resize();

	struct stat st {};
	if (stat(_render_file.c_str(), &st) == -1 || !S_ISREG(st.st_mode) || st.st_size == 0) {
		_all_files[_cur_file].second = false; // silently unselect non-loadable files
		return false;
	}

	StrWide2MB(FileSizeString(st.st_size), _file_size_str);

	if (!IsVideoFile()) {
		return ReadImage();
	}

	DenoteState("Transforming...");

	ToolExec ffprobe(_cancel);
	ffprobe.AddArguments("ffprobe", "-v", "error", "-select_streams", "v:0", "-count_packets",
		"-show_entries", "stream=nb_read_packets", "-of", "csv=p=0", "--",  _render_file);

	if (!ffprobe.Run(_render_file, _file_size_str, "ffmpeg", "Obtaining video frames count...")) {
		return false;
	}
	const auto &frames_count = ffprobe.FetchStdout();

	fprintf(stderr, "%s: frames_count=%s\n", __FUNCTION__, frames_count.c_str());

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

	ToolExec ffmpeg(_cancel);
	ffmpeg.DontCare();
	ffmpeg.AddArguments("ffmpeg", "-i", _render_file,
		"-vf", StrPrintf("select='not(mod(n,%d))',scale=200:-1,tile=3x2", frames_interval), _tmp_file);
	if (!ffmpeg.Run(_render_file, _file_size_str, "ffmpeg",
			"Obtaining 6 video frames of %d for preview...", frames_count_i)) {
		return false;
	}

	if (stat(_tmp_file.c_str(), &st) == -1 || st.st_size == 0) {
		unlink(_tmp_file.c_str());
		_all_files[_cur_file].second = false; // silently unselect non-loadable files
		return false;
	}

	_render_file = _tmp_file;
	return ReadImage();
}

bool ImageView::ReadImage()
{
	const bool use_orientation = g_settings.UseOrientation();

	ToolExec convert(_cancel);

	convert.AddArguments("convert", "--", _render_file,
		"-print", use_orientation ? "%w %h %[exif:orientation]:" : "%w %h :",
		"-depth", "8",
		"rgb:-");

	if (!convert.Run(CurFile(), _file_size_str, "imagemagick", "Convering picture...")) {
		return false;
	}
	std::vector<char> stdout_data;
	convert.FetchStdout(stdout_data);
	// expecting "WIDTH HEIGHT:" followed by RGB data
	size_t print_end = 0;
	while (print_end < stdout_data.size() && print_end < 32 && stdout_data[print_end] != ':') {
		++print_end;
	}
	if (print_end == stdout_data.size()) {
		fprintf(stderr, "%s: no colon in convert output\n", __FUNCTION__);
		_err_str = "ImageMagick 'convert' failed";
		return false;
	}
	stdout_data[print_end] = 0;
	int width = -1, height = -1, orientation = -1;
	int scanned_args = sscanf(stdout_data.data(), "%d %d %d", &width, &height, &orientation);
	if (scanned_args < 2 || width < 0 || height < 0) {
		fprintf(stderr, "%s: bad convert dimensions - '%s'\n", __FUNCTION__, stdout_data.data());
		_err_str = "ImageMagick 'convert' failed";
		return false;
	}
	const unsigned char bytes_per_pixel = 3; // only 24 bit RGB for now
	const size_t expected_stdout_size = size_t(width) * height * bytes_per_pixel + print_end + 1;
	if (stdout_data.size() < expected_stdout_size) {
		fprintf(stderr, "%s: truncated output data - %lu < %lu\n", __FUNCTION__,
			(unsigned long)stdout_data.size(), (unsigned long)expected_stdout_size);
		_err_str = "ImageMagick 'convert' failed";
		return false;
	}
	if (stdout_data.size() > expected_stdout_size) {
		fprintf(stderr, "%s: excessive output data - %lu > %lu\n", __FUNCTION__,
			(unsigned long)stdout_data.size(), (unsigned long)expected_stdout_size);
	}

	_orig_image.Resize(width, height, bytes_per_pixel);
	_orig_image.Assign(stdout_data.data() + print_end + 1);
	_ready_image.Resize();
	_ready_image_scale = -1;
	_scale = -1;
	_rotate = 0;
	_rotated = 0;
	if (use_orientation && orientation > 0) {
		switch (orientation) {
			case 8: // Rotate 270 CW
				_rotate = -1;
				break;
			case 7: // Mirror horizontal and rotate 90 CW
				_orig_image.MirrorH();
				_rotate = 1;
				break;
			case 6: // Rotate 90 CW
				_rotate = 1;
				break;
			case 5: // Mirror horizontal and rotate 270 CW
				_orig_image.MirrorH();
				_rotate = -1;
				break;
			case 4: // irror vertical
				_orig_image.MirrorV();
				break;
			case 3: // Rotate 180
				_rotate = 2;
				break;
			case 2: // Mirror horizontal
				_orig_image.MirrorH();
				break;
			case 1: // Normal
				break;

			default:
				fprintf(stderr, "%s: unsupported orientation - %d\n", __FUNCTION__, orientation);
		}
	}
	fprintf(stderr, "%s: loaded image of %d x %d orientation=%d\n", __FUNCTION__, width, height, orientation);
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

bool ImageView::RefreshWGI()
{
	if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(_wgi), &_wgi)) {
		_err_str = "GetConsoleImageCaps failed";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}
	if ((_wgi.Caps & WP_IMGCAP_RGBA) == 0) {
		_err_str = "backend doesn't support RGBA graphics";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}
	if (_wgi.PixPerCell.X <= 0 || _wgi.PixPerCell.Y <= 0) {
		_err_str = StrPrintf("bad cell size ( %d x %d )", _wgi.PixPerCell.X, _wgi.PixPerCell.Y);
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}
	return true;
}

void ImageView::SetupInitialScale(const int canvas_w, const int canvas_h)
{
	auto rotated_orig_w = _orig_image.Width();
	auto rotated_orig_h = _orig_image.Height();
	if ((_rotate % 2) != 0) {
		std::swap(rotated_orig_w, rotated_orig_h);
	}

	_scale_fit = std::min(double(canvas_w) / double(rotated_orig_w), double(canvas_h) / double(rotated_orig_h));
	_scale_max = std::max(_scale_fit * 4.0, 2.0);
	_scale_min = std::min(_scale_fit / 8.0, 0.5);

	const auto set_defscale = g_settings.GetDefaultScale();
	if (set_defscale == Settings::EQUAL_IMAGE) {
		_scale = 1.0;
	} else if (set_defscale == Settings::EQUAL_SCREEN || canvas_w < rotated_orig_w || canvas_h < rotated_orig_h) {
		_scale = _scale_fit;
	} else {
		_scale = 1.0;
	}

	fprintf(stderr, "%s: min=%f fit=%f max=%f\n", __FUNCTION__, _scale_min, _scale_fit, _scale_max);
}

bool ImageView::SendWholeImage(const SMALL_RECT *area, const Image &img)
{
	// using WP_IMG_ATTACH_BOTTOM if WP_IMGCAP_ATTACH available for
	// incremental image display and quick cancellation on slow connections
	static std::atomic<size_t> s_avg_speed{};
	size_t avg_speed = s_avg_speed;
	if (!avg_speed) {
		avg_speed = SETIMG_INITALLY_ASSUMED_SPEED;
	}
	auto msec = GetProcessUptimeMSec();
	auto chunk_h = img.Height();
	if ((_wgi.Caps & WP_IMGCAP_ATTACH) != 0 && avg_speed != 0) {
		auto estimated_time = img.Size() / avg_speed;
		if (estimated_time >= 2 * SETIMG_DELAY_BASELINE_MSEC) {
			chunk_h = std::max(32, int(img.Height() / (estimated_time / SETIMG_DELAY_BASELINE_MSEC)));
		}
	}

	for (int sent_h = 0; sent_h < img.Height(); ) {
		if ((_cancel && *_cancel) || (!_cancel && CheckForEscAndPurgeAccumulatedInputEvents())) {
			fprintf(stderr, "%s: cancelled at %lu of %lu\n", __FUNCTION__, (unsigned long)sent_h, (unsigned long)img.Height());
			_err_str = "manually cancelled";
			return false;
		}
		auto set_h = img.Height() - sent_h;
		if (set_h > chunk_h + chunk_h / 4) {
			set_h = chunk_h;
		}
		if (!WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID,
				WP_IMG_RGB | WP_IMG_PIXEL_OFFSET | (sent_h ? WP_IMG_ATTACH_BOTTOM : 0),
				area, img.Width(), set_h, img.Data(sent_h * img.Width() * 3))) {
			fprintf(stderr, "%s: error at %d of %d\n", __FUNCTION__, sent_h, img.Height());
			_err_str = "failed to send image to terminal";
			return false;
		}
//		usleep(set_h * viewport_w * 8); // uncomment to simulate some slowness
		sent_h+= set_h;
	}
	msec = GetProcessUptimeMSec() - msec;
	if (img.Size() >= SETIMG_ESTIMATION_SIZE_THRESHOLD && msec >= 1) {
		const size_t cur_speed = std::max(img.Size() / msec, size_t(1));
		size_t speed = s_avg_speed;
		if (speed < cur_speed || (speed - cur_speed > speed / 4) || speed == 0) {
			speed+= cur_speed;
			if (speed != cur_speed) {
				speed/= 2;
			}
			fprintf(stderr, "Imaging rate: %lu -> %lu b/ms; Now: %lu bytes in %lu ms\n",
				(unsigned long)s_avg_speed, (unsigned long)speed, (unsigned long)img.Size(), (unsigned long)msec);
			s_avg_speed = speed;
		}
	}
	return true;
}

bool ImageView::SendWholeViewport(const SMALL_RECT *area, int src_left, int src_top, int viewport_w, int viewport_h)
{
	fprintf(stderr, "ImageView: sending viewport at [%d %d %d %d]\n",
		src_left, src_top, src_left + viewport_w, src_top + viewport_h);

	if (src_left == 0 && src_top == 0 && viewport_w == _ready_image.Width() && viewport_h == _ready_image.Height()) {
		return SendWholeImage(area, _ready_image);
	}

	_tmp_image.Resize(viewport_w, viewport_h, _ready_image.BytesPerPixel());
	_ready_image.Blit(_tmp_image, 0, 0, viewport_w, viewport_h, src_left, src_top);
	return SendWholeImage(area, _tmp_image);
}

bool ImageView::SendScrollAttachH(const SMALL_RECT *area, int src_left, int src_top, int viewport_w, int viewport_h, int delta)
{
	_tmp_image.Resize(abs(delta), viewport_h, _ready_image.BytesPerPixel());

	DWORD64 flags = WP_IMG_RGB | WP_IMG_PIXEL_OFFSET | WP_IMG_SCROLL;
	if (delta > 0) {
		fprintf(stderr, "ImageView: scrolling by %d from left of [%d %d %d %d]\n",
			delta, src_left, src_top, src_left + viewport_w, src_top + viewport_h);
		_ready_image.Blit(_tmp_image, 0, 0, delta, viewport_h, src_left, src_top);
		flags|= WP_IMG_ATTACH_LEFT;
	} else {
		fprintf(stderr, "ImageView: scrolling by %d from right of [%d %d %d %d]\n",
			-delta, src_left, src_top, src_left + viewport_w, src_top + viewport_h);
		_ready_image.Blit(_tmp_image, 0, 0, -delta, viewport_h, src_left + viewport_w + delta, src_top);
		flags|= WP_IMG_ATTACH_RIGHT;
	}

	return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, flags, area,
		_tmp_image.Width(), _tmp_image.Height(), _tmp_image.Data()) != FALSE;
}

bool ImageView::SendScrollAttachV(const SMALL_RECT *area, int src_left, int src_top, int viewport_w, int viewport_h, int delta)
{
	_tmp_image.Resize(viewport_w, abs(delta), _ready_image.BytesPerPixel());

	DWORD64 flags = WP_IMG_RGB | WP_IMG_PIXEL_OFFSET | WP_IMG_SCROLL;
	if (delta > 0) {
		fprintf(stderr, "ImageView: scrolling by %d from top of [%d %d %d %d]\n",
			delta, src_left, src_top, src_left + viewport_w, src_top + viewport_h);
		_ready_image.Blit(_tmp_image, 0, 0, viewport_w, delta, src_left, src_top);
		flags|= WP_IMG_ATTACH_TOP;
	} else {
		fprintf(stderr, "ImageView: scrolling by %d from bottom of [%d %d %d %d]\n",
			-delta, src_left, src_top, src_left + viewport_w, src_top + viewport_h);
		_ready_image.Blit(_tmp_image, 0, 0, viewport_w, -delta, src_left, src_top + viewport_h + delta);
		flags|= WP_IMG_ATTACH_BOTTOM;
	}

	return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, flags, area,
		_tmp_image.Width(), _tmp_image.Height(), _tmp_image.Data()) != FALSE;
}


bool ImageView::EnsureRescaled() // return true if image was rescaled, otherwise false, suppress OOM errors for now
{
	if (_ready_image_scale <= 0 || fabs(_scale -_ready_image_scale) >= 0.01) {
		assert(_scale > 0);
		try {
			_orig_image.Scale(_ready_image, _scale);
			fprintf(stderr, "%s: Rescaled by %f - %d x %d -> %d x %d\n", __FUNCTION__,
				_scale, _orig_image.Width(), _orig_image.Height(), _ready_image.Width(), _ready_image.Height());
			_ready_image_scale = _scale;
			_rotated = 0;
			return true;

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
		} catch (...) {
			fprintf(stderr, "%s: ...\n", __FUNCTION__);
		}
		_ready_image.Resize();
	}
	return false;
}

bool ImageView::RenderImage()
{
	fprintf(stderr, "%s: _pos=%dx%d _size=%dx%d '%s'\n",
		__FUNCTION__, _pos.X, _pos.Y, _size.X, _size.Y, _render_file.c_str());

	if (_render_file.empty()) {
		_err_str = "bad file";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}

	if (_pos.X < 0 || _pos.Y < 0 || _size.X <= 0 || _size.Y <= 0) {
		_err_str = "bad grid";
		fprintf(stderr, "ERROR: %s.\n", _err_str.c_str());
		return false;
	}

	if (!RefreshWGI()) {
		return false;
	}

	int canvas_w = int(_size.X) * _wgi.PixPerCell.X;
	int canvas_h = int(_size.Y) * _wgi.PixPerCell.Y;

	if (_scale <= 0) {
		DenoteState("Rendering...");
		SetupInitialScale(canvas_w, canvas_h);
	}

	const bool rescaled = EnsureRescaled();

	unsigned int rotated_angle = EnsureRotated();

	auto viewport_w = canvas_w;
	auto viewport_h = canvas_h;
	SMALL_RECT area = {_pos.X, _pos.Y, 0, 0};

	if (viewport_w > _ready_image.Width()) {
		auto margin = (viewport_w - _ready_image.Width()) / 2;
		area.Left+= margin / _wgi.PixPerCell.X;  // offset by cells count
		area.Right = margin % _wgi.PixPerCell.X; // extra pixel offset due to WP_IMG_PIXEL_OFFSET
		viewport_w = _ready_image.Width();
	}

	if (viewport_h > _ready_image.Height()) {
		auto margin = (viewport_h - _ready_image.Height()) / 2;
		area.Top+= margin / _wgi.PixPerCell.Y;    // offset by cells count
		area.Bottom = margin % _wgi.PixPerCell.Y; // extra pixel offset due to WP_IMG_PIXEL_OFFSET
		viewport_h = _ready_image.Height();
	}

	int src_left = (_ready_image.Width() > viewport_w) ? (_ready_image.Width() - viewport_w) / 2 : 0;
	int src_top = (_ready_image.Height() > viewport_h) ? (_ready_image.Height() - viewport_h) / 2 : 0;

	if (_dx != 0 && _ready_image.Width() > viewport_w) {
		src_left+= ShiftPercentsToPixels(_dx, _ready_image.Width(), (_ready_image.Width() - viewport_w) / 2);
	} else {
		_dx = 0;
	}

	if (_dy != 0 && _ready_image.Height() > viewport_h) {
		src_top+= ShiftPercentsToPixels(_dy, _ready_image.Height(), (_ready_image.Height() - viewport_h) / 2);
	} else {
		_dy = 0;
	}

	if (!rescaled && _prev_left == src_left && _prev_top == src_top && rotated_angle == 0) {
		fprintf(stderr, "%s: Nothing to do\n", __FUNCTION__);
		return true;
	}

	if (rotated_angle != 0 && !rescaled && (_wgi.Caps & WP_IMGCAP_ROTATE) != 0
			&& _ready_image.Width() <= std::min(canvas_w, canvas_h)
			&& _ready_image.Height() <= std::min(canvas_w, canvas_h)) {
		fprintf(stderr, "ImageView: rotating remote image\n");
		if (WINPORT(RotateConsoleImage)(NULL, WINPORT_IMAGE_ID, &area, rotated_angle)) {
			rotated_angle = 0; // no need to rotate anything else
		}
	}

	bool out = true;
	if (!rescaled && rotated_angle == 0 && (_wgi.Caps & WP_IMGCAP_SCROLL) != 0 && (_wgi.Caps & WP_IMGCAP_ATTACH) != 0 
			&& abs(_prev_left - src_left) < viewport_w && abs(_prev_top - src_top) < viewport_h) {
		if (_prev_left != src_left) {
			out = SendScrollAttachH(&area, src_left, _prev_top, viewport_w, viewport_h, _prev_left - src_left);
		}
		if (_prev_top != src_top) {
			out = SendScrollAttachV(&area, src_left, src_top, viewport_w, viewport_h, _prev_top - src_top);
		}
	} else {
		out = SendWholeViewport(&area, src_left, src_top, viewport_w, viewport_h);
	}
	if (out) {
		_prev_left = src_left;
		_prev_top = src_top;
	}
	return out;
}

void ImageView::DenoteState(const char *stage)
{
	std::string info;
	if (stage) {
		info = stage;
	} else {
		if (_orig_image.Width() != 0 || _orig_image.Height() != 0) {
			info = std::to_string(_orig_image.Width()) + 'x' + std::to_string(_orig_image.Height());
		}
		if (!_file_size_str.empty()) {
			info+= (info.empty() ? "" : ", ") + _file_size_str;
		}
	}

	if (!info.empty()) {
		info.insert(0, 1, ' ');
		info+= ' ';
	}

	std::string pan;
	if (_scale > 0) {
		const char c1 = (_scale - _scale_fit > 0.01) ? '>' : ((_scale - _scale_fit < -0.01) ? '<' : '[');
		const char c2 = (_scale - _scale_fit > 0.01) ? '<' : ((_scale - _scale_fit < -0.01) ? '>' : ']');
		pan+= StrPrintf("%c%d%%%c ", c1, int(_scale * 100), c2);
	}

	if (_dx != 0 || _dy != 0) {
		pan+= StrPrintf("%s%d:%s%d ", (_dx > 0) ? "+" : "", _dx, (_dy > 0) ? "+" : "", _dy);
	}
	if (_rotate != 0) {
		pan+= StrPrintf("%d° ", _rotate * 90);
	}
	if (!pan.empty()) {
		pan.insert(0, 1, ' ');
	}

	DenoteInfoAndPan(info, pan);
}

void ImageView::DenoteInfoAndPan(const std::string &info, const std::string &pan)
{
	fprintf(stderr, "DenoteInfoAndPan: %s'%s' '%s' '%s'\n",
		CurFileSelected() ? "*" : "", CurFile().c_str(), info.c_str(), pan.c_str());
}


void ImageView::JustReset()
{
	_dx = _dy = 0;
	_scale = -1;
	_rotate = 0;
}

///////////////////// ImageView PUBLICs

ImageView::ImageView(size_t initial_file, const std::vector<std::pair<std::string, bool> > &all_files)
	:
	_all_files(all_files),
	_initial_file(initial_file),
	_cur_file(initial_file)
{
	assert(_initial_file < all_files.size());
}

ImageView::~ImageView()
{
	WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
	if (!_tmp_file.empty()) {
		unlink(_tmp_file.c_str());
	}
}

std::unordered_set<std::string> ImageView::GetSelection() const
{
	std::unordered_set<std::string> out;
	for (const auto &it : _all_files) {
		if (it.second) {
			out.insert(it.first);
		}
	}
	return out;
}

bool ImageView::Setup(SMALL_RECT &rc, volatile bool *cancel)
{
	_cancel = cancel;
	_pos.X = rc.Left;
	_pos.Y = rc.Top;
	_size.X = rc.Right > rc.Left ? rc.Right - rc.Left + 1 : 1;
	_size.Y = rc.Bottom > rc.Top ? rc.Bottom - rc.Top + 1 : 1;

	_orig_image.Resize();
	_ready_image.Resize();
	_tmp_image.Resize();
	JustReset();

	_err_str.clear();
	if (!PrepareImage() || !RenderImage()) {
		return false;
	}
	DenoteState();

	return true;
}

void ImageView::Home()
{
	_cur_file = _initial_file;
	JustReset();
	if (PrepareImage() && RenderImage()) {
		DenoteState();
	}
}

bool ImageView::Iterate(bool forward)
{
	for (size_t i = 0;; ++i) { // silently skip bad files
		if (!IterateFile(forward) || i > _all_files.size()) {
			_cur_file = _initial_file;
			return false; // bail out on logic error or infinite loop
		}
		JustReset();
		if (PrepareImage() && RenderImage()) {
			DenoteState();
			return true;
		}
	}
}

void ImageView::Scale(int change)
{
	double ds = change * 4;
	if (fabs(_scale - _scale_fit) < 0.001) {
		if (change < 0) {
			ds*= (_scale_fit - _scale_min);
		} else {
			ds*= (_scale_max - _scale_fit);
		}
	} else if (_scale <= _scale_fit) {
		ds*= (_scale_fit - _scale_min);
	} else {
		ds*= (_scale_max - _scale_fit);
	}

	auto new_scale = _scale + ds / 100.0;
	if (new_scale < _scale_min) {
		new_scale = _scale_min;
	} else if (new_scale > _scale_max) {
		new_scale = _scale_max;
	}

	auto min_special = std::min(_scale_fit, 1.0);
	auto max_special = std::max(_scale_fit, 1.0);
	if (ds > 0) {
		if (_scale < min_special && new_scale > min_special) {
			new_scale = min_special;
		} else if (_scale < max_special && new_scale > max_special) {
			new_scale = max_special;
		}
	} else if (_scale > max_special && new_scale < max_special) {
		new_scale = max_special;
	} else if (_scale > min_special && new_scale < min_special) {
		new_scale = min_special;
	}
	fprintf(stderr, "Scale: %f -> %f\n", _scale, new_scale);
	if (_scale != new_scale) {
		_scale = new_scale;
		RenderImage();
		DenoteState();
	}
}

void ImageView::Rotate(int change)
{
	_rotate+= (change > 0) ? 1 : -1;
	RenderImage();
	DenoteState();
}

void ImageView::Shift(int horizontal, int vertical)
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

COORD ImageView::ShiftByPixels(COORD delta) // returns actual shift in pixels
{
	int saved_dx = _dx, saved_dy = _dy;

	Shift(int(delta.X) * 100 / _ready_image.Width(), int(delta.Y) * 100 / _ready_image.Height());

	return COORD {
		SHORT((_dx - saved_dx) * _ready_image.Width() / 100),
		SHORT((_dy - saved_dy) * _ready_image.Height() / 100)
	};
}

void ImageView::Reset(bool keep_rotation)
{
	int saved_rotate = _rotate;
	JustReset();
	if (keep_rotation) {
		_rotate = saved_rotate;
	}
	RenderImage();
	DenoteState();
}

void ImageView::Select()
{
	if (!_all_files[_cur_file].second) {
		_all_files[_cur_file].second = true;
		DenoteState();
	}
}

void ImageView::Deselect()
{
	if (_all_files[_cur_file].second) {
		_all_files[_cur_file].second = false;
		DenoteState();
	}
}

void ImageView::ToggleSelection()
{
	_all_files[_cur_file].second = !_all_files[_cur_file].second;
	DenoteState();
}

