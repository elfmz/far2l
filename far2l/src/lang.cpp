#include "lang.hpp"

int LastMessageId = -1;

const char* MsgIds[] = {
	#define DECLARE_FARLANGMSG(NAME, ID) #NAME,
	#include "bootstrap/lang.inc"
	#undef DECLARE_FARLANGMSG
};

const char* GetStringId(int id) {
    return MsgIds[id];
}
