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
#include <unordered_map>

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
	FARString text;
	std::string name;
	int pid;
	unsigned long rss;
	unsigned long long cpu_time; // in milliseconds!
	double cpu_usage; // initially set to -1, its calculated out of enumerateProcesses!
};

static const wchar_t CPU_LOAD_PLACEHOLDER[] = L"  ??.?%";

#define AUTOREFRESH_MSEC 1000

static void enumerateProcesses(std::vector<FarPidInfo>& v) 
{
	v.clear();

	FARString text;
#ifdef __linux__
	struct dirent *entry;

	DIR *d = opendir("/proc");
	if (!d) return;

	const unsigned long page_kb = sysconf(_SC_PAGESIZE) / 1024;
	const unsigned long clock_tick = sysconf(_SC_CLK_TCK);
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

		unsigned long long cpu_time = utime + stime;
		if (clock_tick) {
			cpu_time*= 1000;
			cpu_time/= clock_tick;
		}

		text.Format(L"%8d %lc %-12.12s %lc %-16.16s %lc %-40.40s %lc %ls %lc %'8ld Mb", 
			pid, BoxSymbols[BS_V1], uid_name.c_str(), BoxSymbols[BS_V1], 
			proc_comm.c_str(), BoxSymbols[BS_V1], proc_cmdline.c_str(), BoxSymbols[BS_V1], 
			CPU_LOAD_PLACEHOLDER, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ text, proc_cmdline, pid, rss_kb, cpu_time, -1 });
	}
	closedir(d);

#endif
#if defined(__APPLE__)
	auto bytes_count = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
	if (bytes_count <= 0)
		return;

	std::vector<pid_t> pids(128 + bytes_count / sizeof(pid_t));
	bytes_count = proc_listpids(PROC_ALL_PIDS, 0, pids.data(), pids.size() * sizeof(pid_t));
	if (bytes_count <= 0)
		return;

	pids.resize(bytes_count / sizeof(pid_t));
	for (const auto &pid : pids) {
		if (pid <= 0) continue;

		// ---- Get basic BSD info (name, etc.) ----
		struct proc_bsdinfo bsd;
		int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsd, sizeof(bsd));
		if (ret <= 0) continue;

		struct proc_taskinfo task{};
		ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task, sizeof(task));
		if (ret <= 0) continue;

		// CPU time (user + system) in nanoseconds + RSS
		unsigned long long cpu_time = task.pti_total_user + task.pti_total_system;
		cpu_time/= 1000000; // downconvert to wanted milliseconds
//		double cpu_seconds = (task.pti_total_user + task.pti_total_system) / 1e9;
		unsigned long rss_kb = task.pti_resident_size / 1024;

		/*
		printf("PID: %d\n", pid);
		printf("Name: %s\n", bsd.pbi_name);
		printf("CPU time: %.2f s\n", cpu_seconds);
		printf("RSS: %lu KB\n", rss_kb);
		printf("----\n");
		*/

		text.Format(L"%8d %lc %-40.40s %lc %ls %lc %8ld Mb", pid, BoxSymbols[BS_V1], bsd.pbi_name, BoxSymbols[BS_V1], CPU_LOAD_PLACEHOLDER, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ text, bsd.pbi_name, pid, rss_kb, cpu_time, -1 });
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

	FARString strStr;
	for (int i = 0; i < count; i++) {
		struct kinfo_proc *p = &procs[i];

		pid_t pid = p->ki_pid;
		const char *name = p->ki_comm;

		// CPU time (user + system) - convert to milliseconds
		unsigned long long cpu_time = p->ki_rusage.ru_utime.tv_sec + p->ki_rusage.ru_stime.tv_sec;
		cpu_time*= 1000;
		cpu_time+= (p->ki_rusage.ru_utime.tv_usec + p->ki_rusage.ru_stime.tv_usec) / 1000;

/*		double cpu_seconds =
			(p->ki_rusage.ru_utime.tv_sec +
			 p->ki_rusage.ru_stime.tv_sec) +
			(p->ki_rusage.ru_utime.tv_usec +
			 p->ki_rusage.ru_stime.tv_usec) / 1e6;*/

		// Resident memory size (RSS)
		unsigned long rss_kb = p->ki_rssize * getpagesize() / 1024;

		/*
		printf("PID: %d\n", pid);
		printf("Name: %s\n", name);
		printf("CPU time: %.2f s\n", cpu_seconds);
		printf("RSS: %lu KB\n", rss_kb);
		printf("----\n");
		*/

		text.Format(L"%8d %lc %-40.40s %lc %ls %lc %6ld Mb", pid, BoxSymbols[BS_V1], name, BoxSymbols[BS_V1], CPU_LOAD_PLACEHOLDER, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ text, name, pid, rss_kb, cpu_time, -1 });
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
	int sort_key = 'P';
	clock_t last_refresh = 0, schedule_refresh = 0;
	bool autorefresh = true;
	enum
	{
		AT_CHOSEN,
		AT_TOP,
		AT_BOTTOM,
	} keep{AT_TOP};

	struct TimeAndId
	{
		unsigned long long cpu_time{};
		unsigned int id{};
	};
	std::unordered_map<int, TimeAndId> pid2ti;
	std::vector<FarPidInfo> v;

	FARString str_usage;
	for (unsigned int loop_id = 1; !ProcList.Done(); ++loop_id) {
		const auto now = GetProcessUptimeMSec();
		if (last_refresh == 0 || (schedule_refresh && (now >= schedule_refresh || now < last_refresh))) {
			int selected_pos = ProcList.GetSelectPos();
			int selected_pid = selected_pos < (int)v.size() ? v[selected_pos].pid : getpid();
			ProcList.Hide();
			ProcList.DeleteItems();

			ProcList.SetPosition(-1,-1,0,0);

			enumerateProcesses(v);

			// estimate cpu usage
			for (auto &vj : v) {
				auto &pt = pid2ti[vj.pid];
				if (pt.id != 0 && vj.cpu_time >= pt.cpu_time) {
					unsigned long long cpu_time_delta = vj.cpu_time - pt.cpu_time;
					unsigned long long real_time_delta = (now > last_refresh) ? now - last_refresh : 1;
					vj.cpu_usage = double(cpu_time_delta * 100) / real_time_delta;
					if (vj.cpu_usage <= 9) {
						str_usage.Format(L"%.2f%%", vj.cpu_usage);
					} else if (vj.cpu_usage <= 99) {
						str_usage.Format(L"%.1f%%", vj.cpu_usage);
					} else {
						str_usage.Format(L"%d%%", int(vj.cpu_usage));
					}
					if (str_usage.GetLength() < ARRAYSIZE(CPU_LOAD_PLACEHOLDER) - 1) {
						str_usage.Insert(0, L' ', ARRAYSIZE(CPU_LOAD_PLACEHOLDER) - 1 - str_usage.GetLength());
					} else {
						str_usage.Truncate(ARRAYSIZE(CPU_LOAD_PLACEHOLDER) - 1);
					}
					ReplaceStrings(vj.text, CPU_LOAD_PLACEHOLDER, str_usage);
				}
				pt.id = loop_id;
				pt.cpu_time = vj.cpu_time;
			}
			// remove from pid2ti those entries that were not updated, meaning they terminated
			for (auto it = pid2ti.begin(); it != pid2ti.end(); ) {
				if (it->second.id != loop_id) {
					fprintf(stderr, "%s: pid %d terminated\n", __FUNCTION__, it->first);
					it = pid2ti.erase(it);
				} else {
					++it;
				}
			}

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
						return sort_key == 'P' ? b.cpu_usage < a.cpu_usage : a.cpu_usage < b.cpu_usage;
					});
			else if (sort_key == 'm' || sort_key == 'M')
				std::sort(v.begin(), v.end(),
					[sort_key](const FarPidInfo& a, const FarPidInfo& b) {
						return sort_key == 'M' ? b.rss < a.rss : a.rss < b.rss;
					});

			for (const auto &vj : v) {
				MenuItemEx item;
				item.strName = vj.text;
				item.AccelKey = 0;
				if (vj.pid == selected_pid && keep == AT_CHOSEN) {
					item.Flags = LIF_SELECTED;
					selected_pos = -1;
				}
				ProcList.AddItem(&item);
			}
			if (keep == AT_TOP) {
				ProcList.SetSelectPos(selected_pos, 0);
			} else if (keep == AT_BOTTOM) {
				ProcList.SetSelectPos(selected_pos, ProcList.GetItemCount() - 1);
			} else if (selected_pos != -1) {
				selected_pos = std::min(selected_pos, ProcList.GetItemCount() - 1);
				if (selected_pos >= 0) {
					ProcList.SetSelectPos(selected_pos, -1);
				}
			}

			ProcList.Show();
			last_refresh = GetProcessUptimeMSec();
			if (autorefresh) {
				schedule_refresh = last_refresh + AUTOREFRESH_MSEC;
			}
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
			schedule_refresh = GetProcessUptimeMSec();
			break;
		case KEY_CTRLR:
			autorefresh = !autorefresh;
			schedule_refresh = autorefresh ? GetProcessUptimeMSec() : 0;
			break;
		case KEY_NUMDEL:
		case KEY_DEL:
			if (!kill(v[ProcList.GetSelectPos()].pid, SIGTERM)) {
				for (int i = 0; i < 100; ++i, usleep(10000)) { // wait up to second for process exit
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
		case KEY_HOME:
			keep = AT_TOP;
			ProcList.ProcessInput();
			break;
		case KEY_END:
			keep = AT_BOTTOM;
			ProcList.ProcessInput();
			break;
		case KEY_NONE: case KEY_IDLE:
			break;
		default:
			keep = AT_CHOSEN;
			if (schedule_refresh) { // postpone autorefresh while user typing something
				schedule_refresh = GetProcessUptimeMSec() + AUTOREFRESH_MSEC;
			}
			ProcList.ProcessInput();
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
