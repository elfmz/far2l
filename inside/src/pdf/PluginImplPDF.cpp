#include "Globals.h"
#include "PluginImplPDF.h"


#define STR_PLAIN_TXT		"plain.txt"

PluginImplPDF::PluginImplPDF(const char *name)
	: PluginImpl(name)
{
	fprintf(stderr, "Inside::PluginImplPDF('%s')\n", name);
}


bool PluginImplPDF::OnGetFindData(FP_SizeItemList &il, int OpMode)
{
	if (_dir.empty()) {
		return AddUnsized(il, STR_PLAIN_TXT, 0);
	}

	return false;
}

int pdftotext_main(int argc, const char *argv[]);

bool PluginImplPDF::OnGetFile(const char *item_file, const char *data_path, uint64_t len)
{
	if (strcmp(item_file, STR_PLAIN_TXT) == 0) {
		const char *argv[] = {"pdftotext", "-enc", "UTF-8", _name.c_str(), data_path, NULL};
		return pdftotext_main(5, argv) == 0;
	}

	return false;
}


bool PluginImplPDF::OnPutFile(const char *item_file, const char *data_path)
{
	return false;
}

bool PluginImplPDF::OnDeleteFile(const char *item_file)
{
	return false;
}
