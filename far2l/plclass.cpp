#include "headers.hpp"
#include "plclass.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "lasterror.hpp"
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

	FARString strCurPath;
	apiGetCurrentDirectory(strCurPath);

	FARString strModulePath = m_strModuleName;
	CutToSlash(strModulePath);
	if (!FarChDir(strModulePath))
		fprintf(stderr, "Can't chdir for plugin '%ls'\n", m_strModuleName.CPtr());

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
		WINPORT(TranslateErrno)();
		// avoid recurring and even recursive error message
		WorkFlags.Set(PIWF_DONTLOADAGAIN);
		if (!Opt.LoadPlug.SilentLoadPlugin) //убрать в PluginSet
		{
			SetMessageHelp(L"ErrLoadPlugin module");
			Message(MSG_WARNING|MSG_ERRORTYPE, 1, MSG(MError), MSG(MPlgLoadPluginError), m_strModuleName, MSG(MOk));
		}
	}

	GuardLastError Err;
	FarChDir(strCurPath);

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

