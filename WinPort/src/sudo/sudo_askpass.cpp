#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>

#include "sudo_private.h"
#include <wx/wx.h>
#include <wx/display.h>
#include <wx/textdlg.h>
#include <wx/evtloop.h>
#include <wx/apptrait.h>

extern "C" int sudo_askpass(int pipe_sendpass)
{
	wxInitialize();
	const char *far2l_sudo_title = getenv("far2l_sudo_title");
	const char *far2l_sudo_prompt = getenv("far2l_sudo_prompt");
	
	wxPasswordEntryDialog dlg(nullptr, 
		far2l_sudo_prompt ? far2l_sudo_prompt : "Enter sudo password:", 
		far2l_sudo_title ? far2l_sudo_title : "far2l askpass", 
		wxString(), wxCENTRE | wxOK | wxCANCEL);

	int r;
	if ( dlg.ShowModal() == wxID_OK )
	{
		wxString value = dlg.GetValue();
		value+= '\n';
		if (write(pipe_sendpass, value.c_str(), value.size()) <= 0) {
			perror("sudo_askpass - write");
			r = -1;
		} else
			r = 0;
	} else
		r = 1;
	
	return r;
}
