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
// Common class

class TextBuffer {
	char* buffer;
	char* cptr;
	int len;
	int size;
	wchar_t* wc;

#define GAP	1024
	void ensure(int sz) {
		if (sz + len > size) {
			size = sz + len + GAP;
			if (buffer){ 
				buffer = (char*)realloc(buffer, size);
			}
			else {
				buffer = (char*)malloc(size);
			}
			cptr = buffer + len;
		}
	}
#undef GAP

public:
	TextBuffer() {
		buffer = cptr = 0;
		len = size = 0;
		wc = 0;
	}

	~TextBuffer() {
		if (buffer) free(buffer);
		if (wc) free(wc);
		buffer = cptr = 0;
		len = size = 0;
		wc = 0;
	}

	const bool is_empty() const { return len == 0; }
	const int length() const { return len; }
	const char* c_str() const {	return buffer ? (const char*)buffer : ""; }

	const wchar_t* w_str() {
		if (wc) free(wc);
		std::wstring _tmpstr;
    	MB2Wide(c_str(), length(), _tmpstr);
        wc = wcsdup(_tmpstr.c_str());
        return wc;
	}

	void append(char c) {
		ensure(1);
        ++len;
        *cptr++ = c;
        *cptr = 0;
	}

	void append(wchar_t c) {
		wchar_t buf[2] = {c, 0};
		append(buf, 1);
	}

	void append(const char* s, int max = 0) {
		int delta = max > 0 ? max : strlen(s);
		ensure(delta + 1);
		len += delta;
		while(delta--) *cptr++ = *s++;
        *cptr = 0;
	}

	void append(const std::string& s, int max = 0) {
		append((const char*)s.c_str(), max);
	}

	void append(const wchar_t* w, int max = 0) {
		int delta = max > 0 ? max : wcslen(w);
		std::string _tmpstr;
		Wide2MB(w, delta, _tmpstr);
		append(_tmpstr);
	}

	void append(const std::wstring& w, int max = 0) {
		int delta = max > 0 ? max : w.length();
		std::string _tmpstr;
		Wide2MB(w.c_str(), delta, _tmpstr);
		append(_tmpstr);
	}

	void append(const FarTrueColor& color) {
		char buf[10];
		sprintf(buf, "%2.2x%2.2x%2.2x", (int)color.R & 0xFF, (int)color.G & 0xFF, (int)color.B & 0xFF);
		append(buf);
	}
};

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

	bool sameColor(FarTrueColor x) {
		return x.R == color.R && x.G == color.G && x.B == color.B;
	}
};

struct ColorMap {
	CharMapItem* s;
	int len;

	ColorMap(const wchar_t* w, int start, int length) {
		if (start < 0) start = 0;
		len = length <= 0 ? wcslen(w) - start : length;
		s = new CharMapItem[len];
		for(int i = 0; i < len; ++i) s[i] = CharMapItem(w[i + start]);
	}

	~ColorMap() {
		delete[] s;
	}

	void apply(const FarTrueColor& newcolor, int start, int end) {
		for(int i = start; i <= end && i < len; ++i) s[i].color = newcolor;
	}
};

// color conversion

static FarTrueColor ConvertForPrintLAB(const FarTrueColor& in, const FarTrueColor& bg) {
	ColorspaceSupport cs;
	return cs.ConvertForPrintLAB(in, bg);
}

static void appendNewLine(TextBuffer& tb, bool isHtml) {
	tb.append(isHtml ? "<br>\n" : "\n");
}

static void escapeHtmlTags(TextBuffer& tb, const wchar_t c, int tabSize) {
	if (c == L' ') tb.append("&nbsp;");
	else if (c == L'\t') {
		for(int i = 0; i < tabSize; ++i) tb.append("&nbsp;");
	}
	else if (c == L'\r') ;
	else if (c == L'\n') tb.append("<br>\n");
	else if (c == L'&')  tb.append("&amp;");
	else if (c == L'<')  tb.append("&lt;");
	else if (c == L'>')  tb.append("&gt;");
	else tb.append(c);
}

static void escapeHtmlTags(TextBuffer& tb, const wchar_t* s, int len, bool isHtml, int tabSize) {
	if (!isHtml) {
		tb.append(s, len);
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

static bool convertToReducedHTML(TextBuffer& tb, Edit* line, int start, int len, int tabSize)
{
	if (len <= 0) len = line->GetLength() - start;
	int end = start + len;

	const wchar_t *CurStr = 0, *EndSeq = 0;
	int Length;
	ColorItem ci;

	line->GetBinaryString(&CurStr, &EndSeq, Length);

	if (!CurStr) return false;

	// fprintf(stderr, "colorize: `%ls` [%d..%d]\n", CurStr, start, start + end);

	ColorMap map(CurStr, start, end);

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


		map.apply(print, ci.StartPos, ci.EndPos);
	}

	// now we have rendered every character in map so we can iterate through it
	FarTrueColor prev;
	prev.R = prev.G = prev.B = 0;
	bool colored = false;
	// tb.append("<pre>");
	for(int i = 0; i < map.len; ++i) {
		if (!map.s[i].sameColor(prev)) {
			if (colored) tb.append("</font>");
			colored = true;
			tb.append("<font color=\"#");
			tb.append(map.s[i].color);
			tb.append("\">");
			prev = map.s[i].color;
		}
		escapeHtmlTags(tb, map.s[i].c, tabSize);
	}
	if (colored) tb.append("</font>");
	// tb.append("</pre>");

	// fprintf(stderr, "colorize: `%.*ls` => `%s`\n", len, CurStr + start,	tb.c_str());

	tb.append('\n');

	return true;
}

BOOL FileEditor::SendToPrinter()
{
	PrinterSupport printer;
	TextBuffer tb;
	int tab = m_editor->EdOpt.TabSize;

	fprintf(stderr, "Printer caps: HTML=%c, preview=%c, setup dialog=%c\n",
		printer.IsReducedHTMLSupported() ? 'Y' : 'N',
		printer.IsPrintPreviewSupported() ? 'Y' : 'N',
		printer.IsPrinterSetupDialogSupported() ?  'Y' : 'N');

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

			if (Length > 0 && tb.is_empty() && printer.IsReducedHTMLSupported())
				tb.append("<html><body><pre>");

			if(!printer.IsReducedHTMLSupported() || !convertToReducedHTML(tb, Ptr, StartSel, Length, tab)) {
				int Len2 = 0;
				Ptr->GetBinaryString(&CurStr, &EndSeq, Len2);
				escapeHtmlTags(tb, CurStr + StartSel, Length, printer.IsReducedHTMLSupported(), tab);
				tb.append('\n');
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

				if (CopySize > 0 && tb.is_empty() && printer.IsReducedHTMLSupported())
					tb.append("<html><body><pre>");

				if(!printer.IsReducedHTMLSupported() || !convertToReducedHTML(tb, CurPtr, TBlockX, CopySize, tab)) {
					escapeHtmlTags(tb, CurStr + TBlockX, CopySize, printer.IsReducedHTMLSupported(), tab);
					tb.append('\n');
				}
    		}
    	}
	}

	if (!tb.is_empty()) {
		if(printer.IsReducedHTMLSupported())
			tb.append("</pre></body></html>");

		if (printer.IsPrintPreviewSupported()) {
			if (printer.IsReducedHTMLSupported()) 
				printer.ShowPreviewForReducedHTML(strFullFileName.GetWide(), tb.w_str());
			else 
				printer.ShowPreviewForText(strFullFileName.GetWide(), tb.w_str());
		}
		else {
			if (printer.IsReducedHTMLSupported()) 
				printer.PrintReducedHTML(strFullFileName.GetWide(), tb.w_str());
			else 
				printer.PrintText(strFullFileName.GetWide(), tb.w_str());
		}
		return TRUE;
	}

	// get all data in UTF-8 form and save to the temporary UTF-8 file to print, as it might be huge
	char tmpl[] = "/tmp/far2l-editor-printXXXXXX";
	int fd = mkstemp(tmpl);
	FILE* fp = fdopen(fd, "a+");
	std::string _tmpstr;

	if (printer.IsReducedHTMLSupported()) {
		fprintf(fp, "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"></head><body><pre>\n");
	}

	for (Edit *CurPtr = m_editor->TopList; CurPtr; CurPtr = CurPtr->m_next) {
		const wchar_t *SaveStr, *EndSeq;

		CurPtr->GetBinaryString(&SaveStr, &EndSeq, Length);

		TextBuffer tb;
		if (printer.IsReducedHTMLSupported() && convertToReducedHTML(tb, CurPtr, 0, Length, tab)) 
			fprintf(fp, "%s", tb.c_str());
		else {
    		Wide2MB(SaveStr, Length, _tmpstr);
    		fwrite(_tmpstr.data(), 1, _tmpstr.size(), fp);
            fputc('\n', fp);
		}
	}

	if (printer.IsReducedHTMLSupported())
		fprintf(fp, "</pre></body></html>\n");
	fclose(fp);
    
    Length = strlen(tmpl);
    std::wstring _tmpwstr;
    MB2Wide(tmpl, Length, _tmpwstr);

	if (printer.IsPrintPreviewSupported()) {
		if (printer.IsReducedHTMLSupported()) 
			printer.ShowPreviewForHtmlFile(_tmpwstr);
		else 
			printer.ShowPreviewForTextFile(_tmpwstr);
	}
	else {
		if (printer.IsReducedHTMLSupported()) 
			printer.PrintHtmlFile(_tmpwstr);
		else
			printer.PrintTextFile(_tmpwstr);
	}

	// unlink(tmpl);
	return TRUE;
}

//////////////////////////////////
