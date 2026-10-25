#import <wchar.h>
#import <AppKit/NSPasteboard.h>
#import "pasteboard.h"

static NSString *stringFromWchar(const wchar_t *charText, size_t cnt) {
	return [[NSString alloc] initWithBytes:charText length:cnt*sizeof(*charText) encoding:NSUTF32LittleEndianStringEncoding];
}

static NSString *stringFromChar(const char *charText, size_t cnt)
{
	return [NSString stringWithBytes:charText length:cnt encoding:NSUTF8StringEncoding];
}

void CopyToPasteboard(const char* szText, size_t cnt) {
	[[NSPasteboard generalPasteboard] clearContents];
	[[NSPasteboard generalPasteboard] setString:stringFromChar(szText, cnt) forType:NSPasteboardTypeString];
}

void CopyToPasteboard(const wchar_t* wszText, size_t cnt) {
	[[NSPasteboard generalPasteboard] clearContents];
	[[NSPasteboard generalPasteboard] setString:stringFromWchar(wszText, cnt) forType:NSPasteboardTypeString];
}
