#include <string.h>

#include "wxShareBackendOptions.h"
#include <BackendOptions.h>

wxShareBackendOptionsBackend::wxShareBackendOptionsBackend() {}
wxShareBackendOptionsBackend::~wxShareBackendOptionsBackend() {}

namespace WXCustomDrawChar {
	extern BackendOptions* options;
};

void wxShareBackendOptionsBackend::ShareBackendOptions(void* options)
{
	WXCustomDrawChar::options = (BackendOptions*)options;
}
