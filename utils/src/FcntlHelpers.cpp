#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static void FcntlFx(int fd, int cmd_get, int cmd_set, int flag)
{
	for (;;) {
	    int r = fcntl(fd, cmd_get, 0);
		if (r != -1) {
			r = (flag > 0) ? (r | flag) : (r & ~(-flag));
	    	r = fcntl(fd, cmd_set, r);
			if (r != -1) {
				break;
			}
		}
		if (errno != EAGAIN && errno != EINTR) {
			fprintf(stderr,
				"FcntlFx(%d, %d, %d, %d) errno %d\n",
				fd, cmd_get, cmd_set, flag, errno);
			break;
		}
		usleep(1000);
	}
}

void MakeFDNonBlocking(int fd)
{
	FcntlFx(fd, F_GETFL, F_SETFL, O_NONBLOCK);
}

void MakeFDBlocking(int fd)
{
	FcntlFx(fd, F_GETFL, F_SETFL, -O_NONBLOCK);
}

void MakeFDCloexec(int fd)
{
	FcntlFx(fd, F_GETFD, F_SETFD, FD_CLOEXEC);
}

void MakeFDNonCloexec(int fd)
{
	FcntlFx(fd, F_GETFD, F_SETFD, -FD_CLOEXEC);
}
