#include "AWSFile.h"

AWSFile::AWSFile(std::string name, bool isFile): name(name), isFile(isFile)
{
    modified.tv_sec = 0;
    modified.tv_nsec = 0;
}
AWSFile::AWSFile(std::string name, bool isFile, const Aws::Utils::DateTime &date, long long size): name(name), isFile(isFile)
{
    modified.tv_sec = static_cast<time_t>(date.Seconds());
    modified.tv_nsec = 0;
    this->size = size;
}

void AWSFile::UpdateModification(const Aws::Utils::DateTime &date)
{
    auto t = static_cast<time_t>(date.Seconds());
    if (t > modified.tv_sec) {
        modified.tv_sec = t;
    }
}
