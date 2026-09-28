#include "incsrch.h"
#include <utils.h>
#include <KeyFileHelper.h>

#define INI_LOCATION InMyConfig("plugins/incsrch/config.ini")
#define INI_SECTION  "Settings"

static const char sKeyCaseSensitive[] = ("Case sensitive");
static const char sKeyKeepSelection[] = ("Keep selection");
static const char sKeyBeepOnMismatch[] = ("Beep on mismatch");
static const char sKeyRestartEOF[] = ("Restart EOF");
static const char sKeyUseSelection[] = ("Use selection");
static const char sKeyAutoNext[] = ("Auto next");
static const char sKeyBSunroll[] = ("Backspace unroll");

extern "C" {
void RestoreConfig(void)
{
	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	bCaseSensitive = !!kfh.GetInt(sKeyCaseSensitive, bCaseSensitive);
	bKeepSelection = !!kfh.GetInt(sKeyKeepSelection, bKeepSelection);
	bBeepOnMismatch = !!kfh.GetInt(sKeyBeepOnMismatch, bBeepOnMismatch);
	bRestartEOF = !!kfh.GetInt(sKeyRestartEOF, bRestartEOF);
	bUseSelection = !!kfh.GetInt(sKeyUseSelection, bUseSelection);
	bAutoNext = !!kfh.GetInt(sKeyAutoNext, bAutoNext);
	bBSunroll = !!kfh.GetInt(sKeyBSunroll, bBSunroll);
}

void SaveConfig(void)
{
	KeyFileHelper kfh(INI_LOCATION);
	kfh.SetInt(INI_SECTION, sKeyCaseSensitive, bCaseSensitive);
	kfh.SetInt(INI_SECTION, sKeyKeepSelection, bKeepSelection);
	kfh.SetInt(INI_SECTION, sKeyBeepOnMismatch, bBeepOnMismatch);
	kfh.SetInt(INI_SECTION, sKeyRestartEOF, bRestartEOF);
	kfh.SetInt(INI_SECTION, sKeyUseSelection, bUseSelection);
	kfh.SetInt(INI_SECTION, sKeyAutoNext, bAutoNext);
	kfh.SetInt(INI_SECTION, sKeyBSunroll, bBSunroll);
}
}
