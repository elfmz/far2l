#include <string.h>

#include "SDLShareBackendOptions.h"
#include "SDLBackendUtils.h"

SDLShareBackendOptionsBackend::SDLShareBackendOptionsBackend() {}
SDLShareBackendOptionsBackend::~SDLShareBackendOptionsBackend() {}

void SDLShareBackendOptionsBackend::ShareBackendOptions(void* options)
{
	SDLBackend::options = (BackendOptions*)options;
}
