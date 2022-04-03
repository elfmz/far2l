/*
xlat.cpp

XLat - перекодировка

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

/*
[HKEY_CURRENT_USER\Software\Far2\XLat]
"Flags"=dword:00000001
"Layouts"="04090409;04190419"
"Table1"="№АВГДЕЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЯавгдезийклмнопрстуфхцчшщъыьэяёЁБЮ"
"Table2"="#FDULTPBQRKVYJGHCNEA{WXIO}SMZfdultpbqrkvyjghcnea[wxio]sm'z`~<>"
"Rules1"=",??&./б,ю.:^Ж:ж;;$\"@Э\""
"Rules2"="?,&?/.,б.ю^::Ж;ж$;@\"\"Э"
"Rules3"="^::ЖЖ^$;;жж$@\"\"ЭЭ@&??,,бб&/..юю/"
*/

#include "headers.hpp"


#include "ConfigRW.hpp"
#include "config.hpp"
#include "xlat.hpp"
#include "console.hpp"
#include "dirmix.hpp"

Xlator::Xlator(DWORD flags)
{
	KeyFileReadSection xlat_local(InMyConfig("xlats.ini"), Opt.XLat.XLat.GetMB());
	if (xlat_local.SectionLoaded()) {
		InitFromValues(xlat_local);

	} else {
		KeyFileReadSection xlat_global(GetHelperPathName("xlats.ini"), Opt.XLat.XLat.GetMB());
		if (xlat_global.SectionLoaded()) {
			InitFromValues(xlat_global);
		}
	}

	if (flags & XLAT_USEKEYBLAYOUTNAME) {
		fprintf(stderr, "Xlator: XLAT_USEKEYBLAYOUTNAME is not supported\n");
	}
}

void Xlator::Rules::InitFromValue(const std::wstring &v)
{
	for (size_t i = 0; i + 1 < v.size();) {
		emplace_back(std::make_pair(v[i], v[i + 1]));
		for (i+= 2; i < v.size() && v[i] == ' '; ++i) {
		}
	}
}

void Xlator::InitFromValues(KeyFileValues &kfv)
{
	_latin = kfv.GetString("Latin", L"");
	_local = kfv.GetString("Local", L"");

	_after_latin.InitFromValue(kfv.GetString("AfterLatin", L""));
	_after_local.InitFromValue(kfv.GetString("AfterLocal", L""));
	_after_other.InitFromValue(kfv.GetString("AfterOther", L""));

	_min_len_table = std::min(_local.size(), _latin.size());
}

wchar_t Xlator::Transcode(wchar_t chr)
{
	size_t i;
	// chr_old - пред символ
	// цикл по просмотру Chr в таблицах
	// <= _min_len_table так как длина настоящая а начальный индекс 1
	for (i = 0; i < _min_len_table; i++) {
		// символ из латиницы?
		if (chr == _latin[i]) {
			_cur_lang = LATIN; // pred - english
			return _local[i];

		} else if (chr == _local[i]) { // символ из русской?
			_cur_lang = LOCAL; // pred - local
			return _latin[i];
		}
	}

	// особые случаи...
	Rules *rules;
	switch (_cur_lang) {
		case LATIN:
			rules = &_after_latin;
			break;

		case LOCAL:
			rules = &_after_local;
			break;

		default:
			rules = &_after_other;
	}

	for (const auto &rule : *rules) {
		if (chr == rule.first) {
			return rule.second;
		}
	}

	return chr;
}

wchar_t* WINAPI Xlat(wchar_t *Line,
                     int StartPos,
                     int EndPos,
                     DWORD Flags)
{
	if (!Line || !*Line)
		return nullptr;

	int Length = StrLength(Line);
	EndPos = (EndPos == -1) ? Length : Min(EndPos, Length);
	StartPos = Max(StartPos, 0);

	if (StartPos > EndPos || StartPos >= Length) {
		return Line;
	}

	Xlator xlt(Flags);
	if (!xlt.Valid()) {
		return Line;
	}

	for (int j=StartPos; j < EndPos; j++) {
		Line[j] = xlt.Transcode(Line[j]);
	}

	// переключаем раскладку клавиатуры?
	if (Flags & XLAT_SWITCHKEYBLAYOUT) {
		//todo
		fprintf(stderr, "Xlator: XLAT_USEKEYBLAYOUTNAME is not supported\n");
	}

	return Line;
}
