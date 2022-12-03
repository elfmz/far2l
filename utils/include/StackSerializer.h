#pragma once
#include <vector>
#include <stdexcept>
#include <string>
#include "BitTwiddle.hpp"

class StackSerializer
{
	std::vector<unsigned char> _data;
public:
	StackSerializer();
	StackSerializer(const char *str, size_t len);
	StackSerializer(const std::string &str);

	void FromBase64(const char *str, size_t len);
	void FromBase64(const std::string &str);

	void ToBase64(std::string &str) const;
	std::string ToBase64() const;

	bool IsEmpty() const;
	void Clear();

	void Swap(StackSerializer &other);

	////////////

	void Push(const void *ptr, size_t len);
	void Pop(void *ptr, size_t len);

	///

	template <class POD>
		void PushNum(const POD &p)
	{
		const auto px = LITEND(p);
		Push(&px, sizeof(px));
	}

	template <class POD>
		void PopNum(POD &p)
	{
		Pop(&p, sizeof(p));
		p = LITEND(p);
	}

	///

	void PushStr(const char *str);
	void PushStr(const std::string &str);
	void PopStr(std::string &str);
	std::string PopStr();
	char PopChar();
	uint8_t PopU8();
	uint16_t PopU16();
	uint32_t PopU32();
};
