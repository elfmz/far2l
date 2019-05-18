#include <map>
#include <mutex>
#include "BackgroundTasks.h"

static std::map<unsigned long, std::shared_ptr<IBackgroundTask> > g_background_tasks;
static std::mutex g_background_tasks_mutex;
static unsigned long g_background_task_id = 0;

void AddBackgroundTask(std::shared_ptr<IBackgroundTask> &task)
{
	std::lock_guard<std::mutex> locker(g_background_tasks_mutex);
	++g_background_task_id;
	g_background_tasks[g_background_task_id] = task;
}

size_t CountOfAllBackgroundTasks()
{
	std::lock_guard<std::mutex> locker(g_background_tasks_mutex);
	return g_background_tasks.size();
}

size_t CountOfPendingBackgroundTasks()
{
	std::lock_guard<std::mutex> locker(g_background_tasks_mutex);
	size_t out = 0;
	for (auto &task : g_background_tasks) {
		if (task.second->GetStatus() == BTS_ACTIVE || task.second->GetStatus() == BTS_PAUSED) {
			++out;
		}
	}
	return out;
}

void GetBackgroundTasksInfo(BackgroundTasksInfo &info)
{
	std::lock_guard<std::mutex> locker(g_background_tasks_mutex);
	for (auto &it : g_background_tasks) {
		info.emplace_back(BackgroundTaskInfo
			{it.first, it.second->GetStatus(), it.second->GetInformation()});
	}
}

void ShowBackgroundTask(unsigned long id)
{
	std::lock_guard<std::mutex> locker(g_background_tasks_mutex);
	auto it = g_background_tasks.find(id);
	if ( it != g_background_tasks.end()) {
		it->second->Show();
		if (it->second->GetStatus() != BTS_ACTIVE && it->second->GetStatus() != BTS_PAUSED) {
			g_background_tasks.erase(it);
		}
	}
}

void AbortBackgroundTask(unsigned long id)
{
	std::lock_guard<std::mutex> locker(g_background_tasks_mutex);
	auto it = g_background_tasks.find(id);
	if ( it != g_background_tasks.end())
		it->second->Abort();
}

