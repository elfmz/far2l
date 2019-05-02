#include "ConfirmOverwrite.h"
#include "../Globals.h"
#include "../Utils.h"



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

ConfirmOverwrite::ConfirmOverwrite(XferKind xk, XferDirection xd, const std::string &destination, const timespec &src_ts,
	unsigned long long src_size, const timespec &dst_ts, unsigned long long dst_size)
{
	if (xk == XK_COPY) {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,13, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
	} else {
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,13, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
	}

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,62, 0, MDestinationExists);

	_di.NextLine();
	_i_destination = _di.AddAtLine(DI_TEXT, 5,62, 0, destination.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,29, 0, MSourceInfo);
	_i_source_size = _di.AddAtLine(DI_TEXT, 30,39, 0, FileSizeString(src_size).c_str());
	_i_source_timestamp = _di.AddAtLine(DI_TEXT, 42,62, 0, TimeString(src_ts, TSF_FOR_UI).c_str() );

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,29, 0, MDestinationInfo);
	_i_destination_size = _di.AddAtLine(DI_TEXT, 30,39, 0, FileSizeString(dst_size).c_str());
	_i_destination_timestamp = _di.AddAtLine(DI_TEXT, 42,62, 0, TimeString(dst_ts, TSF_FOR_UI).c_str() );

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,62, 0, MOverwriteOptions);

	_di.NextLine();
	_i_remember = _di.AddAtLine(DI_RADIOBUTTON, 5,33, 0, MRememberSelection);
	_i_overwrite = _di.AddAtLine(DI_CHECKBOX, 34,62, DIF_GROUP, MXferDOAOverwrite);

	_di.NextLine();
	_i_skip = _di.AddAtLine(DI_RADIOBUTTON, 5,33, 0, MXferDOASkip);
	_i_overwrite_newer = _di.AddAtLine(DI_RADIOBUTTON, 34,62, 0, MXferDOAOverwriteIfNewer);

	_di.NextLine();
	_i_resume = _di.AddAtLine(DI_RADIOBUTTON, 5,33, 0, MXferDOAResume);
	_i_create_diff_name = _di.AddAtLine(DI_RADIOBUTTON, 34,62, 0, MXferDOACreateDifferentName);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_proceed = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK, nullptr, FDIS_DEFAULT);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);
}


XferOverwriteAction ConfirmOverwrite::Ask(XferOverwriteAction &default_xoa)
{
	_di[_i_overwrite].Selected = 1;

	if (Show(_di[_i_dblbox].Data, 6, 2, FDLG_WARNING) != _i_proceed)
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

