#pragma once
#include "../UI/Defs.h"
#include "../SiteConnection.h"

struct ProgressStateUpdate : std::unique_lock<std::mutex>
{
	ProgressStateUpdate(ProgressState &state);
};


class ProgressStateIOUpdater : public SiteConnection::IOStatusCallback
{
	ProgressState &_state;

protected:
	virtual bool OnIOStatus(unsigned long long transferred);

public:
	ProgressStateIOUpdater(ProgressState &state);
};
