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

static int sudo_askpass_to_pipe(int pipe_sendpass)
{
	wxInitialize();
	const char *title = getenv(SDC_ENV_TITLE);
	const char *prompt = getenv(SDC_ENV_PROMPT);
	if (!title)
		title = "sudo";
	if (!prompt)
		prompt = "Enter sudo password:";
	
	wxPasswordEntryDialog dlg(nullptr, prompt, title, 
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

extern "C" int sudo_main_askpass()
{
	int pipe_sendpass = dup(STDOUT_FILENO);
	int fd = open("/dev/null", O_RDWR);
	if (fd!=-1) {
		dup2(fd, STDOUT_FILENO);
		close(fd);
	} else
		perror("open /dev/null");

	setlocale(LC_ALL, "");//otherwise non-latin keys missing with XIM input method
	int r = sudo_askpass_to_pipe(pipe_sendpass);
	close(fd);
	return r;
}
