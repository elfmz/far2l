#import <Cocoa/Cocoa.h>
#include "touchbar.h"

static bool g_tb_created = false;
static ITouchbarListener *g_tb_listener = nullptr;
static NSTouchBarItemIdentifier TBFKeyIdentifiers[12] = {
	@"com.Far2l.Touchbar.F1", @"com.Far2l.Touchbar.F2", @"com.Far2l.Touchbar.F3", @"com.Far2l.Touchbar.F4",
	@"com.Far2l.Touchbar.F5", @"com.Far2l.Touchbar.F6", @"com.Far2l.Touchbar.F7", @"com.Far2l.Touchbar.F8",
	@"com.Far2l.Touchbar.F9", @"com.Far2l.Touchbar.F10", @"com.Far2l.Touchbar.F11", @"com.Far2l.Touchbar.F12"};

static NSTouchBarItemIdentifier TBCustomizationIdentifier = @"com.Far2l.Touchbar.Customization";
static NSButton *TBButtons[12];

@interface Far2lTouchbarDelegate : NSResponder <NSTouchBarDelegate>
@end

@implementation Far2lTouchbarDelegate

- (NSTouchBar *) makeTouchBar
{
	NSTouchBar *bar = [[[NSTouchBar alloc] init] autorelease];
	bar.delegate = self;
	// Set the default ordering of items.
	bar.defaultItemIdentifiers = @[TBFKeyIdentifiers[0], TBFKeyIdentifiers[1], TBFKeyIdentifiers[2], TBFKeyIdentifiers[3],
		TBFKeyIdentifiers[4], TBFKeyIdentifiers[5], TBFKeyIdentifiers[6], TBFKeyIdentifiers[7],
		TBFKeyIdentifiers[8], TBFKeyIdentifiers[9], TBFKeyIdentifiers[10], TBFKeyIdentifiers[11]  ];
	bar.customizationIdentifier = TBCustomizationIdentifier;
    
	return bar;
}

- (IBAction) actionFKey : (id) sender  {

	for (int i = 0; i < 12; ++i) if ([sender isEqual:TBButtons[i]]) {
		fprintf(stderr, "actionFKey %d\n", i);
		g_tb_listener->OnTouchbarFKey(i);
		return;
	}

	fprintf(stderr, "actionFKey UNKNOWN\n");
}

- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
	for (int i = 0; i < 12; ++i) if ([identifier isEqualToString:TBFKeyIdentifiers[i] ])
	{
		NSButton *btn_fkey = [[[NSButton alloc] init] autorelease];
		[btn_fkey setTitle: [[NSString stringWithFormat:@"F%u", i + 1] autorelease] ];
		TBButtons[i] = btn_fkey;
		[btn_fkey setContentHuggingPriority:1.0 forOrientation:NSLayoutConstraintOrientationVertical];
		[btn_fkey setContentHuggingPriority:1.0 forOrientation:NSLayoutConstraintOrientationHorizontal];
		[btn_fkey setAction:@selector(actionFKey:)];
		[btn_fkey setTarget:self];

		NSColor *color = [[NSColor colorWithCalibratedRed:0.0f green:1.0f blue:1.0f alpha:1.0f] autorelease];
		NSMutableAttributedString *colorTitle = [[[NSMutableAttributedString alloc] initWithAttributedString:[btn_fkey attributedTitle]] autorelease];
		NSRange titleRange = NSMakeRange(0, [colorTitle length]);
		[colorTitle addAttribute:NSForegroundColorAttributeName value:color range:titleRange];
		[btn_fkey setAttributedTitle:colorTitle];

	        NSCustomTouchBarItem *customItem =
			[[[NSCustomTouchBarItem alloc] initWithIdentifier:TBFKeyIdentifiers[i]] autorelease];
		customItem.view = btn_fkey;

        	// We want this label to always be visible no matter how many items are in the NSTouchBar instance.
	        customItem.visibilityPriority = NSTouchBarItemPriorityHigh;
        
        	return customItem;
	    }
    
    return nil;
}

@end


void Touchbar_Register(ITouchbarListener *listener)
{
	g_tb_listener = listener;
	if (g_tb_created)
		return;

	g_tb_created = true;

	if ([[NSApplication sharedApplication] respondsToSelector:@selector(isAutomaticCustomizeTouchBarMenuItemEnabled)])
	{
		[NSApplication sharedApplication].automaticCustomizeTouchBarMenuItemEnabled = YES;
	}

	NSApplication *app = [NSApplication sharedApplication];
	Far2lTouchbarDelegate *a = [[Far2lTouchbarDelegate alloc] init];
	a.nextResponder = app.nextResponder;
	app.nextResponder = a;
}

void Touchbar_Deregister()
{
	g_tb_listener = nullptr;
}
