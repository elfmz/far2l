#include "edsort.h"

#define DIF_DEFAULT 0x200

static constexpr FARMESSAGEFLAGS MB_mask
= FMSG_MB_OK					// =
| FMSG_MB_OKCANCEL			// -
| FMSG_MB_ABORTRETRYIGNORE // -
| FMSG_MB_YESNO				// +
| FMSG_MB_YESNOCANCEL		// +
| FMSG_MB_RETRYCANCEL      // +
;

static auto msg_box(
	const FARMESSAGEFLAGS f, const wchar_t* const* i, const size_t ntext
) {
	auto flags(f);
	auto items(i);
	intptr_t nbutt(0);
	const wchar_t *b1= nullptr, *b2= nullptr, *b3= nullptr;
	const wchar_t* my_items[20]; // big enough to append buttons

	switch (f & MB_mask) {
	case FMSG_MB_YESNO:       b1 = I18N(ps__yes);   b2 = I18N(ps__no); break;
	case FMSG_MB_YESNOCANCEL: b1 = I18N(ps__yes);   b2 = I18N(ps__no); b3 = I18N(ps__cancel); break;
	case FMSG_MB_RETRYCANCEL: b1 = I18N(ps__retry); b2 = I18N(ps__cancel); break;
	}
	if (b1) {
		for (size_t j = 0; j < ntext; ++j) my_items[j] = i[j];
		items = &my_items[0];
		flags &= ~MB_mask;
		my_items[ntext + nbutt++] = b1;
		if (b2) my_items[ntext + nbutt++] = b2;
		if (b3) my_items[ntext + nbutt++] = b3;
	}

	return _PSI.Message(_PSI.ModuleNumber, flags, nullptr, items, ntext+nbutt, nbutt);
}

template <const size_t N>
static inline auto msg_box(const FARMESSAGEFLAGS f, const wchar_t* (&msgs)[N]) {
	return msg_box(f, msgs, N);
}

EdSort::EdSort()
    : column(0),
      myDialog(nullptr)
{
}

LONG_PTR WINAPI EdSort::dlg_proc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if( Msg == DN_INITDIALOG ) {
        myDialog->SetText(myDialog->getID("offset"), L"1");
		return TRUE;
	}
    return _PSI.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void EdSort::Run()
{
	EditorInfo ei={};
	_PSI.EditorControl(ECTL_GETINFO, &ei);

	if (ei.BlockType != BTYPE_STREAM) {
		// working only if not vertical block selection
		const wchar_t* err_msg[] = {
			I18N(ps_title), I18N(ps_err_block_type)
		};
		msg_box(FMSG_WARNING | FMSG_MB_OK, err_msg);
		return;
	}
	int lno;
	bool firstineok = true;
	bool lastlineok = true;
	for (lno = ei.BlockStartLine; lno < ei.TotalLines; lno++) {
		EditorGetString egs = {};
		egs.StringNumber = lno;
		_PSI.EditorControl(ECTL_GETSTRING, &egs);

		if (egs.SelStart == -1 || egs.SelStart == egs.SelEnd) {
			// Stop when reaching the end of the text selection
			break;
		}
		if( lno == ei.BlockStartLine && egs.SelStart != 0) {
			firstineok = false;
		}
		if (egs.SelEnd != -1) {
			lastlineok = false;
			break;
		}
	}
	if( lno < 2 ) {
		// need at least two lines to sort
		const wchar_t* err_msg[] = {
			I18N(ps_title), I18N(ps_err_block_lines)
		};
		msg_box(FMSG_WARNING | FMSG_MB_OK, err_msg);
		return;
	}

	EditorSelect es = {};
	es.BlockType = ei.BlockType;
	es.BlockStartLine = ei.BlockStartLine;
	es.BlockStartPos = 0;
	es.BlockWidth = 0;
	es.BlockHeight = lno - ei.BlockStartLine;
	if( !firstineok ) {
		const wchar_t* err_msg[] = {
			I18N(ps_title), I18N(ps_err_block_whole1), I18N(ps_err_block_whole2f),
			I18N(ps_err_block_whole3), I18N(ps_err_block_whole4)
		};
		auto mbrc = msg_box(FMSG_WARNING | FMSG_MB_YESNOCANCEL, err_msg);
		if( mbrc == 2 || mbrc < 0)
			return;
		// Include/exclude first line from selection
		if( mbrc == 1 )
			es.BlockStartLine += 1;
	}
	if( !lastlineok ) {
		const wchar_t* err_msg[] = {
			I18N(ps_title), I18N(ps_err_block_whole1), I18N(ps_err_block_whole2l),
			I18N(ps_err_block_whole3), I18N(ps_err_block_whole4)
		};
		auto mbrc = msg_box(FMSG_WARNING | FMSG_MB_YESNOCANCEL, err_msg);
		if( mbrc == 2 || mbrc < 0)
			return;
		// Include/exclude last line from selection
		es.BlockHeight = lno - es.BlockStartLine + (mbrc == 0 ? 2 : 1);
	}
	if(!firstineok || !lastlineok){
		_PSI.EditorControl(ECTL_SELECT, &es);
		_PSI.EditorControl(ECTL_REDRAW, nullptr);
	}
	fardialog::DlgTEXT text1(nullptr, I18N(ps_column));
    fardialog::DlgEDIT edit1("offset", 5);
    fardialog::DlgCHECKBOX checkbox3("reverse", I18N(ps_reverse), false);
    fardialog::DlgHLine hline1(nullptr, nullptr);
    fardialog::DlgBUTTON button3("bn_ok", I18N(ps_ok), DIF_CENTERGROUP | DIF_DEFAULT, false, true);
    fardialog::DlgBUTTON button4("bn_cancel", I18N(ps_cancel), DIF_CENTERGROUP);

    fardialog::DlgHSizer hbox1({ &text1, &edit1 });
    fardialog::DlgHSizer hbox2({ &button3, &button4 });

    fardialog::DlgVSizer vbox1({
        &hbox1, // test1, edit1
		&checkbox3,
        &hline1,
        &hbox2, // button3, button4
    });

    auto dlg = fardialog::CreateDialog(
        I18N(ps_title),
        I18N(ps_helptopic),
        0,
        *this,
        &EdSort::dlg_proc,
        vbox1
    );
    myDialog=&dlg;

    const int rc = dlg.DialogRun();
    if ( rc == dlg.getID("bn_ok") ) {
		size_t col=0;
        std::wstring val(myDialog->GetText(myDialog->getID("offset")));
        if( val.length() > 0 )
			swscanf(val.c_str(), L"%lld", &col);
		if( col > 0 ){
			column = col - 1;
			reverse = myDialog->GetCheck(myDialog->getID("reverse")) != 0;
			SortLines();
		}
    }
    dlg.DialogFree();
    myDialog = nullptr;
}

void EdSort::SortLines()
{
	EditorInfo ei={};
	_PSI.EditorControl(ECTL_GETINFO, &ei);

	EditorUndoRedo eur = {};
	eur.Command = EUR_BEGIN;
	_PSI.EditorControl(ECTL_UNDOREDO, &eur);

	std::vector<std::wstring> linesArray;
	for (int lno = ei.BlockStartLine; lno < ei.TotalLines; lno++) {
		EditorGetString egs = {};
		egs.StringNumber = lno;
		_PSI.EditorControl(ECTL_GETSTRING, &egs);

		if (egs.SelStart == -1 || egs.SelStart == egs.SelEnd) {
			// Stop when reaching the end of the text selection
			break;
		}

		linesArray.push_back(egs.StringText);
	}
	_PSI.EditorControl(ECTL_DELETEBLOCK, nullptr);

	std::sort(linesArray.begin(), linesArray.end(), [this](const std::wstring &a, const std::wstring &b) {
		std::wstring sa = a.substr(column);
		std::wstring sb = b.substr(column);
		int rc = WINPORT(CompareString)(0, NORM_IGNORECASE | NORM_STOP_ON_NULL | SORT_STRINGSORT, sa.c_str(), -1, sb.c_str(), -1) - 2;
		if (reverse)
			return rc < 0;
		else
			return rc > 0;
	});

	for(int lno=0; lno < static_cast<int>(linesArray.size()); lno++) {
		EditorSetPosition esp = {};
		esp.CurLine = ei.BlockStartLine;
		esp.CurPos = esp.Overtype = 0;
		esp.CurTabPos = esp.TopScreenLine = esp.LeftPos = -1;
		_PSI.EditorControl(ECTL_SETPOSITION, &esp);

		_PSI.EditorControl(ECTL_INSERTTEXT, (void *)linesArray[lno].c_str());
		_PSI.EditorControl(ECTL_INSERTTEXT, (void *)L"\r");
	}
	{
		EditorSetPosition esp = {};
		esp.CurLine = ei.BlockStartLine;
		esp.CurPos = esp.Overtype = 0;
		esp.CurTabPos = esp.TopScreenLine = esp.LeftPos = -1;
		_PSI.EditorControl(ECTL_SETPOSITION, &esp);

		EditorSelect es = {};
		es.BlockType = ei.BlockType;
		es.BlockStartLine = ei.BlockStartLine;
		es.BlockStartPos = 0;
		es.BlockWidth = 0;
		es.BlockHeight = linesArray.size()+1;
		_PSI.EditorControl(ECTL_SELECT, &es);
	}
	linesArray.clear();

	eur.Command = EUR_END;
	_PSI.EditorControl(ECTL_UNDOREDO, &eur);

	_PSI.EditorControl(ECTL_REDRAW, NULL);
}
