#include "FSClipboardBackend.h"
#include "WinPort.h"
#include <utils.h>
#include <base64.h>

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

	char str_format[64]; sprintf(str_format, "0x%x", format);

	std::string str = _kfh->GetString("Data", str_format);
	return (!str.empty() && str[0] == '!');
}

void *FSClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (!_kfh)
		return nullptr;

	char str_format[64]; sprintf(str_format, "0x%x", format);

	size_t len = WINPORT(ClipboardSize)(data);

	std::string str = base64_encode( (const unsigned char*)data, len);

	str.insert(0, "#");
	_kfh->SetString("Data", str_format, str.c_str());
	return data;
}

void *FSClipboardBackend::OnClipboardGetData(UINT format)
{
	if (!_kfh)
		return nullptr;

	char str_format[64]; sprintf(str_format, "0x%x", format);

	std::string str = _kfh->GetString("Data", str_format);
	if (str.empty() || str[0] != '#')
		return nullptr;

	str.erase(0, 1);
	const std::vector<unsigned char> &data = base64_decode(str);
	void *out = WINPORT(ClipboardAlloc)(data.empty() ? 1 : data.size());
	if (out && !data.empty()) {
		memcpy(out, data.data(), data.size());
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
