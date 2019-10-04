#pragma once
#include <sys/types.h>
#include <string>
#include <vector>

int TTYReviveMe(int std_in, int std_out, bool &far2l_tty, int kickass, const std::string &info);

//////////////////////////////////////////////

struct TTYRevivableInstance
{
	std::string info;
	pid_t pid;
};

void TTYRevivableEnum(std::vector<TTYRevivableInstance> &instances);

int TTYReviveIt(pid_t pid, int std_in, int std_out, bool far2l_tty);
