#include "PathParts.h"
#include "utils.h"

void PathParts::Traverse(const std::string &path)
{
	size_t i = size();
	StrExplode(*this, path, "/");
	while (i < size()) {
		if (operator[](i) == ".") {
			erase(begin() + i);

		} else if (operator[](i) == "..") {
			erase(begin() + i);
			if (i != 0) {
				erase(begin() + i - 1);
				--i;
			} else {
				fprintf(stderr, "PathParts::Traverse: impossible <..> in '%s'\n", path.c_str());
			}
		} else {
			++i;
		}
	}
}

bool PathParts::Starts(PathParts &root) const
{
	if (size() < root.size()) {
		return false;
	}

	for (size_t i = 0; i != root.size(); ++i) {
		if (operator[](i) != root[i]) {
			return false;
		}
	}

	return true;
}

std::string PathParts::Join() const
{
	std::string out;
	for (const auto &p : *this) {
		if (!out.empty()) {
			out+= '/';
		}
		out+= p;
	}
	return out;
}
