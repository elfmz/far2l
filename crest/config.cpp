#include "crest.h"
#include <utils.h>
#include <KeyFileHelper.h>

#define INI_LOCATION InMyConfig("plugins/crest/config.ini")
#define INI_SECTION  "Settings"

static const char sKeyEnabled[] = ("Enabled");
static const char sKeyColor[] = ("Color");
static const char sKeyCenterColor[] = ("Cursor Color");
static const char sKeyRulerColor[] = ("Ruler Color");
static const char sKeyTempShow[] = ("Activators");
static const char sKeyLockShow[] = ("Lockers");
static const char sKeyFlags[] = ("Flags");

extern "C" {
void RestoreConfig(CROptions *Options)
{
	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	Options->Enabled = !!kfh.GetInt(sKeyEnabled, Options->Enabled);
	Options->Color = kfh.GetInt(sKeyColor, Options->Color);
	Options->CenterColor = kfh.GetInt(sKeyCenterColor, Options->CenterColor);
	Options->RulerColor = kfh.GetInt(sKeyRulerColor, Options->RulerColor);
	Options->TempShow = kfh.GetInt(sKeyTempShow, Options->TempShow);
	Options->LockShow = kfh.GetInt(sKeyLockShow, Options->LockShow);
	Options->Flags = kfh.GetInt(sKeyFlags, Options->Flags);
}

void SaveConfig(const CROptions *Options)
{
	KeyFileHelper kfh(INI_LOCATION);
	kfh.SetInt(INI_SECTION, sKeyEnabled, Options->Enabled);
	kfh.SetInt(INI_SECTION, sKeyColor, Options->Color);
	kfh.SetInt(INI_SECTION, sKeyCenterColor, Options->CenterColor);
	kfh.SetInt(INI_SECTION, sKeyRulerColor, Options->RulerColor);
	kfh.SetInt(INI_SECTION, sKeyTempShow, Options->TempShow);
	kfh.SetInt(INI_SECTION, sKeyLockShow, Options->LockShow);
	kfh.SetInt(INI_SECTION, sKeyFlags, Options->Flags);
}
}
