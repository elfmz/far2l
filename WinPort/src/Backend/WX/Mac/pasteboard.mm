#import <wchar.h>
#import <AppKit/NSPasteboard.h>
#import "pasteboard.h"

NSString *stringFromWchar(const wchar_t *charText) {
	return [[NSString alloc] initWithBytes:charText length:wcslen(charText)*sizeof(*charText) encoding:NSUTF32LittleEndianStringEncoding];
}

NSString *stringFromChar(const char *charText)
{
	return [NSString stringWithUTF8String:charText];
}

void CopyToPasteboard(const char* szText) {
	[[NSPasteboard generalPasteboard] clearContents];
//	[[NSPasteboard generalPasteboard] setString:[NSString stringWithUTF8String:szText] forType:NSPasteboardTypeString];
	[[NSPasteboard generalPasteboard] setString:stringFromChar(szText) forType:NSPasteboardTypeString];
}

void CopyToPasteboard(const wchar_t* wszText) {
	[[NSPasteboard generalPasteboard] clearContents];
	[[NSPasteboard generalPasteboard] setString:stringFromWchar(wszText) forType:NSPasteboardTypeString];
}
