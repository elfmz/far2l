#include "FSClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>
#include <base64.h>
#include <UtfConvert.hpp>

FSClipboardBackend::FSClipboardBackend() :
	_shared_resource("fsclip", 0)
{
}


FSClipboardBackend::~FSClipboardBackend()
{
}

bool FSClipboardBackend::OnClipboardOpen()
{
	if (!_shared_resource.LockWrite(5))
		return false;

	_kfh = std::make_shared<KeyFileHelper>(InMyCache("fsclipboard.ini"), true);
	return true;
}

void FSClipboardBackend::OnClipboardClose()
{
	_kfh.reset();
	_shared_resource.UnlockWrite();
}

void FSClipboardBackend::OnClipboardEmpty()
{
	if (!_kfh)
		return;

	_kfh->RemoveSection("Data");
}

bool FSClipboardBackend::OnClipboardIsFormatAvailable(UINT format)
{
	if (!_kfh)
		return false;

	std::string str = _kfh->GetString("Data", ToPrefixedHex(format));
	return (!str.empty() && str[0] == '#');
}

void *FSClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (!_kfh)
		return nullptr;

	std::string str = "#";

	const size_t len = WINPORT(ClipboardSize)(data);

	if (format == CF_UNICODETEXT) {
		UtfConverter<wchar_t, unsigned char> cvt((const wchar_t *)data, len / sizeof(wchar_t));
		str+= base64_encode( (const unsigned char*)cvt.data(), cvt.size());
		format = CF_TEXT;

	} else {
		str+= base64_encode( (const unsigned char*)data, len);
	}

	_kfh->SetString("Data", ToPrefixedHex(format), str.c_str());
	return data;
}

void *FSClipboardBackend::OnClipboardGetData(UINT format)
{
	if (!_kfh)
		return nullptr;

	std::string str = _kfh->GetString("Data", ToPrefixedHex((format == CF_UNICODETEXT) ? CF_TEXT : format));
	if (str.empty() || str[0] != '#')
		return nullptr;

	const std::vector<unsigned char> &data
		= base64_decode(str.data() + 1, str.size() - 1);
	if (data.empty())
		return nullptr;

	void *out;
	if (format == CF_UNICODETEXT) {
		size_t utf8_len = data.size();
		while (utf8_len && !data[utf8_len - 1]) {
			--utf8_len;
		}

		std::wstring ws;
		MB2Wide((const char *)data.data(), utf8_len, ws);

		out = WINPORT(ClipboardAlloc)( (ws.size() + 1) * sizeof(wchar_t));
		if (out) {
			memcpy(out, ws.c_str(), (ws.size() + 1) * sizeof(wchar_t));
		}

	} else {
		out = WINPORT(ClipboardAlloc)(data.empty() ? 1 : data.size());
		if (out && !data.empty()) {
			memcpy(out, data.data(), data.size());
		}
	}

	return out;
}

UINT FSClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	if (!_kfh)
		return 0;

	const std::string &str_format_name = Wide2MB(lpszFormat);
	int id = _kfh->GetInt("Formats", str_format_name, 0);
	if (id == 0) {
		id = _kfh->GetInt("Global", "LastRegisteredFormat", 0);
		++id;
		if (id < 0xC000 || id> 0xFFFF)
			id = 0xC000;

		_kfh->SetInt("Global", "LastRegisteredFormat", id);

		_kfh->SetInt("Formats", str_format_name, id);
	}

	return (UINT)id;
}
