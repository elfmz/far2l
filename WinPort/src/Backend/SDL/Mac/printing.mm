#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <objc/runtime.h>

#include "printing.h"
#include "WideMB.h"

static NSString* ToNSString(const wchar_t* s)
{
	if (!s) {
		return @"";
	}
	const std::string mb = Wide2MB(s);
	return [NSString stringWithUTF8String:mb.c_str()];
}

static inline NSURL* ToFileURL(const wchar_t* s)
{
	return [NSURL fileURLWithPath:ToNSString(s)];
}

static void ShowFileLoadError(const wchar_t* path, NSError *err = nil)
{
	NSAlert* alert = [[NSAlert alloc] init];
	alert.messageText = @"Failed to load file for printing";
	NSString *info = err ? err.localizedDescription : @"Unknown error";
	alert.informativeText = [NSString stringWithFormat:@"%@\n%@", info, ToNSString(path)];
	[alert runModal];
}

static NSString* LoadTextFileOrShowError(const wchar_t* path)
{
	NSError* err = nil;
	NSString* contents =
		[NSString stringWithContentsOfFile:ToNSString(path)
								  encoding:NSUTF8StringEncoding
									 error:&err];
	if (!contents)
		ShowFileLoadError(path, err);
	return contents;
}

static bool LoadHtmlStringFromFile(const wchar_t* path, NSString **htmlOut, NSURL **baseURLOut)
{
	NSURL* url = ToFileURL(path);
	NSError* err = nil;
	NSString* html = [NSString stringWithContentsOfURL:url
											   encoding:NSUTF8StringEncoding
												  error:&err];
	if (!html)
	{
		ShowFileLoadError(path, err);
		return false;
	}

	if (htmlOut)
		*htmlOut = html;
	if (baseURLOut)
		*baseURLOut = [url URLByDeletingLastPathComponent];
	return true;
}

static NSTextView* CreateTextView(NSString* text)
{
	NSTextView* view = [[NSTextView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
	view.string = text;
	view.font = [NSFont fontWithName:@"Menlo" size:10];
	view.verticallyResizable = YES;
	view.horizontallyResizable = NO;
	return view;
}

static void RunPrintOperation(NSPrintOperation* op)
{
	NSWindow* parentWindow = [NSApp keyWindow] ?: [NSApp mainWindow];
	if (parentWindow) {
		[op runOperationModalForWindow:parentWindow delegate:nil didRunSelector:NULL contextInfo:NULL];
	} else {
		[op runOperation];
	}
}

@interface WebViewPrintHelper : NSObject <WKNavigationDelegate>
@property (nonatomic, strong) WKWebView *webView;
@property (nonatomic, copy) void (^completion)(WKWebView *);
@end

static char kWebViewPrintHelperKey;

@implementation WebViewPrintHelper
- (void)finishWithWebView:(WKWebView*)webView
{
	if (!self.completion) return;
	void (^block)(WKWebView*) = self.completion;
	self.completion = nil;

	webView.navigationDelegate = nil;
	objc_setAssociatedObject(webView, &kWebViewPrintHelperKey, nil, OBJC_ASSOCIATION_ASSIGN);
	self.webView = nil;

	block(webView);
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation *)navigation { [self finishWithWebView:webView]; }
- (void)webView:(WKWebView*)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error { [self finishWithWebView:webView]; }
- (void)webView:(WKWebView*)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error { [self finishWithWebView:webView]; }
@end

static void RunWebViewPrintOperationWhenReady(WKWebView *webView, void (^operation)(WKWebView *))
{
	if (!webView.isLoading) {
		operation(webView);
		return;
	}

	WebViewPrintHelper *helper = [[WebViewPrintHelper alloc] init];
	helper.webView = webView;
	helper.completion = operation;
	webView.navigationDelegate = helper;
	objc_setAssociatedObject(webView, &kWebViewPrintHelperKey, helper, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

void MacNativePrintTextW(const wchar_t* text)
{
	@autoreleasepool {
		NSPrintOperation* op = [NSPrintOperation printOperationWithView:CreateTextView(ToNSString(text))];
		[op runOperation];
	}
}

void MacNativePrintHtmlW(const wchar_t* html)
{
	@autoreleasepool {
		WKWebView* web = [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
		[web loadHTMLString:ToNSString(html) baseURL:nil];

		RunWebViewPrintOperationWhenReady(web, ^(WKWebView* readyWeb){
			NSPrintOperation* op = [readyWeb printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
			[op runOperation];
		});
	}
}

void MacNativePrintTextFileW(const wchar_t* path)
{
	@autoreleasepool {
		NSString* text = LoadTextFileOrShowError(path);
		if (!text) return;
		NSPrintOperation* op = [NSPrintOperation printOperationWithView:CreateTextView(text)];
		[op runOperation];
	}
}

void MacNativePrintHtmlFileW(const wchar_t* path)
{
	@autoreleasepool {
		NSString *html = nil;
		NSURL *baseURL = nil;
		if (!LoadHtmlStringFromFile(path, &html, &baseURL)) return;

		WKWebView* web = [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
		[web loadHTMLString:html baseURL:baseURL];

		RunWebViewPrintOperationWhenReady(web, ^(WKWebView* readyWeb){
			NSPrintOperation* op = [readyWeb printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
			[op runOperation];
		});
	}
}

void MacNativeShowPageSetupDialogW()
{
	@autoreleasepool {
		NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
		NSPageLayout* pageLayout = [NSPageLayout pageLayout];
		NSInteger result = [pageLayout runModalWithPrintInfo:printInfo];
		(void)result;
	}
}

static void MacNativeShowPrintDialog(NSView* viewToPrint)
{
	@autoreleasepool {
		NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
		NSPrintOperation* op = [NSPrintOperation printOperationWithView:viewToPrint printInfo:printInfo];
		RunPrintOperation(op);
	}
}

void MacNativePrintPreviewTextW(const wchar_t* text)
{
	@autoreleasepool {
		MacNativeShowPrintDialog(CreateTextView(ToNSString(text)));
	}
}

void MacNativePrintPreviewHtmlW(const wchar_t* html)
{
	@autoreleasepool {
		WKWebView* web = [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
		[web loadHTMLString:ToNSString(html) baseURL:nil];

		RunWebViewPrintOperationWhenReady(web, ^(WKWebView* readyWeb){
			NSPrintOperation* op = [readyWeb printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
			RunPrintOperation(op);
		});
	}
}

void MacNativePrintPreviewTextFileW(const wchar_t* path)
{
	@autoreleasepool {
		NSString* text = LoadTextFileOrShowError(path);
		if (!text) return;
		MacNativeShowPrintDialog(CreateTextView(text));
	}
}

void MacNativePrintPreviewHtmlFileW(const wchar_t* path)
{
	@autoreleasepool {
		NSString *html = nil;
		NSURL *baseURL = nil;
		if (!LoadHtmlStringFromFile(path, &html, &baseURL)) return;

		WKWebView* web = [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
		[web loadHTMLString:html baseURL:baseURL];

		RunWebViewPrintOperationWhenReady(web, ^(WKWebView* readyWeb){
			NSPrintOperation* op = [readyWeb printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
			RunPrintOperation(op);
		});
	}
}
