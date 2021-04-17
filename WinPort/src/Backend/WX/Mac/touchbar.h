#pragma once

struct ITouchbarListener
{
	virtual void OnTouchbarKey(bool alternate, int index) = 0;
};

void Touchbar_Register(ITouchbarListener *listener);
void Touchbar_Deregister();

// titles: array of 12 strings, empty string means key is disabled, NULL means default title
// titles == NULL means all titles are default and its equiv to array of 12 NULLs
bool Touchbar_SetTitles(const char **titles);

void Touchbar_SetAlternate(bool on);
