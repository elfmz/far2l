#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fd = open(argv[1], O_RDONLY);
	for (;;) {
		lseek(fd, 0, SEEK_SET);
		char buf[0x10000] = {0};
		read (fd, buf, sizeof(buf)-1);
		printf("%s\n\n", buf);
		sleep(1);
	}
	close(fd);
}
