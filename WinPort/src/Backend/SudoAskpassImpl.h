#pragma once
#include <string>
#include "../sudo/sudo_askpass_ipc.h"

class SudoAskpassImpl : public ISudoAskpass
{
protected:
	virtual bool OnSudoAskPassword(const std::string &title, const std::string &text, std::string &password);
	virtual bool OnSudoConfirm(const std::string &title, const std::string &text);
};
