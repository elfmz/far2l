#include "headers.hpp"
#include "GitTools.hpp"

FARString GetGitBranchName(FARString const &path)
{
    try {
        std::string cmd = "git -C \"";
        cmd += EscapeCmdStr(path.GetMB());
        cmd += "\" rev-parse --abbrev-ref HEAD";

        std::string branchName;
        if (!POpen(branchName, cmd.c_str()))
            return {};

        StrTrim(branchName, " \t\r\n");
        if (branchName.empty())
            return {};

        auto result = FARString{};
        result.Append("{");
        result.Append(branchName);
        result.Append("} ");
        return result;
    } catch (const std::exception &e) {
        return {};
    }
}