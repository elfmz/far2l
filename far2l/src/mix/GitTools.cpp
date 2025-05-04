#include "headers.hpp"
#include "GitTools.hpp"

#if defined(USELIBGIT2)
#include <git2.hpp>

static git2::environment GitEnvironment;
#endif    // USELIBGIT2

FARString GetGitBranchName(FARString const &path)
{
#if defined(USELIBGIT2)
    auto repo =
            git2::repository::try_discover(GitEnvironment, std::filesystem::path{path.CPtr(), path.CEnd()});

    if (!repo)
        return {};

    auto head = repo->try_get_head();
    if (!head)
        return {};

    auto const branchName = head->shorthand_cstr();
#else     // !USELIBGIT2
    std::string cmd = "git -C \"";
    cmd += EscapeCmdStr(path.GetMB());
    cmd += "\" rev-parse --abbrev-ref HEAD";

    std::string branchName;
    if (!POpen(branchName, cmd.c_str()))
        return {};

    StrTrim(branchName, " \t\r\n");
    if (branchName.empty())
        return {};
#endif    // USELIBGIT2

    auto result = FARString{};
    result.Append("{");
    result.Append(branchName);
    result.Append("} ");
    return result;
}