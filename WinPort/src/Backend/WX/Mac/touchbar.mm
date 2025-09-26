#import <Cocoa/Cocoa.h>
#include <AvailabilityMacros.h>

#include "touchbar.h"

#if defined(MAC_OS_X_VERSION_10_12) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12

#include <string>

#ifndef CONSOLE_FKEYS_COUNT
# define CONSOLE_FKEYS_COUNT 12
#endif

static ITouchbarListener *s_tb_listener = nullptr;
static bool s_tb_alternate = false;

static NSTouchBarItemIdentifier s_tb_customization_identifier = @"com.Far2l.Touchbar.Customization";


//////////

@interface TBButton : NSButton
@property (readonly) NSSize intrinsicContentSize;
- (id)init;
@end

@implementation TBButton {
	NSSize _intrinsicContentSize;
}

@synthesize intrinsicContentSize = _intrinsicContentSize;

- (id)init
{
	self = [super init];
	if (self)
	{
		_intrinsicContentSize = NSMakeSize(0, 0);
	}
	return self;
}

@end

//////////

@interface Far2lTouchbarDelegate : NSResponder <NSTouchBarDelegate>

// MRC-safe: use assign instead of weak
@property (assign) NSResponder *savedNextResponder;

@end

@implementation Far2lTouchbarDelegate {
	NSMutableArray<NSTouchBarItemIdentifier> *key_identifiers;
	NSMutableArray<NSTouchBarItemIdentifier> *all_identifiers;
	NSColor *font_color;
	NSColor *back_color;
	TBButton *buttons[CONSOLE_FKEYS_COUNT];
}

- (instancetype)init
{
	if (self = [super init]) {
		key_identifiers = [[NSMutableArray alloc] init];
		all_identifiers = [[NSMutableArray alloc] init];
		font_color = [NSColor blackColor];
		back_color = [NSColor colorWithCalibratedRed:0 green:0.8 blue:0.8 alpha:1.0f];

		for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i)
		{
			NSString *key_id = [NSString stringWithFormat:@"com.Far2l.Touchbar.F%d", i + 1];
			[key_identifiers addObject:key_id];
			[all_identifiers addObject:key_id];

			if (i == 3 || i == 7) {
				[all_identifiers addObject:NSTouchBarItemIdentifierFixedSpaceSmall];
			}

			buttons[i] = [[TBButton alloc] init];
			[buttons[i] setContentHuggingPriority:1.0 forOrientation:NSLayoutConstraintOrientationVertical];
			[buttons[i] setContentHuggingPriority:1.0 forOrientation:NSLayoutConstraintOrientationHorizontal];
			[buttons[i] setContentCompressionResistancePriority:1.0 forOrientation:NSLayoutConstraintOrientationVertical];
			[buttons[i] setBordered:NO];
			[[buttons[i] cell] setBackgroundColor:back_color];
			[buttons[i] setAction:@selector(actionKey:)];
			[buttons[i] setTarget:self];
		}
	}
	return self;
}

- (NSTouchBar *)makeTouchBar
{
	NSTouchBar *bar = [[NSTouchBar alloc] init];
	bar.delegate = self;
	bar.defaultItemIdentifiers = all_identifiers;
	bar.customizationIdentifier = s_tb_customization_identifier;
	return bar;
}

- (IBAction)actionKey:(id)sender {
	for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i) {
		if ([sender isEqual:buttons[i]]) {
			fprintf(stderr, "actionKey %s%d\n", s_tb_alternate ? "alternate_" : "", i);
			if (s_tb_listener) {
				s_tb_listener->OnTouchbarKey(s_tb_alternate, i);
			}
			return;
		}
	}
	fprintf(stderr, "actionKey UNKNOWN\n");
}

- (nullable NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
	for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i) {
		if ([identifier isEqualToString:key_identifiers[i]]) {
			NSCustomTouchBarItem *customItem =
				[[NSCustomTouchBarItem alloc] initWithIdentifier:key_identifiers[i]];
			customItem.view = buttons[i];
			customItem.visibilityPriority = NSTouchBarItemPriorityHigh;
			return customItem;
		}
	}
	return nil;
}

- (void)setButton:(int)index Title:(const char *)title
{
	NSString *ns_title = nil;
	if (title) {
		ns_title = [NSString stringWithUTF8String:title];
	} else {
		ns_title = [NSString stringWithFormat:@"F%d", index + 1];
	}

	NSMutableAttributedString *att_title =
		[[NSMutableAttributedString alloc] initWithString:ns_title];
	NSRange titleRange = NSMakeRange(0, [att_title length]);
	[att_title addAttribute:NSForegroundColorAttributeName value:font_color range:titleRange];

	[buttons[index] setAttributedTitle:att_title];
	[buttons[index] invalidateIntrinsicContentSize];
}

@end

static Far2lTouchbarDelegate *g_tb_delegate = nullptr;
static std::string s_titles_normal[CONSOLE_FKEYS_COUNT];
static const char *s_titles_alternate[CONSOLE_FKEYS_COUNT] = {
	"", "Ins", "Del", "", "+", "-", "*", "/", "Home", "End", "PageUp", "PageDown"
};

static void Touchbar_ApplyTitlesNormal()
{
	for (size_t i = 0; i < CONSOLE_FKEYS_COUNT; ++i) {
		const char *title = s_titles_normal[i].c_str();
		if (*title) {
			++title;
		} else {
			title = nullptr;
		}
		[g_tb_delegate setButton:(int)i Title:title];
	}
}

static void Touchbar_ApplyTitlesAlternate()
{
	for (size_t i = 0; i < CONSOLE_FKEYS_COUNT; ++i) {
		[g_tb_delegate setButton:(int)i Title:s_titles_alternate[i]];
	}
}

static void Touchbar_ApplyTitles()
{
	if (s_tb_alternate) {
		Touchbar_ApplyTitlesAlternate();
	} else {
		Touchbar_ApplyTitlesNormal();
	}
}

////

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
				g_tb_delegate.savedNextResponder = app.nextResponder;
				app.nextResponder = g_tb_delegate;
				// set initial buttons titles
				Touchbar_ApplyTitles();
			}
		}
	}
}

void Touchbar_Deregister()
{
	if (g_tb_delegate) {
		NSApplication *app = [NSApplication sharedApplication];
		if (app && g_tb_delegate.savedNextResponder) {
			app.nextResponder = g_tb_delegate.savedNextResponder;
		}
	}
	s_tb_listener = nullptr;
}

bool Touchbar_SetTitles(const char **titles)
{
	for (int i = 0; i < CONSOLE_FKEYS_COUNT; ++i) {
		if (titles && titles[i]) {
			s_titles_normal[i] = ' ';
			s_titles_normal[i] += titles[i];
		} else {
			s_titles_normal[i].clear();
		}
	}

	if (!g_tb_delegate) {
		return false;
	}

	if (!s_tb_alternate) {
		Touchbar_ApplyTitlesNormal();
	}

	return true;
}

void Touchbar_SetAlternate(bool on)
{
	if (s_tb_alternate == on) {
		return;
	}

	s_tb_alternate = on;
	if (g_tb_delegate) {
		Touchbar_ApplyTitles();
	}
}

#else

void Touchbar_Register(ITouchbarListener *listener) {}
void Touchbar_Deregister() {}
bool Touchbar_SetTitles(const char **titles) { return false; }
void Touchbar_SetAlternate(bool on) {}

#endif
