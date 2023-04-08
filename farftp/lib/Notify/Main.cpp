#include <all_far.h>

#include "Int.h"

// main() {}

FILE *File = NULL;
/** Called each time FPT plugin starts or finish upload and download single file
 */
static void WINAPI idNotify(const FTNNotify *p)
{
	if (File == (FILE *)1)
		return;

	if (!File) {
		char fnm[MAX_PATH] = "/var/log/farftp_notify.log";
		// fnm[ GetModuleFileName(FTP_Module,fnm,ARRAYSIZE(fnm))] = 0;
		// FTP_Info->StrCpy(strrchr(fnm,'/')+1, "notify.log", -1);
		File = fopen(fnm, "a");

		if (!File)
			File = fopen(fnm, "w");

		if (!File) {
			File = (FILE *)1;
			return;
		}
	}

	if (p->Starting)
		fprintf(File, "%s from %s is started\t[ftp://%s%s:%d]->[%s]\n", p->Upload ? "Upload" : "Download",
				p->RestartPoint ? FTP_Info->Message("%64I", p->RestartPoint) : "begining", p->HostName,
				p->RemoteFile, (int)p->Port, p->LocalFile);
	else
		fprintf(File, "%s started from %llu is %s\t[ftp://%s%s:%d]->[%s]\n",
				p->Upload ? "Upload" : "Download", (long long unsigned int)p->RestartPoint,
				p->Success ? "successfully complete" : "fail", p->HostName, p->RemoteFile, (int)p->Port,
				p->LocalFile);

	fflush(File);
}

// ------------------------------------------------------------------------
// Exported interface
// ------------------------------------------------------------------------
FTPPluginInterface *WINAPI FTPPluginGetInterface_Notify(void)
{
	static NotifyInterface Interface;
	Interface.Magic = FTP_NOTIFY_MAGIC;
	Interface.Notify = idNotify;
	return &Interface;
}

//------------------------------------------------------------------------
