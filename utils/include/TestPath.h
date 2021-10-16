#pragma once

class TestPath
{
	bool _out;

public:
	enum Mode
	{
		EXISTS,
		DIRECTORY,
		REGULAR,
		EXECUTABLE,
	};

	TestPath(const char *path, Mode m = EXISTS);

	inline operator bool() const { return _out; }
};
