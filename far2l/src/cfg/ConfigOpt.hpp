#pragma once

/*
config.cpp

Конфигурация
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

struct ConfigOpt
{
	const char *section;
	const char *key;
	const WORD  bin_size;  // used only with _type == T_BIN
	const bool  save;   // =true - будет записываться в ConfigOptSave()

	enum T
	{
		T_STR = 0,
		T_BIN,
		T_DWORD,
		T_INT,
		T_BOOL,
	} const type : 8;

	union V
	{
		FARString *str;
		BYTE *bin;
		DWORD *dw;
		int *i;
		bool *b;
		void *p;
	} const value;

	union D
	{
		const wchar_t *str;
		const BYTE *bin;
		DWORD dw;
		int i;
		bool b;
		void *p;
	} const def;

	constexpr ConfigOpt(bool save_, const char *section_, const char *key_, WORD size_, BYTE *data_bin_, const BYTE *def_bin_)
		: section{section_}, key{key_}, bin_size{size_}, save{save_},
		type{T_BIN}, value{.bin = data_bin_}, def{.bin = def_bin_}
	{ }

	constexpr ConfigOpt(bool save_, const char *section_, const char *key_, FARString *data_str_, const wchar_t *def_str_)
		: section{section_}, key{key_}, bin_size{0}, save{save_},
		type{T_STR}, value{.str = data_str_}, def{.str = def_str_}
	{ }

	constexpr ConfigOpt(bool save_, const char *section_, const char *key_, DWORD *data_dw_, DWORD def_dw_)
		: section{section_}, key{key_}, bin_size{0}, save{save_},
		type{T_DWORD}, value{.dw = data_dw_}, def{.dw = def_dw_}
	{ }

	constexpr ConfigOpt(bool save_, const char *section_, const char *key_, int *data_i_, int def_i_)
		: section{section_}, key{key_}, bin_size{0}, save{save_},
		type{T_INT}, value{.i = data_i_}, def{.i = def_i_}
	{ }

	constexpr ConfigOpt(bool save_, const char *section_, const char *key_, bool *data_b_, bool def_b_)
		: section{section_}, key{key_}, bin_size{0}, save{save_},
		type{T_BOOL}, value{.b = data_b_}, def{.b = def_b_}
	{ }
};

extern const ConfigOpt g_cfg_opts[];
size_t ConfigOptCount() noexcept;
int ConfigOptGetIndex(const wchar_t *name);
