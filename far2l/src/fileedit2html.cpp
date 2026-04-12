/*
fileedit2html.cpp

Редактирование файла - выгрузка текста из редактора в виде HTML
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include <limits>
#include "fileedit.hpp"
#include "edit.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "printersupport.hpp"

#include <algorithm> 
#include <cmath>


//////////////////////////////////
// Represent char - final code table

struct CharMapItem {
	wchar_t c;
	FarTrueColor color;

	CharMapItem() {
		c = L' ';
		color.R = color.G = color.B = 0; // black;
	}

	CharMapItem(wchar_t cc) {
		c = cc;
		color.R = color.G = color.B = 0; // black;
	}

	bool sameColor(FarTrueColor x) const {
		return x.R == color.R && x.G == color.G && x.B == color.B;
	}

	std::string hexColorCode() const {
		return StrPrintf("%2.2x%2.2x%2.2x", (int)color.R & 0xFF, (int)color.G & 0xFF, (int)color.B & 0xFF);
	}
};

struct ColorMap : std::vector<CharMapItem>
{
	ColorMap(const wchar_t *w, int start, int length)
	{
		const int len = length <= 0 ? wcslen(w) - start : length;
		if (len > 0) {
			reserve(len);
			for (int i = 0; i < len; ++i) {
				emplace_back(w[i + start]);
			}
		}
	}

	void Apply(const FarTrueColor& newcolor, int start, int finish)
	{
		finish = std::min(finish, int(size()) - 1);
		for(int i = start; i <= finish; ++i) {
			operator[](i).color = newcolor;
		}
	}
};

// color conversion

static FarTrueColor ConvertForPrintLAB(const FarTrueColor& in, const FarTrueColor& bg) {
	ColorspaceSupport cs;
	return cs.ConvertForPrintLAB(in, bg);
}

static void appendNewLine(std::string &tb, bool isHtml) {
	tb.append(isHtml ? "<br>\n" : "\n");
}

static void escapeHtmlTags(std::string &tb, const wchar_t c, int tabSize) {
	// if (c == L' ') tb.append("&nbsp;");
	// else 
	if (c == L'\t') {
		for(int i = 0; i < tabSize; ++i) tb.append("&nbsp;");
	}
	else if (c == L'\r') ;
	else if (c == L'\n') tb.append("<br>\n");
	else if (c == L'&')  tb.append("&amp;");
	else if (c == L'<')  tb.append("&lt;");
	else if (c == L'>')  tb.append("&gt;");
	else Wide2MB(&c, 1, tb, true);
}

static void escapeHtmlTags(std::string &tb, const wchar_t* s, int len, bool isHtml, int tabSize) {
	if (!isHtml) {
		Wide2MB(s, len, tb, true);
		return;
	}

	// we need to handle space, tab, newline, & < >
	if (len <= 0) len = wcslen(s);
	for(;len-- && *s; ++s) {
		wchar_t c = *s;
		escapeHtmlTags(tb, c, tabSize);
	}
}

static bool isEmptyOrSpace(const wchar_t* s, int start, int len) 
{
	bool space = true;
	for(int i = start; i < start + len && space; ++i)
		if (s[i] != L' ') space = false;
	return space;
}

#define toI(x) ((int)(x) & 0xFF)
#define toB(x) ((unsigned char)((x) & 0xFF))

static bool convertToReducedHTML(std::string &tb, Edit* line, int start, int len, int tabSize)
{
	if (len <= 0) len = line->GetLength() - start;
	int end = start + len;

	const wchar_t *CurStr = 0, *EndSeq = 0;
	int Length;
	ColorItem ci;

	line->GetBinaryString(&CurStr, &EndSeq, Length);

	if (!CurStr) return false;

	// fprintf(stderr, "colorize: `%ls` [%d..%d]\n", CurStr, start, start + end);

	ColorMap cm(CurStr, start, end);

	for (int i = 0; line->GetColor(&ci, i); ++i) {
		if ((ci.StartPos == -1 && ci.EndPos == -1) || ci.StartPos > ci.EndPos) {
			// background
			continue;
		}

		ci.StartPos = line->RealPosToCell(ci.StartPos);
		ci.EndPos = line->RealPosToCell(ci.EndPos);

		if (ci.EndPos > start + end) ci.EndPos = start + end;

		if (ci.StartPos < 0) ci.StartPos = 0;
		if (ci.EndPos < 0) ci.EndPos = end;

		if (ci.StartPos > end || ci.EndPos < start) continue;

		EditorTrueColor tcol;
		FarTrueColorFromAttributes(tcol.TrueColor, ci.Color);
		FarTrueColor rgb = tcol.TrueColor.Fore; 
		FarTrueColor bgk = tcol.TrueColor.Back; 

		FarTrueColor print = ConvertForPrintLAB(rgb, bgk);

		// Note: tabs might need to update positions


		cm.Apply(print, ci.StartPos, ci.EndPos);
	}

	// now we have rendered every character in cm so we can iterate through it
	if (!cm.empty()) {
		FarTrueColor prev{};
		// tb.append("<pre>");
		for(size_t i = 0; i < cm.size(); ++i) {
			if (i == 0 || !cm[i].sameColor(prev)) {
				if (i != 0) tb.append("</font>");
				tb.append("<font color=\"#");
				tb.append(cm[i].hexColorCode());
				tb.append("\">");
				prev = cm[i].color;
			}
			escapeHtmlTags(tb, cm[i].c, tabSize);
		}
		tb.append("</font>");
		// tb.append("</pre>");
	}

	// fprintf(stderr, "colorize: `%.*ls` => `%s`\n", len, CurStr + start,	tb.c_str());

	tb+= '\n';

	return true;
}

BOOL FileEditor::SendToPrinter()
{
	PrinterSupport printer;
	std::string tb;
	int tab = m_editor->EdOpt.TabSize;

	const wchar_t *CurStr = 0, *EndSeq = 0;
	int StartSel = -1, EndSel = -1;
	int Length = 0;

	// first, try to check against selection
	if (m_editor->BlockStart) { // we have block selection active
    	for (Edit *Ptr = m_editor->BlockStart; Ptr; Ptr = Ptr->m_next) {
    		Ptr->GetSelection(StartSel, EndSel);
    		if (StartSel == -1)	break;

			if (EndSel == -1)
				Length = Ptr->GetLength() - StartSel;
			else
				Length = EndSel - StartSel;

			if (Length > 0 && tb.empty() && printer.IsReducedHTMLSupported())
				tb.append(HTML_PRE_HEADER);

			if(!printer.IsReducedHTMLSupported() || !convertToReducedHTML(tb, Ptr, StartSel, Length, tab)) {
				int Len2 = 0;
				Ptr->GetBinaryString(&CurStr, &EndSeq, Len2);
				escapeHtmlTags(tb, CurStr + StartSel, Length, printer.IsReducedHTMLSupported(), tab);
				tb+= '\n';
			}
    	}
	}
	else if (m_editor->VBlockStart) { // we have vertical block selection active
    	Edit *CurPtr = m_editor->VBlockStart;

    	for (int Line = 0; CurPtr && Line < m_editor->VBlockSizeY; Line++, CurPtr = CurPtr->m_next) {
    		int TBlockX = CurPtr->CellPosToReal(m_editor->VBlockX);
    		int TBlockSizeX = CurPtr->CellPosToReal(m_editor->VBlockX + m_editor->VBlockSizeX) - TBlockX;

    		CurPtr->GetBinaryString(&CurStr, &EndSeq, Length);

    		if (Length > TBlockX) {
    			int CopySize = Length - TBlockX;

				if (CopySize > TBlockSizeX)	CopySize = TBlockSizeX;

				if (CopySize > 0 && tb.empty() && printer.IsReducedHTMLSupported())
					tb.append(HTML_PRE_HEADER);

				if(!printer.IsReducedHTMLSupported() || !convertToReducedHTML(tb, CurPtr, TBlockX, CopySize, tab)) {
					escapeHtmlTags(tb, CurStr + TBlockX, CopySize, printer.IsReducedHTMLSupported(), tab);
					tb+= '\n';
				}
    		}
    	}
	}

	// something is selected
	if (!tb.empty()) {
		if(printer.IsReducedHTMLSupported())
			tb.append(HTML_PRE_FOOTER);

		std::wstring wtb; StrMB2Wide(tb, wtb);
		if (printer.IsPrintPreviewSupported()) {
			if (printer.IsReducedHTMLSupported()) 
				printer.ShowPreviewForReducedHTML(strFullFileName.GetWide(), wtb.c_str());
			else 
				printer.ShowPreviewForText(strFullFileName.GetWide(), wtb.c_str());
		}
		else {
			if (printer.IsReducedHTMLSupported()) 
				printer.PrintReducedHTML(strFullFileName.GetWide(), wtb.c_str());
			else 
				printer.PrintText(strFullFileName.GetWide(), wtb.c_str());
		}
		return TRUE;
	}

	// we need to print whole file
	FILE* fp = printer.BeginPrint();
	if (fp) {
		std::string _tmpstr;

		for (Edit *CurPtr = m_editor->TopList; CurPtr; CurPtr = CurPtr->m_next) {
			const wchar_t *SaveStr, *EndSeq;

			CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);

			std::string tb;
			if (printer.IsReducedHTMLSupported() && convertToReducedHTML(tb, CurPtr, 0, Length, tab))  {
				fputs(tb.c_str(), fp);
			} else {
				Wide2MB(SaveStr, Length, _tmpstr);
				fwrite(_tmpstr.data(), 1, _tmpstr.size(), fp);
				fputc('\n', fp);
			}
		}

		printer.EndPrint(fp);
	}

	return TRUE;
}

//////////////////////////////////
