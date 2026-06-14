#include "AppProvider.hpp"

#include "XDGBasedAppProvider.hpp"
#include "MacOSAppProvider.hpp"

std::unique_ptr<AppProvider> AppProvider::CreateAppProvider(TMsgGetter msg_getter)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
	return std::make_unique<XDGBasedAppProvider>(msg_getter);
#elif defined(__APPLE__)
	return std::make_unique<MacOSAppProvider>(msg_getter);
#else
	return nullptr;
#endif
}


AppProvider* AppProvider::GetInstance(TMsgGetter msg_getter)
{
	static auto s_provider = CreateAppProvider(msg_getter);
	return s_provider.get();
}