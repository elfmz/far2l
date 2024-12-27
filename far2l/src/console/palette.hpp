#pragma once

/*
palette.hpp

Таблица цветов
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

#include <WinCompat.h>
#include <utils.h>
#include <KeyFileHelper.h>
#include "color.hpp"

//class Palette : private NonCopyable
class Palette : NonCopyable
{
//protected:
//    static int count;

//	HANDLE h_file;
//	std::wstring m_file_path;

public:

	union {
		rgbcolor_t palbuff[32];
		struct {
			rgbcolor_t background[16];
			rgbcolor_t foreground[16];
		};
	};

//    static rgbcolor_t cur_palette[32];
    static Palette FARPalette;

	Palette() noexcept;
	~Palette() noexcept;

//current
	static void InitFarPalette( ) noexcept;

	void Set();
	bool Load(KeyFileHelper &kfh) noexcept;
	bool Save(KeyFileHelper &kfh) noexcept;
	void ResetToDefault() noexcept;
//	void ResetToDefaultRGB();
//	void Reset(bool RGB);
//	uint32_t GammaCorrection;
//	bool GammaChanged;

	const rgbcolor_t &operator[](size_t const Index) const
	{
		return palbuff[Index];
	}

	size_t size() const
	{
		return 32;
	}



//	File(const std::wstring &file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition,
//			DWORD flags_and_attributes);
/**
	void open(const std::wstring &file_path, DWORD desired_access, DWORD share_mode,
			DWORD creation_disposition, DWORD flags_and_attributes);
	bool open_nt(const std::wstring &file_path, DWORD desired_access, DWORD share_mode,
			DWORD creation_disposition, DWORD flags_and_attributes) noexcept;
	void close() noexcept;
	bool is_open() const noexcept { return h_file != INVALID_HANDLE_VALUE; }
	HANDLE handle() const noexcept { return h_file; }
	const std::wstring &path() const noexcept { return m_file_path; }
	UInt64 size();
	bool size_nt(UInt64 &file_size) noexcept;
	size_t read(void *data, size_t size);
	bool read_nt(void *data, size_t size, size_t &size_read) noexcept;
	size_t write(const void *data, size_t size);
	bool write_nt(const void *data, size_t size, size_t &size_written) noexcept;
	void set_time(const FILETIME &ctime, const FILETIME &atime, const FILETIME &mtime);
	bool set_time_nt(const FILETIME &ctime, const FILETIME &atime, const FILETIME &mtime) noexcept;
	bool copy_ctime_from(const std::wstring &source_file) noexcept;
	UInt64 set_pos(int64_t offset, DWORD method = FILE_BEGIN);
	bool set_pos_nt(int64_t offset, DWORD method = FILE_BEGIN, UInt64 *new_pos = nullptr) noexcept;
	void set_end();
	bool set_end_nt() noexcept;
	BY_HANDLE_FILE_INFORMATION get_info();
	bool get_info_nt(BY_HANDLE_FILE_INFORMATION &info) noexcept;
	template <typename Type>
	bool io_control_out_nt(DWORD code, Type &data) noexcept
	{
		DWORD size_ret;
		return DeviceIoControl(h_file, code, nullptr, 0, &data, sizeof(Type), &size_ret, nullptr) != 0;
	}
	static DWORD attributes(const std::wstring &file_path) noexcept;
//	static bool attributes_ex(const std::wstring &file_path, WIN32_FILE_ATTRIBUTE_DATA *ex_attrs) noexcept;
//	static bool exists(const std::wstring &file_path) noexcept;
//	static void set_attr(const std::wstring &file_path, DWORD attr);
//	static bool set_attr_nt(const std::wstring &file_path, DWORD attr) noexcept;
//	static bool set_attr_posix(const std::wstring &file_path, DWORD attr) noexcept;
**/

//	static void delete_file(const std::wstring &file_path);
//	static bool delete_file_nt(const std::wstring &file_path) noexcept;
//	static void create_dir(const std::wstring &dir_path);
//	static bool create_dir_nt(const std::wstring &dir_path) noexcept;
//	static void remove_dir(const std::wstring &file_path);
//	static bool remove_dir_nt(const std::wstring &file_path) noexcept;
//	static void move_file(const std::wstring &file_path, const std::wstring &new_path, DWORD flags);
//	static bool
//	move_file_nt(const std::wstring &file_path, const std::wstring &new_path, DWORD flags) noexcept;
//	static FindData get_find_data(const std::wstring &file_path);
//	static bool get_find_data_nt(const std::wstring &file_path, FindData &find_data) noexcept;
};

//extern uint8_t DefaultPalette8bit[SIZE_ARRAY_FARCOLORS];
//extern uint8_t Palette8bit[SIZE_ARRAY_FARCOLORS];
//extern uint8_t BlackPalette8bit[SIZE_ARRAY_FARCOLORS];

//extern uint64_t Palette[SIZE_ARRAY_FARCOLORS];

//void ZeroFarPalette( void );
//void InitFarPalette( void );
//void ConvertCurrentPalette();
