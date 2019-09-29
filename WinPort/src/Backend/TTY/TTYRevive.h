#pragma once
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

bool TTYReviveIt(const TTYRevivableInstance &instance, int std_in, int std_out, bool far2l_tty);

