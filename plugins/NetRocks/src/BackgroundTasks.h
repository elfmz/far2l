#pragma once
#include <string>
#include <memory>
#include <vector>

enum BackgroundTaskStatus
{
	BTS_ACTIVE,
	BTS_PAUSED,
	BTS_COMPLETE,
	BTS_ABORTED
};

struct IBackgroundTask
{
	virtual ~IBackgroundTask() {};

	virtual BackgroundTaskStatus GetStatus() = 0;
	virtual std::string GetInformation() = 0;
	virtual std::string GetDestination(bool &directory) = 0;
	virtual void Show() = 0;
	virtual void Abort() = 0;
};

struct BackgroundTaskInfo
{
	unsigned long id;
	BackgroundTaskStatus status;
	std::string information;
};

typedef std::vector<BackgroundTaskInfo> BackgroundTasksInfo;

void AddBackgroundTask(std::shared_ptr<IBackgroundTask> &task);
size_t CountOfAllBackgroundTasks();
size_t CountOfPendingBackgroundTasks();
void GetBackgroundTasksInfo(BackgroundTasksInfo &info);
void ShowBackgroundTask(unsigned long id);
void AbortBackgroundTask(unsigned long id);
