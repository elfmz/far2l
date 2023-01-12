#import <Foundation/Foundation.h>
#import <AppKit/NSRunningApplication.h>

void MacHide()
{
	[[NSRunningApplication currentApplication] hide];
}

