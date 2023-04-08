#include "MultiArc.hpp"

struct FarStandardFunctions FSF;
struct Options Opt;
struct PluginStartupInfo Info;

class ArcPlugins *ArcPlugin = NULL;

const char *CmdNames[] = {"Extract", "ExtractWithoutPath", "Test", "Delete", "Comment", "CommentFiles", "SFX",
		"Lock", "Protect", "Recover", "Add", "Move", "AddRecurse", "MoveRecurse", "AllFilesMask", "DefExt"};

#ifdef _NEW_ARC_SORT_
char IniFile[MAX_PATH];
const char *SortModes[] = {"None", "Name", "RunRate", "ChoiceRate", "User"};
#endif	//_NEW_ARC_SORT_
