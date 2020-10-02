#include <colorer/io/MemoryFile.h>
#include <colorer/Common.h>
#include <string.h>

voidpf ZCALLBACK mem_open_file_func (voidpf opaque, const char *filename, int mode)
{
  MemoryFile *mf = (MemoryFile*)opaque;
  mf->error = 0;
  mf->pointer = 0;
  return (voidpf)0x666888;
}

uLong ZCALLBACK mem_read_file_func (voidpf opaque, voidpf stream, void *buf, uLong size){

  MemoryFile *mf = (MemoryFile*)opaque;

  if (mf->pointer+(int)size > mf->length) size = mf->length - mf->pointer;
  memmove(buf, mf->stream+mf->pointer, size);
  mf->pointer += size;
  return size;
}


uLong ZCALLBACK mem_write_file_func (voidpf opaque, voidpf stream, const void *buf, uLong size)
{
  // we need this?
  //MemoryFile *mf = (MemoryFile*)opaque;
  return 0;
}

long ZCALLBACK mem_tell_file_func (voidpf opaque, voidpf stream)
{
  MemoryFile *mf = (MemoryFile*)opaque;
  return mf->pointer;
}

long ZCALLBACK mem_seek_file_func (voidpf opaque, voidpf stream, uLong offset, int origin)
{
  MemoryFile *mf = (MemoryFile*)opaque;
  int cpointer;

  switch (origin){
    case ZLIB_FILEFUNC_SEEK_CUR :
      cpointer = mf->pointer;
      cpointer += offset;
      if (cpointer > mf->length) return -1;
      mf->pointer = cpointer;
      break;
    case ZLIB_FILEFUNC_SEEK_END :
      cpointer = mf->length;
      cpointer += offset;
      if (cpointer > mf->length) return -1;
      mf->pointer = cpointer;
      break;
    case ZLIB_FILEFUNC_SEEK_SET :
      cpointer = 0;
      cpointer += offset;
      if (cpointer > mf->length) return -1;
      mf->pointer = cpointer;
      break;
    default:
      return -1;
  }
  return 0;
}

int ZCALLBACK mem_close_file_func (voidpf opaque, voidpf stream)
{
  return 0;
}

int ZCALLBACK mem_error_file_func (voidpf opaque, voidpf stream)
{
  MemoryFile *mf = (MemoryFile*)opaque;
  return mf->error;
}

void fill_mem_filefunc (zlib_filefunc_def*  pzlib_filefunc_def, MemoryFile *source)
{
  pzlib_filefunc_def->opaque = source;

  pzlib_filefunc_def->zopen_file = mem_open_file_func;
  pzlib_filefunc_def->zread_file = mem_read_file_func;
  pzlib_filefunc_def->zwrite_file = mem_write_file_func;
  pzlib_filefunc_def->ztell_file = mem_tell_file_func;
  pzlib_filefunc_def->zseek_file = mem_seek_file_func;
  pzlib_filefunc_def->zclose_file = mem_close_file_func;
  pzlib_filefunc_def->zerror_file = mem_error_file_func;
}


