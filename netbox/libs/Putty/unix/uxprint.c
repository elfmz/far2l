/*
 * Printing interface for PuTTY.
 */

#include <assert.h>
#include <stdio.h>
#include "putty.h"

struct printer_job_tag {
    FILE *fp;
};

printer_job *printer_start_job(char *printer)
{
    printer_job *ret = snew(printer_job);
    /*
     * On Unix, we treat the printer string as the name of a
     * command to pipe to - typically lpr, of course.
     */
    ret->fp = popen(printer, "w");
    if (!ret->fp) {
	sfree(ret);
	ret = NULL;
    }
    return ret;
}

void printer_job_data(printer_job *pj, void *data, int len)
{
    if (!pj)
	return;

    if (fwrite(data, 1, len, pj->fp) < len)
	/* ignore */;
}

void printer_finish_job(printer_job *pj)
{
    if (!pj)
	return;

    pclose(pj->fp);
    sfree(pj);
}

/*
 * There's no sensible way to enumerate printers under Unix, since
 * practically any valid Unix command is a valid printer :-) So
 * these are useless stub functions, and uxcfg.c will disable the
 * drop-down list in the printer configurer.
 */
printer_enum *printer_start_enum(int *nprinters_ptr) {
    *nprinters_ptr = 0;
    return NULL;
}
char *printer_get_name(printer_enum *pe, int i) { return NULL;
}
void printer_finish_enum(printer_enum *pe) { }
