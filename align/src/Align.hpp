struct InitDialogItem
{
	unsigned char Type;
	unsigned char X1, Y1, X2, Y2;
	unsigned char Focus;
	DWORD_PTR Selected;
	unsigned int Flags;
	unsigned char DefaultButton;
	const TCHAR *Data;
};

const TCHAR *GetMsg(int MsgId);
void InitDialogItems(const struct InitDialogItem *Init, struct FarDialogItem *Item, int ItemsNumber);

static struct PluginStartupInfo Info;
struct FarStandardFunctions FSF;
