#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <utils.h>
#include "NotifySh.h"

static std::string GetNotifySH(const char *far2l_path)
{
	std::string out(far2l_path);
	if (TranslateInstallPath_Bin2Share(out)) {
		ReplaceFileNamePart(out, APP_BASENAME "/notify.sh");
	} else {
		ReplaceFileNamePart(out, "notify.sh");
	}

	struct stat s;
	if (stat(out.c_str(), &s) == 0) {
		return out;
	}

	if (TranslateInstallPath_Share2Lib(out) && stat(out.c_str(), &s) == 0) {
		return out;
	}

	return std::string();
}

SHAREDSYMBOL void Far2l_NotifySh(const char *far2l_path, const char *title, const char *text)
{
	try {
		static std::string s_notify_sh = GetNotifySH(far2l_path);
		if (s_notify_sh.empty()) {
			throw std::runtime_error("notify.sh not found");
		}

		// double fork to avoid waiting for notify process that can be long
		pid_t pid = fork();
		if (pid == 0) {
			if (fork() == 0) {
				execl(s_notify_sh.c_str(), s_notify_sh.c_str(), title, text, NULL);
				perror("NotifySh - execl");
			}
			_exit(0);
			exit(0);
		} else if (pid != -1) {
			waitpid(pid, 0, 0);
		} else {
			throw std::runtime_error("fork failed");
		}
	} catch (std::exception &e) {
		fprintf(stderr, "NotifySh('%s', '%s', '%s') - %s\n", e.what(), far2l_path, title, text);
	}
}
