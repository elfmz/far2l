#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <utils.h>
#include <ScopeHelpers.h>

unsigned long CRC32 (
		unsigned long crc,
		const char *buf,
		unsigned int len
		);

#define _tchartodigit(c)    ((c) >= '0' && (c) <= '9' ? (c) - '0' : -1)

void TrimEnd (char *lpStr);
void TrimStart (char *lpStr);
void Trim (char *lpStr);
void strmove(char *dst, const char *src);

int OpenInputFile(const char *path);
int CreateOutputFile(const char *path);
size_t QueryFileSize(int fd);
