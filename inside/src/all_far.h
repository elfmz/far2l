#pragma once
#include <windows.h>
#include <sudo.h>
#include <fcntl.h>
#include <farplug-mb.h>
using namespace oldfar;
#include <farcolor.h>
#include <farkeys.h>
#include "ItemList.h"

#define IS_BIT( val,num )     IS_FLAG(((DWORD)(val)),1UL<<(num)))
#define IS_FLAG( val,flag )   (((val)&(flag))==(flag))
#define SET_FLAG( val,flag )  (val |= (flag))
#define CLR_FLAG( val,flag )  (val &= ~(flag))
#define SWITCH_FLAG( f,v )    do{ if (IS_FLAG(f,v)) CLR_FLAG(f,v); else SET_FLAG(f,v); }while(0)


#define IS_SILENT(v)               ( ((v) & (OPM_FIND|OPM_VIEW|OPM_EDIT)) != 0 )
