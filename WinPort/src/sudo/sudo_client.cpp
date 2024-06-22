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
#include <stdexcept>
#include <LocalSocket.h>
#include "sudo_private.h"
#include "sudo.h"
#include "sudo_askpass_ipc.h"
#include "../../WinCompat.h"

namespace Sudo 
{
	std::string g_sudo_title = "SUDO request";
	std::string g_sudo_prompt = "Enter password";
	std::string g_sudo_confirm = "Confirm privileged operation";

	static std::mutex s_uds_mutex;
	static std::unique_ptr<LocalSocketServer> s_uds;
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
	static bool client_drop_pending = false;

	
	enum {
		RECENT_CURDIRS_LIMIT = 4
	};

	static void CloseClientConnection()
	{
		s_uds.reset();
	}
	
	static bool ClientInitSequence()
	{
		try {
			SudoCommand cmd = SUDO_CMD_PING;
			BaseTransaction bt(*s_uds);
			bt.SendPOD(cmd);
			bt.RecvPOD(cmd);

			if (bt.IsFailed() || cmd!=SUDO_CMD_PING)
				throw std::runtime_error("ping failed");
				
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
					throw std::runtime_error("chdir failed");
					
				fprintf(stderr, "Sudo::ClientInitSequence: chdir='%s' -> %d\n", g_curdir_override.c_str(), r);
			}

		} catch (std::exception &e) {
			fprintf(stderr, "Sudo::ClientInitSequence - %s\n", e.what());
			CloseClientConnection();
			return false;
		}
		
		
		return true;
	}
	

	
	static bool ClientConfirm()
	{
		return SudoAskpassRequestConfirmation() == SAR_OK;
	}

	static int LaunchDispatcher(const std::string &ipc)
	{
		struct stat s = {0};
		
		if (g_sudo_app.empty() || stat(g_sudo_app.c_str(), &s)==-1) {
			throw std::runtime_error("g_sudo_app not configured");
		}

		char *askpass = getenv("SUDO_ASKPASS");
		if (!askpass || !*askpass || stat(askpass, &s)==-1) {
			if (g_askpass_app.empty() || stat(g_askpass_app.c_str(), &s)==-1) {
				throw std::runtime_error("g_askpass_app not configured");
			}
			setenv("SUDO_ASKPASS", g_askpass_app.c_str(), 1);
		}

		int leash[2] = {-1, -1};
		if (pipe(leash)==-1) {
			throw std::runtime_error("pipe-leash");
		}

		MakeFDCloexec(leash[0]);

		int r = fork();
		if (r == 0) {
			// sudo closes all descriptors except std, so put leash[1] into stdin to make it survive
			dup2(leash[1], STDIN_FILENO);

			// override stdout handle otherwise TTY detach doesnt work while sudo client runs
			int fd = open("/dev/null", O_RDWR);
			if (fd != -1) {
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}

			// avoid locking arbitrary current dir and
			// allow starting in portable env (see #1505)
			const char *farhome = getenv("FARHOME");
			if (!farhome || chdir(farhome) == -1) {
				if (chdir("/bin") == -1) {
					perror("chdir");
				}
			}

			//if process doesn't have terminal then sudo caches password per parent pid
			//so don't use intermediate shell for running it!
			r = execlp("sudo", "-n", "-A", "-k", g_sudo_app.c_str(), ipc.c_str(), NULL);
//			r = execlp(g_sudo_app.c_str(), g_sudo_app.c_str(), ipc.c_str(), NULL);
			perror("execl");
			_exit(r);
			exit(r);
		}
		close(leash[1]);
		if ( r == -1) {
			close(leash[0]);
			throw std::runtime_error("fork failed");
		}

		PutZombieUnderControl(r);
		return leash[0];
	}
	
	static bool OpenClientConnection()
	{
		std::string ipc = InMyTempFmt("sudo/%u", getpid());
		try {
			s_uds.reset(new LocalSocketServer(LocalSocket::STREAM, ipc));
			FDScope dispatcher_leash(LaunchDispatcher(ipc));
			s_uds->WaitForClient(dispatcher_leash);

		} catch (std::exception &e) {
			fprintf(stderr, "OpenClientConnection: %s\n", e.what());
			s_uds.reset();
			unlink(ipc.c_str());
			return false;
		}

		return ClientInitSequence();
	}


	static void CheckForCloseClientConnection()
	{
		if (global_client_region_counter != 0)
			return;

		if (client_mode != SCM_DISABLE && !client_drop_pending) {
			if (time(nullptr) - client_password_timestamp < client_password_expiration) {
				return;
			}
		}

		client_drop_pending = false;
		CloseClientConnection();
	}
	
	/////////////////////////////////////////////////////////////////////
	
	void ClientCurDirOverrideReset()
	{
		std::lock_guard<std::mutex> lock(s_uds_mutex);
		g_curdir_override.clear();
	}
	
	void ClientCurDirOverrideSet(const char *path)
	{
		//fprintf(stderr, "ClientCurDirOverride: %s\n", path);
		std::lock_guard<std::mutex> lock(s_uds_mutex);
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
		std::lock_guard<std::mutex> lock(s_uds_mutex);
		if (client_mode == SCM_DISABLE)
			return false;

		ListOfStrings::iterator i = 
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
		std::lock_guard<std::mutex> lock(s_uds_mutex);
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
			std::lock_guard<std::mutex> lock(s_uds_mutex);
			if (!g_curdir_override.empty()) {
				std::string str = g_curdir_override;
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

			std::lock_guard<std::mutex> lock(s_uds_mutex);
			g_sudo_app = sudo_app;
			g_askpass_app = askpass_app ? askpass_app : "";
			client_mode = mode;
			client_password_expiration = password_expiration;
			CheckForCloseClientConnection();
		}

		__attribute__ ((visibility("default"))) void sudo_client_drop()
		{
			std::lock_guard<std::mutex> lock(s_uds_mutex);
			if (global_client_region_counter) {
				client_drop_pending = true;

			} else {
				CloseClientConnection();
			}
		}
		
		
		__attribute__ ((visibility("default"))) void sudo_client_region_enter()
		{
			std::lock_guard<std::mutex> lock(s_uds_mutex);
			global_client_region_counter++;
			thread_client_region_counter.count++;
		}
		
		__attribute__ ((visibility("default"))) void sudo_client_region_leave()
		{
			std::lock_guard<std::mutex> lock(s_uds_mutex);
			ASSERT(global_client_region_counter > 0);
			ASSERT(thread_client_region_counter.count > 0);

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
		std::lock_guard<std::mutex> lock(s_uds_mutex);
		return (s_uds && thread_client_region_counter.count);
	}
	
	bool TouchClientConnection(bool want_modify)
	{
		if (!thread_client_region_counter.count || thread_client_region_counter.cancelled) {
			return false;
		}

		ErrnoSaver es;
		std::lock_guard<std::mutex> lock(s_uds_mutex);
		if (client_mode == SCM_DISABLE)
			return false;

		if (!s_uds) {
			if (thread_client_region_counter.silent_query > 0 && !want_modify)
				return false;

			CloseClientConnection();

			if (!OpenClientConnection()) {//likely bad password or Cancel
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
		std::lock_guard<std::mutex>(s_uds_mutex),
		BaseTransaction(*s_uds), 
		_cmd(cmd)
	{
		try {
			SendPOD(_cmd);
		} catch (...) {
			CloseClientConnection();
			throw;
		}
	}
	
	ClientTransaction::~ClientTransaction()
	{
		try {//should catch all cuz in d-tor
			Finalize();
			if (IsFailed())
				CloseClientConnection();

		} catch (...) {
			fprintf(stderr, "ClientTransaction(%u)::Finalize - exception\n", _cmd);
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
