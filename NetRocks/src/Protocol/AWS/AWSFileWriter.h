#pragma once
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/s3/model/UploadPartRequest.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/AbortMultipartUploadRequest.h>
#include <vector>
#include <iostream>
#include "../Protocol.h"

class AWSFileWriter : public IFileWriter
{
public:
    AWSFileWriter(const std::string& bucket, const std::string& key, const std::shared_ptr<Aws::S3::S3Client> client);
    ~AWSFileWriter();

private:
    const std::string _bucket;
    const std::string _key;
    std::shared_ptr<Aws::S3::S3Client> _client;
    std::string uploadId;
    std::vector<char> buffer;
    const size_t maxPartSize = 5 * 1024 * 1024;
    int partNumber;
    std::vector<Aws::S3::Model::CompletedPart> completedParts;

    void StartMultipartUpload();
    void UploadPart();
    void AbortMultipartUpload();
    void CompleteMultipartUpload();

public:
    virtual void Write(const void* buf, size_t len);
    virtual void WriteComplete();
};
