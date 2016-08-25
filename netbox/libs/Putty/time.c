/*
 * Portable implementation of ltime() for any ISO-C platform where
 * time_t behaves. (In practice, we've found that platforms such as
 * Windows and Mac have needed their own specialised implementations.)
 */

#include <time.h>
#include <assert.h>

struct tm ltime(void)
{
    time_t t;
    time(&t);
    assert (t != ((time_t)-1));
    return *localtime(&t);
}
