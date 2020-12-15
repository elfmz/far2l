
#import <Cocoa/Cocoa.h>

static NSTouchBarItemIdentifier TBFKeyIdentifiers[12] = {
	@"com.Far2l.Touchbar.F1", @"com.Far2l.Touchbar.F2", @"com.Far2l.Touchbar.F3", @"com.Far2l.Touchbar.F4",
	@"com.Far2l.Touchbar.F5", @"com.Far2l.Touchbar.F6", @"com.Far2l.Touchbar.F7", @"com.Far2l.Touchbar.F8",
	@"com.Far2l.Touchbar.F9", @"com.Far2l.Touchbar.F10", @"com.Far2l.Touchbar.F11", @"com.Far2l.Touchbar.F12"};

static NSTouchBarItemIdentifier TBCustomizationIdentifier = @"com.Far2l.Touchbar.Customization";

@interface Far2lTouchbarDelegate : NSResponder <NSTouchBarDelegate>
@end

@implementation Far2lTouchbarDelegate

- (NSTouchBar *) makeTouchBar
{
	NSTouchBar *bar = [[NSTouchBar alloc] init];
	bar.delegate = self;
	// Set the default ordering of items.
	bar.defaultItemIdentifiers = @[TBFKeyIdentifiers[0], TBFKeyIdentifiers[1], TBFKeyIdentifiers[2], TBFKeyIdentifiers[3],
		TBFKeyIdentifiers[4], TBFKeyIdentifiers[5], TBFKeyIdentifiers[6], TBFKeyIdentifiers[7],
		TBFKeyIdentifiers[8], TBFKeyIdentifiers[9], TBFKeyIdentifiers[10], TBFKeyIdentifiers[11]  ];
	bar.customizationIdentifier = TBCustomizationIdentifier;
    
	return bar;
}

- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
	for (int i = 0; i < 12; ++i) if ([identifier isEqualToString:TBFKeyIdentifiers[i] ])
	{
		NSButton *btn_fkey = [[NSButton alloc] init];
		[btn_fkey setTitle: [NSString stringWithFormat:@"F%u", i + 1] ];

	        NSCustomTouchBarItem *customItem =
        	    [[NSCustomTouchBarItem alloc] initWithIdentifier:TBFKeyIdentifiers[i]];
	        customItem.view = btn_fkey;
        
        	// We want this label to always be visible no matter how many items are in the NSTouchBar instance.
	        customItem.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        	return customItem;
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
