#include "headers.hpp"
#include "plclass.hpp"
#include "lang.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "message.hpp"
#include "plugins.hpp"

#include <errno.h>
#include <dlfcn.h>

Plugin::Plugin(PluginManager *owner,
		const FARString &strModuleName,
		const std::string &settingsName,
		const std::string &moduleID)
	:
	m_owner(owner),
	m_strModuleName(strModuleName),
	m_strSettingsName(settingsName),
	m_strModuleID(moduleID)
{
	strRootKey = Opt.strRegRoot;
	strRootKey += L"/Plugins";
}

Plugin::~Plugin()
{
	Lang.Close();
}

void *Plugin::GetModulePFN(const char *fn)
{
	void *out = dlsym(m_hModule, fn);
	if (!out)
		fprintf(stderr, "Plugin '%ls' doesn't export '%s'\n", PointToName(m_strModuleName), fn);

	return out;
}

bool Plugin::OpenModule()
{
	if (m_hModule)
		return true;

	if (WorkFlags.Check(PIWF_DONTLOADAGAIN))
		return false;

	char saved_cwd_buf[MAX_PATH + 1]{};
	char *saved_cwd = sdc_getcwd(saved_cwd_buf, MAX_PATH);

	FARString strModulePath = m_strModuleName.Clone();
	CutToSlash(strModulePath);
	if (sdc_chdir(strModulePath.GetMB().c_str()) == -1 )
		fprintf(stderr, "Error %d chdir for plugin '%ls'\n", errno, m_strModuleName.CPtr());

	const std::string &mbPath = m_strModuleName.GetMB();
	m_hModule = dlopen(mbPath.c_str(), RTLD_LOCAL|RTLD_LAZY);

	if (m_hModule)
	{
		void (*pPluginModuleOpen)(const char *path);
		GetModuleFN(pPluginModuleOpen, "PluginModuleOpen");
		if (pPluginModuleOpen)
			pPluginModuleOpen(mbPath.c_str());
	}
	else
	{
		std::wstring strerr;
		const char *dle = dlerror();
		if (dle) {
			fprintf(stderr, "dlerror: %s\n", dle);
			const char *colon = strchr(dle, ':');
			MB2Wide(colon ? colon + 1 : dle, strerr);
		}

		// avoid recurring and even recursive error message
		WorkFlags.Set(PIWF_DONTLOADAGAIN);
		if (!Opt.LoadPlug.SilentLoadPlugin) //убрать в PluginSet
		{
			SetMessageHelp(L"ErrLoadPlugin module");
			//|MSG_ERRORTYPE
			Message(MSG_WARNING, 1, Msg::Error, strerr.c_str(), Msg::PlgLoadPluginError, m_strModuleName, Msg::Ok);
		}
	}

	ErrnoSaver Err;
	if (saved_cwd)
		sdc_chdir(saved_cwd);

	return (!!m_hModule);
}

void Plugin::CloseModule()
{
	if (m_hModule)
	{
		dlclose(m_hModule);
		m_hModule = nullptr;
	}
}

