#pragma once

#include <FastDelegate.h>
#include <FastDelegateBind.h>

#define DEFINE_CALLBACK_TYPE0(EVENT,  R) \
  typedef fastdelegate::FastDelegate0<R> EVENT
#define DEFINE_CALLBACK_TYPE1(EVENT,  R, T1) \
  typedef fastdelegate::FastDelegate1<R, T1> EVENT
#define DEFINE_CALLBACK_TYPE2(EVENT,  R, T1, T2) \
  typedef fastdelegate::FastDelegate2<R, T1, T2> EVENT
#define DEFINE_CALLBACK_TYPE3(EVENT,  R, T1, T2, T3) \
  typedef fastdelegate::FastDelegate3<R, T1, T2, T3> EVENT
#define DEFINE_CALLBACK_TYPE4(EVENT,  R, T1, T2, T3, T4) \
  typedef fastdelegate::FastDelegate4<R, T1, T2, T3, T4> EVENT
#define DEFINE_CALLBACK_TYPE5(EVENT,  R, T1, T2, T3, T4, T5) \
  typedef fastdelegate::FastDelegate5<R, T1, T2, T3, T4, T5> EVENT
#define DEFINE_CALLBACK_TYPE6(EVENT,  R, T1, T2, T3, T4, T5, T6) \
  typedef fastdelegate::FastDelegate6<R, T1, T2, T3, T4, T5, T6> EVENT
#define DEFINE_CALLBACK_TYPE7(EVENT,  R, T1, T2, T3, T4, T5, T6, T7) \
  typedef fastdelegate::FastDelegate7<R, T1, T2, T3, T4, T5, T6, T7> EVENT
#define DEFINE_CALLBACK_TYPE8(EVENT,  R, T1, T2, T3, T4, T5, T6, T7, T8) \
  typedef fastdelegate::FastDelegate8<R, T1, T2, T3, T4, T5, T6, T7, T8> EVENT

#define MAKE_CALLBACK(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT)

#define TShellExecuteInfoW _SHELLEXECUTEINFOW
#define TSHFileInfoW SHFILEINFOW
#ifndef __linux__
#define TVSFixedFileInfo VS_FIXEDFILEINFO
#else
#define TVSFixedFileInfo char
#endif

