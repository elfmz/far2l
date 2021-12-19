#pragma once
#include <string>
#include "PipeIPC.h"

enum IPCCommand
{
	IPC_INIT = 0xFACE0001, // increment if protocol changed
	IPC_FINI,
	IPC_MODIFIERS,
	IPC_CLIPBOARD_GET,
	IPC_CLIPBOARD_SET,
	IPC_CLIPBOARD_CONTAINS
};

typedef PipeIPCEndpoint<IPCCommand> IPCEndpoint;
