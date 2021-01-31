#pragma once
#include <stdint.h>
#include <stdlib.h>

struct ViewerPrinter
{
	void PrintSpaces(size_t cnt);
	void EnableBOMSkip();
	void SetSelection(bool selection);

	virtual ~ViewerPrinter() {};

	virtual int Length(const wchar_t *str, int limit = -1) = 0;
	virtual void Print(int skip_len, int print_len, const wchar_t *str) = 0;

protected:
	bool ShouldSkip(wchar_t ch);

	bool _bom_skip = false;
	bool _selection = false;
};

struct PlainViewerPrinter : ViewerPrinter
{
	PlainViewerPrinter(int color);
	virtual ~PlainViewerPrinter();

	virtual int Length(const wchar_t *str, int limit = -1);
	virtual void Print(int skip_len, int print_len, const wchar_t *str);

private:
	int _color;
};
