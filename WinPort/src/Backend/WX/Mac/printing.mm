#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <objc/runtime.h>

#include <wx/string.h>
#include "printing.h"

// ------------------- Helper functions -------------------

static NSString* ToNSString(const wxString& s) 
{ 
	return [NSString stringWithUTF8String:s.utf8_str().data()]; 
}

static inline NSURL* ToFileURL(const wxString& s) 
{
	return [NSURL fileURLWithPath:ToNSString(s)]; 
}

static void ShowFileLoadError(const wxString& path, NSError *err = nil)
{
	NSAlert* alert = [[NSAlert alloc] init];
	alert.messageText = @"Failed to load file for printing";
	NSString *info = err ? err.localizedDescription : @"Unknown error";
	alert.informativeText = [NSString stringWithFormat:@"%@\n%@", info, ToNSString(path)];
	[alert runModal];
}

static NSString* LoadTextFileOrShowError(const wxString& path)
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

static bool LoadHtmlStringFromFile(const wxString& path, NSString **htmlOut, NSURL **baseURLOut)
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

// ------------------- WebView printing helper -------------------

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

// ------------------- Public API functions -------------------

void MacNativePrintText(const wxString& text)
{
	@autoreleasepool {
		NSPrintOperation* op = [NSPrintOperation printOperationWithView:CreateTextView(ToNSString(text))];
		[op runOperation];
	}
}

void MacNativePrintHtml(const wxString& html)
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

void MacNativePrintTextFile(const wxString& path)
{
	@autoreleasepool {
		NSString* text = LoadTextFileOrShowError(path);
		if (!text) return;
		NSPrintOperation* op = [NSPrintOperation printOperationWithView:CreateTextView(text)];
		[op runOperation];
	}
}

void MacNativePrintHtmlFile(const wxString& path)
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

void MacNativeShowPageSetupDialog()
{
	@autoreleasepool {
		NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
		NSPageLayout* pageLayout = [NSPageLayout pageLayout];
		NSInteger result = [pageLayout runModalWithPrintInfo:printInfo];
		(void)result; // retain original behavior; user settings updated in printInfo
	}
}

void MacNativeShowPrintDialog(NSView* viewToPrint)
{
	@autoreleasepool {
		NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
		NSPrintOperation* op = [NSPrintOperation printOperationWithView:viewToPrint printInfo:printInfo];
		RunPrintOperation(op);
	}
}

// ------------------- Print preview -------------------

void MacNativePrintPreviewText(const wxString& text)
{
	@autoreleasepool {
		MacNativeShowPrintDialog(CreateTextView(ToNSString(text)));
	}
}

void MacNativePrintPreviewHtml(const wxString& html)
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

void MacNativePrintPreviewTextFile(const wxString& path)
{
	@autoreleasepool {
		NSString* text = LoadTextFileOrShowError(path);
		if (!text) return;
		MacNativeShowPrintDialog(CreateTextView(text));
	}
}

void MacNativePrintPreviewHtmlFile(const wxString& path)
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
