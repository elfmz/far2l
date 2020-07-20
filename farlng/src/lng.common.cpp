#include "lng.common.h"
#include <stdarg.h>
#include <sys/stat.h>

void TrimEnd (char *lpStr)
{
	char *Ptr = lpStr+strlen(lpStr)-1;

	while ( Ptr>=lpStr && (*Ptr==' ' || *Ptr=='\t') )
		*Ptr = 0;
}

void TrimStart (char *lpStr)
{
	char *Ptr = lpStr;

	while ( *Ptr && (*Ptr==' ' || *Ptr=='\t') )
		Ptr++;

	memmove (lpStr, Ptr, strlen(Ptr)+1);
}

void Trim (char *lpStr)
{
	TrimEnd (lpStr);
	TrimStart (lpStr);
}

#define _tchartodigit(c)    ((c) >= '0' && (c) <= '9' ? (c) - '0' : -1)

unsigned long CRC32(
		unsigned long crc,
		const char *buf,
		unsigned int len
		)
{
	static unsigned long crc_table[256];

	if (!crc_table[1])
	{
		unsigned long c;
		int n, k;

		for (n = 0; n < 256; n++)
		{
			c = (unsigned long)n;
			for (k = 0; k < 8; k++) c = (c >> 1) ^ (c & 1 ? 0xedb88320L : 0);
				crc_table[n] = c;
		}
	}

	crc = crc ^ 0xffffffffL;
	while (len-- > 0) {
		crc = crc_table[(crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
	}

	return crc ^ 0xffffffffL;
}

void strmove(char *dst, const char *src)
{
	memmove(dst, src, strlen(src) + 1);
}

int OpenInputFile(const char *path)
{
	return open(path, O_RDONLY, 0644);
}

int CreateOutputFile(const char *path)
{
	return open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

size_t QueryFileSize(int fd)
{
	struct stat s{};
	if (fstat(fd, &s) == -1)
		return 0;

	return (size_t)s.st_size;
}

