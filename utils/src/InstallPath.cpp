#include "utils.h"
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>

#if defined(__FreeBSD__) || defined(__DragonFly__)
# include <malloc_np.h>
#elif __APPLE__
# include <malloc/malloc.h>
#else
# include <malloc.h>
#endif

template <class C>
	static bool TranslateInstallPathT(std::basic_string<C> &path, const C *dir_from, const C *dir_to, const C* prefix)
{
	if (!prefix || !*prefix)
		return false;

	const size_t prefix_len = tzlen(prefix);
	const size_t dir_from_len = tzlen(dir_from);

	if (path.size() < (prefix_len + dir_from_len))
		return false;

	if (memcmp(path.c_str(), prefix, prefix_len * sizeof(C)) != 0)
		return false;

	if (memcmp(path.c_str() + prefix_len, dir_from, dir_from_len * sizeof(C)) != 0)
		return false;

	if (path.size() > (prefix_len + dir_from_len) && path[prefix_len + dir_from_len] != GOOD_SLASH)
		return false;

	path.replace(prefix_len, dir_from_len, dir_to);
	return true;
}

typedef const char *(*tGetPathTranslationPrefixA)();
typedef const wchar_t *(*tGetPathTranslationPrefix)();

static bool TranslateInstallPath(std::wstring &path, const wchar_t *dir_from, const wchar_t *dir_to)
{
	static tGetPathTranslationPrefix pGetPathTranslationPrefix =
		(tGetPathTranslationPrefix)dlsym(RTLD_DEFAULT, "GetPathTranslationPrefix");

	return TranslateInstallPathT(path, dir_from, dir_to, pGetPathTranslationPrefix());
}

static bool TranslateInstallPath(std::string &path, const char *dir_from, const char *dir_to)
{
	static tGetPathTranslationPrefixA pGetPathTranslationPrefixA =
		(tGetPathTranslationPrefixA)dlsym(RTLD_DEFAULT, "GetPathTranslationPrefixA");

	return TranslateInstallPathT(path, dir_from, dir_to, pGetPathTranslationPrefixA());
}

bool TranslateInstallPath_Bin2Share(std::wstring &path)
{
	return (TranslateInstallPath(path, L"bin", L"share") || TranslateInstallPath(path, L"sbin", L"share"));
}

bool TranslateInstallPath_Bin2Share(std::string &path)
{
	return (TranslateInstallPath(path, "bin", "share") || TranslateInstallPath(path, "sbin", "share"));
}

bool TranslateInstallPath_Lib2Share(std::wstring &path)
{
	return TranslateInstallPath(path, L"lib", L"share");
}

bool TranslateInstallPath_Lib2Share(std::string &path)
{
	return TranslateInstallPath(path, "lib", "share");
}

bool TranslateInstallPath_Share2Lib(std::wstring &path)
{
	return TranslateInstallPath(path, L"share", L"lib");
}

bool TranslateInstallPath_Share2Lib(std::string &path)
{
	return TranslateInstallPath(path, "share", "lib");
}

bool TranslateInstallPath_Bin2Lib(std::string &path)
{
	return TranslateInstallPath(path, "bin", "lib/" APP_BASENAME);
}
