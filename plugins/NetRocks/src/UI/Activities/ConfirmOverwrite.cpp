#include <utils.h>
#include <TimeUtils.h>
#include "ConfirmOverwrite.h"
#include "../../Globals.h"


/*                             34                          62
345                        30       39 42                    64
 ====================== Download ============================
| Destination file already exists:                           |
| [EDITBOX                                                 ] |
| Source file information: [99999 Kb]  [DD/MM/YYYY hh:mm.ss] |
| Destination information: [        ]  [DD/MM/YYYY hh:mm.ss] |
|------------------------------------------------------------|
| [ ] Reme&mber my choice for current operation              |
|------------------------------------------------------------|
|  [&Overwrite]           [Overwrite with &newer] [&Skip]    |
|  [&Resume]  [Create &different name]      [    Cancel  ]   |
 ============================================================
    6                     29   34  38                      60
*/

ConfirmOverwrite::ConfirmOverwrite(XferKind xk, XferDirection xd, const std::string &destination, const timespec &src_ts,
	unsigned long long src_size, const timespec &dst_ts, unsigned long long dst_size)
{
	if (xk == XK_COPY) {
		_di.SetBoxTitleItem((xd == XD_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
	} else if (xk == XK_MOVE) {
		_di.SetBoxTitleItem((xd == XD_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
	} else {
		_di.SetBoxTitleItem(MXferRenameTitle);
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
	_i_remember = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MRememberChoice);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_overwrite = _di.AddAtLine(DI_BUTTON, 16,29, DIF_CENTERGROUP, MXferDOAOverwrite);
	_i_overwrite_newer = _di.AddAtLine(DI_BUTTON, 30,62, DIF_CENTERGROUP, MXferDOAOverwriteIfNewer);
	_i_skip = _di.AddAtLine(DI_BUTTON, 5,15, DIF_CENTERGROUP, MXferDOASkip);

	_di.NextLine();
	_i_resume = _di.AddAtLine(DI_BUTTON, 5,15, DIF_CENTERGROUP, MXferDOAResume);
	_i_create_diff_name = _di.AddAtLine(DI_BUTTON, 16,49, DIF_CENTERGROUP, MXferDOACreateDifferentName);
	_di.AddAtLine(DI_BUTTON, 50,62, DIF_CENTERGROUP, MCancel);

	if (G.GetGlobalConfigBool("EnableDesktopNotifications", true)) {
		G.info.FSF->DisplayNotification(L"far2l - NetRocks", G.GetMsgWide(MXferConfirmOverwriteNotify));
	}

	SetDefaultDialogControl(_i_overwrite);
}


XferOverwriteAction ConfirmOverwrite::Ask(XferOverwriteAction &default_xoa)
{
	int r = Show(L"ConfirmOverwrite", 6, 2, FDLG_WARNING);

	XferOverwriteAction out = XOA_CANCEL;

	if (r == _i_skip) {
		out = XOA_SKIP;

	} else if (r == _i_overwrite) {
		out = XOA_OVERWRITE;

	} else if (r == _i_overwrite_newer) {
		out = XOA_OVERWRITE_IF_NEWER;

	} else if (r == _i_resume) {
		out = XOA_RESUME;

	} else if (r == _i_create_diff_name) {
		out = XOA_CREATE_DIFFERENT_NAME;
	}

	if (out != XOA_CANCEL && IsCheckedDialogControl(_i_remember)) {
		default_xoa = out;
	}

	return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////

