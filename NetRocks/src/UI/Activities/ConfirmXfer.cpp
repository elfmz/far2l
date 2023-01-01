#include <utils.h>
#include "ConfirmXfer.h"
#include "../../Globals.h"



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
	: _xk(xk), _xd(xd)
{
	if (_xk == XK_COPY) {
		_i_dblbox = _di.SetBoxTitleItem((_xd == XD_UPLOAD) ? MXferCopyUploadTitle
				: ((_xd == XD_CROSSLOAD) ? MXferCopyCrossloadTitle : MXferCopyDownloadTitle));
		_i_text = _di.Add(DI_TEXT, 5,2,62,2, 0, (_xd == XD_UPLOAD) ? MXferCopyUploadText
				: ((_xd == XD_CROSSLOAD) ? MXferCopyCrossloadText : MXferCopyDownloadText));
	} else if (_xk == XK_MOVE) {
		_i_dblbox = _di.SetBoxTitleItem((_xd == XD_UPLOAD) ? MXferMoveUploadTitle
				: ((_xd == XD_CROSSLOAD) ? MXferMoveCrossloadTitle : MXferMoveDownloadTitle));
		_i_text = _di.Add(DI_TEXT, 5,2,62,2, 0, (_xd == XD_UPLOAD) ? MXferMoveUploadText
				: ((_xd == XD_CROSSLOAD) ? MXferMoveCrossloadText : MXferMoveDownloadText));
	} else {
		ASSERT(_xk == XK_RENAME);
		ASSERT(_xd == XD_DOWNLOAD || _xd == XD_CROSSLOAD);
		_i_dblbox = _di.SetBoxTitleItem(MXferRenameTitle);
		_i_text = _di.Add(DI_TEXT, 5,2,62,2, 0, MXferRenameText);
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

	_i_proceed = _di.Add(DI_BUTTON, 7,10,29,10, DIF_CENTERGROUP, (_xk == XK_RENAME) ? MProceedRename : ( (_xk == XK_COPY) ?
		((_xd == XD_UPLOAD) ? MProceedCopyUpload : ((_xd == XD_CROSSLOAD) ? MProceedCopyCrossload : MProceedCopyDownload))
		:
		((_xd == XD_UPLOAD) ? MProceedMoveUpload : ((_xd == XD_CROSSLOAD) ? MProceedMoveCrossload : MProceedMoveDownload))) );

	_i_cancel = _di.Add(DI_BUTTON, 38,10,58,10, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_text);
	SetDefaultDialogControl(_i_proceed);
}


XferKind ConfirmXfer::Ask(XferOverwriteAction &default_xoa, std::string &destination)
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

	_prev_destination = destination;
	TextToDialogControl(_i_destination, destination);

	if (Show(L"ConfirmXfer", 6, 2) != _i_proceed) {
		return XK_NONE;
	}

	TextFromDialogControl(_i_destination, destination);
	if (destination.empty()) {
		return XK_NONE;
	}

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

	if (_xk == XK_MOVE && (_xd == XD_DOWNLOAD || _xd == XD_CROSSLOAD)
	 && destination.find("/") == std::string::npos) {
		return XK_RENAME;
	}

	return _xk;
}


LONG_PTR ConfirmXfer::DlgProc(int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_EDITCHANGE && param1 == _i_destination) {
		std::string destination;
		TextFromDialogControl(_i_destination, destination);

		if (_xk == XK_RENAME) {
			const bool prev_valid = (!_prev_destination.empty()
				&& _prev_destination.find('/') == std::string::npos);

			if (destination.empty() || destination.find('/') != std::string::npos) {
				if (prev_valid) {
					SetEnabledDialogControl(_i_proceed, false);
				}
			} else if (!prev_valid) {
				SetEnabledDialogControl(_i_proceed, true);
			}


		} else if (destination.empty()) {
			if (!_prev_destination.empty()) {
				SetEnabledDialogControl(_i_proceed, false);
			}

		} else {
			if (_prev_destination.empty()) {
				SetEnabledDialogControl(_i_proceed, true);
			}

			if (_xk == XK_MOVE && (_xd == XD_DOWNLOAD || _xd == XD_CROSSLOAD)) {
				if (destination.find("/") == std::string::npos) {
					if (_prev_destination.find("/") != std::string::npos
					 || _prev_destination.empty()) {
						TextToDialogControl(_i_dblbox, MXferRenameTitle);
						TextToDialogControl(_i_text, MXferRenameText);
						TextToDialogControl(_i_proceed, MProceedRename);
					}
				} else if (_prev_destination.find("/") == std::string::npos
					 || _prev_destination.empty()) {
					TextToDialogControl(_i_dblbox, MXferMoveDownloadTitle);
					TextToDialogControl(_i_text, MXferMoveDownloadText);
					TextToDialogControl(_i_proceed, MProceedMoveDownload);
				}
			}
		}

		_prev_destination.swap(destination);
	}

	return BaseDialog::DlgProc(msg, param1, param2);
}

