/*
 * notiming.c: stub version of timing API.
 * 
 * Used in any tool which needs a subsystem linked against the
 * timing API but doesn't want to actually provide timing. For
 * example, key generation tools need the random number generator,
 * but they don't want the hassle of calling noise_regular() at
 * regular intervals - and they don't _need_ it either, since they
 * have their own rigorous and different means of noise collection.
 */

#include "putty.h"

unsigned long schedule_timer(int ticks, timer_fn_t fn, void *ctx)
{
    return 0;
}

void expire_timer_context(void *ctx)
{
}
