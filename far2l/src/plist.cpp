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

struct FarPidInfo {
	std::wstring text;
	std::string name;
	int pid;
	unsigned long rss;
	unsigned long cpu_ticks;
};

static int is_pid_dir(const char *name) {
    for (const char *p = name; *p; p++)
        if (!isdigit(*p)) return 0;
    return 1;
}

static void enumerateProcesses(std::vector<FarPidInfo>& v) 
{
	v.clear();

#ifdef __linux__
    struct dirent *entry;

    DIR *d = opendir("/proc");
    if (!d) return;

    while ((entry = readdir(d)) != NULL) {
        if (!is_pid_dir(entry->d_name))
            continue;

        int pid = atoi(entry->d_name);

        // ---- Read process name from /proc/<pid>/comm ----
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char shortname[256];
        if (!fgets(shortname, sizeof(shortname), f)) {
            fclose(f);
            continue;
        }
        fclose(f);

        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        f = fopen(path, "r");
        if (!f) continue;

        char name[256];
        *name = 0;
        if (!fgets(name, sizeof(name), f)) {
            fclose(f);
            continue;
        }
        fclose(f);

        // ---- Read CPU and memory from /proc/<pid>/stat ----
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        f = fopen(path, "r");
        if (!f) continue;

        unsigned long utime, stime, rss;
        char commbuf[256], state;

        /*
         * /proc/<pid>/stat format:
         * pid (comm) state ... utime stime ... rss ...
         */
        fscanf(f, "%*d (%[^)]) %c", commbuf, &state);

        // Skip fields until utime (14) and stime (15)
        for (int i = 0; i < 11; i++)
            fscanf(f, "%*s");

        fscanf(f, "%ld %ld", &utime, &stime);

        // Skip fields until rss (24)
        for (int i = 0; i < 7; i++)
            fscanf(f, "%*s");

        fscanf(f, "%ld", &rss);
        fclose(f);

        unsigned long page_kb = sysconf(_SC_PAGESIZE) / 1024;
        unsigned long rss_kb = rss * page_kb;

        char* q = shortname + strlen(shortname) - 1;
        while(q > shortname && isspace(*q)) --q;
        q[1] = 0;

        /*
        printf("PID: %d\n", pid);
        printf("Name: %s", name);
        printf("CPU ticks: %ld (user) + %ld (system)\n", utime, stime);
        printf("RSS: %ld KB\n", rss_kb);
        printf("----\n");
        */
        unsigned long total_time = utime + stime;
        const char* cpuLoad = "?";

        if (total_time < 100)        cpuLoad = "idle";
		else if (total_time < 1000)  cpuLoad = "low";
		else if (total_time < 10000) cpuLoad = "medium";
		else                         cpuLoad = "high";

		char uid_name[128];
		char gid_name[128];
		*uid_name = *gid_name = 0;
        int ruid, euid, suid, fsuid;
        int rgid, egid, sgid, fsgid;

		snprintf(path, sizeof(path), "/proc/%d/status", pid);
		f = fopen(path, "r");
		if (f) {
		    char line[256];
		    while (fgets(line, sizeof(line), f)) {
		        if (strncmp(line, "Uid:", 4) == 0) {
		            sscanf(line + 4, "%d %d %d %d", &ruid, &euid, &suid, &fsuid);
		            // printf("UID: %d (real), %d (effective)\n", ruid, euid);
					struct passwd* pw = getpwuid(ruid);
					if (pw) strcpy(uid_name, pw->pw_name);
		        }
		        if (strncmp(line, "Gid:", 4) == 0) {
		            sscanf(line + 4, "%d %d %d %d", &rgid, &egid, &sgid, &fsgid);
		            //printf("GID: %d (real), %d (effective)\n", rgid, egid);
					struct group* gr = getgrgid(rgid);
					if (gr) strcpy(gid_name, gr->gr_name);
		        }
		    }
		    fclose(f);
		}

		FARString strStr;
		strStr.Format(L"%8d %lc %-12.12s %lc %-16.16s %lc %-40.40s %lc %6s %lc %'8ld Mb", 
			pid, BoxSymbols[BS_V1], uid_name, BoxSymbols[BS_V1], 
			shortname, BoxSymbols[BS_V1], name, BoxSymbols[BS_V1], 
			cpuLoad, BoxSymbols[BS_V1], rss_kb / 1024);
		v.push_back({ strStr.GetWide(), name, pid, rss_kb, total_time });
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
	std::vector<FarPidInfo> v;
	enumerateProcesses(v);

	int GroupsLen = (int)v.size();
	if (GroupsLen < 1) return; // nothing to display

	MenuDataEx Groups[GroupsLen];
	for (int j = 0; j < GroupsLen; ++j) {
		Groups[j] = { v[j].text.c_str(), 0, 0 };
	}

	VMenu ProcList(Msg::ProcessListTitle, Groups, GroupsLen, ScrY - 4);

	ProcList.SetPosition(-1, -1, 0, 0);
	ProcList.SetFlags(VMENU_WRAPMODE | VMENU_NOTCHANGE);
	ProcList.SetBottomTitle(Msg::ProcessListBottom);
	ProcList.ClearDone();

	ProcList.Show();

	while (!ProcList.Done()) {
		FarKey key = ProcList.ReadInput();

		switch (key) {
		case KEY_F1:
			Help::Present(L"TaskList");
			break;
		case 't': case 'T': /* sort by name */
		case 'i': case 'I': /* sort by pid */
		case 'p': case 'P': /* sort by cpu */
		case 'm': case 'M': /* sort by RSS */
		case KEY_CTRLR:	
			ProcList.Hide();
			ProcList.DeleteItems();

			ProcList.SetPosition(-1,-1,0,0);

			enumerateProcesses(v);

			if (key == 't' || key == 'T')
				std::sort(v.begin(), v.end(),
				    [key](const FarPidInfo& a, const FarPidInfo& b) {
				        return key == 'T' ? b.name < a.name : a.name < b.name;
				    });
			else if (key == 'i' || key == 'I')
				std::sort(v.begin(), v.end(),
				    [key](const FarPidInfo& a, const FarPidInfo& b) {
				        return key == 'i' ? a.pid < b.pid : b.pid < a.pid;
				    });
			else if (key == 'p' || key == 'P')
				std::sort(v.begin(), v.end(),
				    [key](const FarPidInfo& a, const FarPidInfo& b) {
				        return key == 'P' ? b.cpu_ticks < a.cpu_ticks : a.cpu_ticks < b.cpu_ticks;
				    });
			else if (key == 'm' || key == 'M')
				std::sort(v.begin(), v.end(),
				    [key](const FarPidInfo& a, const FarPidInfo& b) {
				        return key == 'M' ? b.rss < a.rss : a.rss < b.rss;
				    });

			GroupsLen = (int)v.size();
			for (int j = 0; j < GroupsLen; ++j) {
				MenuItemEx item;
				item.Clear();
				item.strName = v[j].text.c_str();
                item.AccelKey = 0;
				item.Flags = 0;

				ProcList.AddItem(&item);
			}

			ProcList.Show();
			break;
		case KEY_NUMDEL:
		case KEY_DEL: 
			if(!kill(v[ProcList.GetSelectPos()].pid, SIGTERM))
				ProcList.DeleteItem(ProcList.GetSelectPos());
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
