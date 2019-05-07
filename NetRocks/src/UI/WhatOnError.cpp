#include <utils.h>
#include "WhatOnError.h"
#include "../Globals.h"



/*                                                         62
345              20                                          64
 ====================== Download error ======================
| Error:         [EDITBOX                                  ] |
| Object:        [EDITBOX                                  ] |
| Site:          [EDITBOX                                  ] |
|------------------------------------------------------------|
| [ ] Re&member my choice for this operation                 |
|------------------------------------------------------------|
|   [ &Retry ]  [ &Skip ]         [        &Cancel       ]   |
 ============================================================
    6                     29       38                      60
*/

WhatOnError::WhatOnError(WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site)
{
	int title_lng;
	switch (wek) {
		case WEK_DOWNLOAD:  title_lng = MErrorDownloadTitle; break;
		case WEK_UPLOAD:    title_lng = MErrorUploadTitle; break;
		case WEK_CROSSLOAD: title_lng = MErrorCrossloadTitle; break;
		case WEK_MAKEDIR:   title_lng = MErrorMakeDirTitle; break;
		case WEK_RMFILE:    title_lng = MErrorRmFileTitle; break;
		case WEK_RMDIR:     title_lng = MErrorRmDirTitle; break;
		default: {
			fprintf(stderr, "Error: wrong kind %u\n", wek);
			abort();
		}
	}

	_di.Add(DI_DOUBLEBOX, 3,1,64,9, 0, title_lng);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,19, 0, MErrorError);
	_di.AddAtLine(DI_TEXT, 20,62, 0, error.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,19, 0, MErrorObject);
	_di.AddAtLine(DI_TEXT, 20,62, 0, object.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,19, 0, MErrorSite);
	_di.AddAtLine(DI_TEXT, 20,62, 0, site.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_remember = _di.AddAtLine(DI_CHECKBOX, 5,33, 0, MRememberChoice);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_retry = _di.AddAtLine(DI_BUTTON, 5,20, DIF_CENTERGROUP, MRetry);
	_i_skip = _di.AddAtLine(DI_BUTTON, 21,40, DIF_CENTERGROUP, MSkip);
	_di.AddAtLine(DI_BUTTON, 41,60, DIF_CENTERGROUP, MCancel);
}


WhatOnErrorAction WhatOnError::Ask(WhatOnErrorAction &default_wea)
{
	int r = Show(L"WhatOnError", 6, 2, FDLG_WARNING);

	WhatOnErrorAction out = WEA_CANCEL;

	if (r == _i_retry) {
		out = WEA_RETRY;

	} else if (r == _i_skip) {
		out = WEA_SKIP;
	}

	if (out != WEA_CANCEL && IsCheckedDialogControl(_i_remember)) {
		default_wea = out;
	}

	return out;
}
