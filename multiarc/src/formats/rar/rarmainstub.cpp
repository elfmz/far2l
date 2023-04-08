#include <vector>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <utils.h>

extern "C" int libarch_main(int numargs, char *args[]);

struct AutoArgs : std::vector<char *>
{
	AutoArgs()                            = default;
	AutoArgs(const AutoArgs &)            = delete;
	AutoArgs &operator=(const AutoArgs &) = delete;

	FN_NOINLINE void Add(const char *sz, size_t pos)
	{
		assert(pos <= size());
		char *copied_sz = strdup(sz);
		if (!copied_sz) {
			throw std::bad_alloc();
		}

		insert(begin() + pos, copied_sz);
	}

	void Add(const char *sz) { Add(sz, size()); }

	~AutoArgs()
	{
		for (auto p : *this) {
			free(p);
		}
	}
};

int rar_main(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}
	std::vector<char *> plain_args;
	for (int i = 0; i < argc; ++i) {
		plain_args.emplace_back(argv[i]);
	}
	plain_args.emplace_back(nullptr);
	//  plain_args[0] = "unrar";
	execvp("unrar", plain_args.data());

#ifdef HAVE_LIBARCHIVE
	fprintf(stderr, "'unrar' tool not found, falling back to libarchive\n");

	try {
		AutoArgs la_args;
		la_args.Add("libarch");
		switch (argv[1][0]) {
			case 'x':
				la_args.Add("X");
				break;
			case 'e':
				la_args.Add("x");
				break;
			case 't':
				la_args.Add("t");
				break;
			default:
				fprintf(stderr, "Bad argv[1]: '%s'\n", argv[1]);
				return -1;
		}

		int i;

		std::string tmp;
		for (i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "--") == 0) {
				++i;
				break;
			}
			if (strncmp(argv[i], "-p", 2) == 0) {
				tmp = "-pwd=";
				tmp+= &argv[i][2];
				la_args.Add(tmp.c_str());
			}
			if (strncmp(argv[i], "-ap", 3) == 0) {
				tmp = "-@=";
				tmp+= &argv[i][3];
				la_args.Add(tmp.c_str());
			}
			if (argv[i][0] == '-' && argv[i][1] >= '0' && argv[i][1] <= '9' && !argv[i][2]) {
				la_args.Add(argv[i]);
			}
		}
		la_args.Add("--");

		if (i + 1 >= argc) {
			fprintf(stderr, "Nothing to extract\n");
			return 0;
		}

		la_args.Add(argv[i], 2);
		++i;

		if (i + 1 == argc && argv[i][0] == '@') {
			std::ifstream favis(&argv[i][1]);
			if (!favis.is_open()) {
				fprintf(stderr, "Can't open '%s'\n", &argv[i][1]);
				return 1;
			}
			std::string line;
			while (std::getline(favis, line)) {
				StrTrimRight(line, "\r\n");
				if (!line.empty()) {
					la_args.Add(line.c_str());
				}
			}
		} else
			for (; i < argc; ++i) {
				la_args.Add(argv[i]);
			}

		return libarch_main((int)la_args.size(), la_args.data());

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
		return -1;
	}
#else
	fprintf(stderr, "Please install 'unrar' tool\n");
	return -1;
#endif
}
