#ifndef COLORER_PLATFORM_H
#define COLORER_PLATFORM_H

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef WIN32
#if (__cplusplus < 201402L)

// Define std::make_unique for pre-C++14
namespace std {

template <class T>
struct _Unique_if
{
  typedef unique_ptr<T> _Single_object;
};

template <class T>
struct _Unique_if<T[]>
{
  typedef unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N>
struct _Unique_if<T[N]>
{
  typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args&&... args)
{
  return unique_ptr<T>(new T(forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n)
{
  typedef typename remove_extent<T>::type U;
  return unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;
}  // namespace std

#endif  //__cplusplus < 201402L
#endif  // WIN32

#endif  // COLORER_PLATFORM_H
