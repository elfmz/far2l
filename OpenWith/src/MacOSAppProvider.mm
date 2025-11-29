#if defined(__APPLE__)

#include <AvailabilityMacros.h>

#import <Cocoa/Cocoa.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif
#include "MacOSAppProvider.hpp"
#include "lng.hpp"
#include "WideMB.h"
#include "common.hpp"
#include "utils.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>

// A temporary structure to hold application details fetched from the system
// before they are processed into the final CandidateInfo format.
struct MacCandidateTempInfo {
    std::wstring name;
    std::wstring id; // The full path to the .app bundle, used as a unique identifier.
    std::wstring info; // Version string used for disambiguation if names conflict.
};


// ****************************** Implementation ******************************

MacOSAppProvider::MacOSAppProvider(TMsgGetter msg_getter) : AppProvider(std::move(msg_getter))
{
}

// Helper to convert an NSURL object to a UTF-8 encoded std::string path.
static std::string NSURLToPath(NSURL *url) {
    if (!url) return {};
    return std::string([[url path] UTF8String]);
}

// Helper to extract essential application metadata from its bundle.
static MacCandidateTempInfo AppBundleToTempInfo(NSURL *appURL) {
    MacCandidateTempInfo c;
    NSBundle *bundle = [NSBundle bundleWithURL:appURL];
    NSDictionary *infoDict = [bundle infoDictionary];

    // Prefer the display name, but fall back to the filename if it's not available.
    NSString *bundleName = [infoDict objectForKey:@"CFBundleDisplayName"] ?: [infoDict objectForKey:@"CFBundleName"];
    // Get version strings. Prefer the short, user-facing version.
    NSString *bundleShortVersion = [infoDict objectForKey:@"CFBundleShortVersionString"];
    NSString *bundleVersion = [infoDict objectForKey:@"CFBundleVersion"];

    c.name = StrMB2Wide(bundleName ? [bundleName UTF8String] : NSURLToPath(appURL));
    c.id = StrMB2Wide(NSURLToPath(appURL));
    
    // Store the most descriptive version string available for disambiguation.
    if (bundleShortVersion) {
        c.info = StrMB2Wide([bundleShortVersion UTF8String]);
    } else if (bundleVersion) {
        c.info = StrMB2Wide([bundleVersion UTF8String]);
    }

    return c;
}

// Helper to safely escape a string argument for the shell.
// This wraps the argument in single quotes, which is a robust way to prevent shell interpretation.
static std::wstring EscapeForShell(const std::wstring& arg) {
    std::wstring out;
    out.push_back(L'\'');
    for (wchar_t c : arg) {
        if (c == L'\'') {
            // A single quote is escaped by closing the quote, adding an escaped quote,
            // and then re-opening the quote (e.g., 'it's' becomes 'it'\''s').
            out.append(L"'\\''");
        } else {
            out.push_back(c);
        }
    }
    out.push_back(L'\'');
    return out;
}


// Find application candidates that can open all specified files.
// The logic uses a scoring system to rank candidates. The default application for a file type
// receives a higher score, ensuring it appears first in the list.
// To optimize performance when handling many files, this function caches the application lists
// based on the file's Uniform Type Identifier (UTI).
std::vector<CandidateInfo> MacOSAppProvider::GetAppCandidates(const std::vector<std::wstring>& filepaths) {
	// Return immediately if the input vector is empty.
	if (filepaths.empty()) {
		return {};
	}

	// Clear the class-level profile cache for the new operation
	_last_uti_profiles.clear();

	// --- Part 1: Candidate Discovery and Scoring with Caching ---

	// A map to store definitive metadata for every unique app encountered.
	// Key: application ID (full path), Value: application metadata.
	std::unordered_map<std::wstring, MacCandidateTempInfo> all_apps_info;

	// A map to accumulate scores for each candidate application.
	std::unordered_map<std::wstring, int> app_scores;

	// A map to count how many of the selected files each application can open.
	std::unordered_map<std::wstring, int> app_occurrence_count;

	// Define scores for ranking. A default app gets a significantly higher score.
	constexpr int DEFAULT_APP_SCORE = 10;
	constexpr int OTHER_APP_SCORE   = 1;

	// A cache to store the list of applications for a given Uniform Type Identifier (UTI).
	// This dramatically speeds up processing when many files of the same type are selected.
	struct AppListCacheEntry {
		NSURL* default_app;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101000 && defined(__clang__)
		NSArray<NSURL *>* all_apps;
#else
		NSArray *all_apps;
#endif
	};

	// Custom hash and equality functors are required for using NSString* as a key in std::unordered_map.
	struct NSStringHash {
		std::size_t operator()(NSString* const& s) const { return [s hash]; }
	};
	struct NSStringEqual {
		bool operator()(NSString* const& lhs, NSString* const& rhs) const { return [lhs isEqual:rhs]; }
	};
	std::unordered_map<NSString*, AppListCacheEntry, NSStringHash, NSStringEqual> uti_cache;


	// Iterate through each selected file to find and score compatible applications.
	for (const auto& filepath : filepaths) {
		NSString *path = [NSString stringWithUTF8String:StrWide2MB(filepath).c_str()];
		NSURL *fileURL = [NSURL fileURLWithPath:path];
		if (!fileURL) {
			// File path is invalid, cache as inaccessible
			_last_uti_profiles.insert({ std::string(""), false }); // Store empty string for inaccessible
			continue;
		}

		// Determine the file's UTI to use it as a cache key.
		NSString *uti = nil;
		NSError *error = nil;
		[fileURL getResourceValue:&uti forKey:NSURLTypeIdentifierKey error:&error];
		if (error || !uti) {
			// Failed to get UTI (e.g., file not found, permissions), cache as inaccessible
			_last_uti_profiles.insert({ std::string(""), false }); // Store empty string for inaccessible
			continue;
		}

		// --- Begin: Profile Caching Logic ---
		// Cache the file profile (UTI and accessible status).
		const char* uti_cstr = [uti UTF8String];
		std::string uti_std_str = (uti_cstr ? uti_cstr : "");
		_last_uti_profiles.insert({ uti_std_str, true });
		// --- End: Profile Caching Logic ---


		NSURL* defaultAppURL;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101000 && defined(__clang__)
		NSArray<NSURL *>* allAppURLs;
#else
		NSArray *allAppURLs;
#endif

		// Check if the application list for this UTI is already in our cache.
		auto cache_it = uti_cache.find(uti);
		if (cache_it != uti_cache.end()) {
			// Cache hit: Use the stored application lists.
			defaultAppURL = cache_it->second.default_app;
			allAppURLs = cache_it->second.all_apps;
		} else {
			// Cache miss: Query the system for the application lists.
			defaultAppURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:fileURL];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
			allAppURLs = [[NSWorkspace sharedWorkspace] URLsForApplicationsToOpenURL:fileURL];
#elif MAC_OS_X_VERSION_MAX_ALLOWED >= 1080 && defined(__clang__)
			allAppURLs = defaultAppURL ? @[defaultAppURL] : @[];
#else
			if (defaultAppURL) {
				allAppURLs = [NSArray arrayWithObject:defaultAppURL];
			} else {
				allAppURLs = [NSArray array];
			}
#endif
			// Store the results in the cache for subsequent files of the same type.
#ifdef __clang__
			uti_cache[uti] = {defaultAppURL, allAppURLs};
#else
			AppListCacheEntry entry;
			entry.default_app = defaultAppURL;
			entry.all_apps = allAppURLs;
			uti_cache[uti] = entry;
#endif
		}

		// A temporary set to ensure we process each application only once per file.
		std::unordered_set<std::wstring> processed_apps_for_this_file;

		// Process the default application.
		if (defaultAppURL) {
			MacCandidateTempInfo info = AppBundleToTempInfo(defaultAppURL);
			all_apps_info.try_emplace(info.id, info);
			app_scores[info.id] += DEFAULT_APP_SCORE;
			processed_apps_for_this_file.insert(info.id);
			app_occurrence_count[info.id]++;
		}

		// Process all other compatible applications.
#ifdef __clang__
		for (NSURL *appURL in allAppURLs) {
#else
		for (NSUInteger i = 0; i < [allAppURLs count]; i++) {
			NSURL *appURL = [allAppURLs objectAtIndex:i];
#endif
			MacCandidateTempInfo info = AppBundleToTempInfo(appURL);
			if (processed_apps_for_this_file.count(info.id)) {
				continue;
			}
			all_apps_info.try_emplace(info.id, info);
			app_scores[info.id] += OTHER_APP_SCORE;
			processed_apps_for_this_file.insert(info.id);
			app_occurrence_count[info.id]++;
		}
	}

	// --- Part 2: Filtering and Sorting ---

	// A temporary structure to hold candidates that can open all files, along with their final score.
	struct RankedCandidate {
		MacCandidateTempInfo info;
		int score;
		bool operator<(const RankedCandidate& other) const {
			if (score != other.score) return score > other.score;
			return info.name < other.info.name;
		}
	};

	std::vector<RankedCandidate> finalists;
	const size_t num_files = filepaths.size();

	// Filter the list, keeping only applications that can open every selected file.
	for (const auto& [app_id, count] : app_occurrence_count) {
		if (count == num_files) {
			finalists.push_back({ all_apps_info.at(app_id), app_scores.at(app_id) });
		}
	}

	// Sort the finalists based on their score in descending order.
	std::sort(finalists.begin(), finalists.end());

	// --- Part 3: Final List Generation ---

	std::vector<CandidateInfo> result;
	if (finalists.empty()) {
		return result;
	}
	result.reserve(finalists.size());

	// Count name occurrences to identify duplicates that need disambiguation.
	std::unordered_map<std::wstring, int> name_counts;
	for (const auto& candidate : finalists) {
		name_counts[candidate.info.name]++;
	}

	// Build the final list in the correct format.
	for (const auto& candidate : finalists) {
		CandidateInfo final_c;
		final_c.id = candidate.info.id;
		final_c.terminal = false;
		final_c.name = candidate.info.name;
		final_c.multi_file_aware = true;

		// If an app name is duplicated, append its version string to make it unique in the UI.
		if (name_counts[candidate.info.name] > 1 && !candidate.info.info.empty()) {
			final_c.name += L" (" + candidate.info.info + L")";
		}
		result.push_back(final_c);
	}

	return result;
}


// Constructs a single command line using the 'open' utility, which natively handles multiple files.
std::vector<std::wstring> MacOSAppProvider::ConstructLaunchCommands(const CandidateInfo& candidate, const std::vector<std::wstring>& filepaths) {
    if (candidate.id.empty() || filepaths.empty()) {
        return {};
    }

    // The 'open -a <app_path>' command tells the system to open files with a specific application.
    std::wstring cmd = L"open -a " + EscapeForShell(candidate.id);
    for (const auto& filepath : filepaths) {
        cmd += L" " + EscapeForShell(filepath);
    }
    
    return {cmd}; // Return a vector containing the single constructed command.
}

// Fetches detailed information about a candidate application from its bundle.
std::vector<Field> MacOSAppProvider::GetCandidateDetails(const CandidateInfo& candidate) {
    std::vector<Field> details;

    NSString *nsPath = [NSString stringWithUTF8String:StrWide2MB(candidate.id).c_str()];
    NSURL *appURL = [NSURL fileURLWithPath:nsPath];
    if (!appURL) return details;
    
    NSBundle *bundle = [NSBundle bundleWithURL:appURL];
    if (!bundle) return details;

    NSDictionary *infoDict = [bundle infoDictionary];

    NSString *appName = [infoDict objectForKey:@"CFBundleDisplayName"] ?: [infoDict objectForKey:@"CFBundleName"];

    if (appName) {
        details.push_back({m_GetMsg(MAppName), StrMB2Wide([appName UTF8String])});
    }

    details.push_back({m_GetMsg(MFullPath), candidate.id});

    NSString *execName = [infoDict objectForKey:@"CFBundleExecutable"];
    if (execName) {
        details.push_back({m_GetMsg(MExecutableFile), StrMB2Wide([execName UTF8String])});
    }

    NSString *bundleShortVersion = [infoDict objectForKey:@"CFBundleShortVersionString"];
    if (bundleShortVersion) {
        details.push_back({m_GetMsg(MVersion), StrMB2Wide([bundleShortVersion UTF8String])});
    }
    
    NSString *bundleVersion = [infoDict objectForKey:@"CFBundleVersion"];
    if (bundleVersion) {
        details.push_back({m_GetMsg(MBundleVersion), StrMB2Wide([bundleVersion UTF8String])});
    }

    return details;
}


// Collects unique formatted profile strings based on the last GetAppCandidates call.
// This function performs the UTI-to-MIME-type conversion on demand.
std::vector<std::wstring> MacOSAppProvider::GetMimeTypes()
{
	// Use a set to store only the unique formatted profile strings.
	std::unordered_set<std::wstring> unique_profile_strings;
	unique_profile_strings.reserve(_last_uti_profiles.size());

	for (const auto& profile : _last_uti_profiles) {
		if (!profile.accessible) {
			// File was inaccessible (invalid path, permissions, etc.)
			unique_profile_strings.insert(L"(inaccessible)");
			continue;
		}

		// File was accessible, convert its cached UTI to a MIME type.
		std::wstring result_mime;
		NSString *uti = [NSString stringWithUTF8String:profile.uti.c_str()];

		// Use the appropriate API based on the target macOS version.
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 110000 // UTType is available on macOS 11.0+
		// Modern approach for macOS 11.0 and later, converting a UTI to a MIME type.
		UTType *type = [UTType typeWithIdentifier:uti];
		if (type) {
			NSString *mimeStr = type.preferredMIMEType;
			if (mimeStr) result_mime = StrMB2Wide([mimeStr UTF8String]);
		}
#else
		// Legacy approach for older macOS versions.
#ifdef __clang__
		CFStringRef mimeType = UTTypeCopyPreferredTagWithClass((__bridge CFStringRef)uti,
															   kUTTagClassMIMEType);
		if (mimeType) {
			// Transfer ownership of the CFStringRef to ARC.
			NSString *mimeStr = (__bridge_transfer NSString *)mimeType;
			result_mime = StrMB2Wide([mimeStr UTF8String]);
		}
#else // gcc does not support ARC.
		CFStringRef mimeType = UTTypeCopyPreferredTagWithClass((CFStringRef)uti,
															   kUTTagClassMIMEType);
		if (mimeType) {
			NSString *mimeStr = [(NSString *)mimeType autorelease];
			result_mime = StrMB2Wide([mimeStr UTF8String]);
		}
#endif
#endif

		if (result_mime.empty()) {
			// File was accessible, UTI was found, but no MIME type equivalent.
			unique_profile_strings.insert(L"(none)");
		} else {
			// File was accessible and had a corresponding MIME type.
			unique_profile_strings.insert(L"(" + result_mime + L")");
		}
	}

	// Convert the set of unique strings to the required vector format.
	std::vector<std::wstring> result_vec(unique_profile_strings.begin(), unique_profile_strings.end());
	return result_vec;
}

#endif // __APPLE__
