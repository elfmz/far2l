#pragma once
#include <wx/wx.h>
#include "wx/taskbar.h"

class MacDockIcon : public wxTaskBarIcon
{
public:
	MacDockIcon();

	virtual wxMenu *CreatePopupMenu();

	wxDECLARE_EVENT_TABLE();
	void OnMenuNewInstance(wxCommandEvent& );
};
