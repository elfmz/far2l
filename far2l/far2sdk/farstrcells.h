#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

__attribute__ ((visibility("default"))) size_t FarStrCellsCount(const wchar_t *pwz, size_t nw);
__attribute__ ((visibility("default"))) size_t FarStrZCellsCount(const wchar_t *pwz);
__attribute__ ((visibility("default"))) size_t FarStrSizeOfCells(const wchar_t *pwz, size_t nw, size_t &ng, bool round_up);
__attribute__ ((visibility("default"))) size_t FarStrSizeOfCell(const wchar_t *pwz, size_t nw);

__attribute__ ((visibility("default"))) void FarStrCellsTruncateLeft(wchar_t *pwz, size_t &n, size_t ng);
__attribute__ ((visibility("default"))) void FarStrCellsTruncateRight(wchar_t *pwz, size_t &n, size_t ng);
__attribute__ ((visibility("default"))) void FarStrCellsTruncateCenter(wchar_t *pwz, size_t &n, size_t ng);

#ifdef __cplusplus
}
#endif
