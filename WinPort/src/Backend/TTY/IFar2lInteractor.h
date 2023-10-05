#pragma once
#include <StackSerializer.h>

struct IFar2lInteractor
{
	virtual bool Far2lInteract(StackSerializer &stk_ser, bool wait) = 0;
};
