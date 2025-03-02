#include "lang.hpp"

int FirstMessageId = -1;
int LastMessageId = -1;

const char* MsgIds[] = {
	#ifndef BUILD_TYPE_RELEASE
	#define DECLARE_FARLANGMSG(NAME, ID) #NAME,
	#include "bootstrap/lang.inc"
	#undef DECLARE_FARLANGMSG
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
	return "UnavailableInReleaseBuild";
	#endif
}

void FarLangMsg::UpdateLastMessageId(int id) const {
	if (
		(id != Msg::Yes.ID()) &&
		(id != Msg::HYes.ID()) &&
		(id != Msg::No.ID()) &&
		(id != Msg::HNo.ID()) &&
		(id != Msg::Ok.ID()) &&
		(id != Msg::HOk.ID()) &&
		(id != Msg::Cancel.ID()) &&
		(id != Msg::HCancel.ID())
	) {
		if (FirstMessageId == -1)
			FirstMessageId = id;
		LastMessageId = id; 
	}
}
