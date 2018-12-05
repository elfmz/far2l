#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>

#include "sudo_private.h"
#include "sudo_askpass_ipc.h"

extern "C" int sudo_main_askpass()
{
	std::string password;
	switch (SudoAskpassRequestPassword(password)) {
		case SAR_OK:
			password+= '\n';
			if (write(STDOUT_FILENO, password.c_str(), password.size()) <= 0)
				perror("sudo_main_askpass - write");
			return 0;

		case SAR_CANCEL:
			return 1;

		default:
			return -1;
	}
}
