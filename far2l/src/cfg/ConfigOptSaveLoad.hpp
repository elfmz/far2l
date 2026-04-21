#pragma once

enum OptSaveType {
	OST_NONE   = 0,
	OST_COMMON = 0x01,
	OST_PANELS = 0x02,
};

void ConfigOptLoad();
void ConfigOptSave(bool Ask);
void ConfigOptSaveAutoOptions();
void ConfigOptAssertLoaded();
