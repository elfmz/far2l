#include "headers.hpp"
#include <algorithm>
#include "ViewerPrinter.hpp"
#include "interf.hpp"
#include "colors.hpp"
#include "palette.hpp"

static const wchar_t s_many_spaces[] = L"                                                                ";

void ViewerPrinter::PrintSpaces(size_t cnt)
{
	while (cnt) {
		size_t piece = std::min(cnt, sizeof(s_many_spaces)/sizeof(s_many_spaces[0]) - 1);
		Text(s_many_spaces, piece);
		cnt-= piece;
	}
}

void ViewerPrinter::EnableBOMSkip()
{
	_bom_skip = true;
}

void ViewerPrinter::SetSelection(bool selection)
{
	_selection = selection;
}

//

PlainViewerPrinter::PlainViewerPrinter(uint64_t color)
	: _color(color)
{
}

PlainViewerPrinter::~PlainViewerPrinter()
{
}

int PlainViewerPrinter::Length(const wchar_t *str, int limit)
{
	int out;
	for (out = 0; *str && limit != 0; ++str, --limit) {
		if (!ShouldSkip(*str)) {
			if (IsCharFullWidth(*str))
				out+= 2;
			else if (!IsCharXxxfix(*str))
				++out;
		}
	}
	return out;
}

void PlainViewerPrinter::Print(int skip_len, int print_len, const wchar_t *str)
{
	SetColor(_selection ? FarColorToReal(COL_VIEWERSELECTEDTEXT) : _color);

	for(; skip_len > 0 && *str; ++str) {
		if (ShouldSkip(*str))
			;
		else if (IsCharFullWidth(*str))
			skip_len-= 2;
		else if (!IsCharXxxfix(*str))
			skip_len--;
	}

	while (print_len > 0) {
		size_t piece = 0;
		while (str[piece] && !ShouldSkip(str[piece])) {
			++piece;
		}
		if (piece) {
			size_t cells = print_len;
			piece = StrSizeOfCells(str, piece, cells, false);
			if (piece) {
				Text(str, piece);
				print_len-= (int)cells;
			}
		}
		if (!str[piece]) {
			break;
		}
		str+= piece + 1;
	}

	if (print_len > 0) {
		PrintSpaces(print_len);
	}
}
