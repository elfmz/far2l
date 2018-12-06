#pragma once
#include <vector>
#include <stdexcept>
#include <string>

class StackSerializer
{
	std::vector<unsigned char> _data;

public:
	StackSerializer() = default;

	StackSerializer(const char *str, size_t len);
	StackSerializer(const std::string &str);

	void FromBase64(const char *str, size_t len);
	void FromBase64(const std::string &str);

	void ToBase64(std::string &str) const;
	std::string ToBase64() const;

	void Clear();

	void Swap(StackSerializer &other);

	////////////

	void Push(const void *ptr, size_t len);
	void Pop(void *ptr, size_t len);

	///

	template <class POD>
		void PushPOD(const POD &p)
	{
		Push(&p, sizeof(p));
	}

	template <class POD>
		void PopPOD(POD &p)
	{
		Pop(&p, sizeof(p));
	}

	///

	void PushStr(const std::string &str);
	void PopStr(std::string &str);
	std::string PopStr();
	char PopChar();
	uint8_t PopU8();
	uint16_t PopU16();
	uint32_t PopU32();

};
