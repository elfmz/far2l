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
		_di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadText : MXferCopyUploadText);
	} else {
		_di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadText : MXferMoveUploadText);
	}

	_i_destination = _di.Add(DI_EDIT, 5,3,62,3, 0, destination.c_str());

	_di.Add(DI_TEXT, 4,4,63,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.Add(DI_TEXT, 5,5,62,5, 0, MXferDOAText);

	_i_doa_ask = _di.Add(DI_CHECKBOX, 5,6,29,6, 0, MXferDOAAsk);
	_i_doa_overwrite = _di.Add(DI_CHECKBOX, 30,6,62,6, 0, MXferDOAOverwrite);

	_i_doa_skip = _di.Add(DI_CHECKBOX, 5,7,29,7, 0, MXferDOASkip);
	_i_doa_overwrite_newer = _di.Add(DI_CHECKBOX, 30,7,62,7, 0, MXferDOAOverwriteIfNewer);

	_i_doa_resume = _di.Add(DI_CHECKBOX, 5,8,29,8, 0, MXferDOAResume);
	_i_doa_create_diff_name = _di.Add(DI_CHECKBOX, 30,8,62,8, 0, MXferDOACreateDifferentName);

	_di.Add(DI_TEXT, 4,9,63,9, DIF_BOXCOLOR | DIF_SEPARATOR);

	if (xk == XK_COPY) {
		_i_proceed = _di.Add(DI_BUTTON, 7,10,29,10, DIF_CENTERGROUP, (xd == XK_DOWNLOAD) ? MProceedCopyDownload : MProceedCopyUpload);
	} else {
		_i_proceed = _di.Add(DI_BUTTON, 7,10,29,10, DIF_CENTERGROUP, (xd == XK_DOWNLOAD) ? MProceedMoveDownload : MProceedMoveUpload);
	}

	_i_cancel = _di.Add(DI_BUTTON, 38,10,58,10, DIF_CENTERGROUP, MCancel);
}


bool XferConfirm::Confirm(XferDefaultOverwriteAction &xdoa)
{
	int result = G.info.DialogEx(G.info.ModuleNumber, -1, -1, _di.EstimateWidth() + 6, _di.EstimateHeight() + 2,
		_di[_i_dblbox].Data, &_di[0], _di.size(), 0, 0, &sDlgProc, (LONG_PTR)(uintptr_t)this);

	return (result == _i_proceed);
}

