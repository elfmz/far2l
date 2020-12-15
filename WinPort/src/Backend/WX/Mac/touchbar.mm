
#import <Cocoa/Cocoa.h>

static NSTouchBarItemIdentifier TBTestLabelIdentifier = @"com.Far2l.Touchbar.Label";
static NSTouchBarItemIdentifier TBCustomizationIdentifier = @"com.Far2l.Touchbar.Customization";

@interface Far2lTouchbarDelegate : NSResponder <NSTouchBarDelegate>
@end

@implementation Far2lTouchbarDelegate

- (NSTouchBar *) makeTouchBar
{
	NSTouchBar *bar = [[NSTouchBar alloc] init];
	bar.delegate = self;
	// Set the default ordering of items.
	bar.defaultItemIdentifiers = @[TBTestLabelIdentifier];
	bar.customizationIdentifier = TBCustomizationIdentifier;
    
	return bar;
}

- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{

    if ([identifier isEqualToString:TBTestLabelIdentifier])
    {
        NSTextField *theLabel = [NSTextField labelWithString:@"test"];
        
        NSCustomTouchBarItem *customItemForLabel =
            [[NSCustomTouchBarItem alloc] initWithIdentifier:TBTestLabelIdentifier];
        customItemForLabel.view = theLabel;
        
        // We want this label to always be visible no matter how many items are in the NSTouchBar instance.
        customItemForLabel.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        return customItemForLabel;
    }
    
    return nil;
}

@end


void StartupTouchbar()
{
	if ([[NSApplication sharedApplication] respondsToSelector:@selector(isAutomaticCustomizeTouchBarMenuItemEnabled)])
	{
		[NSApplication sharedApplication].automaticCustomizeTouchBarMenuItemEnabled = YES;
	}

	NSApplication *app = [NSApplication sharedApplication];
	Far2lTouchbarDelegate *a = [[Far2lTouchbarDelegate alloc] init];
	a.nextResponder = app.nextResponder;
	app.nextResponder = a;
}
