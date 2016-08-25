/*
 * Stub implementation of the printing interface for PuTTY, for the
 * benefit of non-printing terminal applications.
 */

#include <assert.h>
#include <stdio.h>
#include "putty.h"

struct printer_job_tag {
    int dummy;
};

printer_job *printer_start_job(char *printer)
{
    return NULL;
}

void printer_job_data(printer_job *pj, void *data, int len)
{
}

void printer_finish_job(printer_job *pj)
{
}

printer_enum *printer_start_enum(int *nprinters_ptr)
{
    *nprinters_ptr = 0;
    return NULL;
}
char *printer_get_name(printer_enum *pe, int i)
{
    return NULL;
}
void printer_finish_enum(printer_enum *pe)
{
}
