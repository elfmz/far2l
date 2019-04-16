#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"


class SiteConnectionEditor : protected BaseDialog
{
	std::string _initial_display_name, _display_name;
	std::string _initial_protocol, _protocol, _host, _username, _password, _directory, _options;
	unsigned int _initial_port = 0, _port = 0;

	int _i_display_name = -1, _i_display_name_autogen = -1;
	int _i_dblbox = -1, _i_protocol = -1, _i_host = -1, _i_port = -1;
	int _i_username = -1, _i_password = -1;
	int _i_directory = -1, _i_protocol_options = -1;
	int _i_save = -1, _i_connect = -1, _i_cancel = -1;

	unsigned int _autogen_pending = 0;
	bool _autogen_display_name = false;

	FarDialogItems _di;
	FarListWrapper _di_protocols;


	virtual LONG_PTR DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

	void TextToDialogControl(HANDLE dlg, int ctl, const std::string &str);
	void TextFromDialogControl(HANDLE dlg, int ctl, std::string &str);
	void DataFromDialog(HANDLE dlg);
	void DisplayNameAutogenerateAndApply(HANDLE dlg);
	std::string DisplayNameAutogenerate();
	void AssignDefaultPortNumber(HANDLE dlg);

	void Load();
	bool Save();

public:
	SiteConnectionEditor(const std::string &display_name);


	bool Edit();

	const std::string &DisplayName() const { return _display_name; }
};
