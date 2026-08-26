#include "AppProvider.hpp"
#include "MacOSAppProvider.hpp"
#include "XDGBasedAppProvider.hpp"

namespace openwith
{
	std::unique_ptr<AppProvider> AppProvider::CreateAppProvider()
	{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
		return std::make_unique<XDGBasedAppProvider>();
#elif defined(__APPLE__)
		return std::make_unique<MacOSAppProvider>();
#else
		return nullptr;
#endif
	}


	AppProvider* AppProvider::GetInstance()
	{
		static const std::unique_ptr<AppProvider> instance = CreateAppProvider();
		return instance.get();
	}


	void AppProvider::CheckCancellation() const
	{
		if (_cancel_flag && _cancel_flag->load()) {
			throw OperationCancelledException{};
		}
	}


	void AppProvider::ReportProgress(const ProgressUpdate& update) const
	{
		if (_progress_cb) {
			_progress_cb(update);
		}
	}

} // namespace openwith
