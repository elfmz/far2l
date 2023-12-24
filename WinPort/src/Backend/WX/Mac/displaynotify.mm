#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

bool MacDisplayNotify(const char *title, const char *text)
{
	NSUserNotification *userNotification = [[NSUserNotification alloc] init];
	userNotification.title = [NSString stringWithUTF8String:title];
	userNotification.informativeText = [NSString stringWithUTF8String:text];

	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:userNotification]; //deliverNotification
	return true;
}
