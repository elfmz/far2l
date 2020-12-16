#pragma once

struct ITouchbarListener
{
	virtual void OnTouchbarFKey(int index) = 0;
};

void Touchbar_Register(ITouchbarListener *listener);
void Touchbar_Deregister();
