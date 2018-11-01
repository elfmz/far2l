#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <utils.h>
#include <mutex>
#include <list>
#include <string>
#include <algorithm>
#include "sudo_private.h"
#include "sudo.h"
#include "sudo_askpass_ipc.h"


namespace Sudo 
{
	std::string g_sudo_title = "SUDO request";
	std::string g_sudo_prompt = "Enter password";
	std::string g_sudo_confirm = "Confirm priviledged operation";

	static std::mutex s_client_mutex;
	static int s_client_pipe_send = -1;
	static int s_client_pipe_recv = -1;
	static std::string g_curdir_override;
	static struct ListOfStrings : std::list<std::string> {} g_recent_curdirs;
	
	static std::string g_sudo_app, g_askpass_app;
	
	enum ModifyState
	{
		MODIFY_UNDEFINED = 0,
		MODIFY_ALLOWED,
		MODIFY_DENIED		
	};
	struct ThreadRegionCounter
	{
		int count;
		int silent_query;
		ModifyState modify;
		bool cancelled;
	};
	
	thread_local ThreadRegionCounter thread_client_region_counter = { 0, 0, MODIFY_UNDEFINED, false };
	static int global_client_region_counter = 0;
	static SudoClientMode client_mode = SCM_DISABLE;
	static time_t client_password_expiration = 0;
	static time_t client_password_timestamp = 0;

	
	enum {
		RECENT_CURDIRS_LIMIT = 4
	};

	static void CloseClientConnection()
	{
		CheckedCloseFD(s_client_pipe_send);
		CheckedCloseFD(s_client_pipe_recv);
	}
	
	static bool ClientInitSequence()
	{
		try {
			SudoCommand cmd = SUDO_CMD_PING;
			BaseTransaction bt(s_client_pipe_send, s_client_pipe_recv);
			bt.SendPOD(cmd);
			bt.RecvPOD(cmd);

			if (bt.IsFailed() || cmd!=SUDO_CMD_PING)
				throw "ping failed";
				
			std::string cwd = g_curdir_override;
			if (cwd.empty()) {
				char buf[PATH_MAX + 1] = {0};
				if (getcwd(buf, PATH_MAX))
					cwd = buf;
			}
			if (!cwd.empty()) {
				cmd = SUDO_CMD_CHDIR;
				bt.SendPOD(cmd);
				bt.SendStr(cwd.c_str());
				int r = bt.RecvInt();
				if (r == -1) {
					bt.RecvErrno();
				} else {
					bt.RecvStr(cwd);
				}
				
				bt.RecvPOD(cmd);
			
				if (bt.IsFailed() || cmd!=SUDO_CMD_CHDIR)
					throw "chdir failed";
					
				fprintf(stderr, "Sudo::ClientInitSequence: chdir='%s' -> %d\n", g_curdir_override.c_str(), r);
			}

		} catch (const char *what) {
			fprintf(stderr, "Sudo::ClientInitSequence - %s\n", what);
			CloseClientConnection();
			return false;
		}
		
		
		return true;
	}
	

	
	static bool ClientConfirm()
	{
		return SudoAskpassRequestConfirmation() == SAR_OK;
	}

	static bool LaunchDispatcher(int pipe_request, int pipe_reply)
	{
		struct stat s = {0};
		
		if (g_sudo_app.empty() || stat(g_sudo_app.c_str(), &s)==-1)
			return false;

		char *askpass = getenv("SUDO_ASKPASS");
		if (!askpass || !*askpass || stat(askpass, &s)==-1) {
			if (g_askpass_app.empty() || stat(g_askpass_app.c_str(), &s)==-1)
				return false;
			setenv("SUDO_ASKPASS", g_askpass_app.c_str(), 1);
		}
	
		int r = fork();
		if (r==0) {
			//sudo closes all descriptors except std, so use them
			dup2(pipe_reply, STDOUT_FILENO);
			close(pipe_reply);
			dup2(pipe_request, STDIN_FILENO);
			close(pipe_request);
			if (chdir("/bin") == -1)  //avoid locking arbitrary current dir
				perror("chdir");
			//if process doesn't have terminal then sudo caches password per parent pid
			//so don't use intermediate shell for running it!
			r = execlp("sudo", "-n", "-A", "-k", g_sudo_app.c_str(), NULL);
			perror("execl");
			_exit(r);
			exit(r);
		}
		if ( r == -1)
			return false;
			
		PutZombieUnderControl(r);
		return true;
	}
	
	static bool OpenClientConnection()
	{
		int req[2] = {-1, -1}, rep[2] = {-1, -1};
		if (pipe(req)==-1)
			return false;
		if (pipe(rep)==-1) {
			CheckedCloseFDPair(req);
			return false;
		}
				
		fcntl(req[1], F_SETFD, FD_CLOEXEC);
		fcntl(rep[0], F_SETFD, FD_CLOEXEC);
				
		if (!LaunchDispatcher(req[0], rep[1])) {//likely missing askpass
			perror("g_sudo_launcher\n");
			CheckedCloseFDPair(req);
			CheckedCloseFDPair(rep);
			return false;
		}
		CheckedCloseFD(req[0]);
		CheckedCloseFD(rep[1]);
		s_client_pipe_send = req[1];
		s_client_pipe_recv = rep[0];
		return true;
	}


	static void CheckForCloseClientConnection()
	{
		if (global_client_region_counter != 0)
			return;
			
		if (client_mode != SCM_DISABLE) {
			if (time(nullptr) - client_password_timestamp < client_password_expiration) {
				return;
			}
		}
			
		CloseClientConnection();
	}
	
	/////////////////////////////////////////////////////////////////////
	
	void ClientCurDirOverrideReset()
	{
		std::lock_guard<std::mutex> lock(s_client_mutex);
		g_curdir_override.clear();
	}
	
	void ClientCurDirOverrideSet(const char *path)
	{
		//fprintf(stderr, "ClientCurDirOverride: %s\n", path);
		std::lock_guard<std::mutex> lock(s_client_mutex);
		g_curdir_override = path;
		if (!g_curdir_override.empty()) {
			g_recent_curdirs.push_back(g_curdir_override);
			while (g_recent_curdirs.size() > RECENT_CURDIRS_LIMIT)
				g_recent_curdirs.pop_front();
		}
	}
	
	bool ClientCurDirOverrideSetIfRecent(const char *path)
	{
		std::string str = path;
		std::lock_guard<std::mutex> lock(s_client_mutex);
		if (client_mode == SCM_DISABLE)
			return false;

		ListOfStrings::iterator  i = 
			std::find(g_recent_curdirs.begin(), g_recent_curdirs.end(), str);
		if (i == g_recent_curdirs.end())
			return false;
		
		g_curdir_override.swap(str);
		g_recent_curdirs.erase(i);
		g_recent_curdirs.push_back(g_curdir_override);
		return true;
	}
	
	bool ClientCurDirOverrideQuery(char *path, size_t size)
	{
		std::lock_guard<std::mutex> lock(s_client_mutex);
		if (g_curdir_override.size() >= size ) {
			errno = ERANGE;
			return false;
		}
		
		strcpy(path, g_curdir_override.c_str() );
		return true;
	}
	
	ClientReconstructCurDir::ClientReconstructCurDir(const char * &path)
		: _free_ptr(nullptr), _initial_path(path), _path(path)
	{
		if (path[0] != '/' && path[0]) {
			std::lock_guard<std::mutex> lock(s_client_mutex);
			if (!g_curdir_override.empty()) {
				std::string  str = g_curdir_override;
				if (strcmp(path, ".")==0 || strcmp(path, "./")==0) {
					
				} else if (strcmp(path, "..")==0 || strcmp(path, "../")==0) {
					size_t p = str.rfind('/');
					if (p!=std::string::npos)
						str.resize(p);
				} else if (path[0] == '.' && path[1] == '/') {
					str+= &path[1];
				} else {
					str+= '/';
					str+= &path[0];
				}
				_free_ptr = strdup(str.c_str());
				if (_free_ptr)
					_path = _free_ptr;
			}
		}
	}
	
	ClientReconstructCurDir::~ClientReconstructCurDir()
	{
		_path = _initial_path;
		free(_free_ptr);
	}
	
/////////////////////////
	extern "C" {
		
		
		void sudo_client_configure(SudoClientMode mode, int password_expiration, 
			const char *sudo_app, const char *askpass_app,
			const char *sudo_title, const char *sudo_prompt, const char *sudo_confirm)
		{
			if (sudo_title && *sudo_title) g_sudo_title = sudo_title;
			if (sudo_prompt && *sudo_prompt) g_sudo_prompt = sudo_prompt;
			if (sudo_confirm && *sudo_confirm) g_sudo_confirm = sudo_confirm;

			std::lock_guard<std::mutex> lock(s_client_mutex);
			g_sudo_app = sudo_app;
			g_askpass_app = askpass_app ? askpass_app : "";
			client_mode = mode;
			client_password_expiration = password_expiration;
			CheckForCloseClientConnection();
		}
		
		
		__attribute__ ((visibility("default"))) void sudo_client_region_enter()
		{
			std::lock_guard<std::mutex> lock(s_client_mutex);
			global_client_region_counter++;
			thread_client_region_counter.count++;
		}
		
		__attribute__ ((visibility("default"))) void sudo_client_region_leave()
		{
			std::lock_guard<std::mutex> lock(s_client_mutex);
			assert(global_client_region_counter > 0);
			assert(thread_client_region_counter.count > 0);

			global_client_region_counter--;
			thread_client_region_counter.count--;
			
			if (!thread_client_region_counter.count) {
				thread_client_region_counter.cancelled = false;
				thread_client_region_counter.modify = MODIFY_UNDEFINED;
			}

			CheckForCloseClientConnection();
		}
		
		 __attribute__ ((visibility("default"))) void sudo_silent_query_region_enter()
		{
			thread_client_region_counter.silent_query++;
		}

		 __attribute__ ((visibility("default"))) void sudo_silent_query_region_leave()
		{
			thread_client_region_counter.silent_query--;
		}

	}
	

	bool IsSudoRegionActive()
	{
		std::lock_guard<std::mutex> lock(s_client_mutex);
		return (s_client_pipe_send!=-1 && s_client_pipe_recv!=-1 && thread_client_region_counter.count);
	}
	
	bool TouchClientConnection(bool want_modify)
	{
		if (!thread_client_region_counter.count || thread_client_region_counter.cancelled) {
			return false;
		}

		ErrnoSaver es;
		std::lock_guard<std::mutex> lock(s_client_mutex);
		if (client_mode == SCM_DISABLE)
			return false;

		if (s_client_pipe_send==-1 || s_client_pipe_recv==-1) {
			if (thread_client_region_counter.silent_query > 0 && !want_modify)
				return false;

			CloseClientConnection();
			if (!OpenClientConnection())
				return false;
				
			if (!ClientInitSequence()) {//likely bad password or Cancel
				thread_client_region_counter.cancelled = true;
				return false; 
			}
			
			//assume user confirmed also modify operation with this password
			if (want_modify && client_mode == SCM_CONFIRM_MODIFY )
				thread_client_region_counter.modify = MODIFY_ALLOWED;

			client_password_timestamp = time(nullptr);

		} else if (want_modify && client_mode == SCM_CONFIRM_MODIFY ) {
			if (thread_client_region_counter.modify == MODIFY_UNDEFINED) {
				thread_client_region_counter.modify = 
					ClientConfirm() ? MODIFY_ALLOWED : MODIFY_DENIED;
			}
			return (thread_client_region_counter.modify == MODIFY_ALLOWED);
		}

		return true;
	}


	ClientTransaction::ClientTransaction(SudoCommand cmd) : 
		std::lock_guard<std::mutex>(s_client_mutex),
		BaseTransaction(s_client_pipe_send, s_client_pipe_recv), 
		_cmd(cmd)
	{
		try {
			SendPOD(_cmd);
		} catch (const char *) {
			CloseClientConnection();
			throw;
		}
	}
	
	ClientTransaction::~ClientTransaction()
	{
		try {//should catch cuz used from d-tor
			Finalize();
			if (IsFailed())
				CloseClientConnection();
		} catch (const char * what) {
			fprintf(stderr, "ClientTransaction(%u)::Finalize - %s\n", _cmd, what);
			CloseClientConnection();
		}
	}
	
	void ClientTransaction::NewTransaction(SudoCommand cmd)
	{
		Finalize();
		_cmd = cmd;
		SendPOD(_cmd);
	}
	
	void ClientTransaction::Finalize()
	{
		SudoCommand reply;
		RecvPOD(reply);
		if (reply!=_cmd)
			throw "bad reply";
	}
}
