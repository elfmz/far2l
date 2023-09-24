#pragma once
#include <sys/types.h>

pid_t MakePTYAndFork(int &pty_master);
