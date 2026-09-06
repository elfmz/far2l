#if defined(__APPLE__)

#include <AvailabilityMacros.h>

// If the compiler (e.g., GNU GCC) does not support Clang feature checking macros,
// safely define them to 0 to fallback to classic Objective-C.
#ifndef __has_feature
	#define __has_feature(x) 0
#endif

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
	#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#include "LaunchServicesBridge.hpp"

namespace
{
#ifndef __clang__

	// GCC/MRC: @autoreleasepool does not drain on C++ exceptions.
	// This RAII wrapper ensures the pool is always drained.
	struct AutoreleasePoolGuard
	{
		NSAutoreleasePool *pool;
		AutoreleasePoolGuard() : pool([[NSAutoreleasePool alloc] init]) {}
		~AutoreleasePoolGuard() { [pool drain]; }
		AutoreleasePoolGuard(const AutoreleasePoolGuard&) = delete;
		AutoreleasePoolGuard& operator=(const AutoreleasePoolGuard&) = delete;
	};

#endif

#ifdef __clang__
	#define BEGIN_AUTORELEASE_POOL(name) @autoreleasepool {
	#define END_AUTORELEASE_POOL         }
#else
	#define BEGIN_AUTORELEASE_POOL(name) { AutoreleasePoolGuard name;
	#define END_AUTORELEASE_POOL         }
#endif

	NSURL* FileURLFromUTF8Path(const std::string& path)
	{
		if (path.empty()) {
			return nil;
		}
		NSString *ns_path = [NSString stringWithUTF8String:path.c_str()];
		return ns_path ? [NSURL fileURLWithPath:ns_path] : nil;
	}

} // namespace

namespace openwith::launch_services
{
	std::optional<std::string> ResolveFileUTI(const std::string& filepath)
	{
		BEGIN_AUTORELEASE_POOL(pool)
			NSURL *file_url = FileURLFromUTF8Path(filepath);
			if (!file_url) {
				return std::nullopt;
			}

			NSString *ns_uti = nil;
			NSError *error = nil;
			if (![file_url getResourceValue:&ns_uti forKey:NSURLTypeIdentifierKey error:&error] || !ns_uti) {
				return std::nullopt;
			}
			if (const char* utf8_val = [ns_uti UTF8String]) {
				return std::string(utf8_val);
			}
		END_AUTORELEASE_POOL
		return std::nullopt;
	}


	std::optional<std::string> GetDefaultBundlePath(const std::string& filepath)
	{
		BEGIN_AUTORELEASE_POOL(pool)
			NSURL *file_url = FileURLFromUTF8Path(filepath);
			if (!file_url) {
				return std::nullopt;
			}

			NSURL* default_app_url = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:file_url];
			if (default_app_url && [default_app_url path]) {
				if (const char* utf8_val = [[default_app_url path] UTF8String]) {
					return std::string(utf8_val);
				}
			}
		END_AUTORELEASE_POOL
		return std::nullopt;
	}


	std::vector<std::string> GetCompatibleBundlePaths(const std::string& filepath)
	{
		std::vector<std::string> result;
		BEGIN_AUTORELEASE_POOL(pool)
			NSURL *file_url = FileURLFromUTF8Path(filepath);
			if (!file_url) {
				return result;
			}

			NSArray *compatible_app_urls = nil;
			NSWorkspace *workspace = [NSWorkspace sharedWorkspace];

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 120000
			// Dynamically checks the method availability at runtime
			if ([workspace respondsToSelector:@selector(URLsForApplicationsToOpenURL:)]) {
				compatible_app_urls = [workspace URLsForApplicationsToOpenURL:file_url];
			} else
#endif
			{
				// Fallback for older macOS versions
				if (NSURL* default_app_url = [workspace URLForApplicationToOpenURL:file_url]) {
					compatible_app_urls = [NSArray arrayWithObject:default_app_url];
				} else {
					compatible_app_urls = [NSArray array];
				}
			}

			for (NSUInteger i = 0; i < [compatible_app_urls count]; i++) {
				NSURL *app_url = [compatible_app_urls objectAtIndex:i];
				if (app_url && [app_url path]) {
					if (const char* utf8_val = [[app_url path] UTF8String]) {
						result.emplace_back(utf8_val);
					}
				}
			}
		END_AUTORELEASE_POOL
		return result;
	}


	std::optional<std::unordered_map<std::string, std::string>> ParseAppBundleMetadata(const std::string& bundle_path, const std::vector<std::string>& keys)
	{
		BEGIN_AUTORELEASE_POOL(pool)
			NSURL *app_url = FileURLFromUTF8Path(bundle_path);
			if (!app_url) {
				return std::nullopt;
			}
			NSBundle *bundle = [NSBundle bundleWithURL:app_url];
			if (!bundle) {
				return std::nullopt;
			}
			NSDictionary *info_dict = [bundle infoDictionary];
			if (!info_dict) {
				return std::nullopt;
			}
			std::unordered_map<std::string, std::string> extracted_metadata;
			for (const auto& key : keys) {
				NSString *ns_key = [NSString stringWithUTF8String:key.c_str()];
				if (!ns_key) {
					continue;
				}
				NSString *ns_value = [info_dict objectForKey:ns_key];
				if (ns_value && [ns_value isKindOfClass:[NSString class]]) {
					if (const char* utf8_val = [ns_value UTF8String]) {
						extracted_metadata[key] = utf8_val;
					}
				}
			}
			return std::make_optional(std::move(extracted_metadata));
		END_AUTORELEASE_POOL
	}


	std::string ConvertUTIToMime(const std::string& uti)
	{
		if (uti.empty()) {
			return {};
		}
		std::string mime;
		BEGIN_AUTORELEASE_POOL(pool)
			NSString *ns_uti = [NSString stringWithUTF8String:uti.c_str()];
			if (ns_uti && [ns_uti length] > 0) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 110000 // UTType is available on macOS 11.0+
				// Dynamic class lookup via NSClassFromString
				Class ut_type_class = NSClassFromString(@"UTType");
				if (ut_type_class) {
					if (UTType *type = [ut_type_class typeWithIdentifier:ns_uti]) {
						if (NSString *ns_mime = [type preferredMIMEType]) {
							if (const char *utf8_ptr_mime = [ns_mime UTF8String]) {
								mime = utf8_ptr_mime;
							}
						}
					}
				} else
#endif
				{
					// Legacy approach for older macOS versions.
#if __has_feature(objc_arc)
					CFStringRef cf_mime_tag = UTTypeCopyPreferredTagWithClass((__bridge CFStringRef)ns_uti, kUTTagClassMIMEType);
					if (cf_mime_tag) {
						// Transfer ownership of the CFStringRef to ARC.
						NSString *ns_mime = (__bridge_transfer NSString *)cf_mime_tag;
						if (const char *utf8_ptr_mime = [ns_mime UTF8String]) {
							mime = utf8_ptr_mime;
						}
					}
#else // Manual Retain-Release (MRC) mode or GCC.
					CFStringRef cf_mime_tag = UTTypeCopyPreferredTagWithClass((CFStringRef)ns_uti, kUTTagClassMIMEType);
					if (cf_mime_tag) {
						NSString *ns_mime = [(NSString *)cf_mime_tag autorelease];
						if (const char *utf8_ptr_mime = [ns_mime UTF8String]) {
							mime = utf8_ptr_mime;
						}
					}
#endif
				}
			}
		END_AUTORELEASE_POOL
		return mime;
	}

} // namespace openwith::launch_services

#endif // __APPLE__
