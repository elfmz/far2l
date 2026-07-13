#include <gtest/gtest.h>
#include <ShellCommand.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// RAII wrapper that creates a private temp directory (mkdtemp) and
// allocates files inside it.  Avoids S5443 — hardcoded /tmp is
// world-writable and a symlink-race target in tests.
class TempFile {
	char _dir[64];
	char _path[128];
	bool _owns;
public:
	TempFile(const char *name) : _owns(false) {
		_dir[0] = _path[0] = '\0';
		snprintf(_dir, sizeof(_dir), "/tmp/far2l-test-XXXXXX");
		if (!mkdtemp(_dir)) return;
		_owns = true;
		snprintf(_path, sizeof(_path), "%s/%s", _dir, name);
	}
	~TempFile() {
		if (!_owns) return;
		unlink(_path);
		rmdir(_dir);
	}
	const char *path() const { return _path; }
	bool valid() const { return _dir[0] != '\0'; }
};

class ShellCommandTest : public ::testing::Test
{
protected:
	void SetUp() override {}
	void TearDown() override {}
};

TEST_F(ShellCommandTest, ExecuteEcho)
{
	ShellCommandResult result = ShellCommand::Execute("echo hello");
	EXPECT_TRUE(result.success);
	EXPECT_EQ(result.exit_code, 0);
	EXPECT_NE(result.stdout_output.find("hello"), std::string::npos);
}

TEST_F(ShellCommandTest, ExecuteExitCode)
{
	ShellCommandResult result = ShellCommand::Execute("exit 42");
	EXPECT_TRUE(result.success);
	EXPECT_EQ(result.exit_code, 42);
}

TEST_F(ShellCommandTest, ExecuteStderrCapture)
{
	ShellCommandResult result = ShellCommand::Execute("echo err >&2");
	EXPECT_TRUE(result.success);
	EXPECT_EQ(result.exit_code, 0);
	EXPECT_NE(result.stderr_output.find("err"), std::string::npos);
}

TEST_F(ShellCommandTest, ExecuteWithInput)
{
	ShellCommandResult result = ShellCommand::ExecuteWithInput("cat", "test data");
	EXPECT_TRUE(result.success);
	EXPECT_EQ(result.exit_code, 0);
	EXPECT_NE(result.stdout_output.find("test data"), std::string::npos);
}

TEST_F(ShellCommandTest, ExecuteToFile)
{
	TempFile tf("shellcmd-output");
	ASSERT_TRUE(tf.valid());

	bool ok = ShellCommand::ExecuteToFile("echo file_output", tf.path());
	EXPECT_TRUE(ok);

	std::ifstream in(tf.path());
	ASSERT_TRUE(in.is_open());
	std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();

	EXPECT_NE(content.find("file_output"), std::string::npos);
}

TEST_F(ShellCommandTest, ExecuteToFileFailureAppendsError)
{
	TempFile tf("shellcmd-fail");
	ASSERT_TRUE(tf.valid());

	std::string error_message;
	bool ok = ShellCommand::ExecuteToFile("/nonexistent_command_12345", tf.path(), true, &error_message);
	EXPECT_FALSE(ok);
	EXPECT_FALSE(error_message.empty());

	std::ifstream in(tf.path());
	ASSERT_TRUE(in.is_open());
	std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();

	EXPECT_FALSE(content.empty());
}

TEST_F(ShellCommandTest, ExecuteInvalidCommand)
{
	ShellCommandResult result = ShellCommand::Execute("/nonexistent_command_12345");
	// /bin/sh itself spawns successfully, but the command within exits with non-zero
	EXPECT_TRUE(result.success);
	EXPECT_NE(result.exit_code, 0);
}
