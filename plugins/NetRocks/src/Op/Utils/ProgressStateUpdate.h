#pragma once
#include "../../UI/Defs.h"

struct ProgressStateUpdate : std::unique_lock<std::mutex>
{
	ProgressStateUpdate(ProgressState &state);
};

