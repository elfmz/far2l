/*
plist.cpp

Список процессов (Ctrl-W)
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "plist.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "message.hpp"
#include "config.hpp"
#include "interf.hpp"
#include "execute.hpp"
#include "dirmix.hpp"
#include "manager.hpp"
#include "fileowner.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <vector>
#include <string>

#if defined(__APPLE__)
#include <libproc.h>
#endif

#ifdef __linux__
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__HAIKU__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>     // FreeBSD: kinfo_proc
#endif

struct FarPidInfo
{
	std::wstring text;
	std::string name;
	int pid;
	unsigned long rss;
	unsigned long cpu_ticks;
};

static void enumerateProcesses(std::vector<FarPidInfo>& v) 
{
	v.clear();

#ifdef __linux__
	struct dirent *entry;

	DIR *d = opendir("/proc");
	if (!d) return;

	const unsigned long page_kb = sysconf(_SC_PAGESIZE) / 1024;
	FARString text;
	std::string path, proc_comm, proc_cmdline, proc_stat, proc_status, uid_name;
	std::vector<std::string> parts;
	while ((entry = readdir(d)) != NULL) {
		if (ClassifyNumberStr(entry->d_name) != NK_NUMBER_DEC)
			continue;

		proc_cmdline.clear();

		ReadWholeFile(path.assign("/proc/").append(entry->d_name).append("/cmdline").c_str(), proc_cmdline);
		if (proc_cmdline.empty())
			continue; // likely kernel task

		uid_name.clear();
		proc_comm.clear();
		proc_stat.clear();
		proc_status.clear();

		ReadWholeFile(path.assign("/proc/").append(entry->d_name).append("/stat").c_str(), proc_stat);
		ReadWholeFile(path.assign("/proc/").append(entry->d_name).append("/status").c_str(), proc_status);
		ReadWholeFile(path.assign("/proc/").append(entry->d_name).append("/comm").c_str(), proc_comm);
		StrTrimRight(proc_comm, "\n");

		parts.clear();
		StrExplode(parts, proc_status, "\r\n");
		for (const auto &part : parts) {
			if (StrStartsFrom(part, "Uid:")) {
				unsigned long ruid{}, euid{}, suid{}, fsuid{};
				sscanf(part.c_str() + 4, "%ld %ld %ld %ld", &ruid, &euid, &suid, &fsuid);
				const char *psz = OwnerNameByID(ruid);
				if (psz) uid_name = psz;
			}
		}

		/*
		 * /proc/<pid>/stat format:
		 * pid (comm) state ... utime stime ... rss ...
		 * we know comm already so fill it with - to deal with apps that has '(' or ')' as part of it
		 */
		size_t e1 = proc_stat.find('('), e2 = proc_stat.rfind(')');
		if (e1 != std::string::npos && e2 != std::string::npos && e2 > e1) {
			std::fill(proc_stat.begin() + e1 + 1, proc_stat.begin() + e2, '-');
		}

		parts.clear();
		StrExplode(parts, proc_stat, " ");

		unsigned long utime  = parts.size() > 13 ? DecToULong(parts[13].c_str(), parts[13].size()) : 0;
		unsigned long stime  = parts.size() > 14 ? DecToULong(parts[14].c_str(), parts[14].size()) : 0;
		unsigned long rss_kb = parts.size() > 23 ? DecToULong(parts[23].c_str(), parts[23].size()) * page_kb : 0;

		auto pid = atoi(entry->d_name);

/*		printf("PID: %d\n", pid);
		printf("Cmd: %s", proc_cmdline.c_str());
		printf("CPU ticks: %ld (user) + %ld (system)\n", utime, stime);
		printf("RSS: %ld KB (%s)\n", rss_kb, parts[24].c_str());
		printf("----\n");*/

		unsigned long total_time = utime + stime;
		const char* cpuLoad = "?";

		if (total_time < 100)		cpuLoad = "idle";
		else if (total_time < 1000)  cpuLoad = "low";
		else if (total_time < 10000) cpuLoad = "medium";
		else						 cpuLoad = "high";

		text.Format(L"%8d %lc %-12.12s %lc %-16.16s %lc %-40.40s %lc %6s %lc %'8ld Mb", 
			pid, BoxSymbols[BS_V1], uid_name.c_str(), BoxSymbols[BS_V1], 
			proc_comm.c_str(), BoxSymbols[BS_V1], proc_cmdline.c_str(), BoxSymbols[BS_V1], 
			cpuLoad, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ text.GetWide(), proc_cmdline, pid, rss_kb, total_time });
	}
	closedir(d);

#endif
#if defined(__APPLE__)
	pid_t pids[40960];
	int count = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));

	for (int i = 0; i < count; i++) {
		pid_t pid = pids[i];
		if (pid <= 0) continue;

		// ---- Get basic BSD info (name, etc.) ----
		struct proc_bsdinfo bsd;
		int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsd, sizeof(bsd));
		if (ret <= 0) continue;

		struct proc_taskinfo task;
		ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task, sizeof(task));
		if (ret <= 0) continue;

		// CPU time (user + system) in nanoseconds + RSS
		unsigned long total_time = task.pti_total_user + task.pti_total_system;
		double cpu_seconds = (task.pti_total_user + task.pti_total_system) / 1e9;
		unsigned long rss_kb = task.pti_resident_size / 1024;

		/*
		printf("PID: %d\n", pid);
		printf("Name: %s\n", bsd.pbi_name);
		printf("CPU time: %.2f s\n", cpu_seconds);
		printf("RSS: %lu KB\n", rss_kb);
		printf("----\n");
		*/

		FARString strStr;
		strStr.Format(L"%8d %lc %-40.40s %lc %6.4lf %lc %8ld Mb", pid, BoxSymbols[BS_V1], bsd.pbi_name, BoxSymbols[BS_V1], cpu_seconds, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ strStr.GetWide(), bsd.pbi_name, pid, rss_kb, total_time });
	}

#endif
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__HAIKU__)
	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	struct kinfo_proc *procs = NULL;
	size_t len = 0;

	// First call: get required buffer size
	if (sysctl(mib, 4, NULL, &len, NULL, 0) < 0) {
		perror("sysctl size");
		return;
	}

	procs = malloc(len);
	if (!procs) return;

	// Second call: retrieve process list
	if (sysctl(mib, 4, procs, &len, NULL, 0) < 0) {
		perror("sysctl data");
		free(procs);
		return;
	}

	int count = len / sizeof(struct kinfo_proc);

	for (int i = 0; i < count; i++) {
		struct kinfo_proc *p = &procs[i];

		pid_t pid = p->ki_pid;
		const char *name = p->ki_comm;

		// CPU time (user + system) in microseconds
		unsigned long total_time = p->ki_rusage.ru_utime.tv_sec +
			 p->ki_rusage.ru_stime.tv_sec;
		double cpu_seconds =
			(p->ki_rusage.ru_utime.tv_sec +
			 p->ki_rusage.ru_stime.tv_sec) +
			(p->ki_rusage.ru_utime.tv_usec +
			 p->ki_rusage.ru_stime.tv_usec) / 1e6;

		// Resident memory size (RSS)
		unsigned long rss_kb = p->ki_rssize * getpagesize() / 1024;

		/*
		printf("PID: %d\n", pid);
		printf("Name: %s\n", name);
		printf("CPU time: %.2f s\n", cpu_seconds);
		printf("RSS: %lu KB\n", rss_kb);
		printf("----\n");
		*/

		FARString strStr;
		strStr.Format(L"%8d %lc %-40.40s %lc %6.4lf %lc %6ld Mb", pid, BoxSymbols[BS_V1], name, BoxSymbols[BS_V1], cpu_seconds, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ strStr.GetWide(), name, pid, rss_kb, total_time });
	}

	free(procs);
#endif
}

void ShowProcessList()
{
	MenuDataEx dummy; // will refresh immediately
	VMenu ProcList(Msg::ProcessListTitle, &dummy, 1, ScrY - 4);

	ProcList.SetPosition(-1, -1, 0, 0);
	ProcList.SetFlags(VMENU_WRAPMODE | VMENU_NOTCHANGE);
	ProcList.SetBottomTitle(Msg::ProcessListBottom);
	ProcList.ClearDone();

	ProcList.Show();
	ProcList.SetRegularIdle(true);
	int sort_key = 'i';
	auto last_refresh = 0;
	bool refresh = true;

	std::vector<FarPidInfo> v;

	while (!ProcList.Done()) {
		if (refresh || (GetProcessUptimeMSec() - last_refresh) >= 1000) {
			int selected_pos = ProcList.GetSelectPos();
			int selected_pid = selected_pos < (int)v.size() ? v[selected_pos].pid : -1;
			ProcList.Hide();
			ProcList.DeleteItems();

			ProcList.SetPosition(-1,-1,0,0);

			enumerateProcesses(v);

			if (sort_key == 't' || sort_key == 'T')
				std::sort(v.begin(), v.end(),
					[sort_key](const FarPidInfo& a, const FarPidInfo& b) {
						return sort_key == 'T' ? b.name < a.name : a.name < b.name;
					});
			else if (sort_key == 'i' || sort_key == 'I')
				std::sort(v.begin(), v.end(),
					[sort_key](const FarPidInfo& a, const FarPidInfo& b) {
						return sort_key == 'i' ? a.pid < b.pid : b.pid < a.pid;
					});
			else if (sort_key == 'p' || sort_key == 'P')
				std::sort(v.begin(), v.end(),
					[sort_key](const FarPidInfo& a, const FarPidInfo& b) {
						return sort_key == 'P' ? b.cpu_ticks < a.cpu_ticks : a.cpu_ticks < b.cpu_ticks;
					});
			else if (sort_key == 'm' || sort_key == 'M')
				std::sort(v.begin(), v.end(),
					[sort_key](const FarPidInfo& a, const FarPidInfo& b) {
						return sort_key == 'M' ? b.rss < a.rss : a.rss < b.rss;
					});

			for (const auto &vj : v) {
				MenuItemEx item;
				item.strName = vj.text.c_str();
				item.AccelKey = 0;
				if (vj.pid == selected_pid) {
					item.Flags = LIF_SELECTED;
					selected_pos = -1;
				}
				ProcList.AddItem(&item);
			}
			if (selected_pos != -1) {
				selected_pos = std::min(selected_pos, ProcList.GetItemCount() - 1);
				if (selected_pos >= 0) {
					ProcList.SetSelectPos(selected_pos, -1);
				}
			}

			ProcList.Show();
			last_refresh = GetProcessUptimeMSec();
			refresh = false;
		}
		FarKey key = ProcList.ReadInput();
		switch (key) {
		case KEY_F1:
			Help::Present(L"TaskList");
			break;
		case 't': case 'T':
		case 'i': case 'I':
		case 'p': case 'P':
		case 'm': case 'M':
			sort_key = key;
			refresh = true;
			break;
		case KEY_CTRLR:
			refresh = true;
			break;
		case KEY_NUMDEL:
		case KEY_DEL:
			if (!kill(v[ProcList.GetSelectPos()].pid, SIGTERM)) {
				for (int i = 0; i < 300; ++i, usleep(10000)) { // wait up to  seconds for process exit
					if (kill(v[ProcList.GetSelectPos()].pid, 0) != 0) {
						ProcList.DeleteItem(ProcList.GetSelectPos());
						break;
					}
				}
			}
			break;
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDEL:
			if(!kill(v[ProcList.GetSelectPos()].pid, SIGKILL)) {
				ProcList.DeleteItem(ProcList.GetSelectPos()); // it had no chance to survive
			}
			break;
		default:
			ProcList.ProcessInput();
			break;
		}
	}
}

void ShowProcessList_OldPs()
{
	farExecuteA(GetMyScriptQuoted("ps.sh").c_str(), 0);
	if (FrameManager) {
		auto *current_frame = FrameManager->GetCurrentFrame();
		if (current_frame) {
			FrameManager->RefreshFrame(current_frame);
		}
	}
/*
	for (int i = FrameManager->GetFrameCount(); i > 0; --i) {
		FrameManager->RefreshFrame(i - 1);
	}
*/
}
