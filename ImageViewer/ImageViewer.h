#pragma once
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

class ImageViewer
{
	HANDLE _dlg{};
	std::string _initial_file, _cur_file, _render_file, _tmp_file, _file_size_str;
	std::set<std::string> _selection;
	std::vector<std::string> _all_files;
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

	void RotatePixelData(bool clockwise);
	unsigned int EnsureRotated();
	void ErrorMessage();
	bool IterateFile(bool forward);
	bool IsVideoFile() const;
	bool IdentifyImage();
	bool PrepareImage();
	bool ConvertImage();
	void Blit(int cpy_w, int cpy_h,
			char *dst, int dst_left, int dst_top, int dst_width,
			const char *src, int src_left, int src_top, int src_width);
	bool RenderImage();
	void SetTitleAndStatus(const std::string &title, const std::string &status);
	void DenoteState(const char *stage = NULL);
	void JustReset();

public:
	ImageViewer(const std::string &initial_file, const std::vector<std::string> &all_files, const std::set<std::string> &selection);
	~ImageViewer();

	bool Setup(SMALL_RECT &rc, HANDLE dlg = INVALID_HANDLE_VALUE);
	void Home();
	bool Iterate(bool forward);
	void Scale(int change);
	void Rotate(int change);
	void Shift(int horizontal, int vertical);
	void Reset();
	void Select();
	void Deselect();
	void ToggleSelection();

	const std::set<std::string> &GetSelection() const
	{
		return _selection;
	}
};

