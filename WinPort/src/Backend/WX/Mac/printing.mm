#import <Cocoa/Cocoa.h> 
#import <WebKit/WebKit.h>

#include <wx/string.h>
#include "printing.h"

static NSString* ToNSString(const wxString& s) 
{ 
	return [NSString stringWithUTF8String:s.utf8_str().data()]; 
}

static inline NSURL* ToFileURL(const wxString& s) 
{
	return [NSURL fileURLWithPath: [NSString stringWithUTF8String:s.utf8_str().data()]]; 
}

void MacNativePrintText(const wxString& text) 
{
    @autoreleasepool 
    {
        NSTextView* view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 600, 800)];
        [view setString:ToNSString(text)];
        NSFont* font = [NSFont fontWithName:@"Menlo" size:12];
        [view setFont:font];
        [view setVerticallyResizable:YES];
        [view setHorizontallyResizable:NO];

        NSPrintOperation* op = [NSPrintOperation printOperationWithView:view];
        [op runOperation];
    }
}

void MacNativePrintHtml(const wxString& html) 
{
    @autoreleasepool 
    {
		WKWebView* web = [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
		[web loadHTMLString:ToNSString(html) baseURL:nil];

		NSPrintOperation* op = [web printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
		[op runOperation];
    }
}

void MacNativePrintTextFile(const wxString& path)
{
    @autoreleasepool
    {
        NSString* nsPath = ToNSString(path);

        NSError* err = nil;
        NSString* fileContents =
            [NSString stringWithContentsOfFile:nsPath
                                      encoding:NSUTF8StringEncoding
                                         error:&err];

        if (!fileContents) {
            NSAlert* alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Failed to load file for printing"];
            [alert runModal];
            return;
        }

        NSTextView* view =
            [[NSTextView alloc] initWithFrame:NSMakeRect(0,0,600,800)];

        [view setString:fileContents];
        [view setFont:[NSFont fontWithName:@"Menlo" size:12]];
        [view setVerticallyResizable:YES];

        NSPrintOperation* op =
            [NSPrintOperation printOperationWithView:view];

        [op runOperation];
    }
}

void MacNativePrintHtmlFile(const wxString& path)
{
    @autoreleasepool
    {
        NSURL* url = ToFileURL(path);

        WKWebView* web =
            [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];

        // Load the file directly
        [web loadFileURL:url allowingReadAccessToURL:[url URLByDeletingLastPathComponent]];

        // Wait until the HTML finishes loading
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            NSPrintOperation* op =
                [web printOperationWithPrintInfo:[NSPrintInfo sharedPrintInfo]];
            [op runOperation];
        });
    }
}

void MacNativeShowPageSetupDialog()
{
    @autoreleasepool
    {
        NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
        NSPageLayout* pageLayout = [NSPageLayout pageLayout];
        NSInteger result = [pageLayout runModalWithPrintInfo:printInfo];

        if (result == NSModalResponseOK) {
            // User confirmed settings
            // printInfo now contains updated margins, paper size, orientation, etc.
        }
    }
}

void MacNativeShowPrintDialog(NSView* viewToPrint)
{
    @autoreleasepool
    {
        NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];

        NSPrintOperation* op =
            [NSPrintOperation printOperationWithView:viewToPrint
                                           printInfo:printInfo];
        [op runOperationModalForWindow:nil
                              delegate:nil
                        didRunSelector:NULL
                           contextInfo:NULL];
    }
}

void MacNativePrintPreviewText(const wxString& text) 
{
    @autoreleasepool 
    {
        NSTextView* view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 600, 800)];
        [view setString:ToNSString(text)];
        NSFont* font = [NSFont fontWithName:@"Menlo" size:12];
        [view setFont:font];
        [view setVerticallyResizable:YES];
        [view setHorizontallyResizable:NO];

        MacNativeShowPrintDialog(view);
    }
}

void MacNativePrintPreviewHtml(const wxString& html) 
{
    @autoreleasepool 
    {
		WKWebView* web = [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];
		[web loadHTMLString:ToNSString(html) baseURL:nil];

        MacNativeShowPrintDialog(web);
    }
}

void MacNativePrintPreviewTextFile(const wxString& path)
{
    @autoreleasepool
    {
        NSString* nsPath = ToNSString(path);

        NSError* err = nil;
        NSString* fileContents =
            [NSString stringWithContentsOfFile:nsPath
                                      encoding:NSUTF8StringEncoding
                                         error:&err];

        if (!fileContents) {
            NSAlert* alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Failed to load file for printing"];
            [alert runModal];
            return;
        }

        NSTextView* view =
            [[NSTextView alloc] initWithFrame:NSMakeRect(0,0,600,800)];

        [view setString:fileContents];
        [view setFont:[NSFont fontWithName:@"Menlo" size:12]];
        [view setVerticallyResizable:YES];

        MacNativeShowPrintDialog(view);
    }
}

void MacNativePrintPreviewHtmlFile(const wxString& path)
{
    @autoreleasepool
    {
        NSURL* url = ToFileURL(path);

        WKWebView* web =
            [[WKWebView alloc] initWithFrame:NSMakeRect(0,0,600,800)];

        // Load the file directly
        [web loadFileURL:url allowingReadAccessToURL:[url URLByDeletingLastPathComponent]];

        // Wait until the HTML finishes loading
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
	        MacNativeShowPrintDialog(web);
        });
    }
}

