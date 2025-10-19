/**************************************************************************
 *  Hexitor plug-in for FAR 3.0 modifed by m32 2024 for far2l             *
 *  Copyright (C) 2010-2014 by Artem Senichev <artemsen@gmail.com>        *
 *  https://sourceforge.net/projects/farplugs/                            *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include "file.h"

file::file()
:	_rw_mode(true),
	_handle(INVALID_HANDLE_VALUE),
	_size(0)
{
}


file::~file()
{
	close();
}


DWORD file::open(const wchar_t* file_name)
{
	assert(file_name && *file_name);
	assert(_handle == INVALID_HANDLE_VALUE);

	close();
	if (!file_name || !*file_name)
		return ERROR_INVALID_NAME;

	_name = file_name;

	DWORD rc = ERROR_SUCCESS;

	//Open source file (with write mode)
	_rw_mode = true;
	if (open_file(_rw_mode, _handle) != ERROR_SUCCESS) {
		//Try to open file in Read Only mode
		_rw_mode = false;
		rc = open_file(_rw_mode, _handle);
	}

	if (rc == ERROR_SUCCESS)
		rc = get_file_size(_handle, _size);

	return rc;
}


void file::close()
{
	if (_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(_handle);
		_handle = INVALID_HANDLE_VALUE;
	}
}


DWORD file::switch_mode()
{
	if (is_read_only(_name.c_str()))
		clear_read_only(_name.c_str());

	HANDLE fh = INVALID_HANDLE_VALUE;

	DWORD rc = ERROR_SUCCESS;

	rc = open_file(!_rw_mode, fh);
	if (rc == ERROR_SUCCESS)
		rc = get_file_size(fh, _size);
	else
		CloseHandle(fh);
	if (rc == ERROR_SUCCESS) {
		close();
		_rw_mode = !_rw_mode;
		_handle = fh;
	}

	return rc;
}


DWORD file::read(const UINT64 offset, vector<BYTE>& buffer, const size_t sz) const
{
	assert(_handle != INVALID_HANDLE_VALUE);
	assert(sz && sz < 1024 * 1024);
	assert(offset < _size);

	DWORD rc = set_position(offset);

	if (rc == ERROR_SUCCESS) {
		buffer.resize(sz);
		DWORD length = 0;
		if (!ReadFile(_handle, &buffer.front(), static_cast<DWORD>(sz), &length, nullptr))
			rc = GetLastError();
		buffer.resize(length);
	}

	return rc;
}


DWORD file::save(const map<UINT64, BYTE>& upd_data)
{
	assert(_handle != INVALID_HANDLE_VALUE);
	assert(_rw_mode);

	DWORD rc = ERROR_SUCCESS;

	for (map<UINT64, BYTE>::const_iterator it = upd_data.begin(); rc == ERROR_SUCCESS && it != upd_data.end(); ++it) {
		rc = set_position(it->first);
		if (rc == ERROR_SUCCESS) {
			DWORD bytes_written = 0;
			if (!WriteFile(_handle, &it->second, 1, &bytes_written, nullptr))
				rc = GetLastError();
		}
	}

	return rc;
}


DWORD file::save_as(const wchar_t* file_name, const map<UINT64, BYTE>& upd_data, LPPROGRESS_ROUTINE progress_routine, LPVOID data)
{
	assert(file_name && *file_name);
	assert(_handle != INVALID_HANDLE_VALUE);

	//assert(wcsicmp(file_name, _name.c_str()) != 0);

	DWORD rc = ERROR_SUCCESS;
	/*
	if (!CopyFileEx(_name.c_str(), file_name, progress_routine, data, nullptr, 0))
		rc = GetLastError();
	*/
	LARGE_INTEGER file_size;
	if( !GetFileSizeEx(_handle, &file_size) )
		return GetLastError();
	HANDLE fo = CreateFile(file_name, GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,	//&sa
						FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if( fo == INVALID_HANDLE_VALUE )
		return GetLastError();
	SetFilePointer(_handle, 0, NULL, FILE_BEGIN);
	LARGE_INTEGER copied = {0};
	unsigned char buf[4096];
	DWORD nb=0;
	while( (rc == ERROR_SUCCESS) && (copied.QuadPart < file_size.QuadPart) ){
		if( progress_routine && progress_routine({}, copied, {}, {}, 0, 0, nullptr, nullptr, data) == PROGRESS_CANCEL ){
			rc = ERROR_WRITE_FAULT;
			break;
		}
		if( ReadFile(_handle, buf, sizeof(buf), &nb, NULL) )
		{
			if( WriteFile(fo, buf, nb, &nb, NULL) )
				copied.QuadPart += nb;
			else
				rc = GetLastError();
		}
		else
			rc = GetLastError();
	};
	CloseHandle(fo);
	if( rc != ERROR_SUCCESS ){
		DeleteFile(file_name);
		return rc;
	}

	file new_instance;
	rc = new_instance.open(file_name);
	if (rc == ERROR_SUCCESS) {
		close();
		_handle = new_instance._handle;
		_size = new_instance._size;
		_name = new_instance._name;
		_rw_mode = new_instance._rw_mode;
		new_instance._handle = INVALID_HANDLE_VALUE;
		rc = save(upd_data);
	}

	return rc;
}


DWORD file::open_file(const bool rw_mode, HANDLE& fh) const
{
	DWORD rc = ERROR_SUCCESS;

	const DWORD access_mode = rw_mode ? (GENERIC_WRITE | GENERIC_READ) : GENERIC_READ;
	const DWORD share_mode =  rw_mode ? FILE_SHARE_READ : (FILE_SHARE_READ | FILE_SHARE_WRITE);

	fh = CreateFile(_name.c_str(), access_mode, share_mode, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
	if (fh == INVALID_HANDLE_VALUE)
		rc = GetLastError();

	return rc;
}


DWORD file::get_file_size(const HANDLE fh, UINT64& sz) const
{
	assert(fh != INVALID_HANDLE_VALUE);

	DWORD rc = ERROR_SUCCESS;

	LARGE_INTEGER file_size;
	if (GetFileSizeEx(fh, &file_size))
		sz = file_size.QuadPart;
	else
		rc = GetLastError();

	return rc;
}


DWORD file::set_position(const UINT64 offset) const
{
	assert(_handle != INVALID_HANDLE_VALUE);
	assert(!offset || offset < _size);

	DWORD rc = ERROR_SUCCESS;

	LARGE_INTEGER file_position;
	file_position.QuadPart = offset;
	if (!SetFilePointerEx(_handle, file_position, nullptr, FILE_BEGIN))
		rc = GetLastError();

	return rc;
}


bool file::file_exist(const wchar_t* file_name)
{
	assert(file_name && *file_name);
	return GetFileAttributes(file_name) != INVALID_FILE_ATTRIBUTES;
}


bool file::is_read_only(const wchar_t* file_name)
{
	assert(file_name && *file_name);
	const DWORD attr = GetFileAttributes(file_name);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY);
}


int file::clear_read_only(const wchar_t* file_name)
{
	assert(file_name && *file_name);

	DWORD attr = GetFileAttributes(file_name);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return GetLastError();

	if (attr & FILE_ATTRIBUTE_READONLY) {
		attr ^= FILE_ATTRIBUTE_READONLY;
		if (!SetFileAttributes(file_name, attr))
			return GetLastError();
	}

	return ERROR_SUCCESS;
}
