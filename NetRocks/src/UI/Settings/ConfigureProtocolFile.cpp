#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================ File Protocol options =====================
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

class ProtocolOptionsFile : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_command = -1, _i_extra = -1, _i_command_time_limit = -1;

public:
	ProtocolOptionsFile()
	{
		_di.SetBoxTitleItem(MSFileOptionsTitle);

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,62, 0, MSFileCommand);
		_di.NextLine();
		_i_command = _di.AddAtLine(DI_EDIT, 5,62, 0, "");

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
		_di.AddAtLine(DI_TEXT, 5,62, 0, "$HOST $PORT $USER $PASSWORD $EXTRA"); //$DIRECTORY

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();

		_i_ok = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);
	}


	void Configure(std::string &options)
	{
		StringConfig sc(options);
		TextToDialogControl(_i_command, sc.GetString("Command"));
		TextToDialogControl(_i_extra, sc.GetString("Extra"));
		LongLongToDialogControl(_i_command_time_limit, std::max(3, sc.GetInt("CommandTimeLimit", 30)));
		if (Show(L"ProtocolOptionsFile", 6, 2) == _i_ok) {
			std::string str;
			TextFromDialogControl(_i_command, str);
			sc.SetString("Command", str);
			TextFromDialogControl(_i_extra, str);
			sc.SetString("Extra", str);
			sc.SetInt("CommandTimeLimit", std::max(3, (int)LongLongFromDialogControl(_i_command_time_limit)));
			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolFile(std::string &options)
{
	ProtocolOptionsFile().Configure(options);
}
