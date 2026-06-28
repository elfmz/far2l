#include <gtest/gtest.h>
#include <ProcessCreation.h>
#include <utils.h>
#include <chrono>
#include <thread>
#include <vector>
#include <csignal>
#include <cerrno>
#include <atomic>

class ProcessCreationThreadTest : public ::testing::Test
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

// Test concurrent CreateProcess + CloseHandle from multiple threads
TEST_F(ProcessCreationThreadTest, ConcurrentCreateAndClose)
{
    constexpr int kNumThreads = 4;
    constexpr int kIterations = 10;
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kIterations; ++i) {
                ProcessConfig config;
                config.program = "/bin/echo";
                config.args = {"thread_test"};

                ProcessResult result = GetProcessCreation()->CreateProcess(config);
                if (result.success) {
                    GetProcessCreation()->CloseHandle(&result.handle);
                    success_count.fetch_add(1);
                } else {
                    fail_count.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), kNumThreads * kIterations);
    EXPECT_EQ(fail_count.load(), 0);
}

// Test concurrent SetCallback and CreateProcess — no data race
TEST_F(ProcessCreationThreadTest, ConcurrentSetCallbackAndCreate)
{
    constexpr int kNumThreads = 4;
    constexpr int kIterations = 10;
    std::atomic<int> callback_set_count{0};
    std::atomic<int> success_count{0};

    auto callback = [](const ProcessHandle*, int, void*) {};

    std::vector<std::thread> threads;
    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kIterations; ++i) {
                if (i % 2 == 0) {
                    GetProcessCreation()->SetCallback(callback, reinterpret_cast<void*>(t));
                    callback_set_count.fetch_add(1);
                } else {
                    ProcessConfig config;
                    config.program = "/bin/echo";
                    config.args = {"callback_test"};

                    ProcessResult result = GetProcessCreation()->CreateProcess(config);
                    if (result.success) {
                        GetProcessCreation()->CloseHandle(&result.handle);
                        success_count.fetch_add(1);
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), kNumThreads * kIterations / 2);
    EXPECT_EQ(callback_set_count.load(), kNumThreads * kIterations / 2);
}

// Test WaitProcess and internal reaping from different threads
TEST_F(ProcessCreationThreadTest, WaitProcessFromDifferentThreads)
{
    constexpr int kNumProcesses = 8;
    std::vector<ProcessResult> results;

    for (int i = 0; i < kNumProcesses; ++i) {
        ProcessConfig config;
        config.program = "/bin/sleep";
        config.args = {"0.1"};

        ProcessResult result = GetProcessCreation()->CreateProcess(config);
        ASSERT_TRUE(result.success) << "Failed to create process " << i;
        results.push_back(result);
    }

    std::atomic<int> wait_success_count{0};
    std::vector<std::thread> threads;

    // Half the threads wait on specific processes
    for (int i = 0; i < kNumProcesses / 2; ++i) {
        threads.emplace_back([&, i]() {
            if (GetProcessCreation()->WaitProcess(&results[i].handle, 5000)) {
                wait_success_count.fetch_add(1);
            }
            GetProcessCreation()->CloseHandle(&results[i].handle);
        });
    }

    // Remaining processes are waited from another set of threads
    for (int i = kNumProcesses / 2; i < kNumProcesses; ++i) {
        threads.emplace_back([&, i]() {
            if (GetProcessCreation()->WaitProcess(&results[i].handle, 5000)) {
                wait_success_count.fetch_add(1);
            }
            GetProcessCreation()->CloseHandle(&results[i].handle);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(wait_success_count.load(), kNumProcesses);
}

// Test fire-and-forget no_wait process — zombie reaped correctly
TEST_F(ProcessCreationThreadTest, NoWaitZombieReaping)
{
    constexpr int kNumProcesses = 16;

    for (int i = 0; i < kNumProcesses; ++i) {
        ProcessConfig config;
        config.program = "/bin/echo";
        config.args = {"no_wait_test"};
        config.no_wait = true;

        ProcessResult result = GetProcessCreation()->CreateProcess(config);
        ASSERT_TRUE(result.success) << "Failed to create no_wait process " << i;
        GetProcessCreation()->CloseHandle(&result.handle);
    }

    // Give ZombieControl time to reap processes
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify no zombies remain by trying to create more processes
    for (int i = 0; i < kNumProcesses; ++i) {
        ProcessConfig config;
        config.program = "/bin/echo";
        config.args = {"post_no_wait_test"};

        ProcessResult result = GetProcessCreation()->CreateProcess(config);
        ASSERT_TRUE(result.success) << "Failed to create post-no_wait process " << i;
        GetProcessCreation()->WaitProcess(&result.handle, 5000);
        GetProcessCreation()->CloseHandle(&result.handle);
    }
}

// Test orphaned process reaping — CloseHandle without WaitProcess
// Verifies ZombieControl sweep reaps orphaned PIDs correctly.
TEST_F(ProcessCreationThreadTest, OrphanedProcessReaping)
{
    ProcessConfig config;
    config.program = "/bin/sleep";
    config.args = {"0.1"};

    ProcessResult result = GetProcessCreation()->CreateProcess(config);
    ASSERT_TRUE(result.success);

    pid_t pid = result.handle.pid;
    GetProcessCreation()->CloseHandle(&result.handle);

	// Wait for child to exit (poll to avoid spurious failures under load)
	for (int attempts = 0; attempts < 100; ++attempts) {
		if (kill(pid, 0) == -1 && errno == ESRCH)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

    // Trigger sweep by creating another process (PutZombieUnderControl sweeps)
    ProcessConfig config2;
    config2.program = "/bin/echo";
    config2.args = {"trigger"};
    ProcessResult result2 = GetProcessCreation()->CreateProcess(config2);
    ASSERT_TRUE(result2.success);
    GetProcessCreation()->CloseHandle(&result2.handle);

    // Give sweep time to run
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify the original orphaned PID is fully reaped (not even a zombie)
    EXPECT_EQ(kill(pid, 0), -1);
    EXPECT_EQ(errno, ESRCH) << "PID " << pid << " should have been reaped by ZombieControl";
}
