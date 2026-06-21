#include <gtest/gtest.h>
#include <ProcessCreation.h>
#include <utils.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <unistd.h>

class ProcessCreationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure test runs in a clean state
    }

    void TearDown() override
    {
        // Cleanup if needed
    }
};

// Test basic process creation
TEST_F(ProcessCreationTest, CreateSimpleProcess)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"test"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.handle.pid, 0);

    // Wait for process to complete
    bool wait_result = GetProcessCreation()->WaitProcess(&result.handle, 5000);
    EXPECT_TRUE(wait_result);

    // Clean up
    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test process with arguments
TEST_F(ProcessCreationTest, CreateProcessWithArguments)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"Hello", "World"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.handle.pid, 0);

    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test process with working directory
TEST_F(ProcessCreationTest, CreateProcessWithWorkingDirectory)
{
    char tmpdir[] = "/tmp/far2l-pctest-XXXXXX";
    ASSERT_NE(mkdtemp(tmpdir), nullptr) << "mkdtemp failed";

    ProcessConfig config;
    config.program = "/bin/pwd";
    config.working_directory = tmpdir;

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_TRUE(result.success);

    GetProcessCreation()->CloseHandle(&result.handle);

    rmdir(tmpdir);
}

// Test process with environment variables
TEST_F(ProcessCreationTest, CreateProcessWithEnvironment)
{
    ProcessConfig config;
    config.program = "/bin/sh";
    config.args = {"-c", "echo $TEST_VAR"};
    config.env = {"TEST_VAR=test_value"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_TRUE(result.success);

    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test invalid program path
TEST_F(ProcessCreationTest, InvalidProgramPath)
{
    ProcessConfig config;
    config.program = "/nonexistent/path/to/program";

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// Test GetProcessId
TEST_F(ProcessCreationTest, GetProcessId)
{
    ProcessConfig config;
    config.program = "/bin/sleep";
    config.args = {"1"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    ASSERT_TRUE(result.success);

    uint32_t pid = GetProcessCreation()->GetProcessId(&result.handle);
    EXPECT_EQ(pid, static_cast<uint32_t>(result.handle.pid));

    // Kill the process before cleanup
    GetProcessCreation()->KillProcess(&result.handle, SIGTERM);
    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test KillProcess
TEST_F(ProcessCreationTest, KillProcess)
{
    ProcessConfig config;
    config.program = "/bin/sleep";
    config.args = {"100"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    ASSERT_TRUE(result.success);

    // Give process time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Kill the process
    bool kill_result = GetProcessCreation()->KillProcess(&result.handle, SIGKILL);
    EXPECT_TRUE(kill_result);

    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test WaitProcess with timeout
TEST_F(ProcessCreationTest, WaitProcessWithTimeout)
{
    ProcessConfig config;
    config.program = "/bin/sleep";
    config.args = {"10"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    ASSERT_TRUE(result.success);

    // Wait with short timeout - should timeout
    bool wait_result = GetProcessCreation()->WaitProcess(&result.handle, 500);
    EXPECT_FALSE(wait_result);  // Process should still be running

    // Kill and cleanup
    GetProcessCreation()->KillProcess(&result.handle, SIGKILL);
    GetProcessCreation()->WaitProcess(&result.handle, 1000);
    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test WaitProcess with infinite timeout
TEST_F(ProcessCreationTest, WaitProcessInfinite)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"done"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    ASSERT_TRUE(result.success);

    // Wait with infinite timeout (-1)
    bool wait_result = GetProcessCreation()->WaitProcess(&result.handle, -1);
    EXPECT_TRUE(wait_result);

    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test Daemonize
TEST_F(ProcessCreationTest, Daemonize)
{
    GTEST_SKIP() << "Daemonize test disabled: makes the test process a daemon, "
                 << "which breaks the test framework's process tracking.";
}
// Test PTY creation
TEST_F(ProcessCreationTest, CreateProcessWithPTY)
{
    ProcessConfig config;
    config.program = "/bin/bash";
    config.args = {"-c", "echo test"};
    config.new_session = true;

    PTYHandle pty;
    ProcessResult result = GetProcessCreation()->CreateProcessWithPTY(config, &pty);

    EXPECT_TRUE(result.success);
    EXPECT_GE(pty.master_fd, 0);
    EXPECT_GT(pty.pid, 0);

    // Close PTY
    GetProcessCreation()->ClosePTY(&pty);
}

// Test process with new session
TEST_F(ProcessCreationTest, CreateProcessWithNewSession)
{
    ProcessConfig config;
    config.program = "/bin/bash";
    config.args = {"-c", "ps -o pid,pgid,sid | grep $$"};
    config.new_session = true;

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_TRUE(result.success);

    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test stdin/stdout redirection to null
TEST_F(ProcessCreationTest, RedirectToNull)
{
    ProcessConfig config;
    config.program = "/bin/cat";  // Would hang if reading from stdin
    config.stdin_redirect = "null";
    config.stdout_redirect = "null";
    config.stderr_redirect = "null";

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    EXPECT_TRUE(result.success);

    // Give it a moment then kill
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    GetProcessCreation()->KillProcess(&result.handle, SIGTERM);
    GetProcessCreation()->CloseHandle(&result.handle);
}

// Test CloseHandle is idempotent
TEST_F(ProcessCreationTest, CloseHandleIdempotent)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"test"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);

    ASSERT_TRUE(result.success);

    // Close multiple times should be safe
    GetProcessCreation()->CloseHandle(&result.handle);
    GetProcessCreation()->CloseHandle(&result.handle);  // Second call should not crash

    // Reset handle for next test
    result.handle.pid = -1;
}

// Test ClosePTY is idempotent
TEST_F(ProcessCreationTest, ClosePTYIdempotent)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"test"};

    PTYHandle pty;
    ProcessResult result = GetProcessCreation()->CreateProcessWithPTY(config, &pty);

    if (result.success) {
        // Close multiple times should be safe
        GetProcessCreation()->ClosePTY(&pty);
        GetProcessCreation()->ClosePTY(&pty);  // Second call should not crash
    }
}

// Test ProcessIdZero
TEST_F(ProcessCreationTest, GetProcessIdWithInvalidHandle)
{
    ProcessHandle handle = {};
    handle.pid = -1;

    uint32_t pid = GetProcessCreation()->GetProcessId(&handle);
    EXPECT_EQ(pid, 0u);
}

// Test WaitProcess with invalid handle
TEST_F(ProcessCreationTest, WaitProcessInvalidHandle)
{
    ProcessHandle handle = {};
    handle.pid = -1;

    bool result = GetProcessCreation()->WaitProcess(&handle, 1000);
    EXPECT_FALSE(result);
}

// Test KillProcess with invalid handle
TEST_F(ProcessCreationTest, KillProcessInvalidHandle)
{
    ProcessHandle handle = {};
    handle.pid = -1;

    bool result = GetProcessCreation()->KillProcess(&handle, SIGTERM);
    EXPECT_FALSE(result);
}
// Test pipe stdout capture
TEST_F(ProcessCreationTest, PipeStdoutCapture)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"pipe_test"};
    config.stdout_redirect = "pipe";

    ProcessResult result = GetProcessCreation()->CreateProcess(config);
    ASSERT_TRUE(result.success);
    EXPECT_GE(result.handle.stdout_fd, 0);

    std::string output;
    char buf[256];
    for (;;) {
        ssize_t n = read(result.handle.stdout_fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        output.append(buf, static_cast<size_t>(n));
    }

    EXPECT_TRUE(GetProcessCreation()->WaitProcess(&result.handle, 5000));
    GetProcessCreation()->GetExitCode(&result.handle);
    GetProcessCreation()->CloseHandle(&result.handle);

    EXPECT_NE(output.find("pipe_test"), std::string::npos);
}

// Test pipe stdin feed
TEST_F(ProcessCreationTest, PipeStdinFeed)
{
    ProcessConfig config;
    config.program = "/bin/cat";
    config.stdin_redirect = "pipe";
    config.stdout_redirect = "pipe";

    ProcessResult result = GetProcessCreation()->CreateProcess(config);
    ASSERT_TRUE(result.success);
    EXPECT_GE(result.handle.stdin_fd, 0);
    EXPECT_GE(result.handle.stdout_fd, 0);

    const char *input = "stdin_data";
    size_t written = 0;
    while (written < strlen(input)) {
        ssize_t n = write(result.handle.stdin_fd, input + written, strlen(input) - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        written += static_cast<size_t>(n);
    }
    close(result.handle.stdin_fd);
    result.handle.stdin_fd = -1;

    std::string output;
    char buf[256];
    for (;;) {
        ssize_t n = read(result.handle.stdout_fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        output.append(buf, static_cast<size_t>(n));
    }

    EXPECT_TRUE(GetProcessCreation()->WaitProcess(&result.handle, 5000));
    GetProcessCreation()->GetExitCode(&result.handle);
    GetProcessCreation()->CloseHandle(&result.handle);

    EXPECT_NE(output.find("stdin_data"), std::string::npos);
}

// Test CloseHandle closes pipe FDs
TEST_F(ProcessCreationTest, CloseHandleClosesPipes)
{
    ProcessConfig config;
    config.program = "/bin/echo";
    config.args = {"close_test"};
    config.stdout_redirect = "pipe";

    ProcessResult result = GetProcessCreation()->CreateProcess(config);
    ASSERT_TRUE(result.success);
    EXPECT_GE(result.handle.stdout_fd, 0);

    GetProcessCreation()->CloseHandle(&result.handle);
    EXPECT_EQ(result.handle.stdout_fd, -1);
}
