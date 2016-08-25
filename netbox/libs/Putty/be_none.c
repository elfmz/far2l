/*
 * Linking module for programs that do not support selection of backend
 * (such as pterm).
 */

#include <stdio.h>
#include "putty.h"

Backend *backends[] = {
    NULL
};
