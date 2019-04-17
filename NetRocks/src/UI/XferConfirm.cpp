#include "XferConfirm.h"
#include "../Globals.h"



/*                                                         62
345                      28         39                   60  64
 ====================== Download =============================
| Download selected file &to:                                |
| [EDITBOX                                                 ] |
|------------------------------------------------------------|
| Default overwrite action:                                  |
| [x] &Ask                 [ ] &Overwrite                    |
| [ ] &Skip                [ ] Overwrite with &newer         |
| [ ] &Resume              [ ] Create &different name        |
|------------------------------------------------------------|
|   [  Proceed dowmload   ]        [        Cancel       ]   |
 =============================================================
    6                     29       38                      60
*/

XferConfirm::XferConfirm(XferKind xk, XferDirection xd, const std::string &destination)
{
	if (xk == XK_COPY) {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadText : MXferCopyUploadText);
	} else {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadText : MXferMoveUploadText);
	}

	_i_destination = _di.Add(DI_EDIT, 5,3,62,3, 0, destination.c_str());

	_di.Add(DI_TEXT, 4,4,63,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.Add(DI_TEXT, 5,5,62,5, 0, MXferDOAText);

	_i_doa_ask = _di.Add(DI_RADIOBUTTON, 5,6,29,6, DIF_GROUP, MXferDOAAsk);
	_i_doa_overwrite = _di.Add(DI_RADIOBUTTON, 30,6,62,6, 0, MXferDOAOverwrite);

	_i_doa_skip = _di.Add(DI_RADIOBUTTON, 5,7,29,7, 0, MXferDOASkip);
	_i_doa_overwrite_newer = _di.Add(DI_RADIOBUTTON, 30,7,62,7, 0, MXferDOAOverwriteIfNewer);

	_i_doa_resume = _di.Add(DI_RADIOBUTTON, 5,8,29,8, 0, MXferDOAResume);
	_i_doa_create_diff_name = _di.Add(DI_RADIOBUTTON, 30,8,62,8, 0, MXferDOACreateDifferentName);

	_di.Add(DI_TEXT, 4,9,63,9, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 7,10,29,10, DIF_CENTERGROUP, (xk == XK_COPY) ?
		((xd == XK_DOWNLOAD) ? MProceedCopyDownload : MProceedCopyUpload) :
		((xd == XK_DOWNLOAD) ? MProceedMoveDownload : MProceedMoveUpload) );

	_i_cancel = _di.Add(DI_BUTTON, 38,10,58,10, DIF_CENTERGROUP, MCancel);
}


bool XferConfirm::Confirm(XferDefaultOverwriteAction &xdoa)
{
	switch (xdoa) {
		case XDOA_SKIP:
			_di[_i_doa_skip].Selected = 1;
			break;

		case XDOA_RESUME:
			_di[_i_doa_resume].Selected = 1;
			break;

		case XDOA_OVERWRITE:
			_di[_i_doa_overwrite].Selected = 1;
			break;

		case XDOA_OVERWRITE_IF_NEWER:
			_di[_i_doa_overwrite_newer].Selected = 1;
			break;

		case XDOA_CREATE_DIFFERENT_NAME:
			_di[_i_doa_create_diff_name].Selected = 1;
			break;

		case XDOA_ASK: default:
			_di[_i_doa_ask].Selected = 1;
	}

	if (Show(6, 2, _di[_i_dblbox].Data) != _i_proceed)
		return false;

	if (_di[_i_doa_skip].Selected) {
		xdoa = XDOA_SKIP;

	} else if (_di[_i_doa_resume].Selected) {
		xdoa = XDOA_RESUME;

	} else if (_di[_i_doa_overwrite].Selected) {
		xdoa = XDOA_OVERWRITE;

	} else if (_di[_i_doa_overwrite_newer].Selected) {
		xdoa = XDOA_OVERWRITE_IF_NEWER;

	} else if (_di[_i_doa_create_diff_name].Selected) {
		xdoa = XDOA_CREATE_DIFFERENT_NAME;

	} else {
		xdoa = XDOA_ASK;
	}

	return true;
}

