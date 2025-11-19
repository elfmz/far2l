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
#include <aws/core/client/AWSError.h>
#include <aws/s3/S3Errors.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include "../Protocol.h"

S3Repository::S3Repository(const std::string host, unsigned int port, const std::string accessKey, const std::string secret, const std::string &protocolOptions)
{
    Aws::InitAPI(_options);

	StringConfig options(protocolOptions);
	auto useragent = options.GetString("UserAgent");
	auto region = options.GetString("Region");

	Aws::Client::ClientConfiguration config;
    if ((port > 0) && (port != 433)) {
        config.endpointOverride = Trim(host) + ":" + std::to_string(port);
    } else {
	    config.endpointOverride = Trim(host);
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

	if (!region.empty()) {
    	config.region = region;
    }

	auto endpointProvider = Aws::MakeShared<Aws::S3::S3EndpointProvider>(Aws::Endpoint::DEFAULT_ENDPOINT_PROVIDER_TAG);

    if (accessKey.empty() && secret.empty()) {
        auto credentialsProvider = Aws::Auth::DefaultAWSCredentialsProviderChain();
        auto credentials = credentialsProvider.GetAWSCredentials();

        if (credentials.GetAWSAccessKeyId().empty() || credentials.GetAWSSecretKey().empty()) {
            throw ProtocolError("Invalid credentals");
        }
        _client = std::make_shared<Aws::S3::S3Client>(config, endpointProvider);
    } else if (accessKey.empty()) {
        throw ProtocolError("Invalid login/access key");
    } else if (secret.empty()) {
        throw ProtocolError("Invalid password/secret");
    } else {
        Aws::Auth::AWSCredentials credentials(accessKey, secret);
        _client = std::make_shared<Aws::S3::S3Client>(credentials, endpointProvider, config);
    }
}

S3Repository::~S3Repository()
{
    Aws::ShutdownAPI(_options);
}

std::string S3Repository::Trim(const std::string& str) {
    const std::string unwanted = "/\\ ";
    auto result = str;

    size_t start = result.find_first_not_of(unwanted);
    if (start == std::string::npos) {
        result.clear();
        return result;
    }
    result.erase(0, start);

    size_t end = result.find_last_not_of(unwanted);
    result.erase(end + 1);

    return result;
}


std::string S3Repository::GetS3ErrorTypeString(Aws::S3::S3Errors errorType)
{
    switch (errorType) {
        case Aws::S3::S3Errors::INCOMPLETE_SIGNATURE: return "INCOMPLETE_SIGNATURE";
        case Aws::S3::S3Errors::INTERNAL_FAILURE: return "INTERNAL_FAILURE";
        case Aws::S3::S3Errors::INVALID_ACTION: return "INVALID_ACTION";
        case Aws::S3::S3Errors::INVALID_CLIENT_TOKEN_ID: return "INVALID_CLIENT_TOKEN_ID";
        case Aws::S3::S3Errors::INVALID_PARAMETER_COMBINATION: return "INVALID_PARAMETER_COMBINATION";
        case Aws::S3::S3Errors::INVALID_QUERY_PARAMETER: return "INVALID_QUERY_PARAMETER";
        case Aws::S3::S3Errors::INVALID_PARAMETER_VALUE: return "INVALID_PARAMETER_VALUE";
        case Aws::S3::S3Errors::MISSING_ACTION: return "MISSING_ACTION";
        case Aws::S3::S3Errors::MISSING_AUTHENTICATION_TOKEN: return "MISSING_AUTHENTICATION_TOKEN";
        case Aws::S3::S3Errors::MISSING_PARAMETER: return "MISSING_PARAMETER";
        case Aws::S3::S3Errors::OPT_IN_REQUIRED: return "OPT_IN_REQUIRED";
        case Aws::S3::S3Errors::REQUEST_EXPIRED: return "REQUEST_EXPIRED";
        case Aws::S3::S3Errors::SERVICE_UNAVAILABLE: return "SERVICE_UNAVAILABLE";
        case Aws::S3::S3Errors::THROTTLING: return "THROTTLING";
        case Aws::S3::S3Errors::VALIDATION: return "VALIDATION";
        case Aws::S3::S3Errors::ACCESS_DENIED: return "ACCESS_DENIED";
        case Aws::S3::S3Errors::RESOURCE_NOT_FOUND: return "RESOURCE_NOT_FOUND";
        case Aws::S3::S3Errors::UNRECOGNIZED_CLIENT: return "UNRECOGNIZED_CLIENT";
        case Aws::S3::S3Errors::MALFORMED_QUERY_STRING: return "MALFORMED_QUERY_STRING";
        case Aws::S3::S3Errors::SLOW_DOWN: return "SLOW_DOWN";
        case Aws::S3::S3Errors::REQUEST_TIME_TOO_SKEWED: return "REQUEST_TIME_TOO_SKEWED";
        case Aws::S3::S3Errors::INVALID_SIGNATURE: return "INVALID_SIGNATURE";
        case Aws::S3::S3Errors::SIGNATURE_DOES_NOT_MATCH: return "SIGNATURE_DOES_NOT_MATCH";
        case Aws::S3::S3Errors::INVALID_ACCESS_KEY_ID: return "INVALID_ACCESS_KEY_ID";
        case Aws::S3::S3Errors::REQUEST_TIMEOUT: return "REQUEST_TIMEOUT";
        case Aws::S3::S3Errors::NETWORK_CONNECTION: return "NETWORK_CONNECTION";
        case Aws::S3::S3Errors::BUCKET_ALREADY_EXISTS: return "BUCKET_ALREADY_EXISTS";
        case Aws::S3::S3Errors::BUCKET_ALREADY_OWNED_BY_YOU: return "BUCKET_ALREADY_OWNED_BY_YOU";
        case Aws::S3::S3Errors::ENCRYPTION_TYPE_MISMATCH: return "ENCRYPTION_TYPE_MISMATCH";
        case Aws::S3::S3Errors::INVALID_OBJECT_STATE: return "INVALID_OBJECT_STATE";
        case Aws::S3::S3Errors::INVALID_REQUEST: return "INVALID_REQUEST";
        case Aws::S3::S3Errors::INVALID_WRITE_OFFSET: return "INVALID_WRITE_OFFSET";
        case Aws::S3::S3Errors::NO_SUCH_BUCKET: return "NO_SUCH_BUCKET";
        case Aws::S3::S3Errors::NO_SUCH_KEY: return "NO_SUCH_KEY";
        case Aws::S3::S3Errors::NO_SUCH_UPLOAD: return "NO_SUCH_UPLOAD";
        case Aws::S3::S3Errors::OBJECT_ALREADY_IN_ACTIVE_TIER: return "OBJECT_ALREADY_IN_ACTIVE_TIER";
        case Aws::S3::S3Errors::OBJECT_NOT_IN_ACTIVE_TIER: return "OBJECT_NOT_IN_ACTIVE_TIER";
        case Aws::S3::S3Errors::TOO_MANY_PARTS: return "TOO_MANY_PARTS";
        default: return "UNKNOWN_ERROR";
    }
}


ProtocolError S3Repository::ConstructProtocolError(const Aws::Client::AWSError<Aws::S3::S3Errors>& error, const std::string& context)
{
    std::string errorMessage;

    if (!context.empty()) {
        errorMessage += context + ": ";
    }

    auto msg = error.GetMessage();
    if (msg.empty()) {
        errorMessage += GetS3ErrorTypeString(error.GetErrorType());
    } else {
        errorMessage += msg;
    }

    return ProtocolError(errorMessage);
}

std::vector<AWSFile> S3Repository::ListBuckets()
{
    Aws::S3::Model::ListBucketsRequest request;
    Aws::String continuationToken;
    std::vector<AWSFile> ls;

    // ListBuckets only returns the first 1000 buckets, so we need to handle pagination using continuation tokens.
    do {
        if (!continuationToken.empty()) {
            request.SetContinuationToken(continuationToken);
        }
        auto outcome = _client->ListBuckets(request);
        if (outcome.IsSuccess()) {
            const auto& buckets = outcome.GetResult().GetBuckets();
            for (const auto& bucket : buckets) {
                ls.push_back(AWSFile(bucket.GetName(), false, bucket.GetCreationDate(), 0));
            }
            continuationToken = outcome.GetResult().GetContinuationToken();
        } else {
            throw ConstructProtocolError(outcome.GetError(), "List buckets");
        }
    } while (!continuationToken.empty());

    return ls;
}

std::vector<AWSFile> S3Repository::ListFolder(const std::string &path)
{
    Aws::S3::Model::ListObjectsV2Request request;
    Aws::String continuationToken;
    size_t prefixLen = 0;

    auto localPath = Path(path);
    request.SetBucket(localPath.bucket());
    if (localPath.hasKey()) {
        request.SetPrefix(localPath.keyWithSlash());
        prefixLen = localPath.keyWithSlash().length();
    }

    request.SetDelimiter("/"); // Group common prefixes (directories)

    std::vector<AWSFile> ls;
    // ListObjectsV2 only returns the first 1000 objects, so we need to handle pagination using continuation tokens.
    do {
        if (!continuationToken.empty()) {
            request.SetContinuationToken(continuationToken);
        }

        auto outcome = _client->ListObjectsV2(request);

        if (outcome.IsSuccess()) {
            const auto& contents = outcome.GetResult().GetContents();  // Get the "files" under the prefix.

            for (const auto& object : contents) {
                Aws::String key = object.GetKey().substr(prefixLen);
                ls.push_back(AWSFile(key, true, object.GetLastModified(), object.GetSize()));
            }

            auto commonPrefixes = outcome.GetResult().GetCommonPrefixes(); // Get the "directories" under the prefix.
            for (const auto& prefix : commonPrefixes) {
                Aws::String dir = prefix.GetPrefix().substr(prefixLen);
                dir.erase(dir.size() - 1); // Remove trailing slash
                if (!dir.empty()) {
                    ls.push_back(AWSFile(dir, false, Aws::Utils::DateTime(), 0));
                }
            }

            continuationToken = outcome.GetResult().GetNextContinuationToken();

        } else {
            throw ConstructProtocolError(outcome.GetError(), "List dir");
        }
    } while (!continuationToken.empty());

    return ls;
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
        return AWSFile(ExtractFileName(localPath.key()), false); // No modification date or size for folders
    }
    Aws::S3::Model::HeadObjectRequest request;
	request.SetBucket(localPath.bucket());
	request.SetKey(localPath.key());

    auto outcome = _client->HeadObject(request);

    if (outcome.IsSuccess()) {
        const auto& object = outcome.GetResult();
        return AWSFile(ExtractFileName(path), true, object.GetLastModified(), object.GetContentLength());
	} else {
        throw ConstructProtocolError(outcome.GetError(), "Access denied");
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
        throw ConstructProtocolError(outcome.GetError(), "Create bucket");
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
        throw ConstructProtocolError(outcome.GetError(), "Create dir");
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
            throw ConstructProtocolError(outcome.GetError());
        }
    }

    if (localPath.key().empty()) {
        Aws::S3::Model::DeleteBucketRequest request;
        request.SetBucket(localPath.bucket());
        auto outcome = _client->DeleteBucket(request);
        if (!outcome.IsSuccess()) {
            throw ConstructProtocolError(outcome.GetError());
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
        throw ConstructProtocolError(outcome.GetError());
    }
}

