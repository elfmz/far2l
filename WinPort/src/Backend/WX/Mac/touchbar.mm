#import <Cocoa/Cocoa.h>
#include "touchbar.h"

#ifndef CONSOLE_FKEYS_COUNT
# define CONSOLE_FKEYS_COUNT 12
#endif

static ITouchbarListener *s_tb_listener = nullptr;

static NSTouchBarItemIdentifier s_tb_customization_identifier = @"com.Far2l.Touchbar.Customization";


//////////

@interface TBButton : NSButton
{
}


@property (readonly) NSSize intrinsicContentSize;

- (id)init;

@end


@implementation TBButton
@synthesize intrinsicContentSize = _intrinsicContentSize;


- (id)init
{
    self = [super init];
    if (self)
    {
        self->_intrinsicContentSize = NSMakeSize(0, 0);
    }

    return self;
}

@end

//////////

@interface Far2lTouchbarDelegate : NSResponder <NSTouchBarDelegate>
@end

@implementation Far2lTouchbarDelegate
NSMutableArray<NSTouchBarItemIdentifier> *key_identifiers;
NSMutableArray<NSTouchBarItemIdentifier> *all_identifiers;
NSColor *font_color;
NSColor *back_color;
TBButton *buttons[CONSOLE_FKEYS_COUNT];

- (instancetype)init
{
	if (self = [super init]) {
		key_identifiers = [NSMutableArray array];
		all_identifiers = [NSMutableArray array];
		font_color = [NSColor blackColor];
		back_color = [NSColor colorWithCalibratedRed:0 green:0.8 blue:0.8 alpha:1.0f];
		for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i)
		{
			NSString *key_id = [NSString stringWithFormat:@"com.Far2l.Touchbar.F%d", i + 1];
			[key_identifiers addObject:key_id ];
			[all_identifiers addObject:key_id ];
			if (i == 3 || i == 7) {
				[all_identifiers addObject:NSTouchBarItemIdentifierFixedSpaceSmall ];
			}

			buttons[i] = [[TBButton alloc] init];
			[buttons[i] setContentHuggingPriority:1.0 forOrientation:NSLayoutConstraintOrientationVertical];
			[buttons[i] setContentHuggingPriority:1.0 forOrientation:NSLayoutConstraintOrientationHorizontal];
			[buttons[i] setBordered:NO];
			[[buttons[i] cell] setBackgroundColor:back_color];
			[buttons[i] setAction:@selector(actionKey:)];
			[buttons[i] setTarget:self];

//			[self setButton:i Title:nullptr];
		}
	}
	return self;
}

- (NSTouchBar *) makeTouchBar
{
	NSTouchBar *bar = [[NSTouchBar alloc] init];

	bar.delegate = self;
	bar.defaultItemIdentifiers = all_identifiers;
	bar.customizationIdentifier = s_tb_customization_identifier;

	return bar;
}

- (IBAction) actionKey : (id) sender  {
	for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i) if ([sender isEqual:buttons[i]])
	{
		fprintf(stderr, "actionKey %d\n", i);
		if (s_tb_listener)
		{
			s_tb_listener->OnTouchbarKey(i);
		}
		return;
	}
	fprintf(stderr, "actionKey UNKNOWN\n");
}

- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
	for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i) if ([identifier isEqualToString:key_identifiers[i] ])
	{
	        NSCustomTouchBarItem *customItem =
			[[NSCustomTouchBarItem alloc] initWithIdentifier:key_identifiers[i]];
		customItem.view = buttons[i];
		customItem.visibilityPriority = NSTouchBarItemPriorityHigh;
//		[buttons[i] invalidateIntrinsicContentSize];
		return customItem;
	}

	return nil;
}

- (void)setButton:(int)index Title:(const char *)title
{
	NSString *ns_title = nullptr;
	if (title) {
		ns_title = [NSString stringWithUTF8String:title];

	} else {
		ns_title = [NSString stringWithFormat:@"F%d", index + 1];
	}

	NSMutableAttributedString *att_title = [[NSMutableAttributedString alloc] initWithString:ns_title];
	NSRange titleRange = NSMakeRange(0, [att_title length]);
	[att_title addAttribute:NSForegroundColorAttributeName value:font_color range:titleRange];

	[buttons[index] setAttributedTitle:att_title];
	[buttons[index] invalidateIntrinsicContentSize];
}


@end

static Far2lTouchbarDelegate *g_tb_delegate = nullptr;

void Touchbar_Register(ITouchbarListener *listener)
{
	s_tb_listener = listener;
	if (g_tb_delegate)
		return;

	if ([[NSApplication sharedApplication] respondsToSelector:@selector(isAutomaticCustomizeTouchBarMenuItemEnabled)])
	{
		[NSApplication sharedApplication].automaticCustomizeTouchBarMenuItemEnabled = YES;

		NSApplication *app = [NSApplication sharedApplication];
		if (app)
		{
			g_tb_delegate = [[Far2lTouchbarDelegate alloc] init];
			if (g_tb_delegate)
			{
				g_tb_delegate.nextResponder = app.nextResponder;
				app.nextResponder = g_tb_delegate;

				// set initial buttons titles
				Touchbar_SetTitles(nullptr);
			}
		}
	}
}

void Touchbar_Deregister()
{
	s_tb_listener = nullptr;
}

bool Touchbar_SetTitles(const char **titles)
{
	if (!g_tb_delegate)
		return false;

	for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i)
	{
		[g_tb_delegate setButton:i Title:(titles ? titles[i] : nullptr)];
	}

	return true;
}

