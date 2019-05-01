#include "ConfirmOverwrite.h"
#include "../Globals.h"



/*                             34                          62
345                        30       39 42                    64
 ====================== Download ============================
| Destination file aready exists:                            |
| [EDITBOX                                                 ] |
| Source file information: [99999 Kb]  [DD/MM/YYYY hh:mm.ss] |
| Destination information: [        ]  [DD/MM/YYYY hh:mm.ss] |
|------------------------------------------------------------|
| Options to proceed:                                        |
| [ ] Reme&mber selection      [ ] &Overwrite                |
| [ ] &Skip                    [ ] Overwrite with &newer     |
| [ ] &Resume                  [ ] Create &different name    |
|------------------------------------------------------------|
|   [  Proceed dowmload   ]        [        Cancel       ]   |
 ============================================================
    6                     29   34  38                      60
*/

ConfirmOverwrite::ConfirmOverwrite(XferKind xk, XferDirection xd, const std::string &destination)
{
	if (xk == XK_COPY) {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,11, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
	} else {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,11, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
	}

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,62, 0, MDestinationExists);

	_di.NextLine();
	_i_destination = _di.AddAtLine(DI_EDIT, 5,62, 0, destination.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,29, 0, MSourceInfo);
	_i_source_size = _di.AddAtLine(DI_TEXT, 30,39, 0, "???");
	_i_source_timestamp = _di.AddAtLine(DI_TEXT, 42,62, 0, "???");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,29, 0, MDestinationInfo);
	_i_destination_size = _di.AddAtLine(DI_TEXT, 30,39, 0, "???");
	_i_destination_timestamp = _di.AddAtLine(DI_TEXT, 42,62, 0, "???");

	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.AddAtLine(DI_TEXT, 5,62, 0, MOverwriteOptions);

	_i_remember = _di.Add(DI_RADIOBUTTON, 5,33, 0, MRememberSelection);
	_i_overwrite = _di.Add(DI_CHECKBOX, 34,62, DIF_GROUP, MXferDOAOverwrite);

	_i_skip = _di.Add(DI_RADIOBUTTON, 5,33, 0, MXferDOASkip);
	_i_overwrite_newer = _di.Add(DI_RADIOBUTTON, 34,62, 0, MXferDOAOverwriteIfNewer);

	_i_resume = _di.Add(DI_RADIOBUTTON, 5,33, 0, MXferDOAResume);
	_i_create_diff_name = _di.Add(DI_RADIOBUTTON, 34,62, 0, MXferDOACreateDifferentName);

	_di.Add(DI_TEXT, 4,9,63,9, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 7,10,29,10, DIF_CENTERGROUP, MOK, nullptr, FDIS_DEFAULT);

	_i_cancel = _di.Add(DI_BUTTON, 38,10,58,10, DIF_CENTERGROUP, MCancel);
}


XferOverwriteAction ConfirmOverwrite::Ask(XferOverwriteAction &default_xoa)
{
	_di[_i_overwrite].Selected = 1;

	if (Show(_di[_i_dblbox].Data, 6, 2) != _i_proceed)
		return XOA_CANCEL;

	XferOverwriteAction out;

	if (_di[_i_skip].Selected) {
		out = XOA_SKIP;

	} else if (_di[_i_resume].Selected) {
		out = XOA_RESUME;

	} else if (_di[_i_overwrite].Selected) {
		out = XOA_OVERWRITE;

	} else if (_di[_i_overwrite_newer].Selected) {
		out = XOA_OVERWRITE_IF_NEWER;

	} else if (_di[_i_create_diff_name].Selected) {
		out = XOA_CREATE_DIFFERENT_NAME;

	} else { // WTF
		return XOA_CANCEL;
	}

	if (_di[_i_remember].Selected) {
		default_xoa = out;
	}

	return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////

