#pragma once

typedef struct _CROptions{
  int   Enabled;
  int   Color;
  int   CenterColor;
  int   RulerColor;
  int   TempShow;
  int   LockShow;
  int   Flags;
} CROptions;

#ifdef __cplusplus
extern "C" {
#endif
extern void RestoreConfig(CROptions *Options);
extern void SaveConfig(const CROptions *Options);
#ifdef __cplusplus
} // extern "C"
#endif
