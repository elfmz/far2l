#include "headers.hpp"
#include <algorithm>
#include "ViewerPrinter.hpp"
#include "interf.hpp"
#include "colors.hpp"

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

bool ViewerPrinter::ShouldSkip(wchar_t ch)
{
	if (_bom_skip && ch == 0xFEFF) {
		return true;
	}

	return false;
}

//

PlainViewerPrinter::PlainViewerPrinter(int color)
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
			++out;
		}
	}
	return out;
}

void PlainViewerPrinter::Print(int skip_len, int print_len, const wchar_t *str)
{
	SetColor(_selection ? COL_VIEWERSELECTEDTEXT : _color);
	while (*str && skip_len) {
		--skip_len;
		++str;
	}

	int str_print_len;
	for (int i = str_print_len = 0;; ) {
		if (!str[i] || str_print_len + i >= print_len || ShouldSkip(str[i])) {
			if (i) {
				Text(str, i);
				str_print_len+= i;
			}
			if (!str[i] || str_print_len + i >= print_len) {
				break;
			}
			str+= i + 1;
			i = 0;

		} else {
			++i;
		}
	}

	if (print_len > str_print_len) {
		PrintSpaces(print_len - str_print_len);
	}
}
