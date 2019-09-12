#include <fcntl.h>
#include <algorithm>
#include <utils.h>
#include <ScopeHelpers.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 =============== NetRocks global options ====================
| [x] Enable desktop notifications                           |
| [ ] <ENTER> to execute files remotely when possible        |
| [ ] Connect using proxy (requires tsocks library)          |
|     [ Edit tsocks configuration file ]                     |
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
	int _i_use_proxy = -1, _i_edit_tsocks_config = -1;

	int _i_ok = -1, _i_cancel = -1;

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_INITDIALOG) {
			struct stat s{};
			if (stat(DEFAULT_TSOCKS_CONFIG, &s) == -1) {
				SetEnabledDialogControl(_i_use_proxy, false);
				SetEnabledDialogControl(_i_edit_tsocks_config, false);
			} else {
				SetEnabledDialogControl(_i_edit_tsocks_config, IsCheckedDialogControl(_i_use_proxy));
			}

		} else if (msg == DN_BTNCLICK && param1 == _i_use_proxy) {
			bool checked = IsCheckedDialogControl(_i_use_proxy);
			SetEnabledDialogControl(_i_edit_tsocks_config, checked);
			if (checked) {
				EnsureTSocksConfigExists();
			}
			return TRUE;

		} else if (msg == DN_BTNCLICK && param1 == _i_edit_tsocks_config) {
			G.info.Editor(StrMB2Wide(G.tsocks_config).c_str(),
				NULL, -1, -1, -1, -1, EF_DISABLEHISTORY, 1, 1, CP_UTF8);
			return TRUE;
		}

		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	ConfigurePlugin()
	{
		_di.SetBoxTitleItem(MPluginOptionsTitle);

		_di.SetLine(2);
		_i_enable_desktop_notifications = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MEnableDesktopNotifications);

		_di.NextLine();
		_i_enter_exec_remotely = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MEnterExecRemotely);

		_di.NextLine();
		_i_use_proxy = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MConnectUsingProxy);
		_di.NextLine();
		_i_edit_tsocks_config = _di.AddAtLine(DI_BUTTON, 9,60, 0, MEditTSocksConfig);

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
		SetCheckedDialogControl( _i_enable_desktop_notifications, G.global_config->GetInt("Options", "EnableDesktopNotifications", 1) != 0);
		SetCheckedDialogControl( _i_enter_exec_remotely, G.global_config->GetInt("Options", "EnterExecRemotely", 1) != 0);
		SetCheckedDialogControl( _i_use_proxy, G.global_config->GetInt("Options", "UseProxy", 0) != 0);

		if (Show(L"PluginOptions", 6, 2) == _i_ok) {
			G.global_config->PutInt("Options", "EnableDesktopNotifications", IsCheckedDialogControl(_i_enable_desktop_notifications) ? 1 : 0);
			G.global_config->PutInt("Options", "EnterExecRemotely", IsCheckedDialogControl(_i_enter_exec_remotely) ? 1 : 0);
			G.global_config->PutInt("Options", "UseProxy", IsCheckedDialogControl(_i_use_proxy) ? 1 : 0);
			G.global_config->Save();
		}
	}
};

void ConfigurePluginGlobalOptions()
{
	ConfigurePlugin().Configure();
}
