/*
 * uxsel.c
 * 
 * This module is a sort of all-purpose interchange for file
 * descriptors. At one end it talks to uxnet.c and pty.c and
 * anything else which might have one or more fds that need
 * select()-type things doing to them during an extended program
 * run; at the other end it talks to pterm.c or uxplink.c or
 * anything else which might have its own means of actually doing
 * those select()-type things.
 */

#include <assert.h>

#include "putty.h"
#include "tree234.h"

struct fd {
    int fd;
    int rwx;			       /* 4=except 2=write 1=read */
    uxsel_callback_fn callback;
    int id;			       /* for uxsel_input_remove */
};

static tree234 *fds;

static int uxsel_fd_cmp(void *av, void *bv)
{
    struct fd *a = (struct fd *)av;
    struct fd *b = (struct fd *)bv;
    if (a->fd < b->fd)
	return -1;
    if (a->fd > b->fd)
	return +1;
    return 0;
}
static int uxsel_fd_findcmp(void *av, void *bv)
{
    int *a = (int *)av;
    struct fd *b = (struct fd *)bv;
    if (*a < b->fd)
	return -1;
    if (*a > b->fd)
	return +1;
    return 0;
}

void uxsel_init(void)
{
    fds = newtree234(uxsel_fd_cmp);
}

/*
 * Here is the interface to fd-supplying modules. They supply an
 * fd, a set of read/write/execute states, and a callback function
 * for when the fd satisfies one of those states. Repeated calls to
 * uxsel_set on the same fd are perfectly legal and serve to change
 * the rwx state (typically you only want to select an fd for
 * writing when you actually have pending data you want to write to
 * it!).
 */

void uxsel_set(int fd, int rwx, uxsel_callback_fn callback)
{
    struct fd *newfd;

    uxsel_del(fd);

    if (rwx) {
	newfd = snew(struct fd);
	newfd->fd = fd;
	newfd->rwx = rwx;
	newfd->callback = callback;
	newfd->id = uxsel_input_add(fd, rwx);
	add234(fds, newfd);
    }
}

void uxsel_del(int fd)
{
    struct fd *oldfd = find234(fds, &fd, uxsel_fd_findcmp);
    if (oldfd) {
	uxsel_input_remove(oldfd->id);
	del234(fds, oldfd);
	sfree(oldfd);
    }
}

/*
 * And here is the interface to select-functionality-supplying
 * modules. 
 */

int next_fd(int *state, int *rwx)
{
    struct fd *fd;
    fd = index234(fds, (*state)++);
    if (fd) {
	*rwx = fd->rwx;
	return fd->fd;
    } else
	return -1;
}

int first_fd(int *state, int *rwx)
{
    *state = 0;
    return next_fd(state, rwx);
}

int select_result(int fd, int event)
{
    struct fd *fdstruct = find234(fds, &fd, uxsel_fd_findcmp);
    /*
     * Apparently this can sometimes be NULL. Can't see how, but I
     * assume it means I need to ignore the event since it's on an
     * fd I've stopped being interested in. Sigh.
     */
    if (fdstruct)
        return fdstruct->callback(fd, event);
    else
        return 1;
}
