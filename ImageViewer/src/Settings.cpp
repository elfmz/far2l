#include "Common.h"
#include "Settings.h"
#include <KeyFileHelper.h>
#include <utils.h>
#include <wchar.h>

#define DEFAULT_IMAGE_MASKS \
				"*.ai *.ani *.avif *.bmp *.bw *.cdr *.cel *.cgm *.cmx *.cpt *.cur *.dcx *.dds *.dib " \
				"*.emf *.eps *.flc *.fli *.fpx *.gif *.icl *.ico *.iff *.indd *.j2k *.jp2 *.jpc *.jpe *.jpeg " \
				"*.jpeg2000 *.jpg *.jps *.kra *.lbm *.mng *.mpo *.pbm *.pcx *.pdn *.pgm *.pic *.png *.pns " \
				"*.ppm *.psd *.psp *.ras *.rgb *.rle *.sai *.sgi *.spr *.svg *.tga *.tif *.tiff *.wbmp " \
				"*.webp *.wmf *.xbm *.xcf *.xpm"

#define DEFAULT_VIDEO_MASKS \
				"*.3g2 *.3gp *.asf *.avchd *.avi *.divx *.enc *.flv *.ifo *.m1v *.m2ts " \
				"*.m2v *.m4p *.m4v *.mkv *.mov *.mp2 *.mp4 *.mpe *.mpeg *.mpg *.mpv *.mts " \
				"*.ogm *.qt *.ra *.ram *.rmvb *.swf *.ts *.vob *.vob *.webm *.wm *.wmv"

#define INI_PATH           "plugins/ImageViewer/config.ini"
#define INI_SECTION        "Settings"
#define INI_DEFAULTSCALE   "DefaultScale"
#define INI_USEORIENTATION "UseOrientation"
#define INI_OPENBYENTER    "OpenByEnter"
#define INI_OPENBYCPGDN    "OpenByCtrlPgDn"
#define INI_OPENINQV       "OpenInQV"
#define INI_OPENINFV       "OpenInFV"
#define INI_IMAGEMASKS     "ImageMasks"
#define INI_VIDEOMASKS     "VideoMasks"

Settings g_settings;

Settings::Settings()
{
	_ini_path = InMyConfig(INI_PATH);

	KeyFileReadSection kfh(_ini_path, INI_SECTION);
	_use_orientation = kfh.GetInt(INI_USEORIENTATION, _use_orientation) != 0;
	_open_by_enter = kfh.GetInt(INI_OPENBYENTER, _open_by_enter) != 0;
	_open_by_cpgdn = kfh.GetInt(INI_OPENBYCPGDN, _open_by_cpgdn) != 0;
	_open_in_qv = kfh.GetInt(INI_OPENINQV, _open_in_qv) != 0;
	_open_in_fv = kfh.GetInt(INI_OPENINFV, _open_in_fv) != 0;
	_image_masks = kfh.GetString(INI_IMAGEMASKS, DEFAULT_IMAGE_MASKS);
	_video_masks = kfh.GetString(INI_VIDEOMASKS, DEFAULT_VIDEO_MASKS);

	unsigned int default_scale = kfh.GetUInt(INI_DEFAULTSCALE, _default_scale);
	if (default_scale < (unsigned int)INVALID_SCALE_EDGE_VALUE) {
		_default_scale = (DefaultScale)default_scale;
	}
}

void Settings::SetDefaultScale(DefaultScale default_scale)
{
	fprintf(stderr, "%s: %u\n", __FUNCTION__, (unsigned int)default_scale);
	_default_scale = default_scale;
	KeyFileHelper kfh(_ini_path);
	kfh.SetUInt(INI_SECTION, INI_DEFAULTSCALE, (unsigned int)_default_scale);
}

const wchar_t *Settings::Msg(int msgId)
{
	return g_far.GetMsg(g_far.ModuleNumber, msgId);
}

void Settings::configurationMenuDialog()
{
	const int w = 50, h = 15;

	struct FarDialogItem fdi[] = {
	/* 0 */ {DI_DOUBLEBOX, 1,   1,   w - 2, h - 2, 0,	 {}, 0, 0,	Msg(M_TITLE), 0},
	/* 1 */ {DI_CHECKBOX,  3,   2,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_USEORIENTATION), 0},
	/* 2 */ {DI_CHECKBOX,  3,   3,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENBYENTER), 0},
	/* 3 */ {DI_CHECKBOX,  3,   4,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENBYCTRLPGDN), 0},
	/* 4 */ {DI_CHECKBOX,  3,   5,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENINQVIEW), 0},
	/* 5 */ {DI_CHECKBOX,  3,   6,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENINFVIEW), 0},
	/* 6 */ {DI_TEXT,      3,   7,   w - 9, 0,     FALSE, {}, 0, 0,	Msg(M_TEXT_IMAGEMASKS), 0},
	/* 7 */ {DI_EDIT,      3,   8,   w - 4, 0,     0,  {},  0, 0, nullptr, 0},
	/* 8 */ {DI_TEXT,      3,   9,   w - 9, 0,     FALSE, {}, 0, 0,	Msg(M_TEXT_VIDEOMASKS), 0},
	/* 9 */ {DI_EDIT,      3,   10,  w - 4, 0,     0,  {}, 0, 0, nullptr, 0},
	/*10 */ {DI_SINGLEBOX, 2,   11,  w - 3, 0,     0,  {}, DIF_BOXCOLOR|DIF_SEPARATOR, 0, nullptr, 0},
	/*11 */ {DI_BUTTON,    11,  12,  0,	    0,     FALSE, {}, 0, TRUE, Msg(M_OK), 0},
	/*12 */ {DI_BUTTON,    26,  12,  0,	    0,     FALSE, {}, 0, 0,	Msg(M_CANCEL), 0}
	};

	fdi[1].Param.Selected = _use_orientation;
	fdi[2].Param.Selected = _open_by_enter;
	fdi[3].Param.Selected = _open_by_cpgdn;
	fdi[4].Param.Selected = _open_in_qv;
	fdi[5].Param.Selected = _open_in_fv;
	std::wstring image_masks, video_masks;
	StrMB2Wide(_image_masks, image_masks);
	StrMB2Wide(_video_masks, video_masks);

	fdi[7].PtrData = image_masks.c_str();
	fdi[9].PtrData = video_masks.c_str();

	auto dlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, w, h, L"settings", fdi, ARRAYSIZE(fdi), 0, 0, nullptr, 0);

	auto r = g_far.DialogRun(dlg);
	if (r == ARRAYSIZE(fdi) - 2) {
		_use_orientation = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 1, 0) == BSTATE_CHECKED);
		_open_by_enter = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
		_open_by_cpgdn = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 3, 0) == BSTATE_CHECKED);
		_open_in_qv = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 4, 0) == BSTATE_CHECKED);
		_open_in_fv = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 5, 0) == BSTATE_CHECKED);
		image_masks = (const wchar_t *)g_far.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 7, 0);
		video_masks = (const wchar_t *)g_far.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 9, 0);

		StrWide2MB(image_masks, _image_masks);
		StrWide2MB(video_masks, _video_masks);

		KeyFileHelper kfh(_ini_path);
		kfh.SetInt(INI_SECTION, INI_USEORIENTATION, _use_orientation);
		kfh.SetInt(INI_SECTION, INI_OPENBYENTER, _open_by_enter);
		kfh.SetInt(INI_SECTION, INI_OPENBYCPGDN, _open_by_cpgdn);
		kfh.SetInt(INI_SECTION, INI_OPENINQV, _open_in_qv);
		kfh.SetInt(INI_SECTION, INI_OPENINFV, _open_in_fv);

		if (_image_masks != DEFAULT_IMAGE_MASKS) {
			kfh.SetString(INI_SECTION, INI_IMAGEMASKS, _image_masks);
		} else { // in case its same as default - erase it, so if/when defaults will change - it will be updated
			kfh.RemoveKey(INI_SECTION, INI_IMAGEMASKS);
		}

		if (_video_masks != DEFAULT_VIDEO_MASKS) {
			kfh.SetString(INI_SECTION, INI_VIDEOMASKS, _video_masks);
		} else { // in case its same as default - erase it, so if/when defaults will change - it will be updated
			kfh.RemoveKey(INI_SECTION, INI_VIDEOMASKS);
		}
	}

	g_far.DialogFree(dlg);
}


static bool MatchAnyOfWildcardsICE(const char *name, const std::string &masks)
{
	const char *last_slash = strrchr(name, '/');
	if (last_slash && last_slash[1]) {
		name = last_slash + 1;
	}
	for (size_t i = 0, j = 0; i <= masks.size(); ++i) {
		if (i == masks.size() || masks[i] == ';' || masks[i] == ',' || masks[i] == ' ') {
			auto mask = masks.substr(j, i - j);
			StrTrim(mask);
			if (!mask.empty() && MatchWildcardICE(name, mask.c_str())) {
				return true;
			}
			j = i + 1;
		}
	}
	return false;
}

bool Settings::MatchImageFile(const char *name) const
{
	return MatchAnyOfWildcardsICE(name, _image_masks);
}

bool Settings::MatchVideoFile(const char *name) const
{
	return MatchAnyOfWildcardsICE(name, _video_masks);
}
