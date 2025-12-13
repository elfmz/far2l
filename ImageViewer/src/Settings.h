#ifndef FAR_SETTINGS_H
#define FAR_SETTINGS_H
#include <string>
#include <stdint.h>
#include "lng.h"

class Settings
{
public:
	enum DefaultScale {
		EQUAL_SCREEN = 0,
		LESSOREQUAL_SCREEN,
		EQUAL_IMAGE,

		INVALID_SCALE_EDGE_VALUE // must be last
	};

private:
	std::string _ini_path;

	DefaultScale _default_scale{EQUAL_SCREEN};

	bool _use_orientation = true;
	bool _open_by_enter = false;
	bool _open_by_cpgdn = true;
	bool _open_in_qv = false;
	bool _open_in_fv = false;

	std::string _image_masks, _video_masks;

	struct Commands : std::vector<std::pair<std::wstring, std::wstring>> {}_commands;

	bool ExtraCommandsMenuInternal(Commands &commands, std::string *selected_cmd = nullptr);

	void SaveCommands();

public:
	Settings();
	const wchar_t *Msg(int msgId);

	void ConfigurationDialog();

	bool UseOrientation() const { return _use_orientation; }
	bool OpenByEnter() const { return _open_by_enter; }
	bool OpenByCtrlPgDn() const { return _open_by_cpgdn; }
	bool OpenInQV() const { return _open_in_qv; }
	bool OpenInFV() const { return _open_in_fv; }

	DefaultScale GetDefaultScale() const { return _default_scale; }
	void SetDefaultScale(DefaultScale default_scale);

	bool MatchImageFile(const char *name) const;
	bool MatchVideoFile(const char *name) const;
	bool MatchFile(const char *name) const { return MatchImageFile(name) || MatchVideoFile(name); }

	std::string ExtraCommandsMenu();
};

extern Settings g_settings;

#endif // FAR_SETTINGS_H
