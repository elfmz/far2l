#include "Confirm.h"
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

XferConfirm::XferConfirm(XferKind xk, XferDirection xd, const std::string &destination)
{
	if (xk == XK_COPY) {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,11, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadText : MXferCopyUploadText);
	} else {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,11, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
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
		((xd == XK_DOWNLOAD) ? MProceedMoveDownload : MProceedMoveUpload), nullptr, FDIS_DEFAULT);

	_i_cancel = _di.Add(DI_BUTTON, 38,10,58,10, DIF_CENTERGROUP, MCancel);
}


bool XferConfirm::Ask(XferDefaultOverwriteAction &xdoa)
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

	if (Show(_di[_i_dblbox].Data, 6, 2) != _i_proceed)
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

///////////////////////////////////////////////////////////////////////////////////////////////



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

RemoveConfirm::RemoveConfirm(const std::string &site_dir)
{
	_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,6, 0, MRemoveTitle);
	_di.Add(DI_TEXT, 5,2,62,2, 0, MRemoveText);

	_di.Add(DI_TEXT, 5,3,62,3, 0, site_dir.c_str());

	_di.Add(DI_TEXT, 4,4,63,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 7,5,29,5, DIF_CENTERGROUP, MProceedRemoval);
	_i_cancel = _di.Add(DI_BUTTON, 38,5,58,5, DIF_CENTERGROUP, MCancel, nullptr, FDIS_DEFAULT);
}


bool RemoveConfirm::Ask()
{
	return (Show(_di[_i_dblbox].Data, 6, 2, FDLG_WARNING) == _i_proceed);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////

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

MakeDirConfirm::MakeDirConfirm(const std::string &default_name)
{
	_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,50,6, 0, MMakeDirTitle);
	_di.Add(DI_TEXT, 5,2,48,2, 0, MMakeDirText);

	_i_dir_name = _di.Add(DI_EDIT, 5,3,48,3, DIF_HISTORY, default_name.c_str(), "NetRocks_History_MakeDir", FDIS_FOCUSED);

	_di.Add(DI_TEXT, 4,4,49,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_proceed = _di.Add(DI_BUTTON, 6,5,23,5, DIF_CENTERGROUP, MProceedMakeDir, nullptr, FDIS_DEFAULT);
	_i_cancel = _di.Add(DI_BUTTON, 30,5,45,5, DIF_CENTERGROUP, MCancel);
}

std::string MakeDirConfirm::Ask()
{
	std::string out;
	if (Show(_di[_i_dblbox].Data, 6, 2) == _i_proceed) {
		out = _di[_i_dir_name].Data;
	}
	return out;
}

