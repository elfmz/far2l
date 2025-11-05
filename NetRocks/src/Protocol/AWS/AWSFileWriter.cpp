#include "AWSFileWriter.h"

AWSFileWriter::AWSFileWriter(const std::string& bucket, const std::string& key, const std::shared_ptr<Aws::S3::S3Client> client)
    : _bucket(bucket), _key(key), _client(client), partNumber(1)
{
    StartMultipartUpload();
}

AWSFileWriter::~AWSFileWriter()
{
    try {
        CompleteMultipartUpload();
    } catch (const std::exception& e) {
        AbortMultipartUpload();
    }
}

void AWSFileWriter::StartMultipartUpload() {
    Aws::S3::Model::CreateMultipartUploadRequest request;
    request.SetBucket(_bucket);
    request.SetKey(_key);

    auto outcome = _client->CreateMultipartUpload(request);
    if (!outcome.IsSuccess()) {
        throw ProtocolError("Failed upload: " + outcome.GetError().GetMessage());
    }

    uploadId = outcome.GetResult().GetUploadId();
}

void AWSFileWriter::Write(const void* buf, size_t len) {
    const char* data = static_cast<const char*>(buf);
    buffer.insert(buffer.end(), data, data + len);

    if (buffer.size() >= maxPartSize) {
        UploadPart();
    }
}

void AWSFileWriter::WriteComplete()
{
    CompleteMultipartUpload();
}

void AWSFileWriter::UploadPart() {
    if (buffer.empty()) {
        return;
    }

    Aws::S3::Model::UploadPartRequest request;
    request.SetBucket(_bucket);
    request.SetKey(_key);
    request.SetUploadId(uploadId);
    request.SetPartNumber(partNumber);

    auto stream = Aws::MakeShared<Aws::StringStream>("S3MultipartWriter");
    stream->write(buffer.data(), buffer.size());
    stream->flush();

    request.SetBody(stream);
    request.SetContentLength(static_cast<long>(buffer.size()));

    auto outcome = _client->UploadPart(request);
    if (!outcome.IsSuccess()) {
        throw ProtocolError(outcome.GetError().GetMessage());
    }

    Aws::S3::Model::CompletedPart part;
    part.SetETag(outcome.GetResult().GetETag());
    part.SetPartNumber(partNumber);
    completedParts.push_back(part);

    ++partNumber;

    buffer.clear();
}

void AWSFileWriter::CompleteMultipartUpload() {
    if (!buffer.empty()) {
        UploadPart();
    }

    Aws::S3::Model::CompleteMultipartUploadRequest request;
    request.SetBucket(_bucket);
    request.SetKey(_key);
    request.SetUploadId(uploadId);

    Aws::S3::Model::CompletedMultipartUpload completedUpload;
    completedUpload.SetParts(completedParts);
    request.SetMultipartUpload(completedUpload);

    auto outcome = _client->CompleteMultipartUpload(request);
    if (!outcome.IsSuccess()) {
        throw ProtocolError("Failed: " + outcome.GetError().GetMessage());
    }
}

void AWSFileWriter::AbortMultipartUpload() {
    Aws::S3::Model::AbortMultipartUploadRequest request;
    request.SetBucket(_bucket);
    request.SetKey(_key);
    request.SetUploadId(uploadId);

    _client->AbortMultipartUpload(request);
}
