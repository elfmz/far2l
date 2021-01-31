#pragma once
#include <stdint.h>
#include <stdlib.h>

struct ViewerPrinter
{
	void PrintSpaces(size_t cnt);
	void EnableBOMSkip();
	bool ShouldSkip(wchar_t ch);

	virtual ~ViewerPrinter() {};

	virtual int Length(const wchar_t *str, int limit = -1) = 0;
	virtual void Print(int skip_len, int print_len, const wchar_t *str) = 0;

protected:
	bool _bom_skip = false;
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
