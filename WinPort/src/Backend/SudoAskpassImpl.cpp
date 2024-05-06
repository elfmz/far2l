#include <vector>
#include <utils.h>
#include <RandomString.h>
#include <crc64.h>
#include <fcntl.h>
#include <SavedScreen.h>
#include "Backend.h"
#include "SudoAskpassImpl.h"

class SudoAskpassScreen
{
	ConsoleInputPriority _cip;
	struct FinalScreenUpdater
	{
		// deliver WINDOW_BUFFER_SIZE_EVENT after screen restored
		// to ensure proper repaints of far2l's controls
		INPUT_RECORD ir{};

		FinalScreenUpdater()
		{
			unsigned int w{80}, h{25};
			g_winport_con_out->GetSize(w, h);
			ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
			ir.Event.WindowBufferSizeEvent.dwSize.X = w;
			ir.Event.WindowBufferSizeEvent.dwSize.Y = h;
		}

		~FinalScreenUpdater()
		{
			ir.Event.WindowBufferSizeEvent.bDamaged = TRUE;
			g_winport_con_in->Enqueue(&ir, 1);
		}
	} _fsu;

	SavedScreen _ss;
	unsigned int _width = 0, _height = 0;
	std::string _title, _text, _key_hint;
	std::wstring _input;
	enum Result {
		RES_PENDING,
		RES_OK,
		RES_CANCEL
	} _result = RES_PENDING;
	bool _password_expected = false;
	bool _need_repaint = true;

	// 0 - password is empty, 1 - non-empty but yet not hashed, otherwise - hash value
	uint64_t _panno_hash = 0;

	SMALL_RECT _rect{0}; // filled by Repaint()

	void DispatchInputKey(const KEY_EVENT_RECORD &rec)
	{
		if (!rec.bKeyDown)
			return;

		if (rec.wVirtualKeyCode == VK_RETURN) {
			_result = RES_OK;

		} else if (rec.wVirtualKeyCode == VK_ESCAPE || rec.wVirtualKeyCode == VK_F10) {
			_result = RES_CANCEL;

		} else if (rec.wVirtualKeyCode == VK_BACK) {
			if (!_input.empty()) {
				_input.resize(_input.size() - 1);
				if (_input.empty() && _password_expected && _panno_hash != 0) {
					// immediately indicate password became empty
					_panno_hash = 0;
					_need_repaint = true;
				}
			}

		} else if (rec.uChar.UnicodeChar) {
			_input+= rec.uChar.UnicodeChar;
			if (_password_expected && _panno_hash == 0) {
				// immediately indicate password became non-empty
				_panno_hash = 1;
				_need_repaint = true;
			}
		}
	}

	void DispatchInputMouse(const MOUSE_EVENT_RECORD &rec)
	{
	}

	void DispatchInput()
	{
		INPUT_RECORD ir;
		while (g_winport_con_in->Dequeue(&ir, 1, _cip)) {
			switch (ir.EventType) {
				case WINDOW_BUFFER_SIZE_EVENT:
					_ss.Restore();
					_fsu.ir = ir;
					_need_repaint = true;
					break;

				case KEY_EVENT:
					DispatchInputKey(ir.Event.KeyEvent);
					break;

				case MOUSE_EVENT:
					DispatchInputMouse(ir.Event.MouseEvent);
					break;
			}
		}
	}

	void WriteCentered(const std::string &str, SHORT t)
	{
		std::wstring wstr;
		StrMB2Wide(str, wstr);
		SHORT l = (_width > wstr.size()) ? (_width - wstr.size()) / 2 : 0;
		COORD pos = {l, t};
		g_winport_con_out->WriteStringAt(wstr.c_str(), wstr.size(), pos);
	}

	void Repaint()
	{
		g_winport_con_out->GetSize(_width, _height);

		const SHORT w = std::max(_key_hint.size(), std::max(_title.size(), _text.size())) + 5;

		_rect.Left = std::max(0, (SHORT(_width) - w) / 2);
		_rect.Right = _rect.Left + w - 1;
		_rect.Top = 0;
		_rect.Bottom = _rect.Top + 4;
		const DWORD64 AttrFrame = _password_expected
			? FOREGROUND_RED | BACKGROUND_RED
			: FOREGROUND_RED | BACKGROUND_RED | BACKGROUND_GREEN;
		const DWORD64 AttrInner =
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

		CHAR_INFO ci{};
		CI_SET_WCHAR(ci, L' ');
		for (SHORT y = _rect.Top; y <= _rect.Bottom; ++y) {
			for (SHORT x = _rect.Left; x <= _rect.Right; ++x) {
				ci.Attributes = (x == _rect.Left || x == _rect.Right || y == _rect.Top || y == _rect.Bottom)
					? AttrFrame : AttrInner;
				COORD pos{x, y};
				// force repant if frame is damaged (likely by app's interface painted from other thread)
				if (!_need_repaint && (y == _rect.Top || x == _rect.Left || x == _rect.Right)) {
					CHAR_INFO ci_orig{};
					g_winport_con_out->Read(ci_orig, pos);
					if (ci_orig.Char.UnicodeChar != ci.Char.UnicodeChar || ci_orig.Attributes != ci.Attributes) {
						_need_repaint = true;
					}
				}
				if (_need_repaint) {
					g_winport_con_out->Write(ci, pos);
				}
			}
		}

		if (_need_repaint) {
			g_winport_con_out->SetAttributes(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			WriteCentered(_title, _rect.Top + 1);
			WriteCentered(_key_hint, _rect.Top + 3);

			g_winport_con_out->SetAttributes(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			WriteCentered(_text, _rect.Top + 2);
			if (_password_expected) {
				PaintPasswordPanno();
			}
			_need_repaint = false;
		}
	}

	uint64_t TypedPasswordHash()
	{
		if (_input.empty())
			return 0; // always zero for empty password

		uint64_t hash64 = crc64(0x1215b814a,
			(const unsigned char *)_input.c_str(), _input.size() * sizeof(*_input.c_str()));

		const std::string &salt_file = InMyConfig("askpass.salt");
		std::string salt;
		if (!ReadWholeFile(salt_file.c_str(), salt, 0x1000)) {
			salt.clear();
			RandomStringAppend(salt, 0x20, 0x20, RNDF_NZ);
			FDScope fd(salt_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT | O_CLOEXEC, 0600);
			if (fd.Valid()) {
				WriteAll(fd, salt.c_str(), salt.size());
			}
		}

		hash64 = crc64(hash64, (const unsigned char *)salt.c_str(), salt.size() * sizeof(*salt.c_str()));
		return (hash64 > 1) ? hash64 : 2; // bigger than 1 for nonempty password
	}

	void PaintPasswordPanno()
	{
		// if input is empty: paint 3 white squares
		// if input is non-empty but not yet hashed: paint 3 yellow squares
		// else: paint 3 different glyphs of different colors all deduced from hash

		std::vector<wchar_t> glyphs{L'■', L'▲', L'●'};
		std::vector<uint64_t> colors { // should be same size as glyphs
			BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
			BACKGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
			BACKGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
		};

		uint32_t hash32 = uint32_t(_panno_hash ^ (_panno_hash >> 32));

		for (SHORT i = 0, ii = SHORT(glyphs.size()); i < ii; ++i, hash32>>= 8) {
			CHAR_INFO ci{};
			if (_panno_hash > 1) { // for hashed input use 3 different colors
				const size_t color_index = (hash32 & 0xf) % colors.size();
				ci.Attributes = colors[color_index];
				colors.erase(colors.begin() + color_index);

			} else if (_panno_hash == 1) { // for unhashed nonempty input use yellow
				ci.Attributes = BACKGROUND_RED | FOREGROUND_INTENSITY
					| FOREGROUND_RED | FOREGROUND_GREEN;

			} else { // for empty input use white
				ci.Attributes = BACKGROUND_RED | FOREGROUND_INTENSITY
					| FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			}

			if (_panno_hash > 1) { // for hashed input show 3 different shapes
				const size_t glyph_index = ((hash32 >> 4) & 0xf) % glyphs.size();
				CI_SET_WCHAR(ci, glyphs[glyph_index]);
				glyphs.erase(glyphs.begin() + glyph_index);

			} else { // for empty/unhashed input show 3 squares
				CI_SET_WCHAR(ci, glyphs[0]);
			}

			// * 2 is for extra gap for better distinction
			COORD pos{SHORT(SHORT(_width / 2) + i * 2 - ii), _rect.Bottom};
			g_winport_con_out->Write(ci, pos);
		}
	}

public:
	SudoAskpassScreen(const std::string &title, const std::string &text, bool password_expected) :
		_cip(g_winport_con_in),
		_title(title),
		_text(text),
		_key_hint("Confirm by <Enter> Cancel by <Esc>"),
		_password_expected(password_expected)
	{
		_input.reserve(64);// to help secure cleanup in d-tor
		Repaint();
	}

	~SudoAskpassScreen()
	{
		for (wchar_t &c : _input) {
			*static_cast<volatile wchar_t *>(&c) = 0;
		}
	}

	bool Loop()
	{
		for (;;) {
			if (g_winport_con_in->WaitForNonEmptyWithTimeout(700, _cip)) {
				DispatchInput();

			} else {
				if (_password_expected) {
					const uint64_t hash = TypedPasswordHash();
					if (_panno_hash != hash) {
						_panno_hash = hash;
					}
				}

				if (_result != RES_PENDING)
					return _result == RES_OK;

				Repaint();
			}
		}
	}

	void GetInput(std::string &str)
	{
		StrWide2MB(_input, str);
	}
};

bool SudoAskpassImpl::OnSudoAskPassword(const std::string &title, const std::string &text, std::string &password)
{
	SudoAskpassScreen sas(title, text, true);
	if (!sas.Loop())
		return false;

	sas.GetInput(password);
	return true;
}

bool SudoAskpassImpl::OnSudoConfirm(const std::string &title, const std::string &text)
{
	SudoAskpassScreen sas(title, text, false);
	return sas.Loop();
}
