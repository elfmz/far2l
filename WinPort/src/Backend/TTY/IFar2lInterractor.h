#pragma once
#include <StackSerializer.h>

struct IFar2lInterractor
{
	virtual bool Far2lInterract(StackSerializer &stk_ser, bool wait) = 0;
};
