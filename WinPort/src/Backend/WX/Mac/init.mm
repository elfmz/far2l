#import <Cocoa/Cocoa.h>
#include <AvailabilityMacros.h>

#include <string>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

void MacInit()
{
	NSString *language = [[NSLocale currentLocale] localeIdentifier];
	if (language) {
		char sz[64] = {0};
		const char *lang = [language UTF8String];
		fprintf(stderr, "MacInit: lang='%s'\n", lang);
		const char *env = getenv("LANG");
		if (!env || !*env) {
			snprintf(sz, sizeof(sz) - 1, "%s.UTF-8", lang);
			setenv("LANG", sz, 1);
		}
	} else {
		fprintf(stderr, "MacInit: no lang\n");
	}
}

