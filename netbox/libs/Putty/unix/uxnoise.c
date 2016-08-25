/*
 * Noise generation for PuTTY's cryptographic random number
 * generator.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "putty.h"
#include "ssh.h"
#include "storage.h"

static int read_dev_urandom(char *buf, int len)
{
    int fd;
    int ngot, ret;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
	return 0;

    ngot = 0;
    while (ngot < len) {
	ret = read(fd, buf+ngot, len-ngot);
	if (ret < 0) {
	    close(fd);
	    return 0;
	}
	ngot += ret;
    }

    close(fd);

    return 1;
}

/*
 * This function is called once, at PuTTY startup. It will do some
 * slightly silly things such as fetching an entire process listing
 * and scanning /tmp, load the saved random seed from disk, and
 * also read 32 bytes out of /dev/urandom.
 */

void noise_get_heavy(void (*func) (void *, int))
{
    char buf[512];
    FILE *fp;
    int ret;
    int got_dev_urandom = 0;

    if (read_dev_urandom(buf, 32)) {
	got_dev_urandom = 1;
	func(buf, 32);
    }

    fp = popen("ps -axu 2>/dev/null", "r");
    if (fp) {
	while ( (ret = fread(buf, 1, sizeof(buf), fp)) > 0)
	    func(buf, ret);
	pclose(fp);
    } else if (!got_dev_urandom) {
	fprintf(stderr, "popen: %s\n"
		"Unable to access fallback entropy source\n", strerror(errno));
	exit(1);
    }

    fp = popen("ls -al /tmp 2>/dev/null", "r");
    if (fp) {
	while ( (ret = fread(buf, 1, sizeof(buf), fp)) > 0)
	    func(buf, ret);
	pclose(fp);
    } else if (!got_dev_urandom) {
	fprintf(stderr, "popen: %s\n"
		"Unable to access fallback entropy source\n", strerror(errno));
	exit(1);
    }

    read_random_seed(func);
    random_save_seed();
}

void random_save_seed(void)
{
    int len;
    void *data;

    if (random_active) {
	random_get_savedata(&data, &len);
	write_random_seed(data, len);
	sfree(data);
    }
}

/*
 * This function is called every time the random pool needs
 * stirring, and will acquire the system time.
 */
void noise_get_light(void (*func) (void *, int))
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    func(&tv, sizeof(tv));
}

/*
 * This function is called on a timer, and grabs as much changeable
 * system data as it can quickly get its hands on.
 */
void noise_regular(void)
{
    int fd;
    int ret;
    char buf[512];
    struct rusage rusage;

    if ((fd = open("/proc/meminfo", O_RDONLY)) >= 0) {
	while ( (ret = read(fd, buf, sizeof(buf))) > 0)
	    random_add_noise(buf, ret);
	close(fd);
    }
    if ((fd = open("/proc/stat", O_RDONLY)) >= 0) {
	while ( (ret = read(fd, buf, sizeof(buf))) > 0)
	    random_add_noise(buf, ret);
	close(fd);
    }
    getrusage(RUSAGE_SELF, &rusage);
    random_add_noise(&rusage, sizeof(rusage));
}

/*
 * This function is called on every keypress or mouse move, and
 * will add the current time to the noise pool. It gets the scan
 * code or mouse position passed in, and adds that too.
 */
void noise_ultralight(unsigned long data)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    random_add_noise(&tv, sizeof(tv));
    random_add_noise(&data, sizeof(data));
}
