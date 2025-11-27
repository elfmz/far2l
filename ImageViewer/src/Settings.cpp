#include "Common.h"
#include "Settings.h"
#include <KeyFileHelper.h>
#include <utils.h>
#include <wchar.h>

Settings g_settings;


#define INI_PATH          "plugins/ImageViewer/config.ini"
#define INI_SECTION       "Settings"
#define INI_OPENBYENTER   "OpenByEnter"
#define INI_OPENBYCPGDB   "OpenByCtrlPgDn"
#define INI_OPENINQV      "OpenInQV"
#define INI_OPENINFV      "OpenInFV"
#define INI_IMAGEMASKS    "ImageMasks"
#define INI_VIDEOMASKS    "VideoMasks"

Settings::Settings()
{
	_ini_path = InMyConfig(INI_PATH);

	KeyFileReadSection kfh(_ini_path, INI_SECTION);
	_open_by_enter = kfh.GetInt(INI_OPENBYENTER, _open_by_enter) != 0;
	_open_by_cpgdn = kfh.GetInt(INI_OPENBYCPGDB, _open_by_cpgdn) != 0;
	_open_in_qv = kfh.GetInt(INI_OPENINQV, _open_in_qv) != 0;
	_open_in_fv = kfh.GetInt(INI_OPENINFV, _open_in_fv) != 0;
	_image_masks = kfh.GetString(INI_IMAGEMASKS, _image_masks.c_str());
	_video_masks = kfh.GetString(INI_VIDEOMASKS, _video_masks.c_str());
}

const wchar_t *Settings::Msg(int msgId)
{
	return g_far.GetMsg(g_far.ModuleNumber, msgId);
}

void Settings::configurationMenuDialog()
{
	const int w = 50, h = 14;

	struct FarDialogItem fdi[] = {
	/* 0 */ {DI_DOUBLEBOX, 1,   1,   w - 2, h - 2, 0,	 {}, 0, 0,	Msg(M_TITLE), 0},
	/* 1 */ {DI_CHECKBOX,  3,   2,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENBYENTER), 0},
	/* 2 */ {DI_CHECKBOX,  3,   3,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENBYCTRLPGDN), 0},
	/* 3 */ {DI_CHECKBOX,  3,   4,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENINQVIEW), 0},
	/* 4 */ {DI_CHECKBOX,  3,   5,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENINFVIEW), 0},
	/* 5 */ {DI_TEXT,      3,   6,   w - 9, 0,     FALSE, {}, 0, 0,	Msg(M_TEXT_IMAGEMASKS), 0},
	/* 6 */ {DI_EDIT,      3,   7,   w - 4, 0,     0,  {},  0, 0, nullptr, 0},
	/* 7 */ {DI_TEXT,      3,   8,   w - 9, 0,     FALSE, {}, 0, 0,	Msg(M_TEXT_VIDEOMASKS), 0},
	/* 8 */ {DI_EDIT,      3,   9,   w - 4, 0,     0,  {}, 0, 0, nullptr, 0},
	/* 9 */ {DI_SINGLEBOX, 2,   10,   w - 3, 0,     0,  {}, DIF_BOXCOLOR|DIF_SEPARATOR, 0, nullptr, 0},
	/*10 */ {DI_BUTTON,    11,  11,  0,	    0,     FALSE, {}, 0, TRUE, Msg(M_OK), 0},
	/*11 */ {DI_BUTTON,    26,  11,  0,	    0,     FALSE, {}, 0, 0,	Msg(M_CANCEL), 0}
	};

	fdi[1].Param.Selected = _open_by_enter;
	fdi[2].Param.Selected = _open_by_cpgdn;
	fdi[3].Param.Selected = _open_in_qv;
	fdi[4].Param.Selected = _open_in_fv;
	std::wstring image_masks, video_masks;
	StrMB2Wide(_image_masks, image_masks);
	StrMB2Wide(_video_masks, video_masks);

	fdi[6].PtrData = image_masks.c_str();
	fdi[8].PtrData = video_masks.c_str();

	auto dlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, w, h, L"settings", fdi, ARRAYSIZE(fdi), 0, 0, nullptr, 0);

	auto r = g_far.DialogRun(dlg);
	if (r == ARRAYSIZE(fdi) - 2) {
		_open_by_enter = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 1, 0) == BSTATE_CHECKED);
		_open_by_cpgdn = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
		_open_in_qv = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 3, 0) == BSTATE_CHECKED);
		_open_in_fv = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 4, 0) == BSTATE_CHECKED);
		image_masks = (const wchar_t *)g_far.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 6, 0);
		video_masks = (const wchar_t *)g_far.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 8, 0);

		StrWide2MB(image_masks, _image_masks);
		StrWide2MB(video_masks, _video_masks);

		KeyFileHelper kfh(_ini_path);
		kfh.SetInt(INI_SECTION, INI_OPENBYENTER, _open_by_enter);
		kfh.SetInt(INI_SECTION, INI_OPENBYCPGDB, _open_by_cpgdn);
		kfh.SetInt(INI_SECTION, INI_OPENINQV, _open_in_qv);
		kfh.SetInt(INI_SECTION, INI_OPENINFV, _open_in_fv);
		kfh.SetString(INI_SECTION, INI_IMAGEMASKS, _image_masks);
		kfh.SetString(INI_SECTION, INI_VIDEOMASKS, _video_masks);
	}

	g_far.DialogFree(dlg);
}


static bool MatchSemicolonSeparatedWildcardsICE(const char *name, const std::string &masks)
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
	return MatchSemicolonSeparatedWildcardsICE(name, _image_masks);
}

bool Settings::MatchVideoFile(const char *name) const
{
	return MatchSemicolonSeparatedWildcardsICE(name, _video_masks);
}
