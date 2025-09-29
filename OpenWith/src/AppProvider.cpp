#include "AppProvider.hpp"

#include "XDGBasedAppProvider.hpp"
#include "MacOSAppProvider.hpp"
#include "DummyAppProvider.hpp"

std::unique_ptr<AppProvider> AppProvider::CreateAppProvider(TMsgGetter msg_getter)
{
	std::unique_ptr<AppProvider> provider;
#ifdef __linux__
	provider = std::make_unique<XDGBasedAppProvider>(msg_getter);
#elif defined(__APPLE__)
	provider = std::make_unique<MacOSAppProvider>(msg_getter);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
	provider = std::make_unique<XDGBasedAppProvider>(msg_getter);
#else
	provider = std::make_unique<DummyAppProvider>(msg_getter);
#endif

	if (provider) {
		provider->LoadPlatformSettings();
	}

	return provider;
}
