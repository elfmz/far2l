#pragma once
#include <vector>
#include <string>
#include <map>

struct PathParts : std::vector<std::string>
{
	void Traverse(const std::string &path);

	std::string Join() const;
	bool Starts(PathParts &root) const;
};

///

template <class PathNodeAttributesT> struct PathNode;

template <class PathNodeAttributesT>
	struct PathNode : PathNodeAttributesT, std::map<std::string, PathNode<PathNodeAttributesT> >
{
	PathNode<PathNodeAttributesT> *Find(PathParts::const_iterator root, PathParts::const_iterator edge)
	{
		if (root == edge)
			return this;

		auto it = std::map<std::string, PathNode<PathNodeAttributesT> >::find(*root);
		if (it == std::map<std::string, PathNode<PathNodeAttributesT> >::end())
			return nullptr;

		++root;
		return it->second.Find(root, edge);
	}

	PathNode<PathNodeAttributesT> *Ensure(PathParts::const_iterator root, PathParts::const_iterator edge)
	{
		if (root == edge)
			return this;

		auto &subnode = std::map<std::string, PathNode<PathNodeAttributesT> >::operator[](*root);

		++root;
		return subnode.Ensure(root, edge);
	}

	void Clear()
	{
		std::map<std::string, PathNode<PathNodeAttributesT> >::clear();
		PathNodeAttributesT::operator =(PathNodeAttributesT());
	}
};

