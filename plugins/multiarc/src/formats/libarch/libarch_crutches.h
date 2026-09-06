#pragma once
#include "libarch_utils.h"


const char *LibArch_EntryPathname(struct archive_entry *e);
uint64_t LibArch_UnpackedSizeOfGZ(LibArchOpenRead *arc);
uint64_t LibArch_UnpackedSizeOfXZ(LibArchOpenRead *arc);
