#pragma once

#include "../Protocol.h"
#include <aws/s3/S3Client.h>


class AWSFileReader : public IFileReader
{
private:
    std::shared_ptr<Aws::S3::S3Client> _client;
	std::string _backet;
	std::string _key;
	unsigned long long _position;
	unsigned long long _size;
	std::vector<char> buffer;
	static constexpr unsigned long long MAX_BUFFER = 10 * 1024 * 1024;
	unsigned long long floatBufferSize;

    size_t Download(std::string backet, std::string key, size_t offset, size_t len);

public:
	AWSFileReader(
        std::shared_ptr<Aws::S3::S3Client> client,
        const std::string& backet,
        const std::string& key,
        unsigned long long position,
        unsigned long long size
    );

	virtual size_t Read(void *buf, size_t len);
};

