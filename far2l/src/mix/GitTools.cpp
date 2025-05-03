#include "headers.hpp"
#include "GitTools.hpp"

#if defined(USELIBGIT2)
#	include <git2.hpp>

static git2::environment GitEnvironment;
#endif // USELIBGIT2

FARString GetGitBranchName(FARString const& path)
{
#if defined(USELIBGIT2)
	auto repo = git2::repository::try_discover(GitEnvironment, std::filesystem::path{path.CPtr(), path.CEnd()});

	if (!repo)
		return {};

	auto head = repo->try_get_head();
	if (!head)
		return {};

	auto result = FARString{};
	result.Append("{");
	result.Append(head->shorthand_cstr());
	result.Append("} ");
	return result;
#else // !USELIBGIT2
	return {};
#endif // USELIBGIT2
}