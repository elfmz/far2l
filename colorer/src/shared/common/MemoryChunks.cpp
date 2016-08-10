#include<common/Vector.h>
#include<memory.h>
#include<stdio.h>
#include<time.h>

#include<common/MemoryChunks.h>

size_t total_req = 0;
int new_calls = 0;
int free_calls = 0;

extern "C" {
  size_t get_total_req(){ return total_req; }
  int get_new_calls(){ return new_calls; }
  int get_free_calls(){ return free_calls; }
}

/**
  @ingroup common @{
*/
#if COLORER_FEATURE_USE_DL_MALLOC
extern "C"{
  void *dlmalloc(size_t size);
  void dlfree(void *ptr);
}

void *operator new(size_t size){

#if MEMORY_PROFILE
  total_req += size;
  new_calls++;
#endif
  return dlmalloc(size);
}

void operator delete(void *ptr){
#if MEMORY_PROFILE
  free_calls++;
#endif
  dlfree(ptr);
  return;
}

void *operator new[](size_t size){
#if MEMORY_PROFILE
  total_req += size;
  new_calls++;
#endif
  return dlmalloc(size);
}

void operator delete[](void *ptr){
#if MEMORY_PROFILE
  free_calls++;
#endif
  dlfree(ptr);
  return;
};
#endif

/**
  List of currently allocated memory chunks with size CHUNK_SIZE
*/
static Vector<byte*> *chunks = null;

/**
  Pointer to the last allocated chunk
*/
static byte *currentChunk = null;

/**
  Currently used size of the last allocated chunk
*/
static int currentChunkAlloc = 0;

/**
  Number of allocation instances.
*/
static int allocCount = 0;

/**
  Allocates @c size number of bytes and returns valid pointer.
  @param size Requested number of bytes.
  @throw Exception If no more memory, Exception is thrown.
*/
void *chunk_alloc(size_t size){
  if (size >= CHUNK_SIZE+4) throw Exception(DString("Too big memory request"));
  /* Init static - cygwin problems workaround */
  if (chunks == null){
    chunks = new Vector<byte*>;
  };

  if (chunks->size() == 0){
    currentChunk = new byte[CHUNK_SIZE];
    chunks->addElement(currentChunk);
    currentChunkAlloc = 0;
  };
  size = ((size-1) | 0x3) + 1; // 4-byte aling
  if (currentChunkAlloc+size > CHUNK_SIZE){
    currentChunk = new byte[CHUNK_SIZE];
    chunks->addElement(currentChunk);
    currentChunkAlloc = 0;
  };
  void *retVal = (void*)(currentChunk+currentChunkAlloc);
  currentChunkAlloc += (int)size;
  allocCount++;
  //printf("ca:%d - %db, all=%dKb\n", allocCount, size, ((chunks->size()-1)*CHUNK_SIZE+currentChunkAlloc)/1024);
  //printf("calloc\t%d\n", clock());
  return retVal;
};

/**
  Deallocates previously allocated memory.
  @param ptr Pointer, returned by @c chunk_alloc call.
*/
void chunk_free(void *ptr){
  if (ptr == null) return;
  //printf("cfree\t%d\n", clock());
  allocCount--;
  if (allocCount == 0){
    for(int idx = 0; idx < chunks->size(); idx++){
      delete[] chunks->elementAt(idx);
    };
    chunks->setSize(0);
  };
//  printf("cf:%d, ", allocCount);
};

/**
  @}
*/


/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
