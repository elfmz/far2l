#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================ SMB Protocol options =====================
| Workgroup:                    [                          ] |
| [ ] Enum network with SMB                                  |
| [ ] Enum network with NMB                                  |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

class ProtocolOptionsSMB : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_workgroup = -1;
	int _i_enum_by_smb = -1, _i_enum_by_nmb = -1;

	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_BTNCLICK
		 && !IsCheckedDialogControl(_i_enum_by_nmb)
		 && !IsCheckedDialogControl(_i_enum_by_smb)) {
			if (param1 == _i_enum_by_nmb) {
				SetCheckedDialogControl( _i_enum_by_smb, true);
			} else if (param1 == _i_enum_by_smb) {
				SetCheckedDialogControl( _i_enum_by_nmb, true);
			}
		}

		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	ProtocolOptionsSMB()
	{
		_di.SetBoxTitleItem(MSMBOptionsTitle);

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,34, 0, MSMBWorkgroup);
		_i_workgroup = _di.AddAtLine(DI_EDIT, 35,62, 0, "");

		_di.NextLine();
		_i_enum_by_smb = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MSMBEnumBySMB);
		_di.NextLine();
		_i_enum_by_nmb = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MSMBEnumByNMB);

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

		TextToDialogControl(_i_workgroup, sc.GetString("Workgroup"));

		auto enum_by = (unsigned int)sc.GetInt("EnumBy", 0xff);
		SetCheckedDialogControl( _i_enum_by_smb, enum_by & 1);
		SetCheckedDialogControl( _i_enum_by_nmb, enum_by & 2);

		if (Show(L"ProtocolOptionsSMB", 6, 2) == _i_ok) {
			std::string str;
			TextFromDialogControl(_i_workgroup, str);
			sc.SetString("Workgroup", str);

			enum_by = IsCheckedDialogControl(_i_enum_by_smb) ? 1 : 0;
			if (IsCheckedDialogControl(_i_enum_by_nmb)) enum_by|= 2;
			sc.SetInt("EnumBy", enum_by);
			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolSMB(std::string &options)
{
	ProtocolOptionsSMB().Configure(options);
}
