#pragma once
#include <string>
#include "PipeIPC.h"

enum TTYXIPCCommand
{
	IPC_INIT = 0xFACE0003, // increment if protocol changed
	IPC_FINI,
	IPC_INSPECT_KEY_EVENT,
	IPC_CLIPBOARD_GET,
	IPC_CLIPBOARD_SET,
	IPC_CLIPBOARD_CONTAINS,
};

typedef PipeIPCEndpoint<TTYXIPCCommand> TTYXIPCEndpoint;
