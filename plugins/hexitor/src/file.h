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

#pragma once

#include "common.h"

typedef DWORD CALLBACK LPPROGRESS_ROUTINE(LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID);
#define PROGRESS_CANCEL 0
#define PROGRESS_CONTINUE 1

class file
{
public:
	file();
	~file();

	/**
	 * Open file
	 * \param file_name file name
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD open(const wchar_t* file_name);

	/**
	 * Close file
	 */
	void close();

	/**
	 * Switch file mode (RW/RO)
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD switch_mode();

	/**
	 * Check if file was opened in RW mode
	 * \return true if file is writable, false if it is in read-only mode
	 */
	inline bool writable() const { return _rw_mode; }

	/**
	 * Read file
	 * \param offset start position
	 * \param buffer buffer to read
	 * \param sz max buffer size
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD read(const UINT64 offset, vector<BYTE>& buffer, const size_t sz) const;

	/**
	 * Save file
	 * \param upd_data map with updated data description (offset, byte value)
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD save(const map<UINT64, BYTE>& upd_data);

	/**
	 * Save file with new name
	 * \param file_name new file name
	 * \param upd_data map with updated data description (offset, byte value)
	 * \param progress_routine pointer to progress routine function
	 * \param data data passed to progress_routine
	 * \return error code (ERROR_SUCCESS if no error, ERROR_REQUEST_ABORTED if operation aborted by user)
	 */
	DWORD save_as(const wchar_t* file_name, const map<UINT64, BYTE>& upd_data, LPPROGRESS_ROUTINE progress_routine, LPVOID data);

	/**
	 * Check for file read only attribute
	 * \return true if file has read only attribute
	 */
	inline bool read_only() const		{ return is_read_only(_name.c_str()); }

	/**
	 * Get name name
	 * \return name name
	 */
	inline const wchar_t* name() const	{ return _name.c_str(); }

	/**
	 * Get file size
	 * \return file size
	 */
	inline UINT64 size() const			{ return _size; }

	/**
	 * Check for file existing
	 * \param file_name checked file name
	 * \return true if file exist
	 */
	static bool file_exist(const wchar_t* file_name);

	/**
	 * Check for file read only attribute
	 * \param file_name checked file name
	 * \return true if file has read only attribute
	 */
	static bool is_read_only(const wchar_t* file_name);

	/**
	 * Clear read only attribute
	 * \param file_name file name
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	static int clear_read_only(const wchar_t* file_name);

private:
	/**
	 * Open file
	 * \param rw_mode read-write mode (true = RW, false = RO)
	 * \param fh file handle
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD open_file(const bool rw_mode, HANDLE& fh) const;

	/**
	 * Get file size
	 * \param fh file handle
	 * \param sz file size
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD get_file_size(const HANDLE fh, UINT64& sz) const;

	/**
	 * Set position in the file
	 * \param offset offset
	 * \return error code (ERROR_SUCCESS if no error)
	 */
	DWORD set_position(const UINT64 offset) const;

private:
	wstring	_name;			///< File name
	bool	_rw_mode;		///< Flag that file is opened in read/write mode
	HANDLE	_handle;		///< File read handle
	UINT64	_size;			///< File size in bytes
};
