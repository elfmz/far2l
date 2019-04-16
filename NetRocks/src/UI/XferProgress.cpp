#include "XferProgress.h"
#include "../Globals.h"


/*                                                         62
345                  24       33      41 44        54  58  60  64
 ====================== Download =============================
| Current file:                                              |
| [EDITBOX                                                 ] |
| Current file progress:       [====================]  ###%  |
| Total progress:              [====================]  ###%  |
| Current file time  SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Total time         SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Speed (?bps)     CURRENT:   [TEXT   ]  AVERAGE:  [TEXT   ] |
|------------------------------------------------------------|
| Default overwrite action:           [COMBOBOX            ] |
|------------------------------------------------------------|
| [    &Background    ]    [    &Pause    ]  [   &Cancel   ] |
 =============================================================
  5                   25   30             45 48            60
*/

XferProgress::XferProgress(XferKind xk, XferDirection xd, const std::string &destination)
{
	_di_doa.Add(_i_doa_ask);
	_di_doa.Add(_i_doa_skip);
	_di_doa.Add(_i_doa_resume);
	_di_doa.Add(_i_doa_overwrite);
	_di_doa.Add(_i_doa_overwrite_newer);
	_di_doa.Add(_i_doa_create_diff_name);

	if (xk == XK_COPY) {
		_di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadText : MXferCopyUploadText);
	} else {
		_di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadText : MXferMoveUploadText);
	}

	_di.Add(DI_TEXT, 5,3,62,3, 0, MXferCurrentFile);
	_i_cur_file = _di.Add(DI_EDIT, 5,4,62,4, 0, "...");

	_di.Add(DI_TEXT, 5,5,33,5, 0, MXferCurrentProgress);
	_i_cur_file_progress_bar = _di.Add(DI_TEXT, 34,5,55,5, 0, "[                    ]");
	_i_cur_file_progress_perc = _di.Add(DI_TEXT, 58,5,61,5, 0, "???%");

	_di.Add(DI_TEXT, 5,6,33,6, 0, MXferTotalProgress);
	_i_total_progress_bar = _di.Add(DI_TEXT, 34,5,55,5, 0, "[                    ]");
	_i_total_progress_perc = _di.Add(DI_TEXT, 58,5,61,5, 0, "???%");

	_di.Add(DI_TEXT, 5,7,32,7, 0, MXferCurrentTimeSpent);
	_i_current_time_spent = _di.Add(DI_TEXT, 33,7,41,7, 0, "???:??.??");
	_di.Add(DI_TEXT, 44,7,53,7, 0, MXferRemain);
	_i_current_time_remain = _di.Add(DI_TEXT, 54,7,60,7, 0, "???:??.??");

	_di.Add(DI_TEXT, 5,8,32,8, 0, MXferTotalTimeSpent);
	_i_total_time_spent = _di.Add(DI_TEXT, 33,8,41,8, 0, "???:??.??");
	_di.Add(DI_TEXT, 44,8,53,8, 0, MXferRemain);
	_i_total_time_remain = _di.Add(DI_TEXT, 54,8,60,8, 0, "???:??.??");


	_i_speed_current_label = _di.Add(DI_TEXT, 5,9,32,9, 0, MXferSpeedCurrent);
	_i_speed_current = _di.Add(DI_TEXT, 33,9,41,9, 0, "???");
	_di.Add(DI_TEXT, 44,9,53,9, 0, MXferAverage);
	_i_total_time_remain = _di.Add(DI_TEXT, 54,9,60,9, 0, "???");

	_di.Add(DI_TEXT, 4,10,63,10, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.Add(DI_TEXT, 5,11,40,11, 0, MXferDOAText);

	_i_doa = _di.Add(DI_COMBOBOX, 41,11,60,11, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
	_di[_i_doa].ListItems = _di_doa.Get();

	_di.Add(DI_TEXT, 4,12,63,12, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_background = _di.Add(DI_BUTTON, 5,13,25,13, DIF_CENTERGROUP, MBackground);
	_i_pause_resume = _di.Add(DI_BUTTON, 30,13,45,13, DIF_CENTERGROUP, MPause); // MResume
	_i_cancel = _di.Add(DI_BUTTON, 48,13,60,13, DIF_CENTERGROUP, MCancel);
}


bool XferProgress::Confirm(XferDefaultOverwriteAction &xdoa)
{
	int result = G.info.DialogEx(G.info.ModuleNumber, -1, -1, _di.EstimateWidth() + 6, _di.EstimateHeight() + 2,
		_di[_i_dblbox].Data, &_di[0], _di.size(), 0, 0, &sDlgProc, (LONG_PTR)(uintptr_t)this);

	return (result == _i_proceed);
}

