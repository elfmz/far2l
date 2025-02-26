#include "lang.hpp"

int FirstMessageId = -1;
int LastMessageId = -1;

const char* MsgIds[] = {
	#ifndef BUILD_TYPE_RELEASE
	#define DECLARE_FARLANGMSG(NAME, ID) #NAME,
	#include "bootstrap/lang.inc"
	#undef DECLARE_FARLANGMSG
	"UnavailableInReleaseBuild"
	#endif
};

const char* GetStringId(int id) {
	#ifndef BUILD_TYPE_RELEASE
	if ((id >= 0) && (id < (int)(sizeof(MsgIds) / sizeof(MsgIds[0])))) {
	    return MsgIds[id];
	} else {
		return "UnknownMessage";
	}
	#else
	return MsgIds[0];
	#endif
}
