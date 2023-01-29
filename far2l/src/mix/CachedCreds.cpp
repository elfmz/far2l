#include "headers.hpp"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <utils.h>
#include "FARString.hpp"

static std::string UncachedComputerName()
{
	char buf[0x100] = {};
	if (gethostname(&buf[0], ARRAYSIZE(buf) - 1) == -1) {
		perror("GetComputerNameUncached - gethostname");
		return std::string();
	}
	return buf;
}

static std::string UncachedUserName()
{
	struct passwd *pw = getpwuid(getuid());
	if (!pw || !pw->pw_name) {
		perror("GetUserNameUncached - getpwuid");
		return std::string();
	}
	return pw->pw_name;
}

//

const FARString &CachedComputerName()
{
	static FARString s_out = UncachedComputerName();
	return s_out;
}

const FARString &CachedUserName()
{
	static FARString s_out = UncachedUserName();
	return s_out;
}

const FARString &CachedHomeDir()
{
	static FARString s_out = GetMyHome();
	return s_out;
}
