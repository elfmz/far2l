#include <dlfcn.h>
#include <string.h>
#include <utils.h>
#include <TestPath.h>
#include <sys/wait.h>
#include "TTYXGlue.h"
#include "TTYX/TTYX.h"
#include "WinPort.h"

class TTYXGlue : public ITTYXGlue
{
	pid_t _broker_pid;
	IPCEndpoint _ipc;

public:
	TTYXGlue(pid_t broker_pid, int fdr, int fdw)
		:
		_broker_pid(broker_pid),
		_ipc(fdr, fdw)
	{
		try {
			_ipc.SendCommand(IPC_INIT);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
		}
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

	virtual DWORD GetModifiers() noexcept
	{
		DWORD out = 0;
		try {
			_ipc.SendCommand(IPC_MODIFIERS);
			_ipc.RecvPOD(out);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
		}
		return out;
	}

	virtual bool SetClipboard(const std::string &s) noexcept
	{
		try {
			_ipc.SendCommand(IPC_CLIPBOARD_SET);
			_ipc.SendString(s);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
			return false;
		}
		return true;
	}

	virtual bool GetClipboard(std::string &s) noexcept
	{
		try {
			_ipc.SendCommand(IPC_CLIPBOARD_GET);
			_ipc.RecvString(s);

		} catch (std::exception &e) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
			_ipc.SetFD(-1, -1);
			return false;
		}
		return true;
	}
};

ITTYXGluePtr StartTTYX(const char *full_exe_path)
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

		ITTYXGluePtr out = std::make_shared<TTYXGlue>(p, ipc_fd.broker2master[0], ipc_fd.master2broker[1]);
		// all FDs are in place, so avoid automatic closing of pipes FDs in ipc_fd's d-tor
		ipc_fd.Detach();
		return out;

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}

	return ITTYXGluePtr();
}

////////////////////////////////////////////////////////////////////////////////////////

TTYXClipboard::TTYXClipboard(ITTYXGluePtr &ttyx)
	: _ttyx(ttyx)
{
}

TTYXClipboard::~TTYXClipboard()
{
}

bool TTYXClipboard::OnClipboardOpen()
{
	if (!_fs_fallback.OnClipboardOpen()) {
		return false;
	}
	_empty_pending = false;
	return true;
}

void TTYXClipboard::OnClipboardClose()
{
	if (_empty_pending) {
		_ttyx->SetClipboard(std::string());
	}
	_fs_fallback.OnClipboardClose();
}

void TTYXClipboard::OnClipboardEmpty()
{
	_empty_pending = true;
	_fs_fallback.OnClipboardEmpty();
}

bool TTYXClipboard::OnClipboardIsFormatAvailable(UINT format)
{
	return format == CF_UNICODETEXT || _fs_fallback.OnClipboardIsFormatAvailable(format);
}

void *TTYXClipboard::OnClipboardSetData(UINT format, void *data)
{
	_empty_pending = false;
	if (format == CF_UNICODETEXT) {
		size_t dlen = wcsnlen((const wchar_t *)data, WINPORT(ClipboardSize)(data) / sizeof(wchar_t));
		std::string str;
		Wide2MB((const wchar_t *)data, dlen, str);
		_ttyx->SetClipboard(str);

	} else if (format == CF_TEXT) {
		size_t dlen = strnlen((const char *)data, WINPORT(ClipboardSize)(data));
		std::string str((const char *)data, dlen);
		_ttyx->SetClipboard(str);
	}

	_fs_fallback.OnClipboardSetData(format, data);
	return data;
}

void *TTYXClipboard::OnClipboardGetData(UINT format)
{
	if (format != CF_UNICODETEXT) {
		return _fs_fallback.OnClipboardGetData(format);
	}

	std::string str;
	_ttyx->GetClipboard(str);

	std::wstring ws;
	StrMB2Wide(str, ws);

	const size_t sz = (ws.size() + 1) * sizeof(wchar_t);
	void *out = WINPORT(ClipboardAlloc)(sz);
	memcpy(out, ws.c_str(), sz);

	return out;
}

UINT TTYXClipboard::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	return _fs_fallback.OnClipboardRegisterFormat(lpszFormat);
}
