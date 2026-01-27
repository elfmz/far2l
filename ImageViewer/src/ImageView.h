#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <memory>
#include <cstdio>
#include <unordered_set>
#include <signal.h>
#include <dirent.h>
#include <utils.h>
#include <math.h>
#include "Image.h"

class ImageView
{
	volatile bool *_cancel{nullptr};

	std::string _render_file, _tmp_file, _file_size_str;
	std::vector<std::pair<std::string, bool> > _all_files;
	size_t _initial_file{}, _cur_file{};
	WinportGraphicsInfo _wgi{}; // updated during RenderImage before actual rendering

	std::string _err_str;

	Image _orig_image, _ready_image, _tmp_image;
	double _ready_image_scale{0}, _scale{-1}, _scale_fit{-1}, _scale_min{0.1}, _scale_max{4};
	COORD _pos{}, _size{};
	int _prev_left{0}, _prev_top{0};
	int _dx{0}, _dy{0}; // image viewport shift in percents of corresponding _ready_image dimensions
	signed char _rotate{0}, _rotated{0}; // [-3..3]: 0 - not rotated, 1 - rotated by 90, 2 - by 180, -1 - by -90 etc
	bool _mirror_h{false}, _mirrored_h{false};
	bool _mirror_v{false}, _mirrored_v{false};
	bool _force_render{false};

	bool IterateFile(bool forward);
	bool PrepareImage();
	bool ReadImage();
	void ApplyEXIFOrientation(int orientation);

	bool RefreshWGI();
	void SetupInitialScale(const int canvas_w, const int canvas_h);
	bool EnsureReadyAndScaled();
	uint16_t EnsureTransformed();

	bool SendWholeImage(const SMALL_RECT *area, const Image &img);
	bool SendWholeViewport(const SMALL_RECT *area, int src_left, int src_top, int viewport_w, int viewport_h);
	bool SendScrollAttachH(const SMALL_RECT *area, int src_left, int src_top, int viewport_w, int viewport_h, int delta);
	bool SendScrollAttachV(const SMALL_RECT *area, int src_left, int src_top, int viewport_w, int viewport_h, int delta);
	bool RenderImage();
	void DenoteState(const char *stage = NULL);
	void JustReset(bool keep_rotmir = false);

protected:
	virtual void DenoteInfoAndPan(const std::string &info, const std::string &pan);
	bool CurFileSelected() const { return _all_files[_cur_file].second; }
	const std::string &CurFile() const { return _all_files[_cur_file].first; }

public:
	ImageView(size_t initial_file, const std::vector<std::pair<std::string, bool> > &all_files);
	~ImageView();


	const std::string &ErrorString() const { return _err_str; }

	std::unordered_set<std::string> GetSelection() const;

	bool Setup(SMALL_RECT &rc, volatile bool *cancel = nullptr);

	void Home();
	bool Iterate(bool forward);
	void Scale(int change);
	void Rotate(int change);
	void Shift(int horizontal, int vertical);
	COORD ShiftByPixels(COORD delta);
	void MirrorH();
	void MirrorV();
	void Reset(bool keep_rotmir);
	void ForceShow()
	{
		_force_render = true;
		RenderImage();
		DenoteState();
	};
	void Select();
	void Deselect();
	void ToggleSelection();
	void RunProcessingCommand();
};

