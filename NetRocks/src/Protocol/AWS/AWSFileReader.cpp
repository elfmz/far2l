#include "AWSFileReader.h"
#include <aws/s3/model/GetObjectRequest.h>

AWSFileReader::AWSFileReader(
        std::shared_ptr<Aws::S3::S3Client> client,
        const std::string& backet,
        const std::string& key,
        unsigned long long position,
        unsigned long long size
    ): _client(client), _backet(backet), _key(key), _position(position), _size(size)
{
    floatBufferSize = std::min((size - position) / 10, MAX_BUFFER);
    if (floatBufferSize == 0) {
        floatBufferSize = std::min(size - position, MAX_BUFFER);
    }
}

size_t AWSFileReader::Read(void *buf, size_t len)
{
    if (buffer.empty()) {
        if (_position + floatBufferSize > _size) {
            Download(_backet, _key, _position, _size - _position);
        } else {
            Download(_backet, _key, _position, floatBufferSize);
        }
    }
    size_t bytesRead = std::min(len, buffer.size());
    std::memcpy(buf, buffer.data(), bytesRead);
    buffer.erase(buffer.begin(), buffer.begin() + bytesRead);

    _position += bytesRead;
    return bytesRead;
}

size_t AWSFileReader::Download(std::string backet, std::string key, size_t offset, size_t len)
{
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(backet);
    request.SetKey(key);
    Aws::String range = "bytes=" + std::to_string(offset) + "-" + std::to_string(offset + len - 1);
    request.SetRange(range);

    auto outcome = _client->GetObject(request);
    if (!outcome.IsSuccess())
    {
        throw ProtocolError("Failed to get file: " + outcome.GetError().GetMessage());
    }

    auto &stream = outcome.GetResult().GetBody();
    size_t currentSize = buffer.size();
    buffer.resize(currentSize + len);
    stream.read(buffer.data() + currentSize, len);

    return stream.gcount();
}
