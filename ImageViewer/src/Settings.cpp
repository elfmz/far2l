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
#define INI_SETTINGS       "Settings"
#define INI_COMMANDS       "Commands(dev)"
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

	KeyFileReadHelper kfh(_ini_path);
	const KeyFileValues *sv = kfh.GetSectionValues(INI_SETTINGS);
	if (sv) {
		_use_orientation = sv->GetInt(INI_USEORIENTATION, _use_orientation) != 0;
		_open_by_enter = sv->GetInt(INI_OPENBYENTER, _open_by_enter) != 0;
		_open_by_cpgdn = sv->GetInt(INI_OPENBYCPGDN, _open_by_cpgdn) != 0;
		_open_in_qv = sv->GetInt(INI_OPENINQV, _open_in_qv) != 0;
		_open_in_fv = sv->GetInt(INI_OPENINFV, _open_in_fv) != 0;
		_image_masks = sv->GetString(INI_IMAGEMASKS, DEFAULT_IMAGE_MASKS);
		_video_masks = sv->GetString(INI_VIDEOMASKS, DEFAULT_VIDEO_MASKS);

		unsigned int default_scale = sv->GetUInt(INI_DEFAULTSCALE, _default_scale);
		if (default_scale < (unsigned int)INVALID_SCALE_EDGE_VALUE) {
			_default_scale = (DefaultScale)default_scale;
		}
	} else {
		_image_masks = DEFAULT_IMAGE_MASKS;
		_video_masks = DEFAULT_VIDEO_MASKS;
	}

	const KeyFileValues *cv = kfh.GetSectionValues(INI_COMMANDS);
	if (cv) {
		for (unsigned int i = 0; ;++i) {
			const std::string &name_key = StrPrintf("Name_%u", i);
			const std::string &cmd_key = StrPrintf("Cmd_%u", i);
			if (!cv->HasKey(name_key) || !cv->HasKey(cmd_key)) {
				break;
			}
			_commands.emplace_back(cv->GetString(name_key, L""), cv->GetString(cmd_key, L""));
		}
	} else {
		_commands.emplace_back(L"&Normalize", L"convert -size {W}x{H} -depth 8 -normalize rgb:- rgb:-");
		_commands.emplace_back(L"&White balance", L"convert -size {W}x{H} -depth 8 -separate -contrast-stretch 0.5%x0.5% -combine rgb:- rgb:-");
		_commands.emplace_back(L"&Grayscale", L"convert -size {W}x{H} -depth 8 -colorspace Gray rgb:- rgb:-");
	}
}

void Settings::SaveCommands()
{
	KeyFileHelper kfh(_ini_path);
	kfh.RemoveSection(INI_COMMANDS);
	if (_commands.empty()) { // ensure section exists
		kfh.SetString(INI_COMMANDS, "_", "");
	}else for (unsigned int i = 0; i < (unsigned int)_commands.size(); ++i) {
		kfh.SetString(INI_COMMANDS, StrPrintf("Name_%u", i), _commands[i].first.c_str());
		kfh.SetString(INI_COMMANDS, StrPrintf("Cmd_%u", i), _commands[i].second.c_str());
	}
}

void Settings::SetDefaultScale(DefaultScale default_scale)
{
	fprintf(stderr, "%s: %u\n", __FUNCTION__, (unsigned int)default_scale);
	_default_scale = default_scale;
	KeyFileHelper kfh(_ini_path);
	kfh.SetUInt(INI_SETTINGS, INI_DEFAULTSCALE, (unsigned int)_default_scale);
}

const wchar_t *Settings::Msg(int msgId)
{
	return g_far.GetMsg(g_far.ModuleNumber, msgId);
}

bool Settings::ExtraCommandsMenuInternal(Commands &commands, std::string *selected_cmd)
{
	wchar_t command_name[0x100]{};
	wchar_t command_line[0x1000]{};
	int selected_idx = 0;
	for (;;) {
		std::vector<FarMenuItem> menu_items;
		for (const auto &cmd : commands) {
			auto &mi = menu_items.emplace_back();
			mi.Text = cmd.first.c_str();
			if ((int)menu_items.size() - 1 == selected_idx) {
				mi.Selected = 1;
			}
		}
		// Display the menu and get the user's selection.
		constexpr int break_keys[] = {VK_F4, VK_INSERT, VK_DELETE, 0};
		int break_code = -1;

		selected_idx = g_far.Menu(g_far.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE | FMENU_CHANGECONSOLETITLE,
				   Msg(M_EXTRA_COMMANDS_TITLE), L"F4 INS DEL ENTER ESC", L"ExtraCommands", break_keys, &break_code, menu_items.data(), menu_items.size());
		if (selected_idx < 0 || selected_idx >= (int)_commands.size()) {
			return false; // User cancelled the menu (e.g., with Esc)
		}

		if (break_code == 2) {
			commands.erase(commands.begin() + selected_idx);
		} else if (break_code == 0 || break_code == 1) {
			const wchar_t *initial_name = (break_code == 0) ? commands[selected_idx].first.c_str() : NULL;
			const wchar_t *initial_line = (break_code == 0) ? commands[selected_idx].second.c_str() : NULL;
			std::wstring help_topic = g_far.ModuleName;
			std::string::size_type help_topic_n = help_topic.rfind(LGOOD_SLASH);
			if (help_topic_n != std::string::npos) {
				help_topic = L'<' + help_topic.substr(0, help_topic_n + 1) + L'>';
				help_topic += L"ExtraCommands";
			}
			if (g_far.InputBox(Msg(M_INPUT_CMDNAME_TITLE), Msg(M_INPUT_CMDNAME_PROMPT),
								NULL, initial_name, command_name, ARRAYSIZE(command_name) - 1,
								help_topic_n != std::string::npos ? help_topic.c_str() : NULL, 0)
			 && g_far.InputBox(Msg(M_INPUT_CMDLINE_TITLE), Msg(M_INPUT_CMDLINE_PROMPT),
								NULL, initial_line, command_line, ARRAYSIZE(command_line) - 1,
								help_topic_n != std::string::npos ? help_topic.c_str() : NULL, 0)) {
				std::pair<std::wstring, std::wstring> command(command_name, command_line);
				if (break_code == 0) {
					commands[selected_idx] = command;
				} else {
					commands.insert(commands.begin() + selected_idx, command);
				}
			}
		} else {
			if (selected_cmd) {
				*selected_cmd = StrWide2MB(_commands[selected_idx].second);
			}
			return true;
		}
	}
}

std::string Settings::ExtraCommandsMenu()
{
	auto commands_copy = _commands;
	std::string selected_cmd;
	if (!ExtraCommandsMenuInternal(commands_copy, &selected_cmd)) {
		return std::string();
	}
	if (_commands != commands_copy) {
		_commands = std::move(commands_copy);
		SaveCommands();
	}
	return selected_cmd;
}

void Settings::ConfigurationDialog()
{
	const int w = 50, h = 16;

	struct FarDialogItem fdi[] = {
	/* 0 */ {DI_DOUBLEBOX, 1,   1,   w - 2, h - 2, 0,	 {}, 0, 0,	Msg(M_TITLE), 0},
	/* 1 */ {DI_CHECKBOX,  3,   2,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_USEORIENTATION), 0},
	/* 2 */ {DI_SINGLEBOX, 2,   3,   w - 3, 0,     0,  {}, DIF_BOXCOLOR|DIF_SEPARATOR, 0, nullptr, 0},
	/* 3 */ {DI_CHECKBOX,  3,   4,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENBYENTER), 0},
	/* 4 */ {DI_CHECKBOX,  3,   5,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENBYCTRLPGDN), 0},
	/* 5 */ {DI_CHECKBOX,  3,   6,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENINQVIEW), 0},
	/* 6 */ {DI_CHECKBOX,  3,   7,   0,	    0,     TRUE,  {}, 0, 0,	Msg(M_TEXT_OPENINFVIEW), 0},
	/* 7 */ {DI_TEXT,      3,   8,   w - 9, 0,     FALSE, {}, 0, 0,	Msg(M_TEXT_IMAGEMASKS), 0},
	/* 8 */ {DI_EDIT,      3,   9,   w - 4, 0,     0,  {},  0, 0, nullptr, 0},
	/* 9 */ {DI_TEXT,      3,   10,  w - 9, 0,     FALSE, {}, 0, 0,	Msg(M_TEXT_VIDEOMASKS), 0},
	/*10 */ {DI_EDIT,      3,   11,  w - 4, 0,     0,  {}, 0, 0, nullptr, 0},
	/*11 */ {DI_SINGLEBOX, 2,   12,  w - 3, 0,     0,  {}, DIF_BOXCOLOR|DIF_SEPARATOR, 0, nullptr, 0},
	/*12 */ {DI_BUTTON,    4,   13,  0,	    0,     FALSE, {}, 0, 0, Msg(M_EXTRA_COMMANDS), 0},
	/*13 */ {DI_BUTTON,    27,  13,  0,	    0,     FALSE, {}, 0, TRUE, Msg(M_OK), 0},
	/*14 */ {DI_BUTTON,    35,  13,  0,	    0,     FALSE, {}, 0, 0,	Msg(M_CANCEL), 0}
	};

	fdi[1].Param.Selected = _use_orientation;
	fdi[3].Param.Selected = _open_by_enter;
	fdi[4].Param.Selected = _open_by_cpgdn;
	fdi[5].Param.Selected = _open_in_qv;
	fdi[6].Param.Selected = _open_in_fv;
	std::wstring image_masks, video_masks;
	StrMB2Wide(_image_masks, image_masks);
	StrMB2Wide(_video_masks, video_masks);

	fdi[8].PtrData = image_masks.c_str();
	fdi[10].PtrData = video_masks.c_str();

	auto dlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, w, h, L"settings", fdi, ARRAYSIZE(fdi), 0, 0, nullptr, 0);
	auto commands_copy = _commands;
	int r;
	for (;;) {
		r = g_far.DialogRun(dlg);
		if (r != ARRAYSIZE(fdi) - 3) {
			break;
		}
		auto commands_copy_copy = commands_copy;
		if (ExtraCommandsMenuInternal(commands_copy)) {
			commands_copy = std::move(commands_copy_copy);
		}
	};

	if (r == ARRAYSIZE(fdi) - 2) {
		if (_commands != commands_copy) {
			_commands = commands_copy;
			SaveCommands();
		}
		_use_orientation = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 1, 0) == BSTATE_CHECKED);
		_open_by_enter = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 3, 0) == BSTATE_CHECKED);
		_open_by_cpgdn = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 4, 0) == BSTATE_CHECKED);
		_open_in_qv = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 5, 0) == BSTATE_CHECKED);
		_open_in_fv = (g_far.SendDlgMessage(dlg, DM_GETCHECK, 6, 0) == BSTATE_CHECKED);
		image_masks = (const wchar_t *)g_far.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 8, 0);
		video_masks = (const wchar_t *)g_far.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 10, 0);

		StrWide2MB(image_masks, _image_masks);
		StrWide2MB(video_masks, _video_masks);

		KeyFileHelper kfh(_ini_path);
		kfh.SetInt(INI_SETTINGS, INI_USEORIENTATION, _use_orientation);
		kfh.SetInt(INI_SETTINGS, INI_OPENBYENTER, _open_by_enter);
		kfh.SetInt(INI_SETTINGS, INI_OPENBYCPGDN, _open_by_cpgdn);
		kfh.SetInt(INI_SETTINGS, INI_OPENINQV, _open_in_qv);
		kfh.SetInt(INI_SETTINGS, INI_OPENINFV, _open_in_fv);

		if (_image_masks != DEFAULT_IMAGE_MASKS) {
			kfh.SetString(INI_SETTINGS, INI_IMAGEMASKS, _image_masks);
		} else { // in case its same as default - erase it, so if/when defaults will change - it will be updated
			kfh.RemoveKey(INI_SETTINGS, INI_IMAGEMASKS);
		}

		if (_video_masks != DEFAULT_VIDEO_MASKS) {
			kfh.SetString(INI_SETTINGS, INI_VIDEOMASKS, _video_masks);
		} else { // in case its same as default - erase it, so if/when defaults will change - it will be updated
			kfh.RemoveKey(INI_SETTINGS, INI_VIDEOMASKS);
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
