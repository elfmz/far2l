#include "edsort.h"

#define DIF_DEFAULT 0x200
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

    const intptr_t rc = dlg.DialogRun();
    if (rc >= 0 && rc == dlg.getID("bn_ok") ) {
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

	if (ei.BlockType != BTYPE_STREAM) {
		// working only if not vertical block selection
		return;
	}

	EditorGetString egs = {};
	egs.StringNumber = ei.BlockStartLine;
	_PSI.EditorControl(ECTL_GETSTRING, &egs);
	if (egs.SelEnd != -1) {
		return;
	}

	EditorUndoRedo eur = {};
	eur.Command = EUR_BEGIN;
	_PSI.EditorControl(ECTL_UNDOREDO, &eur);

	std::vector<std::wstring> linesArray;
	for (int lno = ei.BlockStartLine; lno < ei.TotalLines; lno++) {
		EditorSetPosition esp = {};
		esp.CurLine = lno;
		esp.CurPos = esp.Overtype = 0;
		esp.CurTabPos = esp.TopScreenLine = esp.LeftPos = -1;
		_PSI.EditorControl(ECTL_SETPOSITION, &esp);

		EditorGetString egs = {};
		egs.StringNumber = -1;
		_PSI.EditorControl(ECTL_GETSTRING, &egs);

		if (egs.SelStart == -1 || egs.SelStart == egs.SelEnd) {
			// Stop when reaching the end of the text selection
			break;
		}

		if (egs.SelEnd != -1) {
			// Extend selection to line end
			EditorSelect es = {};
			es.BlockType = ei.BlockType;
			es.BlockStartLine = ei.BlockStartLine;
			es.BlockStartPos = 0;
			es.BlockWidth = 0;
			es.BlockHeight = lno - ei.BlockStartLine + 1;
			_PSI.EditorControl(ECTL_SELECT, &es);
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
