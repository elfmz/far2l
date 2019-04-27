#pragma once
#include "../../UI/Defs.h"
#include "../../SiteConnection.h"

struct ProgressStateUpdate : std::unique_lock<std::mutex>
{
	ProgressStateUpdate(ProgressState &state);
};


class ProgressStateUpdaterCallback : public SiteConnection::IOStatusCallback, public SiteConnection::EnumStatusCallback
{
	ProgressState &_state_ref;

protected:
	virtual bool OnIOStatus(unsigned long long transferred);
	virtual bool OnEnumEntry();

public:
	ProgressStateUpdaterCallback(ProgressState &state);
};
