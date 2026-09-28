#ifndef COLORER_MEMORYFILE_H
#define COLORER_MEMORYFILE_H

#include <minizip/unzip.h>
#include <vector>
#include "colorer/Common.h"

typedef struct
{
  const unsigned char* stream;
  int length;
  int pointer;
  int error;
} MemoryFile;

std::unique_ptr<std::vector<byte>> unzip(const byte* src, int size, const UnicodeString& path_in_zip);
voidpf ZCALLBACK mem_open_file_func(voidpf opaque, const char* filename, int mode);
uLong ZCALLBACK mem_read_file_func(voidpf opaque, voidpf stream, void* buf, uLong size);
uLong ZCALLBACK mem_write_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size);
long ZCALLBACK mem_tell_file_func(voidpf opaque, voidpf stream);
long ZCALLBACK mem_seek_file_func(voidpf opaque, voidpf stream, uLong offset, int origin);
int ZCALLBACK mem_close_file_func(voidpf opaque, voidpf stream);
int ZCALLBACK mem_error_file_func(voidpf opaque, voidpf stream);
void fill_mem_filefunc(zlib_filefunc_def* pzlib_filefunc_def, MemoryFile* source);

#endif
