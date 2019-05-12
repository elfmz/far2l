#include <utils.h>
#include "ConfirmXfer.h"
#include "../Globals.h"



/*                                                         62
345                      28         39                   60  64
 ====================== Download ============================
| Download selected file &to:                                |
| [EDITBOX                                                 ] |
|------------------------------------------------------------|
| Default overwrite action:                                  |
| [x] &Ask                 [ ] &Overwrite                    |
| [ ] &Skip                [ ] Overwrite with &newer         |
| [ ] &Resume              [ ] Create &different name        |
|------------------------------------------------------------|
|   [  Proceed dowmload   ]        [        Cancel       ]   |
 ============================================================
    6                     29       38                      60
*/

ConfirmXfer::ConfirmXfer(XferKind xk, XferDirection xd)
{
	if (xk == XK_COPY) {
		_di.Add(DI_DOUBLEBOX, 3,1,64,11, 0,
			(xd == XK_UPLOAD) ? MXferCopyUploadTitle : ((xd == XK_CROSSLOAD) ? MXferCopyCrossloadTitle : MXferCopyDownloadTitle));
		_di.Add(DI_TEXT, 5,2,62,2, 0,
			(xd == XK_UPLOAD) ? MXferCopyUploadText : ((xd == XK_CROSSLOAD) ? MXferCopyCrossloadText : MXferCopyDownloadText));
	} else {
		_di.Add(DI_DOUBLEBOX, 3,1,64,11, 0,
			(xd == XK_UPLOAD) ? MXferMoveUploadTitle : ((xd == XK_CROSSLOAD) ? MXferMoveCrossloadTitle : MXferMoveDownloadTitle));
		_di.Add(DI_TEXT, 5,2,62,2, 0,
			(xd == XK_UPLOAD) ? MXferMoveUploadText : ((xd == XK_CROSSLOAD) ? MXferMoveCrossloadText : MXferMoveDownloadText));
	}

	_i_destination = _di.Add(DI_EDIT, 5,3,62,3, 0, "");

	_di.Add(DI_TEXT, 4,4,63,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.Add(DI_TEXT, 5,5,62,5, 0, MXferDOAText);

	_i_ask = _di.Add(DI_RADIOBUTTON, 5,6,29,6, DIF_GROUP, MXferDOAAsk);
	_i_overwrite = _di.Add(DI_RADIOBUTTON, 30,6,62,6, 0, MXferDOAOverwrite);

	_i_skip = _di.Add(DI_RADIOBUTTON, 5,7,29,7, 0, MXferDOASkip);
	_i_overwrite_newer = _di.Add(DI_RADIOBUTTON, 30,7,62,7, 0, MXferDOAOverwriteIfNewer);

	_i_resume = _di.Add(DI_RADIOBUTTON, 5,8,29,8, 0, MXferDOAResume);
	_i_create_diff_name = _di.Add(DI_RADIOBUTTON, 30,8,62,8, 0, MXferDOACreateDifferentName);

	_di.Add(DI_TEXT, 4,9,63,9, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 7,10,29,10, DIF_CENTERGROUP, (xk == XK_COPY) ?
		((xd == XK_UPLOAD) ? MProceedCopyUpload : ((xd == XK_CROSSLOAD) ? MProceedCopyCrossload : MProceedCopyDownload))
		:
		((xd == XK_UPLOAD) ? MProceedMoveUpload : ((xd == XK_CROSSLOAD) ? MProceedMoveCrossload : MProceedMoveDownload)));

	_i_cancel = _di.Add(DI_BUTTON, 38,10,58,10, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_proceed);
	SetDefaultDialogControl(_i_proceed);
}


bool ConfirmXfer::Ask(XferOverwriteAction &default_xoa, std::string &destination)
{
	switch (default_xoa) {
		case XOA_SKIP:
			SetCheckedDialogControl(_i_skip);
			break;

		case XOA_RESUME:
			SetCheckedDialogControl(_i_resume);
			break;

		case XOA_OVERWRITE:
			SetCheckedDialogControl(_i_overwrite);
			break;

		case XOA_OVERWRITE_IF_NEWER:
			SetCheckedDialogControl(_i_overwrite_newer);
			break;

		case XOA_CREATE_DIFFERENT_NAME:
			SetCheckedDialogControl(_i_create_diff_name);
			break;

		case XOA_ASK: default:
			SetCheckedDialogControl(_i_ask);
	}

	TextToDialogControl(_i_destination, destination);

	if (Show(L"ConfirmXfer", 6, 2) != _i_proceed)
		return false;

	TextFromDialogControl(_i_destination, destination);

	if (IsCheckedDialogControl(_i_skip)) {
		default_xoa = XOA_SKIP;

	} else if (IsCheckedDialogControl(_i_resume)) {
		default_xoa = XOA_RESUME;

	} else if (IsCheckedDialogControl(_i_overwrite)) {
		default_xoa = XOA_OVERWRITE;

	} else if (IsCheckedDialogControl(_i_overwrite_newer)) {
		default_xoa = XOA_OVERWRITE_IF_NEWER;

	} else if (IsCheckedDialogControl(_i_create_diff_name)) {
		default_xoa = XOA_CREATE_DIFFERENT_NAME;

	} else {
		default_xoa = XOA_ASK;
	}

	return true;
}
