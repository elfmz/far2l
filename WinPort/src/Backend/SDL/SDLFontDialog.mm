#include "SDLFontDialog.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <CoreText/CoreText.h>
#import <dispatch/dispatch.h>

#include <algorithm>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

static const NSInteger kFARFontChooseButtonTag = 0xF001;
static const NSInteger kFARFontCancelButtonTag = 0xF002;

static NSString *FARPathFromDescriptor(CTFontDescriptorRef desc)
{
	if (!desc) {
		return nil;
	}
	CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(desc, kCTFontURLAttribute);
	if (!url) {
		return nil;
	}
	NSString *path = nil;
	CFStringRef cfPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	if (cfPath) {
		path = [NSString stringWithString:(__bridge NSString *)cfPath];
		CFRelease(cfPath);
	}
	CFRelease(url);
	return path;
}

@interface FARFontPanelController : NSObject <NSWindowDelegate>
{
	NSFontPanel *_panel;
	NSFontManager *_manager;
	id _previousTarget;
	SEL _previousAction;
	BOOL _modalFinished;
	SDLFontDialogStatus _status;
	NSString *_selectedPath;
	CGFloat _selectedPointSize;
	NSInteger _selectedFaceIndex;
}
- (SDLFontDialogStatus)runWithOutput:(SDLFontSelection &)selection;
- (BOOL)captureSelectedFontShowingAlert:(BOOL)showAlert;
- (void)installActionButtonsIfNeeded;
- (void)confirmButtonPressed:(id)sender;
- (void)cancelButtonPressed:(id)sender;
@end

static NSString *FARPathFromFont(NSFont *font)
{
	if (!font) {
		return nil;
	}

	NSString *path = FARPathFromDescriptor((__bridge CTFontDescriptorRef)font.fontDescriptor);
	if ([path length] > 0) {
		return path;
	}

	NSString *postscript = [font fontName];
	if ([postscript length] == 0) {
		return nil;
	}

	NSDictionary *attrs = @{ (id)kCTFontNameAttribute: postscript };
	CTFontDescriptorRef namedDesc = CTFontDescriptorCreateWithAttributes((__bridge CFDictionaryRef)attrs);
	if (namedDesc) {
		path = FARPathFromDescriptor(namedDesc);
		if ([path length] == 0) {
			CFArrayRef matches = CTFontDescriptorCreateMatchingFontDescriptors(namedDesc, NULL);
			if (matches) {
				CFIndex count = CFArrayGetCount(matches);
				for (CFIndex i = 0; i < count && [path length] == 0; ++i) {
					CTFontDescriptorRef item = (CTFontDescriptorRef)CFArrayGetValueAtIndex(matches, i);
					path = FARPathFromDescriptor(item);
				}
				CFRelease(matches);
			}
		}
		CFRelease(namedDesc);
	}
	if ([path length] > 0) {
		return path;
	}

	CTFontRef ctFont = CTFontCreateWithName((CFStringRef)postscript, [font pointSize], NULL);
	if (ctFont) {
		CFURLRef url = (CFURLRef)CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
		if (url) {
			CFStringRef cfPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
			if (cfPath) {
				path = [NSString stringWithString:(__bridge NSString *)cfPath];
				CFRelease(cfPath);
			}
			CFRelease(url);
		}
		CFRelease(ctFont);
	}
	return path;
}

static NSInteger FARLocateFaceIndex(NSString *path, NSString *postscriptName)
{
	if (!path || !postscriptName) {
		return -1;
	}
	const char *fsPath = [path fileSystemRepresentation];
	const char *psName = [postscriptName UTF8String];
	if (!fsPath || !psName) {
		return -1;
	}

	FT_Library library = nullptr;
	if (FT_Init_FreeType(&library) != 0) {
		return -1;
	}
	auto cleanup = [&]() {
		if (library) {
			FT_Done_FreeType(library);
			library = nullptr;
		}
	};

	FT_Face probe = nullptr;
	if (FT_New_Face(library, fsPath, 0, &probe) != 0) {
		cleanup();
		return -1;
	}
	const FT_Long totalFaces = std::max<FT_Long>(1, probe->num_faces);
	FT_Done_Face(probe);

	const FT_Long limit = std::min<FT_Long>(totalFaces, 32); // reasonable guard
	for (FT_Long idx = 0; idx < limit; ++idx) {
		FT_Face face = nullptr;
		if (FT_New_Face(library, fsPath, idx, &face) != 0) {
			continue;
		}
		const char *candidate = FT_Get_Postscript_Name(face);
		const bool match = (candidate && std::string(candidate) == psName);
		FT_Done_Face(face);
		if (match) {
			cleanup();
			return static_cast<NSInteger>(idx);
		}
	}

	cleanup();
	return -1;
}

static NSInteger FARFaceIndexFromFont(NSFont *font, NSString *pathHint = nil)
{
	if (!font) {
		return -1;
	}
	NSString *postscript = [font fontName];
	if ([postscript length] == 0) {
		return -1;
	}
	NSString *path = pathHint;
	if (![path length]) {
		path = FARPathFromFont(font);
	}
	if (![path length]) {
		return -1;
	}
	return FARLocateFaceIndex(path, postscript);
}

@implementation FARFontPanelController

- (instancetype)init
{
    self = [super init];
    if (self) {
        _status = SDLFontDialogStatus::Cancelled;
        _modalFinished = NO;
        _selectedFaceIndex = -1;
    }
    return self;
}

- (void)dealloc
{
    [_selectedPath release];
    [super dealloc];
}

- (void)restoreFontManager
{
    if (_manager) {
        [_manager setTarget:_previousTarget];
        [_manager setAction:_previousAction];
    }
}

- (void)finishModalIfNeeded
{
    if (!_modalFinished) {
        _modalFinished = YES;
        [NSApp stopModal];
    }
}

- (void)windowWillClose:(NSNotification *)notification
{
    (void)notification;
    _status = SDLFontDialogStatus::Cancelled;
    [self finishModalIfNeeded];
}

- (void)farHandleFontSelection:(id)sender
{
	(void)sender;
	(void)[self captureSelectedFontShowingAlert:NO];
}

- (SDLFontDialogStatus)runWithOutput:(SDLFontSelection &)selection
{
    NSApplication *app = [NSApplication sharedApplication];
    if (!app) {
        return SDLFontDialogStatus::Failed;
    }

    _manager = [NSFontManager sharedFontManager];
    if (!_manager) {
        return SDLFontDialogStatus::Failed;
    }

    _previousTarget = [_manager target];
    _previousAction = [_manager action];
    [_manager setTarget:self];
    [_manager setAction:@selector(farHandleFontSelection:)];

    _panel = [_manager fontPanel:YES];
    if (!_panel) {
        [self restoreFontManager];
        return SDLFontDialogStatus::Failed;
    }

    [_panel setDelegate:self];
    [_panel setWorksWhenModal:YES];
    [_panel center];
    [_panel makeKeyAndOrderFront:nil];
    [self installActionButtonsIfNeeded];

    _modalFinished = NO;
    _status = SDLFontDialogStatus::Cancelled;
    [app runModalForWindow:_panel];

    [_panel orderOut:nil];
    [_panel setDelegate:nil];
    [self restoreFontManager];

	if (_status == SDLFontDialogStatus::Chosen && _selectedPath.length > 0) {
		if (_selectedPointSize <= 0.0) {
			NSFont *fallbackRef = [NSFont userFixedPitchFontOfSize:[NSFont systemFontSize]];
			if (!fallbackRef) {
				fallbackRef = [NSFont systemFontOfSize:[NSFont systemFontSize]];
			}
			if (_panel && fallbackRef) {
				NSFont *finalFont = [_panel panelConvertFont:fallbackRef];
				if (finalFont) {
					_selectedPointSize = [finalFont pointSize];
				}
			}
			if (_selectedPointSize <= 0.0 && fallbackRef) {
				_selectedPointSize = [fallbackRef pointSize];
			}
			if (_selectedPointSize <= 0.0) {
				_selectedPointSize = [NSFont systemFontSize];
			}
		}
		selection.path = std::string([_selectedPath UTF8String]);
		selection.point_size = (_selectedPointSize > 0.0) ? static_cast<float>(_selectedPointSize) : 0.0f;
		selection.face_index = static_cast<int>(_selectedFaceIndex);
	} else {
		selection.face_index = -1;
	}
	return _status;
}

- (BOOL)captureSelectedFontShowingAlert:(BOOL)showAlert
{
	NSFont *selected = [_manager selectedFont];
	NSFont *reference = selected;
	if (!reference) {
		reference = [NSFont userFixedPitchFontOfSize:12.0];
	}
	if (!reference) {
		reference = [NSFont systemFontOfSize:[NSFont systemFontSize]];
	}
	NSFont *panelFont = _panel ? [_panel panelConvertFont:reference] : nil;
	NSFont *font = nil;
	if (panelFont) {
		font = panelFont;
	} else if (selected) {
		font = selected;
	} else {
		font = [_manager convertFont:reference];
	}
	if (!font) {
		if (showAlert) {
			NSBeep();
		}
		return NO;
	}

    NSString *path = FARPathFromFont(font);
    if ([path length] == 0) {
        if (showAlert) {
            NSAlert *alert = [[[NSAlert alloc] init] autorelease];
            [alert setMessageText:@"Unable to use selected font"];
            [alert setInformativeText:@"macOS did not provide a file path for this font. Please select another one."];
            [alert runModal];
        }
        return NO;
    }

	[_selectedPath release];
	_selectedPath = [path copy];
	_selectedFaceIndex = FARFaceIndexFromFont(font, _selectedPath);

	CGFloat pointSize = 0.0;
	if (_panel) {
		NSFont *panelFontForSize = [_panel panelConvertFont:font ? font : reference];
		if (panelFontForSize) {
			pointSize = [panelFontForSize pointSize];
			if (_selectedFaceIndex < 0) {
				_selectedFaceIndex = FARFaceIndexFromFont(panelFontForSize, _selectedPath);
			}
		}
	}
	if (pointSize <= 0.0 && font) {
		pointSize = [font pointSize];
		if (_selectedFaceIndex < 0) {
			_selectedFaceIndex = FARFaceIndexFromFont(font, _selectedPath);
		}
	}
	if (pointSize <= 0.0 && reference) {
		const CGFloat refSize = [reference pointSize];
		if (refSize > 0.0) {
			pointSize = refSize;
			if (_selectedFaceIndex < 0) {
				_selectedFaceIndex = FARFaceIndexFromFont(reference);
			}
		}
	}
	if (pointSize <= 0.0) {
		pointSize = [NSFont systemFontSize];
	}
	_selectedPointSize = pointSize;
	return YES;
}

- (void)confirmButtonPressed:(id)sender
{
    (void)sender;
    if ([self captureSelectedFontShowingAlert:YES]) {
        _status = SDLFontDialogStatus::Chosen;
        [self finishModalIfNeeded];
    }
}

- (void)cancelButtonPressed:(id)sender
{
    (void)sender;
    _status = SDLFontDialogStatus::Cancelled;
    [self finishModalIfNeeded];
}

- (void)installActionButtonsIfNeeded
{
    if (!_panel) {
        return;
    }
    NSView *content = [_panel contentView];
    if (!content) {
        return;
    }

    if ([content viewWithTag:kFARFontChooseButtonTag]) {
        return;
    }

    NSButton *chooseButton = [[[NSButton alloc] initWithFrame:NSZeroRect] autorelease];
    [chooseButton setTitle:@"Choose Font"];
    [chooseButton setBezelStyle:NSBezelStyleRounded];
    [chooseButton setButtonType:NSButtonTypeMomentaryPushIn];
    [chooseButton setTarget:self];
    [chooseButton setAction:@selector(confirmButtonPressed:)];
    [chooseButton setTag:kFARFontChooseButtonTag];
    [chooseButton setTranslatesAutoresizingMaskIntoConstraints:NO];

    NSButton *cancelButton = [[[NSButton alloc] initWithFrame:NSZeroRect] autorelease];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setBezelStyle:NSBezelStyleRounded];
    [cancelButton setButtonType:NSButtonTypeMomentaryPushIn];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(cancelButtonPressed:)];
    [cancelButton setTag:kFARFontCancelButtonTag];
    [cancelButton setTranslatesAutoresizingMaskIntoConstraints:NO];

    [content addSubview:chooseButton];
    [content addSubview:cancelButton];

    [NSLayoutConstraint activateConstraints:@[
        [chooseButton.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-16.0],
        [chooseButton.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-12.0],
        [cancelButton.trailingAnchor constraintEqualToAnchor:chooseButton.leadingAnchor constant:-12.0],
        [cancelButton.bottomAnchor constraintEqualToAnchor:chooseButton.bottomAnchor]
    ]];
}

@end

SDLFontDialogStatus SDLShowFontPicker(SDLFontSelection &selection)
{
	__block SDLFontDialogStatus status = SDLFontDialogStatus::Cancelled;
	void (^panelBlock)(void) = ^{
		@autoreleasepool {
			FARFontPanelController *controller = [[FARFontPanelController alloc] init];
			if (controller) {
				status = [controller runWithOutput:selection];
				[controller release];
			} else {
				status = SDLFontDialogStatus::Failed;
			}
        }
    };
    if ([NSThread isMainThread]) {
        panelBlock();
    } else {
        dispatch_sync(dispatch_get_main_queue(), panelBlock);
    }
    return status;
}

@interface FARFontMenuHandler : NSObject
@property(nonatomic, assign) void (*callback)(void *);
@property(nonatomic, assign) void *context;
- (void)far2lChangeFont:(id)sender;
@end

@implementation FARFontMenuHandler
- (void)far2lChangeFont:(id)sender
{
    (void)sender;
    if (self.callback) {
        self.callback(self.context);
    }
}
@end

void SDLInstallMacFontMenu(void (*callback)(void *), void *context)
{
    if (!callback) {
        return;
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        @autoreleasepool {
            static FARFontMenuHandler *handler = nil;
            if (!handler) {
                handler = [[FARFontMenuHandler alloc] init];
            }
            handler.callback = callback;
            handler.context = context;

            NSMenu *mainMenu = [NSApp mainMenu];
            if (!mainMenu) {
                return;
            }

            NSMenuItem *settingsItem = [mainMenu itemWithTitle:@"Settings"];
            if (!settingsItem) {
                settingsItem = [[NSMenuItem alloc] initWithTitle:@"Settings" action:nil keyEquivalent:@""];
                [mainMenu addItem:settingsItem];
            }

            NSMenu *settingsMenu = settingsItem.submenu;
            if (!settingsMenu) {
                settingsMenu = [[NSMenu alloc] initWithTitle:@"Settings"];
                [settingsItem setSubmenu:settingsMenu];
            }

            NSMenuItem *changeFontItem = [settingsMenu itemWithTitle:@"Change Font…"];
            if (!changeFontItem) {
                changeFontItem = [[NSMenuItem alloc] initWithTitle:@"Change Font…" action:@selector(far2lChangeFont:) keyEquivalent:@""];
                [settingsMenu addItem:changeFontItem];
            }
            [changeFontItem setTarget:handler];
        }
    });
}

#else
SDLFontDialogStatus SDLShowFontPicker(std::string &)
{
    return SDLFontDialogStatus::Unsupported;
}

void SDLInstallMacFontMenu(void (*)(void *), void *)
{
}
#endif
