#include <utils.h>
#include "Confirm.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ======================= Remove =============================
| Remove selected files from:                                |
| [TEXTBOX                                                 ] |
|------------------------------------------------------------|
|   [  Proceed removal    ]        [        Cancel       ]   |
 ============================================================
    6                     29       38                      60
*/

ConfirmRemove::ConfirmRemove(const std::string &site_dir)
{
	_di.SetBoxTitleItem(MRemoveTitle);
	_di.Add(DI_TEXT, 5,2,62,2, 0, MRemoveText);

	_di.Add(DI_TEXT, 5,3,62,3, 0, site_dir.c_str());

	_di.Add(DI_TEXT, 4,4,63,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 7,5,29,5, DIF_CENTERGROUP, MProceedRemoval);
	_i_cancel = _di.Add(DI_BUTTON, 38,5,58,5, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_proceed);
	SetDefaultDialogControl(_i_proceed);
}


bool ConfirmRemove::Ask()
{
	return (Show(L"ConfirmRemove", 6, 2, FDLG_WARNING) == _i_proceed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

/*                                                         62
345                      28         39                   60  64
 ======================= Remove =============================
| Remove selected connection sites settings?                 |
|------------------------------------------------------------|
|   [  Proceed removal    ]        [        Cancel       ]   |
 ============================================================
    6                     29       38                      60
*/

ConfirmSitesDisposition::ConfirmSitesDisposition(What w, bool mv)
	: _warning(mv)
{
	switch (w) {
		case W_REMOVE:
			_di.SetBoxTitleItem(MRemoveSitesTitle);
			_di.Add(DI_TEXT, 5,2,62,2, 0, MRemoveSitesText);
			_di.Add(DI_TEXT, 4,3,63,3, DIF_BOXCOLOR | DIF_SEPARATOR);
			_i_proceed = _di.Add(DI_BUTTON, 7,4,29,4, DIF_CENTERGROUP, MProceedRemoval);
			_warning = true;
			break;

		case W_RELOCATE:
			_di.SetBoxTitleItem(mv ? MMoveSitesTitle : MCopySitesTitle);
			_di.Add(DI_TEXT, 5,2,62,2, 0, mv ? MMoveSitesText : MCopySitesText);
			_di.Add(DI_TEXT, 4,3,63,3, DIF_BOXCOLOR | DIF_SEPARATOR);
			_i_proceed = _di.Add(DI_BUTTON, 7,4,29,4, DIF_CENTERGROUP, MOK);
			break;

		case W_IMPORT:
			_di.SetBoxTitleItem(mv ? MImportMoveSitesTitle : MImportCopySitesTitle);
			_di.Add(DI_TEXT, 5,2,62,2, 0, mv ? MImportMoveSitesText : MImportCopySitesText);
			_di.Add(DI_TEXT, 4,3,63,3, DIF_BOXCOLOR | DIF_SEPARATOR);
			_i_proceed = _di.Add(DI_BUTTON, 7,4,29,4, DIF_CENTERGROUP, MOK);
			break;

		case W_EXPORT:
			_di.SetBoxTitleItem(mv ? MExportMoveSitesTitle : MExportCopySitesTitle);
			_di.Add(DI_TEXT, 5,2,62,2, 0, mv ? MExportMoveSitesText : MExportCopySitesText);
			_di.Add(DI_TEXT, 4,3,63,3, DIF_BOXCOLOR | DIF_SEPARATOR);
			_i_proceed = _di.Add(DI_BUTTON, 7,4,29,4, DIF_CENTERGROUP, MOK);
			break;
	}

	_i_cancel = _di.Add(DI_BUTTON, 38,4,58,4, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl((w == W_REMOVE) ? _i_cancel : _i_proceed);
	SetDefaultDialogControl((w == W_REMOVE) ? _i_cancel : _i_proceed);
}


bool ConfirmSitesDisposition::Ask()
{
	return (Show(L"ConfirmSitesDisposition", 6, 2, _warning ? FDLG_WARNING : 0) == _i_proceed);
}

//////////////////////////////////////////////////////////////////////////

/*
345                                            50
 ============ Create directory ================
| Enter name of directory to create:           |
| [EDITBOX                                   ] |
|----------------------------------------------|
|   [       Ok      ]      [    Cancel    ]    |
 ==============================================
    6                     29       38
*/

ConfirmMakeDir::ConfirmMakeDir(const std::string &default_name)
{
	_di.SetBoxTitleItem(MMakeDirTitle);
	_di.Add(DI_TEXT, 5,2,48,2, 0, MMakeDirText);

	_i_dir_name = _di.Add(DI_EDIT, 5,3,48,3, DIF_HISTORY, default_name.c_str(), "NetRocks_History_MakeDir");

	_di.Add(DI_TEXT, 4,4,49,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 6,5,23,5, DIF_CENTERGROUP, MProceedMakeDir);
	_i_cancel = _di.Add(DI_BUTTON, 30,5,45,5, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_dir_name);
	SetDefaultDialogControl(_i_proceed);
}

std::string ConfirmMakeDir::Ask()
{
	std::string out;
	if (Show(L"ConfirmMakeDir", 6, 2) == _i_proceed) {
		TextFromDialogControl(_i_dir_name, out);
	}
	return out;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
