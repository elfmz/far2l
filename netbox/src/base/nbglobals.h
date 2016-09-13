#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <new>

#ifdef USE_DLMALLOC
#include "../../libs/dlmalloc/malloc-2.8.6.h"

#define nb_malloc(size) dlcalloc(1, size)
#define nb_calloc(count, size) dlcalloc(count, size)
#define nb_realloc(ptr, size) dlrealloc(ptr, size)

#if defined(__cplusplus)
#define nb_free(ptr) dlfree(reinterpret_cast<void *>(ptr))
#else
#define nb_free(ptr) dlfree((void *)(ptr))
#endif // if defined(__cplusplus)

#else

#define nb_malloc(size) malloc(size)
#define nb_calloc(count, size) calloc(count, size)
#define nb_realloc(ptr, size) realloc(ptr, size)

#if defined(__cplusplus)
#define nb_free(ptr) free(reinterpret_cast<void *>(ptr))
#else
#define nb_free(ptr) free((void *)(ptr))
#endif // if defined(__cplusplus)

#endif // ifdef USE_DLMALLOC

#if defined(_MSC_VER)
#if (_MSC_VER < 1900)

#ifndef noexcept
#define noexcept throw()
#endif

#endif
#endif

#ifndef STRICT
#define STRICT 1
#endif

#if defined(__cplusplus)

inline void * operator_new(size_t size)
{
  void * p = nb_malloc(size);
  /*if (!p)
  {
    static std::bad_alloc badalloc;
    throw badalloc;
  }*/
  return p;
}

inline void operator_delete(void * p)
{
  nb_free(p);
}

#endif // if defined(__cplusplus)

#ifdef USE_DLMALLOC
/// custom memory allocation
#define DEF_CUSTOM_MEM_ALLOCATION_IMPL            \
  public:                                         \
  void * operator new(size_t sz)                  \
  {                                               \
    return operator_new(sz);                      \
  }                                               \
  void operator delete(void * p, size_t)          \
  {                                               \
    operator_delete(p);                           \
  }                                               \
  void * operator new[](size_t sz)                \
  {                                               \
    return operator_new(sz);                      \
  }                                               \
  void operator delete[](void * p, size_t)        \
  {                                               \
    operator_delete(p);                           \
  }                                               \
  void * operator new(size_t, void * p)           \
  {                                               \
    return p;                                     \
  }                                               \
  void operator delete(void *, void *)            \
  {                                               \
  }                                               \
  void * operator new[](size_t, void * p)         \
  {                                               \
    return p;                                     \
  }                                               \
  void operator delete[](void *, void *)          \
  {                                               \
  }

#ifdef _DEBUG
#define CUSTOM_MEM_ALLOCATION_IMPL DEF_CUSTOM_MEM_ALLOCATION_IMPL \
  void * operator new(size_t sz, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    return operator_new(sz); \
  } \
  void * operator new[](size_t sz, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    return operator_new(sz); \
  } \
  void operator delete(void * p, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    operator_delete(p); \
  } \
  void operator delete[](void * p, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    operator_delete(p); \
  }
#else
#define CUSTOM_MEM_ALLOCATION_IMPL DEF_CUSTOM_MEM_ALLOCATION_IMPL
#endif // ifdef _DEBUG

#else
#define CUSTOM_MEM_ALLOCATION_IMPL
#endif // ifdef USE_DLMALLOC

#if defined(__cplusplus)

namespace nballoc
{
  inline void destruct(char *) {}
  inline void destruct(wchar_t *) {}
  template <typename T>
  inline void destruct(T * t) { t->~T(); }
} // namespace nballoc

template <typename T> struct custom_nballocator_t;

template <> struct custom_nballocator_t<void>
{
public:
  typedef void * pointer;
  typedef const void * const_pointer;
  // reference to void members are impossible.
  typedef void value_type;
  template <class U>
    struct rebind { typedef custom_nballocator_t<U> other; };
};

template <typename T>
struct custom_nballocator_t
{
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T * pointer;
  typedef const T * const_pointer;
  typedef T & reference;
  typedef const T & const_reference;
  typedef T value_type;

  template <class U> struct rebind { typedef custom_nballocator_t<U> other; };
  inline custom_nballocator_t() noexcept {}
  inline custom_nballocator_t(const custom_nballocator_t &) noexcept {}

  template <class U> custom_nballocator_t(const custom_nballocator_t<U> &) noexcept {}

  ~custom_nballocator_t() noexcept {}

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }

  pointer allocate(size_type s, void const * = 0)
  {
    if (0 == s)
      return nullptr;
    pointer temp = reinterpret_cast<pointer>(nb_malloc(s * sizeof(T)));
#if !defined(__MINGW32__)
    if (temp == nullptr)
      throw std::bad_alloc();
#endif
    return temp;
  }

  void deallocate(pointer p, size_type)
  {
    nb_free(p);
  }

  size_type max_size() const noexcept
  {
    // return std::numeric_limits<size_t>::max() / sizeof(T);
    return size_t(-1) / sizeof(T);
  }

  void construct(pointer p, const T & val)
  {
    new(reinterpret_cast<void *>(p)) T(val);
  }

  void destroy(pointer p)
  {
    nballoc::destruct(p);
  }
};

template <typename T, typename U>
inline bool operator==(const custom_nballocator_t<T> &, const custom_nballocator_t<U> &)
{
  return true;
}

template <typename T, typename U>
inline bool operator!=(const custom_nballocator_t<T> &, const custom_nballocator_t<U> &)
{
  return false;
}

#endif // if defined(__cplusplus)
