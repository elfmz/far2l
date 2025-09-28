#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "MacOSAppProvider.hpp"
#include "lng.hpp"
#include "WideMB.h"
#include "common.hpp"
#include "utils.h"
#include <vector>
#include <string>
#include <unordered_map>

struct MacCandidateTempInfo {
    std::wstring name;
    std::wstring id;
    std::wstring info;
};


// ****************************** Implementation ******************************

MacOSAppProvider::MacOSAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
}

static std::string NSURLToPath(NSURL *url) {
    if (!url) return {};
    NSString *path = [url path];
    return path ? std::string([path UTF8String]) : std::string{};
}

static MacCandidateTempInfo AppBundleToTempInfo(NSURL *appURL) {
    MacCandidateTempInfo c;

    NSBundle *bundle = [NSBundle bundleWithURL:appURL];
    NSString *bundleName = [[bundle infoDictionary] objectForKey:@"CFBundleName"];
    NSString *bundleShortVersion = [[bundle infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    NSString *bundleVersion = [[bundle infoDictionary] objectForKey:@"CFBundleVersion"];

    c.name = StrMB2Wide(bundleName ? [bundleName UTF8String] : NSURLToPath(appURL));
    c.id = StrMB2Wide(NSURLToPath(appURL));
    c.info = bundleShortVersion ? StrMB2Wide([bundleShortVersion UTF8String])
             : ( bundleVersion ? StrMB2Wide([bundleVersion UTF8String]): L"") ;

    return c;
}

static std::wstring EscapeForShell(const std::wstring& arg) {
    std::wstring out;
    out.push_back(L'"');
    for (wchar_t c : arg) {
        if (c == L'\\' || c == L'"' || c == L'$' || c == L'`') {
            out.push_back(L'\\');
        }
        out.push_back(c);
    }
    out.push_back(L'"');
    return out;
}

std::vector<CandidateInfo> MacOSAppProvider::GetAppCandidates(const std::wstring &pathname) {
    std::vector<CandidateInfo> result;
    std::vector<MacCandidateTempInfo> temp_candidates;

    NSString *nsPath = [NSString stringWithUTF8String:StrWide2MB(pathname).c_str()];
    NSURL *fileURL = [NSURL fileURLWithPath:nsPath];
    if (!fileURL) return result;

    // Default application
    NSURL *defaultApp = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:fileURL];
    if (defaultApp) {
        temp_candidates.push_back(AppBundleToTempInfo(defaultApp));
    }

    // All applications that can open the file
    NSArray<NSURL *> *apps = [[NSWorkspace sharedWorkspace] URLsForApplicationsToOpenURL:fileURL];
    for (NSURL *appURL in apps) {
        if (defaultApp && [appURL isEqual:defaultApp]) {
            continue;
        }
        temp_candidates.push_back(AppBundleToTempInfo(appURL));
    }


    std::unordered_map<std::wstring, int> nameCount;
    for (const auto &c : temp_candidates) {
        nameCount[c.name]++;
    }


    result.reserve(temp_candidates.size());
    for (const auto &temp_c : temp_candidates) {
        CandidateInfo final_c;
        final_c.id = temp_c.id;
        final_c.terminal = false;
        final_c.name = temp_c.name;
        
        if (nameCount[temp_c.name] > 1 && !temp_c.info.empty()) {
            final_c.name += L" (" + temp_c.info + L")";
        }
        result.push_back(final_c);
    }

    return result;
}


std::wstring MacOSAppProvider::ConstructCommandLine(const CandidateInfo &candidate,
                                                    const std::wstring &pathname) {
    if (candidate.id.empty() || pathname.empty()) 
        return L"";

    std::wstring cmd = L"open -a " + EscapeForShell(candidate.id) + L" " + EscapeForShell(pathname);
    return cmd;
}


std::vector<Field> MacOSAppProvider::GetCandidateDetails(const CandidateInfo& candidate) {
    std::vector<Field> details;

    NSString *nsPath = [NSString stringWithUTF8String:StrWide2MB(candidate.id).c_str()];
    NSURL *appURL = [NSURL fileURLWithPath:nsPath];
    if (!appURL) return details;
    
    NSBundle *bundle = [NSBundle bundleWithURL:appURL];
    if (!bundle) return details;

    NSDictionary *infoDict = [bundle infoDictionary];

    details.push_back({L"Full path:", candidate.id});
    details.push_back({L"Name:", candidate.name});

    NSString *execName = [infoDict objectForKey:@"CFBundleExecutable"];
    if (execName) {
        details.push_back({L"Executable file:", StrMB2Wide([execName UTF8String])});
    }

    NSString *bundleShortVersion = [infoDict objectForKey:@"CFBundleShortVersionString"];
    if (bundleShortVersion) {
        details.push_back({L"Version:", StrMB2Wide([bundleShortVersion UTF8String])});
    }
    
    NSString *bundleVersion = [infoDict objectForKey:@"CFBundleVersion"];
    if (bundleVersion) {
        details.push_back({L"Bundle version:", StrMB2Wide([bundleVersion UTF8String])});
    }

    return details;
}

std::wstring MacOSAppProvider::GetMimeType(const std::wstring &pathname) {
    NSString *nsPath = [NSString stringWithUTF8String:StrWide2MB(pathname).c_str()];
    NSURL *fileURL = [NSURL fileURLWithPath:nsPath];
    if (!fileURL) return L"application/octet-stream";

    NSString *uti = nil;
    NSError *error = nil;
    [fileURL getResourceValue:&uti forKey:NSURLTypeIdentifierKey error:&error];
    if (!uti) return L"application/octet-stream";

    std::wstring result;

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000 // UTType is available on macOS 11.0+
    UTType *type = [UTType typeWithIdentifier:uti];
    if (type) {
        NSString *mimeStr = type.preferredMIMEType;
        if (mimeStr) result = StrMB2Wide([mimeStr UTF8String]);
    }
#else
    CFStringRef mimeType = UTTypeCopyPreferredTagWithClass((__bridge CFStringRef)uti,
                                                           kUTTagClassMIMEType);
    if (mimeType) {
        NSString *mimeStr = (__bridge_transfer NSString *)mimeType;
        result = StrMB2Wide([mimeStr UTF8String]);
        // CFRelease(mimeType) is not needed due to __bridge_transfer
    }
#endif

    return result.empty() ? L"application/octet-stream" : result;
}

#endif // __APPLE__
