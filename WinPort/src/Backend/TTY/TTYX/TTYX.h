#pragma once
#include <string>
#include "PipeIPC.h"

enum IPCCommand
{
	IPC_INIT = 0xFACE0000, // change if protocol changed
	IPC_FINI,
	IPC_MODIFIERS,
	IPC_CLIPBOARD_GET,
	IPC_CLIPBOARD_SET,
};

typedef PipeIPCEndpoint<IPCCommand> IPCEndpoint;
