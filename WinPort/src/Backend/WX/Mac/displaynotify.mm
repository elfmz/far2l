#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <AvailabilityMacros.h>

bool MacDisplayNotify(const char *title, const char *text)
{
	NSString *nsTitle = [NSString stringWithUTF8String:title];
	NSString *nsText  = [NSString stringWithUTF8String:text];

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
	NSUserNotification *userNotification = [[NSUserNotification alloc] init];
	userNotification.title = nsTitle;
	userNotification.informativeText = nsText;

	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:userNotification]; //deliverNotification
#else
	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:nsTitle];
	[alert setInformativeText:nsText];
	[alert addButtonWithTitle:@"OK"];
	[alert runModal];
#endif
	return true;
}
