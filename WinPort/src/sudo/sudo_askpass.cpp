#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>

#include "sudo_common.h"
#include <wx/wx.h>
#include <wx/display.h>
#include <wx/textdlg.h>

extern "C" int sudo_askpass(int pipe_sendpass)
{
	wxInitialize();
	wxPasswordEntryDialog dlg(nullptr, "Enter sudo password", "Operation requires priviledges elevation", 
		wxString(), wxCENTRE | wxOK | wxCANCEL);
	if ( dlg.ShowModal() == wxID_OK )
	{
		wxString value = dlg.GetValue();
		value+= '\n';
		if (write(pipe_sendpass, value.c_str(), value.size()) <= 0) {
			perror("sudo_askpass - write");
			return -1;
		}
		return 0;
	}
	
	return 1;
}
