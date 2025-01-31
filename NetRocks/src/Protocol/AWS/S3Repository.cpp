#include "S3Repository.h"
#include <StringConfig.h>
#include <aws/core/Aws.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListBucketsRequest.h>
#include <aws/s3/model/ListBucketsResult.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ListObjectsV2Result.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/endpoint/DefaultEndpointProvider.h>
#include "../Protocol.h"

S3Repository::S3Repository(const std::string host, unsigned int port, const std::string accessKey, const std::string secret, const std::string &protocolOptions)
{
    Aws::InitAPI(_options);

	StringConfig options(protocolOptions);
	auto useragent = options.GetString("UserAgent");
	auto region = options.GetString("Region");

	Aws::Client::ClientConfiguration config;
    if ((port > 0) && (port != 433)) {
        config.endpointOverride = host + ":" + std::to_string(port);
    } else {
	    config.endpointOverride = host;
    }

	if (options.GetInt("UseProxy", 0) != 0) {
        config.proxyHost = options.GetString("ProxyHost");
        config.proxyPort = options.GetInt("ProxyPort");
		if (options.GetInt("AuthProxy", 0) != 0) {
			config.proxyUserName = options.GetString("ProxyUsername");
			config.proxyPassword = options.GetString("ProxyPassword");
		}
	}

	if (!useragent.empty()) {
		config.userAgent = useragent;
	}

	if (region.empty()) {
    	config.region = "us-east-1";
    } else {
    	config.region = region;
    }

	Aws::Auth::AWSCredentials credentials(accessKey, secret);
	auto endpointProvider = Aws::MakeShared<Aws::S3::S3EndpointProvider>(Aws::Endpoint::DEFAULT_ENDPOINT_PROVIDER_TAG);
    _client = std::make_shared<Aws::S3::S3Client>(credentials, endpointProvider, config);
}

S3Repository::~S3Repository()
{
    Aws::ShutdownAPI(_options);
}

std::vector<AWSFile> S3Repository::ListBuckets()
{
    Aws::S3::Model::ListBucketsRequest request;
    auto outcome = _client->ListBuckets();
    if (outcome.IsSuccess()) {
        const auto& buckets = outcome.GetResult().GetBuckets();
        std::vector<AWSFile> ls;
        for (const auto& bucket : buckets) {
            ls.push_back(AWSFile(bucket.GetName(), false, bucket.GetCreationDate(), 0));
        }
        return ls;
    } else {
        throw ProtocolError("Directory open error", (outcome.GetError().GetMessage()).c_str());
    }
}

std::vector<AWSFile> S3Repository::ListFolder(const std::string &path)
{
    Aws::S3::Model::ListObjectsV2Request request;
    size_t prefixLen = 0;

    auto localPath = Path(path);
    request.SetBucket(localPath.bucket());
    if (localPath.hasKey()) {
        request.SetPrefix(localPath.keyWithSlash());
        prefixLen = localPath.keyWithSlash().length();
    }

    auto outcome = _client->ListObjectsV2(request);

    if (outcome.IsSuccess()) {
        const auto& contents = outcome.GetResult().GetContents();

        std::vector<AWSFile> ls;
        std::map<Aws::String, AWSFile> folders;
        for (const auto& object : contents) {
            Aws::String key = object.GetKey().substr(prefixLen);
            size_t last_slash_pos = key.find_last_of("/");
            if (last_slash_pos == std::string::npos) {
                ls.push_back(AWSFile(key, true, object.GetLastModified(), object.GetSize()));
            } else {
                size_t pos = key.find('/');
                if (pos != Aws::String::npos) {
                    auto dir = key.substr(0, pos);
                    auto result = folders.emplace( 
                        dir, 
                        AWSFile(dir, false, object.GetLastModified(), object.GetSize())
                    );

                    if (!result.second) {
                        result.first->second.size += object.GetSize();
                        result.first->second.UpdateModification(object.GetLastModified());
                    }
                }
            }

        }

        for (const auto& pair : folders) {
            ls.push_back(pair.second);
        }

        return ls;

    } else {
        throw ProtocolError("Directory open error", (path + outcome.GetError().GetMessage()).c_str());
    }

}

bool S3Repository::IsFolder(const Path& localPath) {
    Aws::S3::Model::ListObjectsV2Request listRequest;
    listRequest.SetBucket(localPath.bucket());
    listRequest.SetPrefix(localPath.keyWithSlash());
    listRequest.SetMaxKeys(1);

    auto listOutcome = _client->ListObjectsV2(listRequest);
    if (listOutcome.IsSuccess()) {
        return !listOutcome.GetResult().GetContents().empty();
    }

    return false;
}

AWSFile S3Repository::GetFileInfo(const std::string &path)
{
    Path localPath(path);

    if (IsFolder(localPath) || localPath.key().empty()) {
        Aws::S3::Model::ListObjectsV2Request request;
        request.SetBucket(localPath.bucket());
        request.SetPrefix(localPath.keyWithSlash());

        auto outcome = _client->ListObjectsV2(request);
        if (outcome.IsSuccess()) {
            auto result = AWSFile(ExtractFileName(localPath.key()), false);
            const auto& contents = outcome.GetResult().GetContents();
            for (const auto& object : contents) {
                result.size += object.GetSize();
                result.UpdateModification(object.GetLastModified());
            }
            return result;

        } else {
            throw ProtocolError("Cannot access directory: ", (path + outcome.GetError().GetMessage()).c_str());
        }
    }
		
    Aws::S3::Model::HeadObjectRequest request;
	request.SetBucket(localPath.bucket());
	request.SetKey(localPath.key());

    auto outcome = _client->HeadObject(request);

    if (outcome.IsSuccess()) {
        const auto& object = outcome.GetResult();
        return AWSFile(ExtractFileName(path), true, object.GetLastModified(), object.GetContentLength());
	} else {
		throw ProtocolError("Cannot access file: ", (path + outcome.GetError().GetMessage()).c_str());
	}

}

std::string S3Repository::ExtractFileName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1); 
}

std::shared_ptr<AWSFileWriter> S3Repository::GetUploader(const std::string& path) 
{
    Path localPath(path);
    return std::make_shared<AWSFileWriter>(localPath.bucket(), localPath.key(), _client);
}

std::shared_ptr<AWSFileReader> S3Repository::GetDownloader(const std::string& path, unsigned long long position, unsigned long long size)
{
    Path localPath(path);
    return std::make_shared<AWSFileReader>(_client, localPath.bucket(), localPath.key(), position, size);
}

void S3Repository::CreateBucket(const std::string& bucket)
{
    Aws::S3::Model::CreateBucketRequest request;
    request.SetBucket(bucket);
    auto outcome = _client->CreateBucket(request);

    if (!outcome.IsSuccess()) {
        throw ProtocolError("Failed to create directory: ", outcome.GetError().GetMessage().c_str());
    }
}

void S3Repository::CreateDirectory(const std::string& path)
{
    Path localPath(path);
    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(localPath.bucket());
    request.SetKey(localPath.keyWithSlash());
    request.SetBody(Aws::MakeShared<Aws::StringStream>(""));
    auto outcome = _client->PutObject(request);

    if (!outcome.IsSuccess()) {
        throw ProtocolError("Failed to create directory: ", outcome.GetError().GetMessage().c_str());
    }
}

void S3Repository::DeleteDirectory(const std::string& path)
{
    Path localPath(path);
    std::string folderKey = localPath.keyWithSlash();

    for (auto val : ListFolder(path)) {
        Aws::S3::Model::DeleteObjectRequest request;
        request.SetBucket(localPath.bucket());
        request.SetKey(folderKey + val.name);
        auto outcome = _client->DeleteObject(request);
        if (!outcome.IsSuccess()) {
            throw ProtocolError("Failed to delete directory: ", outcome.GetError().GetMessage().c_str());
        }
    }

    if (localPath.key().empty()) {
        Aws::S3::Model::DeleteBucketRequest request;
        request.SetBucket(localPath.bucket());
        auto outcome = _client->DeleteBucket(request);
        if (!outcome.IsSuccess()) {
            throw ProtocolError("Failed to delete backet: ", outcome.GetError().GetMessage().c_str());
        }
    }
}

void S3Repository::DeleteFile(const std::string& path)
{
    Path localPath(path);
    Aws::S3::Model::DeleteObjectRequest request;
    request.SetBucket(localPath.bucket());
    request.SetKey(localPath.key());
    auto outcome = _client->DeleteObject(request);
    if (!outcome.IsSuccess()) {
        throw ProtocolError("Failed to delete file: ", outcome.GetError().GetMessage().c_str());
    }
}

