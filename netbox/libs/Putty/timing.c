/*
 * timing.c
 * 
 * This module tracks any timers set up by schedule_timer(). It
 * keeps all the currently active timers in a list; it informs the
 * front end of when the next timer is due to go off if that
 * changes; and, very importantly, it tracks the context pointers
 * passed to schedule_timer(), so that if a context is freed all
 * the timers associated with it can be immediately annulled.
 *
 *
 * The problem is that computer clocks aren't perfectly accurate.
 * The GETTICKCOUNT function returns a 32bit number that normally
 * increases by about 1000 every second. On windows this uses the PC's
 * interrupt timer and so is only accurate to around 20ppm.  On unix it's
 * a value that's calculated from the current UTC time and so is in theory
 * accurate in the long term but may jitter and jump in the short term.
 *
 * What PuTTY needs from these timers is simply a way of delaying the
 * calling of a function for a little while, if it's occasionally called a
 * little early or late that's not a problem. So to protect against clock
 * jumps schedule_timer records the time that it was called in the timer
 * structure. With this information the run_timers function can see when
 * the current GETTICKCOUNT value is after the time the event should be
 * fired OR before the time it was set. In the latter case the clock must
 * have jumped, the former is (probably) just the normal passage of time.
 *
 */

#include <assert.h>
#include <stdio.h>

#include "putty.h"
#include "tree234.h"

struct timer {
    timer_fn_t fn;
    void *ctx;
    unsigned long now;
    unsigned long when_set;
};

static tree234 *timers = NULL;
static tree234 *timer_contexts = NULL;
static unsigned long now = 0L;

static int compare_timers(void *av, void *bv)
{
    struct timer *a = (struct timer *)av;
    struct timer *b = (struct timer *)bv;
    long at = a->now - now;
    long bt = b->now - now;

    if (at < bt)
	return -1;
    else if (at > bt)
	return +1;

    /*
     * Failing that, compare on the other two fields, just so that
     * we don't get unwanted equality.
     */
#if defined(__LCC__) || defined(__clang__)
    /* lcc won't let us compare function pointers. Legal, but annoying. */
    {
	int c = memcmp(&a->fn, &b->fn, sizeof(a->fn));
	if (c)
	    return c;
    }
#else    
    if (a->fn < b->fn)
	return -1;
    else if (a->fn > b->fn)
	return +1;
#endif

    if (a->ctx < b->ctx)
	return -1;
    else if (a->ctx > b->ctx)
	return +1;

    /*
     * Failing _that_, the two entries genuinely are equal, and we
     * never have a need to store them separately in the tree.
     */
    return 0;
}

static int compare_timer_contexts(void *av, void *bv)
{
    char *a = (char *)av;
    char *b = (char *)bv;
    if (a < b)
	return -1;
    else if (a > b)
	return +1;
    return 0;
}

static void init_timers(void)
{
    if (!timers) {
	timers = newtree234(compare_timers);
	timer_contexts = newtree234(compare_timer_contexts);
	now = GETTICKCOUNT();
    }
}

unsigned long schedule_timer(int ticks, timer_fn_t fn, void *ctx)
{
    unsigned long when;
    struct timer *t, *first;

    init_timers();

    now = GETTICKCOUNT();
    when = ticks + now;

    /*
     * Just in case our various defences against timing skew fail
     * us: if we try to schedule a timer that's already in the
     * past, we instead schedule it for the immediate future.
     */
    if (when - now <= 0)
	when = now + 1;

    t = snew(struct timer);
    t->fn = fn;
    t->ctx = ctx;
    t->now = when;
    t->when_set = now;

    if (t != add234(timers, t)) {
	sfree(t);		       /* identical timer already exists */
    } else {
	add234(timer_contexts, t->ctx);/* don't care if this fails */
    }

    first = (struct timer *)index234(timers, 0);
    if (first == t) {
	/*
	 * This timer is the very first on the list, so we must
	 * notify the front end.
	 */
	timer_change_notify(first->now);
    }

    return when;
}

/*
 * Call to run any timers whose time has reached the present.
 * Returns the time (in ticks) expected until the next timer after
 * that triggers.
 */
int run_timers(unsigned long anow, unsigned long *next)
{
    struct timer *first;

    init_timers();

    now = GETTICKCOUNT();

    while (1) {
	first = (struct timer *)index234(timers, 0);

	if (!first)
	    return FALSE;	       /* no timers remaining */

	if (find234(timer_contexts, first->ctx, NULL) == NULL) {
	    /*
	     * This timer belongs to a context that has been
	     * expired. Delete it without running.
	     */
	    delpos234(timers, 0);
	    sfree(first);
	} else if (now - (first->when_set - 10) >
		   first->now - (first->when_set - 10)) {
	    /*
	     * This timer is active and has reached its running
	     * time. Run it.
	     */
	    delpos234(timers, 0);
	    first->fn(first->ctx, first->now);
	    sfree(first);
	} else {
	    /*
	     * This is the first still-active timer that is in the
	     * future. Return how long it has yet to go.
	     */
	    *next = first->now;
	    return TRUE;
	}
    }
}

/*
 * Call to expire all timers associated with a given context.
 */
void expire_timer_context(void *ctx)
{
    init_timers();

    /*
     * We don't bother to check the return value; if the context
     * already wasn't in the tree (presumably because no timers
     * ever actually got scheduled for it) then that's fine and we
     * simply don't need to do anything.
     */
    del234(timer_contexts, ctx);
}
