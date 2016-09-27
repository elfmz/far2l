#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <string.h>

int main_inserter(int argc, char** argv)
{
	if(argc != 4)
	{
		const auto NamePtr = strrchr(argv[0],  '/');
		std::cout << "Usage:\n" << (NamePtr? NamePtr+1 : argv[0]) << " input_template_file output_template_file new_lng_file" << std::endl;
		return -1;
	}

	const std::string InFeedName = argv[1], OutFeedName = argv[2], LngName = argv[3];
	
	std::ifstream Feed(InFeedName), Lng(LngName);

	std::cout << "Reading " << LngName << std::endl;

	std::string LngHeader;
	std::getline(Lng, LngHeader);

	std::list<std::string> LngLines;

	std::string Buffer;
	while(!Lng.eof())
	{
		std::getline(Lng, Buffer);
		if(!Buffer.compare(0, 1, "\""))
		{
			LngLines.push_back(Buffer);
		}
	}

	std::cout << "Reading " << InFeedName << std::endl;

	std::list<std::string> FeedLines;

	while(!Feed.eof())
	{
		getline(Feed, Buffer);
		FeedLines.push_back(Buffer);
	}

	size_t ConstsCount = 0;
	for(auto i = FeedLines.begin(); i != FeedLines.end(); ++i)
	{
		// assume that all constants starts with 'M'.
		if(!i->compare(0, 1, "M"))
		{
			++ConstsCount;
		}
	}

	if(ConstsCount != LngLines.size())
	{
		std::cerr << "Error: lines count mismatch: " << InFeedName << " - " << ConstsCount << ", " << LngName << " - " << LngLines.size() << std::endl;
		return -1;
	}

	if(FeedLines.back().empty())
	{
		FeedLines.pop_back();
	}

	auto Ptr = FeedLines.begin();

	if(!Ptr->compare(0, 14, "\xef\xbb\xbfm4_include("))
	{
		++Ptr;
	}

#define SKIP_EMPTY_LINES_AND_COMMENTS while(Ptr->empty() || !Ptr->compare(0, 1, "#")) {++Ptr;}

	SKIP_EMPTY_LINES_AND_COMMENTS

	// skip header name
	++Ptr;

	SKIP_EMPTY_LINES_AND_COMMENTS

	std::stringstream strStream(*Ptr);
	size_t Num;
	strStream >> Num;
	std::cout << Num << " languages found." << std::endl;

	auto NumPtr = Ptr;
	++Ptr;

	bool Update = false;
	size_t UpdateIndex = Num;

	for(size_t i = 0; i < Num; ++i, ++Ptr)
	{
		SKIP_EMPTY_LINES_AND_COMMENTS
		if(!Ptr->compare(0, LngName.length(), LngName))
		{
			Update = true;
			UpdateIndex = i;
			break;
		}
	}

	if(Update)
	{
		std::cout << LngName << " already exist (id == " << UpdateIndex << "). Updating." << std::endl;
	}
	else
	{
		std::cout << "Inserting new language (id == " << UpdateIndex << ") from " << LngName << std::endl;
		strStream.clear();
		strStream << Num+1;
		*NumPtr = strStream.str();

		std::string ShortLngName = LngHeader.substr(LngHeader.find(L'=', 0)+1);
		ShortLngName.resize(ShortLngName.find(L','), 0);

		std::string FullLngName = LngHeader.substr(LngHeader.find(L',', 0)+1);
		FeedLines.insert(Ptr, LngName+" " + ShortLngName + " \"" + FullLngName + "\"");
	}

	for(auto i = LngLines.begin(); i != LngLines.end(); ++i)
	{
		// assume that all constants start with 'M'.
		while(Ptr->compare(0, 1, "M"))
		{
			++Ptr;
		}
		++Ptr;

		for(size_t j = 0; j < UpdateIndex || !UpdateIndex; ++j)
		{
			while(Ptr != FeedLines.end() && Ptr->compare(0, 1, "\"") && Ptr->compare(0, 5, "upd:\""))
			{
				++Ptr;
			}
			if(!UpdateIndex)
			{
				break;
			}
			++Ptr;
		}

		if(Update)
		{
			const char* Str = Ptr->c_str();
			size_t l = Ptr->length();
			if(!Ptr->compare(0, 4, "upd:"))
			{
				Str += 4;
				l -= 4;
			}
			if(i->compare(0, l, Str))
			{
				*Ptr = *i;
			}
		}
		else
		{
			FeedLines.insert(Ptr, *i);
		}
	}

	std::cout << "Writing to " << OutFeedName << std::endl;

	std::ofstream oFeed(OutFeedName);

	for(auto i = FeedLines.begin(); i != FeedLines.end(); ++i)
	{
		oFeed << *i << L'\n';
	}

	std::cout << "Done." << std::endl;

	return 0;
}
