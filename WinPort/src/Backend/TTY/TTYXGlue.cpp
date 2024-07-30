#include <dlfcn.h>
#include <string.h>
#include <map>
#include <string>
#include <mutex>
#include <utils.h>
#include <TestPath.h>
#include <sys/wait.h>
#include "TTYXGlue.h"
#include "TTYX/TTYX.h"
#include "WinPort.h"

class TTYXGlue : public ITTYXGlue
{
	pid_t _broker_pid;
	TTYXIPCEndpoint _ipc;
	bool _xi = false;

public:
	TTYXGlue(pid_t broker_pid, int fdr, int fdw)
		:
		_broker_pid(broker_pid),
		_ipc(fdr, fdw)
	{
	}

	virtual ~TTYXGlue()
	{
		try {
			_ipc.SendCommand(IPC_FINI);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
		}
		_ipc.SetFD(-1, -1);
		waitpid(_broker_pid, 0, 0);
	}

	void Initialize(bool allow_xi)
	{
		_ipc.SendCommand(IPC_INIT);
		_ipc.SendPOD(allow_xi);

		const auto reply = _ipc.RecvCommand();
		if (reply != IPC_INIT)
			throw PipeIPCError("bad init reply", reply);

		_ipc.RecvPOD(_xi);
	}

	virtual bool HasXi() noexcept
	{
		return _xi;
	}

	virtual bool SetClipboard(const ITTYXGlue::Type2Data &t2d) noexcept
	{
		try {
			_ipc.SendCommand(IPC_CLIPBOARD_SET);
			for (const auto &it : t2d) {
				_ipc.SendPOD(size_t(it.second.size()));
				_ipc.SendString(it.first);
				if (!it.second.empty()) {
					_ipc.Send(it.second.data(), it.second.size());
				}
			}
			_ipc.SendPOD(size_t(-1));

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
			return false;
		}
		return true;
	}

	virtual bool GetClipboard(const std::string &type, std::vector<unsigned char> &data) noexcept
	{
		try {
			data.clear();
			_ipc.SendCommand(IPC_CLIPBOARD_GET);
			_ipc.SendString(type);
			ssize_t size = -1;
			_ipc.RecvPOD(size);
			if (size > 0) {
				data.resize(size);
				_ipc.Recv(data.data(), data.size());
			}
			return (size >= 0);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
			return false;
		}
	}

	virtual bool ContainsClipboard(const std::string &type) noexcept
	{
		try {
			_ipc.SendCommand(IPC_CLIPBOARD_CONTAINS);
			_ipc.SendString(type);
			bool reply = false;
			_ipc.RecvPOD(reply);
			return reply;

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
			return false;
		}
	}

	virtual void InspectKeyEvent(KEY_EVENT_RECORD &event) noexcept
	{
		const KEY_EVENT_RECORD saved_event = event;
		try {
			_ipc.SendCommand(IPC_INSPECT_KEY_EVENT);
			_ipc.SendPOD(event);
			_ipc.RecvPOD(event);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
			event = saved_event;
		}
	}
};

ITTYXGluePtr StartTTYX(const char *full_exe_path, bool allow_xi)
{
	const char *d = getenv("DISPLAY");
	if (!d || !*d) {
		return ITTYXGluePtr();
	}

	try {
		std::string broker_path = full_exe_path;
		ReplaceFileNamePart(broker_path, "far2l_ttyx.broker");
		TranslateInstallPath_Bin2Lib(broker_path);
		if (!TestPath(broker_path).Executable()) {
			// likely compiled without TTYX, thats not an error, just such a configuration
			return ITTYXGluePtr();
		}

		PipeIPCFD ipc_fd;

		pid_t p = fork();
		if (p == (pid_t)-1) {
			throw PipeIPCError("failed to fork", errno);

		} else if (p == 0) {
			execl(broker_path.c_str(), broker_path.c_str(), ipc_fd.broker_arg_r, ipc_fd.broker_arg_w, NULL);
			perror("StartTTYX - execl");
			_exit(-1);
			exit(-1);
		}

		auto out = std::make_shared<TTYXGlue>(p, ipc_fd.broker2master[0], ipc_fd.master2broker[1]);
		// all FDs are in place, so avoid automatic closing of pipes FDs in ipc_fd's d-tor
		ipc_fd.Detach();
		out->Initialize(allow_xi);
		return out;

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}

	return ITTYXGluePtr();
}


////////////////////////////////////////////////////////////////////////////////////////

static class TTYXCustomFormats : std::map<UINT, std::string>, std::mutex
{
	UINT _next_index = 0;
	const std::string _empty;

public:
	UINT Register(LPCWSTR lpszFormat)
	{
		std::string format = Wide2MB(lpszFormat);

		std::unique_lock<std::mutex> lock(*this);
		for (const auto &i : *this) {
			if (i.second == format)
				return i.first;
		}

		for (;;) {
			_next_index++;
			if (_next_index < 0xC000 || _next_index > 0xFFFF)
				_next_index = 0xC000;
			if (find(_next_index) == end()) {
				insert(value_type(_next_index, format));
				return _next_index;
			}
		}
	}

	const std::string &Lookup(UINT format)
	{
		if (format == CF_UNICODETEXT || format == CF_TEXT) {
			return _empty;
		}

		std::unique_lock<std::mutex> lock(*this);
		const_iterator i = find(format);
		if ( i == end() ) {
			throw std::runtime_error("bad format");
		}

		return i->second;
	}

} g_ttyx_custom_formats;

TTYXClipboard::TTYXClipboard(ITTYXGluePtr &ttyx)
	: _ttyx(ttyx)
{
}

TTYXClipboard::~TTYXClipboard()
{
}

bool TTYXClipboard::OnClipboardOpen()
{
	_pending_set.reset();
	return _fallback_clipboard ? _fallback_clipboard->OnClipboardOpen() : true;
}

void TTYXClipboard::OnClipboardClose()
{
	if (_fallback_clipboard)
		_fallback_clipboard->OnClipboardClose();

	if (_pending_set) {
		if (!_ttyx->SetClipboard(*_pending_set)) {
			// SetClipboard may fail only due to IPC failure (e.g. if broken terminated)
			if (!_fallback_clipboard) {
				fprintf(stderr, "TTYXClipboard::OnClipboardClose: switching to fallback\n");
				_fallback_clipboard.reset(new FSClipboardBackend);
			}
		}
		_pending_set.reset();
	}
}

void TTYXClipboard::OnClipboardEmpty()
{
	if (_fallback_clipboard)
		_fallback_clipboard->OnClipboardEmpty();

	_pending_set.reset(new ITTYXGlue::Type2Data);
}

bool TTYXClipboard::OnClipboardIsFormatAvailable(UINT format)
{
	if (_fallback_clipboard)
		return _fallback_clipboard->OnClipboardIsFormatAvailable(format);

	try {
		std::string format_name = g_ttyx_custom_formats.Lookup(format);
		return _ttyx->ContainsClipboard(format_name);

	} catch (std::exception &e) {
		fprintf(stderr, "TTYXClipboard::OnClipboardIsFormatAvailable(0x%x): %s\n", format, e.what());
		return false;
	}
}

void *TTYXClipboard::OnClipboardSetData(UINT format, void *data)
{
	if (_fallback_clipboard)
		return _fallback_clipboard->OnClipboardSetData(format, data);

	try {
		std::string format_name = g_ttyx_custom_formats.Lookup(format);
		if (!_pending_set) {
			_pending_set.reset(new ITTYXGlue::Type2Data);
		}

		size_t len = WINPORT(ClipboardSize)(data);

		auto &d = (*_pending_set)[format_name];
		if (format == CF_UNICODETEXT) {
			std::string str;
			Wide2MB((const wchar_t *)data, wcsnlen((const wchar_t *)data, len / sizeof(wchar_t)), str);
			d.resize(str.size());
			memcpy(d.data(), str.c_str(), d.size());

		} else {
			if (format == CF_TEXT) {
				len = strnlen((const char *)data, len);
			}
			d.resize(len);
			memcpy(d.data(), data, d.size());
		}

		return data;

	} catch (std::exception &e) {
		fprintf(stderr, "TTYXClipboard::OnClipboardSetData(0x%x): %s\n", format, e.what());
		return nullptr;
	}
}

void *TTYXClipboard::OnClipboardGetData(UINT format)
{
	if (_fallback_clipboard)
		return _fallback_clipboard->OnClipboardGetData(format);

	void *out = nullptr;
	try {
		std::string format_name = g_ttyx_custom_formats.Lookup(format);
		std::vector<unsigned char> data;
		if (_ttyx->GetClipboard(format_name, data)) {
			if (format == CF_UNICODETEXT || format == CF_TEXT) {
				const char *utf8 = (const char *)data.data();
				size_t utf8_len = strnlen(utf8, data.size());
				if (format == CF_UNICODETEXT) {
					std::wstring ws;
					MB2Wide(utf8, utf8_len, ws);
					const size_t sz = (ws.size() + 1) * sizeof(wchar_t);
					out = WINPORT(ClipboardAlloc)(sz);
					if (out)
						memcpy(out, ws.c_str(), sz);
				} else {
					out = WINPORT(ClipboardAlloc)(utf8_len + 1);
					if (out)
						memcpy(out, data.data(), utf8_len + 1);
				}

			} else {
				out = WINPORT(ClipboardAlloc)(data.size());
				if (out)
					memcpy(out, data.data(), data.size());
			}
		}
	} catch (std::exception &e) {
		fprintf(stderr, "TTYXClipboard::OnClipboardGetData(0x%x): %s\n", format, e.what());
	}

	return out;
}

UINT TTYXClipboard::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	if (_fallback_clipboard)
		return _fallback_clipboard->OnClipboardRegisterFormat(lpszFormat);

	try {
		return g_ttyx_custom_formats.Register(lpszFormat);

	} catch (std::exception &e) {
		fprintf(stderr, "TTYXClipboard::OnClipboardRegisterFormat('%ls'): %s\n", lpszFormat, e.what());
		return 0;
	}
}
