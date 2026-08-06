#include <gtest/gtest.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <string>
#include <cstring>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "WinPort/src/Backend/TTY/TTYCaps.h"

class TTYCapsDECRQMTest : public ::testing::Test {
protected:
	int master_fd = -1;
	int slave_fd = -1;
	pid_t child_pid = -1;

	void SetUp() override {
#if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
		GTEST_SKIP() << "PTY not available on this platform";
#endif
		master_fd = posix_openpt(O_RDWR | O_NOCTTY);
		ASSERT_NE(master_fd, -1) << "posix_openpt failed: " << errno;
		ASSERT_EQ(grantpt(master_fd), 0) << "grantpt failed: " << errno;
		ASSERT_EQ(unlockpt(master_fd), 0) << "unlockpt failed: " << errno;

		const char *slave_name = ptsname(master_fd);
		ASSERT_NE(slave_name, nullptr) << "ptsname failed: " << errno;

		slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
		ASSERT_NE(slave_fd, -1) << "open slave failed: " << errno;

		// Disable cooked-mode buffering and echo so replies are delivered
		// immediately to the slave fd without line-discipline transformation.
		struct termios t;
		if (tcgetattr(slave_fd, &t) == 0) {
			t.c_lflag &= ~(ECHO | ECHOCTL | ECHOE | ECHOK | ECHONL | ICANON | ISIG | IEXTEN);
			t.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR | BRKINT | ISTRIP | INPCK);
			t.c_oflag &= ~(OPOST | ONLCR | OCRNL);
			t.c_cc[VMIN] = 1;
			t.c_cc[VTIME] = 0;
			tcsetattr(slave_fd, TCSANOW, &t);
		}
	}

	void TearDown() override {
		if (slave_fd != -1) {
			close(slave_fd);
		}
		if (master_fd != -1) {
			close(master_fd);
		}
		if (child_pid > 0) {
			int status = 0;
			waitpid(child_pid, &status, 0);
		}
	}

	void ForkResponder(const char *reply) {
		child_pid = fork();
		ASSERT_NE(child_pid, -1) << "fork failed: " << errno;
		if (child_pid == 0) {
			close(slave_fd);
			char buf[4096];
			std::string received;
			bool responded = false;
			while (true) {
				ssize_t n = read(master_fd, buf, sizeof(buf));
				if (n <= 0) {
					break;
				}
				received.append(buf, n);
				if (!responded) {
					size_t pos = received.find("\x1b[5n");
					if (pos != std::string::npos) {
						write(master_fd, reply, strlen(reply));
						responded = true;
					}
				}
			}
			close(master_fd);
			_exit(0);
		}
		close(master_fd);
		master_fd = -1;
	}

	TTYCaps RunSetup() {
		setenv("TERM", "xterm-256color", 1);
		unsetenv("TERM_PROGRAM");
		unsetenv("TERMINAL_EMULATOR");

		TTYRestrict restrict{};
		restrict.emoji = true;
		restrict.far2l = true;

		TTYCaps caps{};
		caps.Setup(slave_fd, slave_fd, restrict);
		return caps;
	}
};

TEST_F(TTYCapsDECRQMTest, SupportedSetsSynchronizedUpdates) {
	ForkResponder("\x1b[0n\x1b[?2026;1$y");
	TTYCaps caps = RunSetup();
	EXPECT_TRUE(caps.synchronized_updates)
		<< "DECRQM Ps=1 should enable synchronized_updates";
}

TEST_F(TTYCapsDECRQMTest, NotSupportedClearsSynchronizedUpdates) {
	ForkResponder("\x1b[0n\x1b[?2026;0$y");
	TTYCaps caps = RunSetup();
	EXPECT_FALSE(caps.synchronized_updates)
		<< "DECRQM Ps=0 should disable synchronized_updates";
}

TEST_F(TTYCapsDECRQMTest, UnknownFallsBackToTermHeuristic) {
	// DECRQM Ps=2 means "unknown/undetermined" — TERM heuristic should decide.
	// With TERM=xterm-256color heuristic says false, so final result is false.
	ForkResponder("\x1b[0n\x1b[?2026;2$y");
	TTYCaps caps = RunSetup();
	EXPECT_FALSE(caps.synchronized_updates)
		<< "DECRQM Ps=2 should fall back to TERM heuristic (xterm = false)";
}

TEST_F(TTYCapsDECRQMTest, NoReplyFallsBackToTermHeuristic) {
	// Terminal that does not understand DECRQM sends no reply.
	// Fallback to TERM heuristic (xterm = false).
	ForkResponder("\x1b[0n");
	TTYCaps caps = RunSetup();
	EXPECT_FALSE(caps.synchronized_updates)
		<< "Missing DECRQM reply should fall back to TERM heuristic";
}

TEST_F(TTYCapsDECRQMTest, HeuristicOverriddenWhenDecrqmSaysSupported) {
	// Even with TERM=xterm-256color (heuristic false), DECRQM Ps=1 overrides to true.
	ForkResponder("\x1b[0n\x1b[?2026;1$y");
	TTYCaps caps = RunSetup();
	EXPECT_TRUE(caps.synchronized_updates)
		<< "DECRQM should override TERM heuristic";
}
