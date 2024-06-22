#include <string>
#include "utils.h"
#include "crc64.h"

void FilePathHashSuffix(std::string &pathname)
{
	uint64_t suffix = 0xC001000CAC4E;

	size_t p = pathname.rfind('/');
	if (p != std::string::npos) {
		suffix = crc64(suffix,
			(const unsigned char *)pathname.c_str(), p);
		pathname.erase(0, p + 1);
	}

	p = pathname.rfind('.');
	if (p != std::string::npos) {
		suffix = crc64(suffix,
			(const unsigned char *)pathname.c_str() + p, pathname.size() - p);
		pathname.resize(p);
	}

	for (auto &c : pathname) {
		if (c == '#' || c == ';' || c == '[' || c == ']'
				|| c == '\r' || c == '\n' || c == '\t' || c == ' ') {
			suffix = crc64(suffix, (const unsigned char *)&c, 1);
			c = '_';
		}
	}

	pathname+= ToPrefixedHex(suffix, "@");
}
