#pragma once
#include "fileholder.hpp"


class GreppedFileHolder : public FileHolder
{
	FileHolderPtr _parent;
	bool _grepped{false};

public:
	GreppedFileHolder(FileHolderPtr &parent);
	virtual ~GreppedFileHolder();

	FileHolderPtr ParentFileHolder();
	bool Grep();
};
