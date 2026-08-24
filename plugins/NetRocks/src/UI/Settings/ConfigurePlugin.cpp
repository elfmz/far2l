#include <fcntl.h>
#include <algorithm>
#include <stdexcept>
#include <utils.h>
#include <ScopeHelpers.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 =============== NetRocks global options ====================
| [x] Enable desktop notifications                           |
| [x] <ENTER> to execute files remotely when possible        |
| [x] Smart symlinks copying                                 |
| Use of chmod:                    [COMBOBOX               ] |
| [ ] Remember working directory in site settings            |
| Connections pool expiration (seconds):               [   ] |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

#define DEFAULT_TSOCKS_CONFIG "/etc/tsocks.conf"

static void EnsureTSocksConfigExists()
{
	struct stat s{};
	if (sdc_stat(G.tsocks_config.c_str(), &s) == -1) try {
		FDScope fd_dst(open(G.tsocks_config.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0600));
		if (fd_dst == -1) {
			throw std::runtime_error("create config failed");
		}

		FDScope fd_src(open(DEFAULT_TSOCKS_CONFIG, O_RDONLY));
		if (fd_src == -1) {
			throw std::runtime_error("open default config failed");
		}

		for (;;) {
			ssize_t r = ReadWritePiece(fd_src, fd_dst);
			if (r == 0) break;
			if (r < 0) {
				throw std::runtime_error("copy content failed");
			}
		}
	} catch (std::exception &ex) {
		fprintf(stderr, "EnsureTSocksConfigExists: %s path='%s'\n", ex.what(), G.tsocks_config.c_str());
		unlink(G.tsocks_config.c_str());
	}
}

class ConfigurePlugin : protected BaseDialog
{
	int _i_enable_desktop_notifications = -1;
	int _i_enter_exec_remotely = -1;
	int _i_smart_symlinks_copy = -1;
	int _i_use_of_chmod = -1;
	int _i_remember_directory = -1;
	int _i_conn_pool_expiration = -1;

	int _i_ok = -1, _i_cancel = -1;

	FarListWrapper _di_use_of_chmod;

public:
	ConfigurePlugin()
	{
		_di_use_of_chmod.Add(MUseOfChmod_Auto);
		_di_use_of_chmod.Add(MUseOfChmod_Always);
		_di_use_of_chmod.Add(MUseOfChmod_Never);

		_di.SetBoxTitleItem(MPluginOptionsTitle);

		_di.SetLine(2);
		_i_enable_desktop_notifications = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MEnableDesktopNotifications);

		_di.NextLine();
		_i_enter_exec_remotely = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MEnterExecRemotely);

		_di.NextLine();
		_i_smart_symlinks_copy = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MSmartSymlinksCopy);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,35, 0, MUseOfChmod);
		_i_use_of_chmod = _di.AddAtLine(DI_COMBOBOX, 36,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_use_of_chmod].ListItems = _di_use_of_chmod.Get();

		_di.NextLine();
		_i_remember_directory = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MRememberDirectory);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,58, 0, MConnPoolExpiration);
		_i_conn_pool_expiration = _di.AddAtLine(DI_FIXEDIT, 59,62, DIF_MASKEDIT, "30", "9999");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();

		_i_ok = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);
	}


	void Configure()
	{
		SetCheckedDialogControl( _i_enable_desktop_notifications, G.GetGlobalConfigBool("EnableDesktopNotifications", true) );
		SetCheckedDialogControl( _i_enter_exec_remotely, G.GetGlobalConfigBool("EnterExecRemotely", true) );
		SetCheckedDialogControl( _i_smart_symlinks_copy, G.GetGlobalConfigBool("SmartSymlinksCopy", true) );
		SetDialogListPosition( _i_use_of_chmod, G.GetGlobalConfigInt("UseOfChmod", 0) );
		SetCheckedDialogControl( _i_remember_directory, G.GetGlobalConfigBool("RememberDirectory", false) );
		LongLongToDialogControl( _i_conn_pool_expiration, G.GetGlobalConfigInt("ConnectionsPoolExpiration", 30) );

		if (Show(L"PluginOptions", 6, 2) == _i_ok) {
			auto gcw = G.GetGlobalConfigWriter();
			gcw.SetBool("EnableDesktopNotifications", IsCheckedDialogControl(_i_enable_desktop_notifications) );
			gcw.SetBool("EnterExecRemotely", IsCheckedDialogControl(_i_enter_exec_remotely) );
			gcw.SetBool("SmartSymlinksCopy", IsCheckedDialogControl(_i_smart_symlinks_copy) );
			gcw.SetInt("UseOfChmod", GetDialogListPosition(_i_use_of_chmod));
			gcw.SetBool("RememberDirectory", IsCheckedDialogControl(_i_remember_directory) );
			gcw.SetInt("ConnectionsPoolExpiration", LongLongFromDialogControl( _i_conn_pool_expiration) );
		}
	}
};

void ConfigurePluginGlobalOptions()
{
	ConfigurePlugin().Configure();
}
