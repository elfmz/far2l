#include "headers.hpp"
#include <assert.h>
#include "vtansi_kitty.h"
#include "base64.h"

static std::string KittyID(int id)
{
	return StrPrintf("KITTY:%d", id);
}

VTAnsiKitty::VTAnsiKitty(IVTShell *vts) : _vts(vts)
{
	fprintf(stderr, "VTAnsiKitty was born\n");
}

VTAnsiKitty::~VTAnsiKitty()
{
	fprintf(stderr, "VTAnsiKitty destroyed\n");
	for (auto &it : _images) {
		if (it.second.shown) {
			WINPORT(DeleteConsoleImage)(NULL, KittyID(it.first).c_str());
		}
	}
}

const char *VTAnsiKitty::ShowImage(int id, Image &img)
{
	WinportGraphicsInfo wgi{};
	if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(wgi), &wgi)
			|| (wgi.Caps & WP_IMGCAP_RGBA) == 0 || wgi.PixPerCell.Y == 0) {
		return "BACKEND_UNSUPPORTED";
	}
	CONSOLE_SCREEN_BUFFER_INFO  csbi{};
	WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi );

	DWORD64 flags = 0;
	if (img.fmt == 100) {
		flags = WP_IMG_PNG;
	} else if (img.fmt == 24) {
		flags = WP_IMG_RGB;
		if (img.data.size() < size_t(img.width) * img.height * 3) {
			return "TRUNCATED_RGB";
		}
	} else if (img.fmt == 32) {
		flags = WP_IMG_RGBA;
		if (img.data.size() < size_t(img.width) * img.height * 4) {
			return "TRUNCATED_RGBA";
		}
	} else {
		return "BAD_FMT";
	}
	SMALL_RECT area{-1, -1,
			SHORT(img.rows > 0 ? csbi.dwCursorPosition.X + img.rows - 1 : -1),
			SHORT(img.cols > 0 ? csbi.dwCursorPosition.Y + img.cols - 1 : -1)
	};
	if (img.cols < 0 && img.rows < 0 && (img.ofsx > 0 || img.ofsy > 0)) {
		area.Right = img.ofsx;
		area.Bottom = img.ofsy;
		flags|= WP_IMG_PIXEL_OFFSET;
	}
	if (_images_hide_cnt == 0) {
		if (img.fmt == 100) {
			if (!WINPORT(SetConsoleImage)(NULL, KittyID(id).c_str(),
					flags, &area, img.data.size(), 1, img.data.data())) {
				return "BACKEND_ERROR";
			}
		} else if (!WINPORT(SetConsoleImage)(NULL, KittyID(id).c_str(),
				flags, &area, img.width, img.height, img.data.data())) {
			return "BACKEND_ERROR";
		}
	}
	img.shown = true;

	return nullptr;
}

const char *VTAnsiKitty::AddImage(char action, char medium,
		int id, int fmt, int width, int height, int rows, int cols, int ofsx, int ofsy, int more, const char *b64data, size_t b64len)
{
	if (medium != 0 && medium != 'd') { // only escape codes are supported
		return "BAD_MEDIUM";
	}
	if (fmt > 0 && fmt != 24 && fmt != 32 && fmt != 100) {
		return "BAD_FORMAT";
	}
	if (action == 'q') {
		return nullptr;
	}
	auto &img = _images[id];
	if (img.shown) {
		img.data.clear();
		img.shown = false;
		img.show = false;
	}
	if (action == 'T') {
		img.show = true;
	}
	if (fmt > 0) {
		img.fmt = fmt;
	}
	if (width > 0) {
		img.width = width;
	}
	if (height > 0) {
		img.height = height;
	}
	if (rows > 0) {
		img.rows = rows;
	}
	if (rows > 0) {
		img.cols = cols;
	}
	if (ofsx > 0) {
		img.ofsx = ofsx;
	}
	if (ofsy > 0) {
		img.ofsy = ofsy;
	}

	base64_decode(img.data, b64data, b64len);
	if (!img.show || more) {
		return nullptr;
	}
	const char *err = ShowImage(id, img);
	if (!err) {
		return nullptr;
	}
	_images.erase(id);
	return err;
}

const char *VTAnsiKitty::DisplayImage(int id)
{
	auto it = _images.find(id);
	if (it == _images.end()) {
		return "BAD_ID";
	}
	it->second.show = true;
	const char *err = ShowImage(id, it->second);
	if (err) {
		_images.erase(it);
		return err;
	}
	return nullptr;
}

const char *VTAnsiKitty::RemoveImage(int id)
{
	if (!_images.erase(id)) {
		return "BAD_ID";
	}

	if (!WINPORT(DeleteConsoleImage)(NULL, KittyID(id).c_str())) {
		return "BACKEND_ERROR";
	}

	return nullptr;
}

class KittyArgs
{
	const char *_str;
	int _vals['z' - 'A' + 1];
	int _data_pos;

	static int C2VI(char c)
	{
		if (c >= 'A' && c <= 'z') {
			return c - 'A';
		}
		return -1;
	}

public:
	KittyArgs(const char *s, int l)
		:
		_str(s),
		_data_pos(l) // by default set data pos to the end of string
	{
		for (auto &v : _vals) {
			v = -1;
		}
		for (int i = 0, j = 0; ; ++i) {
			if (i == l || s[i] == ';' || s[i] == ',') {
				if (i + 1 > j && s[j + 1] == '=') {
					int vi = C2VI(s[j]);
					if (vi >= 0) {
						_vals[vi] = j + 2;
					}
				}
				if (i == l) {
					break;
				}
				if (s[i] == ';') {
					_data_pos = i + 1;
					break;
				}
				j = i + 1;
			}
		}
		fprintf(stderr, "KITTY: '%.*s'\n", _data_pos, s);
	}

	int GetInt(char c, int def = -1) const
	{
		int vi = C2VI(c);
		if (vi < 0 || _vals[vi] < 0) {
			return def;
		}
		int out = 0;
		for (int i = _vals[vi]; i < _data_pos; ++i) {
			if (_str[i] < '0' || _str[i] > '9') {
				break;
			}
			out*= 10;
			out+= _str[i] - '0';
		}
		return out;
	}

	char GetChar(char c, char def = 0) const
	{
		int vi = C2VI(c);
		if (vi < 0 || _vals[vi] < 0) {
			return def;
		}
		return  _str[_vals[vi]];
	}

	int GetDataPos() const
	{
		return _data_pos;
	}
};

void VTAnsiKitty::InterpretControlString(const char *s, size_t l)
{
	//a=T,f=%u,t=d,s=%u,v=%u,i=%u,m=%u;AAAA
	//a=d,d=I,i=%u
	KittyArgs ka(s, l);
	const char *error = NULL;
	switch (ka.GetChar('a')) {
		case 0: case 't': case 'T': case 'q': {
			error = AddImage(
				ka.GetChar('a'),
				ka.GetChar('t'), // target
				ka.GetInt('i'), // image ID
				ka.GetInt('f'), // format: 24, 32, 100
				ka.GetInt('s'), // width pixels
				ka.GetInt('v'), // height pixels
				ka.GetInt('c'), // columns (display width in cells)
				ka.GetInt('r'), // rows (display height in cells)
				ka.GetInt('X'), // in-cell pixel X-offset
				ka.GetInt('Y'), // in-cell pixel Y-offset
				ka.GetInt('m'), // more to follow
				s + ka.GetDataPos(), l - ka.GetDataPos()); 
		} break;
		case 'p':
			error = DisplayImage(ka.GetChar('i'));
			break;
		case 'd':
			error = RemoveImage(ka.GetChar('i'));
			break;
	}
	if (ka.GetInt('i') <= 0) {
		return;
	}
	char q = ka.GetChar('q');
	if (q == '2' || (q == '1' && !error)) {
		return;
	}
	
	std::string response = StrPrintf("\e_Gi=%d;", ka.GetInt('i'));
	response+= error ? error : "OK";
	response+= "\e\\";
	_vts->InjectInput(response.c_str());
}

bool VTAnsiKitty::HasImages() const
{
	for (const auto &it : _images) {
		if (it.second.shown) {
			return true;
		}
	}
	return false;
}

void VTAnsiKitty::HideImages()
{
	++_images_hide_cnt;
	if (_images_hide_cnt == 1) {
		for (auto &it : _images) {
			if (it.second.shown) {
				WINPORT(DeleteConsoleImage)(NULL, KittyID(it.first).c_str());
			}
		}
	}
}

void VTAnsiKitty::ShowImages()
{
	assert(_images_hide_cnt > 0);
	--_images_hide_cnt;
	if (_images_hide_cnt == 0) {
		for (auto &it : _images) {
			if (it.second.shown) {
				ShowImage(it.first, it.second);
			}
		}
	}
}

