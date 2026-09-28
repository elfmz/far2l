#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


struct CodePage
{
	int id;
	std::string name;
};

static struct Codepages : std::vector<CodePage>, std::mutex
{
	void Add(int id, const wchar_t *name)
	{
		emplace_back();
		auto &cp = back();
		cp.id = id;
		cp.name = ToDec((unsigned int)id);
		if (cp.name.size() < 6) {
			cp.name.append(6 - cp.name.size(), ' ');
		}
		if (G.fsf.BoxSymbols) {
			Wide2MB(&G.fsf.BoxSymbols[BS_V1], 1, cp.name, true);
		} else {
			cp.name+= '|';
		}
		cp.name+= ' ';
		Wide2MB(name, cp.name, true);
	}

} s_codepages;


static BOOL __stdcall EnumCodePagesProc(LPWSTR lpwszCodePage)
{
	const int id = _wtoi(lpwszCodePage);

	CPINFOEX cpiex{};
	if (id != CP_UTF8 && id != CP_UTF16LE && id != CP_UTF16BE && id != CP_UTF32LE && id != CP_UTF32BE) {
		if (WINPORT(GetCPInfoEx)((UINT)id, 0, &cpiex)) {
			s_codepages.Add(id, cpiex.CodePageName);
		}
	}

	return TRUE;
}


/*                                                         62
345                      28         39                   60  64
 ================ File Protocol options =====================
| Keep alive:                      [INTEG]                   |
| Codepage:                        [COMBOBOX               ] |
| Time adjust, seconds:            [99999]                   |
| Command to execute on connect:                             |
| [EDIT....................................................] |
| Extra string passed to command:                            |
| [EDIT....................................................] |
| Limit command execution time, seconds:                [99] |
| Extra environment variables available for command process: |
| $HOST $PORT $USER $PASSWORD $DIRECTORY                     |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

class ExtraSiteSettings : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_keepalive = -1, _i_codepage = -1, _i_timeadjust = -1;
	int _i_command = -1, _i_command_deinit = -1, _i_extra = -1, _i_command_time_limit = -1;
	FarListWrapper _di_codepages;

public:
//	ExtraSiteSettings() { }

	void Configure(std::string &options)
	{
		StringConfig sc(options);
		int codepage = sc.GetInt("CodePage", CP_UTF8);

		{
			std::lock_guard<std::mutex> codepages_locker(s_codepages);
			if (s_codepages.empty()) {
				s_codepages.Add(CP_UTF8, L"UTF8");
				WINPORT(EnumSystemCodePages)(EnumCodePagesProc, 0);
			}
			for (const auto &cp : s_codepages) {
				_di_codepages.Add(cp.name.c_str(), (cp.id == codepage) ? LIF_SELECTED : 0);
			}
		}

		_di.SetBoxTitleItem(MSFileOptionsTitle);

		char sz[32];

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,56, 0, MKeepAlive);
		itoa(sc.GetInt("KeepAlive", 0), sz, 10);
		_i_keepalive = _di.AddAtLine(DI_FIXEDIT, 57,62, DIF_MASKEDIT, sz, "999999");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,56, 0, MTimeAdjust);
		itoa(sc.GetInt("TimeAdjust", 0), sz, 10);
		_i_timeadjust = _di.AddAtLine(DI_FIXEDIT, 57,62, DIF_MASKEDIT, sz, "#99999");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,37, 0, MCodepage);
		_i_codepage = _di.AddAtLine(DI_COMBOBOX, 38,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_codepage].ListItems = _di_codepages.Get();

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, 0, MSFileCommand);
		_di.NextLine();
		_i_command = _di.AddAtLine(DI_EDIT, 5,62, 0, "");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, 0, MSFileCommandDeinit);
		_di.NextLine();
		_i_command_deinit = _di.AddAtLine(DI_EDIT, 5,62, 0, "");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, 0, MSFileExtra);
		_di.NextLine();
		_i_extra = _di.AddAtLine(DI_EDIT, 5,62, 0, "");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,50, 0, MSFileCommandTimeLimit);
		_i_command_time_limit = _di.AddAtLine(DI_FIXEDIT, 60,62, DIF_MASKEDIT, "30", "999");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, 0, MSFileCommandVarsHint);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, 0, "$HOST $PORT $USER $PASSWORD $EXTRA $SINGULAR $STORAGE"); //$DIRECTORY

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_ok = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);

		TextToDialogControl(_i_command, sc.GetString("Command"));
		TextToDialogControl(_i_command_deinit, sc.GetString("CommandDeinit"));
		TextToDialogControl(_i_extra, sc.GetString("Extra"));

		LongLongToDialogControl(_i_command_time_limit, std::max(3, sc.GetInt("CommandTimeLimit", 30)));
		if (Show(L"ExtraSiteSettings", 6, 2) == _i_ok) {
			std::string str;
			TextFromDialogControl(_i_command, str);
			sc.SetString("Command", str);
			TextFromDialogControl(_i_command_deinit, str);
			sc.SetString("CommandDeinit", str);
			TextFromDialogControl(_i_extra, str);
			sc.SetString("Extra", str);
			sc.SetInt("CommandTimeLimit", std::max(3, (int)LongLongFromDialogControl(_i_command_time_limit)));
			sc.SetInt("KeepAlive", std::max(0, (int)LongLongFromDialogControl(_i_keepalive)));
			sc.SetInt("TimeAdjust", (int)LongLongFromDialogControl(_i_timeadjust));

			{
				int cp_index = GetDialogListPosition(_i_codepage);
				std::lock_guard<std::mutex> codepages_locker(s_codepages);
				if (cp_index >= 0 && cp_index < (int)s_codepages.size()) {
					sc.SetInt("CodePage", s_codepages[cp_index].id);
				}
			}
			options = sc.Serialize();
		}
	}
};

void ConfigureExtraSiteSettings(std::string &options)
{
	ExtraSiteSettings().Configure(options);
}
