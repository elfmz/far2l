#include "unicode/uchar.h"
#include "unicode/utypes.h"
#include "unicode/stringoptions.h"
#include "stdio.h"

/// Usage:
/// g++ -O2 ./CharClasses_mk.cpp -o /tmp/CharClasses_mk -licuuc && /tmp/CharClasses_mk > CharClasses.cpp

template <class FN>
	static void WriteFunc(const char *name, FN fn, bool checkLength)
{
	UChar32 c, last = 0x10ffff;
	UChar32 start = 0;
	printf("bool %s(wchar_t c)\n", name);
	printf("{\n");
	printf("\tswitch (c) {\n");
	for (c = 1; c <= last + 1; ++c) {
		const bool matched = (c <= last) && fn(c);
		if (matched) {
			if (!start) {
				start = c;
			}

		} else if (start) {
			if (start + 2 == c) {
				printf("\t\tcase 0x%x: case 0x%x:\n", (unsigned int)start, (unsigned int)c - 1);
			} else if (start + 1 < c) {
				printf("\t\tcase 0x%x ... 0x%x:\n", (unsigned int)start, (unsigned int)c - 1);
			} else {
				//printf("(c == 0x%x)\n", (unsigned int)start);
				printf("\t\tcase 0x%x:\n", (unsigned int)start);
			}
			start = 0;
		}
	}
	if (checkLength) {
		printf("\t\t\treturn (wcwidth(c) == 0);\n");
	} else {
		printf("\t\t\treturn (wcwidth(c) == 2);\n");
	}
	printf("\t\tdefault: return false;\n");
	printf("\t}\n");
	printf("}\n\n");
}

int main()
{
//	printf("%u\n", u_getIntPropertyValue(0xcbe, UCHAR_GENERAL_CATEGORY));
//	return -1;
	UChar32 c, last = 0x10ffff;
	UChar32 unstable_start = 0;
	bool first = true;
	printf("// this file autogenerated by CharClasses_mk.cpp\n\n");
	printf("#include <wchar.h>\n\n");

	WriteFunc("IsCharFullWidth", [](wchar_t c)->bool {
		const auto ea_width = u_getIntPropertyValue(c, UCHAR_EAST_ASIAN_WIDTH);
		return ea_width == U_EA_FULLWIDTH || ea_width == U_EA_WIDE;
	}, false);

	WriteFunc("IsCharPrefix", [](wchar_t c)->bool {
		const auto jt = u_getIntPropertyValue(c, UCHAR_JOINING_TYPE);
		const auto cat = u_getIntPropertyValue(c, UCHAR_GENERAL_CATEGORY);
		return (cat == U_SURROGATE || jt == U_JT_RIGHT_JOINING);
	}, true);

	WriteFunc("IsCharSuffix", [](wchar_t c)->bool {
		const auto block = u_getIntPropertyValue(c, UCHAR_BLOCK);
		const auto jt = u_getIntPropertyValue(c, UCHAR_JOINING_TYPE);
		const auto cat = u_getIntPropertyValue(c, UCHAR_GENERAL_CATEGORY);
		return ( (jt != U_JT_NON_JOINING && jt != U_JT_TRANSPARENT && jt != U_JT_RIGHT_JOINING && jt != U_JT_DUAL_JOINING)
			|| cat == U_NON_SPACING_MARK || cat == U_COMBINING_SPACING_MARK
			|| block == UBLOCK_COMBINING_DIACRITICAL_MARKS
			|| block == UBLOCK_COMBINING_MARKS_FOR_SYMBOLS
			|| block == UBLOCK_COMBINING_HALF_MARKS
			|| block == UBLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT
			|| block == UBLOCK_COMBINING_DIACRITICAL_MARKS_EXTENDED);
	}, true);

	printf("bool IsCharXxxfix(wchar_t c)\n");
	printf("{\n");
	printf("\treturn IsCharPrefix(c) || IsCharSuffix(c);\n");
	printf("}\n");
}
