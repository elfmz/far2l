#include <ConsoleInput.h>
#include "SudoAskpassImpl.h"

extern ConsoleInput g_winport_con_in;

bool SudoAskpassImpl::OnSudoAskPassword(const std::string &title, const std::string &text, std::string &password)
{
	ConsoleInputPriority cip(g_winport_con_in);
	return false;
}

bool SudoAskpassImpl::OnSudoConfirm(const std::string &title, const std::string &text)
{
	ConsoleInputPriority cip(g_winport_con_in);
	return false;
}
