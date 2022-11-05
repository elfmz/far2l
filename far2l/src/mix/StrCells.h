#pragma once

size_t StrCellsCount(const wchar_t *pwz, size_t nw);
size_t StrZCellsCount(const wchar_t *pwz);
size_t StrSizeOfCells(const wchar_t *pwz, size_t nw, size_t &ng, bool round_up);
size_t StrSizeOfCell(const wchar_t *pwz, size_t nw);

void StrCellsTruncateLeft(wchar_t *pwz, size_t &n, size_t ng);
void StrCellsTruncateRight(wchar_t *pwz, size_t &n, size_t ng);
void StrCellsTruncateCenter(wchar_t *pwz, size_t &n, size_t ng);
