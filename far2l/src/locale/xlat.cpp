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
#include <optional>


class Xlator
{
	std::wstring _latin, _local;
	struct Rules : std::vector<std::pair<wchar_t, wchar_t>>
	{
		void InitFromValue(const std::wstring &v)
		{
			clear();
			for (size_t i = 0; i + 1 < v.size();) {
				emplace_back(std::make_pair(v[i], v[i + 1]));
				for (i+= 2; i < v.size() && v[i] == ' '; ++i) {
				}
			}
		}

	} _after_latin, _after_local, _after_other;

	size_t _min_len_table{0};
	enum
	{
		UNKNOWN,
		LATIN,
		LOCAL,
	} _cur_lang{UNKNOWN};

	void InitFromValues(KeyFileValues &kfv)
	{
		_latin = kfv.GetString("Latin", L"");
		_local = kfv.GetString("Local", L"");

		_after_latin.InitFromValue(kfv.GetString("AfterLatin", L""));
		_after_local.InitFromValue(kfv.GetString("AfterLocal", L""));
		_after_other.InitFromValue(kfv.GetString("AfterOther", L""));

		_min_len_table = std::min(_local.size(), _latin.size());
	}

public:
	Xlator()
	{
		Reinit();
	}

	void Reinit()
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
	}

	bool Valid() const { return _min_len_table != 0; }

	void TrackKeypress(wchar_t chr)
	{
		for (size_t i = 0; i < _min_len_table; i++) {
			// символ из латиницы?
			if (chr == _latin[i]) {
				_cur_lang = LATIN; // pred - english
				break;

			} else if (chr == _local[i]) { // символ из русской?
				_cur_lang = LOCAL; // pred - local
				break;
			}
		}
	}

	wchar_t Transcode(wchar_t chr)
	{
		// chr_old - пред символ
		// цикл по просмотру Chr в таблицах
		// <= _min_len_table так как длина настоящая а начальный индекс 1
		for (size_t i = 0; i < _min_len_table; i++) {
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
		const Rules *rules;
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
};

///////////////

static time_t s_xlator_time = 0;
static std::optional<Xlator> s_xlator;

static Xlator &GetXlator()
{
	time_t now = time(NULL);
	if (!s_xlator.has_value()) {
		s_xlator.emplace();
		s_xlator_time = now;
	} else if (now - s_xlator_time > 10) {
		s_xlator.value().Reinit();
		s_xlator_time = now;
	}
	return s_xlator.value();
}

void XlatReinit()
{
	s_xlator.reset();
}

wchar_t XlatOneChar(wchar_t Chr)
{
	return GetXlator().Transcode(Chr);
}

wchar_t* WINAPI Xlat(wchar_t *Line, int StartPos, int EndPos, DWORD Flags) // Flags accepted but not yet implemented
{
	if (!Line || !*Line)
		return nullptr;

	int Length = StrLength(Line);
	EndPos = (EndPos == -1) ? Length : Min(EndPos, Length);
	StartPos = Max(StartPos, 0);

	if (StartPos > EndPos || StartPos >= Length) {
		return Line;
	}

	Xlator &xlator = GetXlator();
	if (!xlator.Valid()) {
		return Line;
	}

	for (int j=StartPos; j < EndPos; j++) {
		Line[j] = xlator.Transcode(Line[j]);
	}

	return Line;
}

void XlatTrackKeypress(wchar_t Key)
{
	GetXlator().TrackKeypress(Key);
}
