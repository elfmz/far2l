#include "headers.hpp"
#include "GitTools.hpp"

#if defined(USELIBGIT2)
#include <codecvt>
#include <locale>

#include <git2.hpp>

static std::wstring_convert<std::codecvt_utf8<wchar_t>> Utf8Conv;
static git2::environment GitEnvironment;
#endif    // USELIBGIT2

FARString GetGitBranchName(FARString const &path)
{
    try {
#if defined(USELIBGIT2)
        auto pathStr = std::wstring_view{path.CPtr(), static_cast<size_t>(path.CEnd() - path.CPtr())};
        auto pathStrUtf8 = Utf8Conv.to_bytes(pathStr.begin(), pathStr.end());
        auto repo = git2::repository::try_discover(GitEnvironment, std::filesystem::path{pathStrUtf8});

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
    } catch (const std::exception &e) {
        return {};
    }
}