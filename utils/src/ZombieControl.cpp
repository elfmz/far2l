#include <set>
#include <mutex>
#include <sys/wait.h>

class ZombieControl : std::set<pid_t>
{
	std::mutex _mutex;

public:
	void Pud(pid_t pid)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		insert(pid);
		for (iterator i = begin(); i!=end(); ) {
			int r;
			if (waitpid(*i, &r, WNOHANG)==*i)
				 i = erase(i);
			else
				++i;
		}
	}
} g_zombie_control;

void PutZombieUnderControl(pid_t pid)
{
	g_zombie_control.Pud(pid);
}
