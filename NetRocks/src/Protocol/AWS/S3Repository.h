#pragma once
#include <string>
#include <vector>
#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include "AWSFile.h"
#include "AWSFileWriter.h"
#include "AWSFileReader.h"

class S3Repository
{
private:

    class Path {
    public:
        Path(const std::string& path): _bucket(ParseBucket(path)), _key(ParseKey(path)) {}
        const std::string bucket() const { return _bucket; }
        const std::string key() const { return _key; }
        const std::string keyWithSlash() const { return _key + "/"; }
        bool hasKey() const { return !_key.empty(); }

    private:
        const std::string _bucket;
        const std::string _key;

        static std::string ParseBucket(const std::string& path) {
            auto pos = path.find('/');
            if (pos == std::string::npos) {
                return path;
            }
            return path.substr(0, pos);
        }

        static std::string ParseKey(const std::string& path) {
            std::string trimmedPath = path;
            while (!trimmedPath.empty() && trimmedPath.back() == '/') {
                trimmedPath.pop_back();
            }

            auto pos = trimmedPath.find('/');
            if (pos == std::string::npos) {
                return "";
            }

            return trimmedPath.substr(pos + 1);
        }
    };

    std::shared_ptr<Aws::S3::S3Client> _client;
    Aws::SDKOptions _options;

    bool IsFolder(const Path& localPath);


public:
    S3Repository(const std::string host, unsigned int port, const std::string accessKey, const std::string secret, const std::string &protocol_options);
    ~S3Repository();

    static std::string ExtractFileName(const std::string& path);

    std::vector<AWSFile> ListBuckets();
    std::vector<AWSFile> ListFolder(const std::string &path);

    AWSFile GetFileInfo(const std::string &path);

    std::shared_ptr<AWSFileReader> GetDownloader(const std::string& path, unsigned long long position, unsigned long long size);
    std::shared_ptr<AWSFileWriter> GetUploader(const std::string& path);

    void CreateBucket(const std::string &path);
    void CreateDirectory(const std::string &path);
    void DeleteDirectory(const std::string &path);
    void DeleteFile(const std::string& path);
};