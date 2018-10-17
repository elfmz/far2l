#if __FreeBSD__
# include <malloc_np.h>
#elif __APPLE__
# include <malloc/malloc.h>
#else
# include <malloc.h>
#endif

#include "FSClipboardBackend.h"
#include <utils.h>

void RegUnescape(std::string &s);
void RegEscape(std::string &s);


FSClipboardBackend::FSClipboardBackend() :
	_shared_resource("fsclip", 0)
{
	_default_backend = WinPortClipboard_SetBackend(this);
}


FSClipboardBackend::~FSClipboardBackend()
{
	WinPortClipboard_SetBackend(_default_backend);
}

bool FSClipboardBackend::OnClipboardOpen()
{
	if (!_shared_resource.LockWrite(5))
		return false;

	_kfh = std::make_shared<KeyFileHelper>(InMyConfig("fsclipboard.ini").c_str(), true);
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

#ifdef _WIN32
	size_t len = _msize(data);
#elif defined(__APPLE__)
	size_t len = malloc_size(data);
#else
	size_t len = malloc_usable_size(data);
#endif
	std::string str((const char *)data, len);
	RegEscape(str);
	str.insert(0, "!");
	_kfh->PutString("Data", str_format, str.c_str());
	return data;
}

void *FSClipboardBackend::OnClipboardGetData(UINT format)
{
	if (!_kfh)
		return nullptr;

	char str_format[64]; sprintf(str_format, "0x%x", format);

	std::string str = _kfh->GetString("Data", str_format);
	if (str.empty() || str[0] != '!')
		return nullptr;

	str.erase(0, 1);
	RegUnescape(str);
	void *out = malloc(str.empty() ? 1 : str.size());
	memcpy(out, &str[0], str.size());
	return out;
}

UINT FSClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	if (!_kfh)
		return 0;

	const std::string &str_format_name = Wide2MB(lpszFormat);
	int id = _kfh->GetInt("Formats", str_format_name.c_str(), 0);
	if (id == 0) {
		id = _kfh->GetInt("Global", "LastRegisteredFormat", 0);
		++id;
		if (id < 0xC000 || id> 0xFFFF)
			id = 0xC000;

		_kfh->PutInt("Global", "LastRegisteredFormat", id);

		_kfh->PutInt("Formats", str_format_name.c_str(), id);
	}

	return (UINT)id;
}
