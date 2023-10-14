#include <utils.h>
#include "Globals.h"
#include "SitesConfig.h"
#include "StringConfig.h"

// TODO: remove this code in year >= 2022
#ifdef WINPORT_REGISTRY

#define FTP_PWD_LEN 150 //max encrypted pwd length

bool SplitLocationSpecification(const char *specification,
	std::string &protocol, std::string &host, unsigned int &port,
	std::string &username, std::string &password, std::string &directory);

static std::string DecryptPassword(BYTE Src[FTP_PWD_LEN])
{
	std::string out;
	BYTE XorMask = (Src[0]^Src[1]) | 80;
	int n;
	//char *Dest = _Dest;
	//Log(( "DecryptPassword: %02X %02X %02X %02X %02X %02X",Src[0],Src[1],Src[2],Src[3],Src[4],Src[5] ));

	if(Src[0] && Src[1] && Src[2])
		for(n = 2; n < FTP_PWD_LEN; n++)
		{
			unsigned char b = (unsigned char)(Src[n] ^ XorMask);
			if (b == 0 || b == XorMask) break;
			out+= (char)b;
		}

	//Log(( "DecryptPassword: [%s]",_Dest ));
	return out;
}

bool ImportFarFtpSites()
{
	HKEY key, host_key;
	bool out = true;

	if (WINPORT(RegOpenKeyEx)(HKEY_CURRENT_USER, L"Software/Far2/Plugins/FTP/Hosts", 0, GENERIC_READ, &key) == ERROR_SUCCESS) {
		TCHAR name[0x100] = {};
		for (DWORD i = 0;WINPORT(RegEnumKey)(key, i, name, ARRAYSIZE(name) - 1) == ERROR_SUCCESS; ++i) {
			if (wcsncmp(name, L"Item", 4) == 0 && WINPORT(RegOpenKeyEx)(key, name, 0, GENERIC_READ, &host_key) == ERROR_SUCCESS) {
				char host_name[0x1000] = {}, user[0x1000] = {};
				BYTE password[0x1000] = {};
				DWORD passive_mode = 1, ask_login = 0;
				DWORD len = sizeof(host_name) - 1;
				WINPORT(RegQueryValueEx) (host_key, L"HostName", NULL, NULL, (LPBYTE)&host_name[0], &len);
				fprintf(stderr, "SITE: %s\n", host_name);

				len = sizeof(user) - 1;
				WINPORT(RegQueryValueEx) (host_key, L"User", NULL, NULL, (LPBYTE)&user[0], &len);

				len = sizeof(password) - 1;
				WINPORT(RegQueryValueEx) (host_key, L"Password", NULL, NULL, (LPBYTE)&password[0], &len);

				len = sizeof(passive_mode);
				WINPORT(RegQueryValueEx) (host_key, L"PassiveMode", NULL, NULL, (LPBYTE)&passive_mode, &len);

				len = sizeof(ask_login);
				WINPORT(RegQueryValueEx) (host_key, L"AskLogin", NULL, NULL, (LPBYTE)&ask_login, &len);

				WINPORT(RegCloseKey)(host_key);

				std::string site = host_name;
				unsigned int spec_port = 21;
				std::string spec_protocol, spec_host, spec_username, spec_password, spec_directory;
				SplitLocationSpecification(site.c_str(), spec_protocol,
					spec_host, spec_port, spec_username, spec_password, spec_directory);

				if (spec_host.empty()) {
					spec_host = site;
				}

				if (user[0]) {
					spec_username = user;
				}

				if (password[0]) {
					spec_password = DecryptPassword(password);
				}

				for (;;) {
					size_t p = site.find('/');
					if (p == std::string::npos) break;
					site[p] = '\\';
				}

				SitesConfigLocation sites_cfg_location;
				sites_cfg_location.Make("FTP");
				if (!sites_cfg_location.Change("FTP")) {
					out = false;
					break;
				}

				SitesConfig sites_cfg(sites_cfg_location);

				for (unsigned int i = 1, l = site.size(); !sites_cfg.GetProtocol(site).empty(); ++i) {
					site.resize(l);
					site+= StrPrintf(" (%u)", i);
				}

				StringConfig sc("");
				sc.SetInt("Passive", passive_mode ? 1 : 0);

				sites_cfg.SetProtocol(site, "ftp");
				sites_cfg.SetHost(site, spec_host);
				sites_cfg.SetUsername(site, spec_username);
				sites_cfg.SetPassword(site, spec_password);
				sites_cfg.SetDirectory(site, spec_directory);
				sites_cfg.SetPort(site, spec_port);
				sites_cfg.SetLoginMode(site, ask_login ? 0 : 2);
				sites_cfg.SetProtocolOptions(site, "ftp", sc.Serialize());

			}
		}
		WINPORT(RegCloseKey)(key);
	}

	return out;
}
#else

bool ImportFarFtpSites() { return true; }

#endif
