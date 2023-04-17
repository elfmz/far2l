#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "dockicon.h"

enum
{
	PU_NEW_INSTANCE = 10001,
};

wxBEGIN_EVENT_TABLE(MacDockIcon, wxTaskBarIcon)
	EVT_MENU(PU_NEW_INSTANCE, MacDockIcon::OnMenuNewInstance)
wxEND_EVENT_TABLE()

MacDockIcon::MacDockIcon(): wxTaskBarIcon(wxTBI_DOCK)
{}

wxMenu *MacDockIcon::CreatePopupMenu()
{
	wxMenu *menu = new wxMenu;
	menu->Append(PU_NEW_INSTANCE, wxT("&New instance"));
	return menu;
}

void MacDockIcon::OnMenuNewInstance(wxCommandEvent& )
{
	wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
	wxString fn_str = fn.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	fn_str+= fn.GetName();
	const char *fn_psz = fn_str.mb_str();

	pid_t pid = fork();
	if (pid == 0) {
		pid = fork();
		if (pid == 0) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, fn_psz);
			execl(fn_psz, fn_psz, nullptr);
			fprintf(stderr, "%s: execl error %d\n", __FUNCTION__, errno);
		}
		_exit(0);
		exit(0);

	} else if (pid != -1) {
		waitpid(pid, nullptr, 0);

	} else {
		fprintf(stderr, "%s: fork error %u\n", __FUNCTION__, errno);
	}
}
