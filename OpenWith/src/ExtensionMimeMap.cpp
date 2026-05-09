#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

#include "XDGBasedAppProvider.hpp"
#include <string>
#include <unordered_map>

const std::unordered_map<std::string, std::string>& XDGBasedAppProvider::GetExtMimeMap()
{
    static const std::unordered_map<std::string, std::string> map = {
		// Shell / scripts / source code

		{".sh",    "application/x-shellscript"},
		{".bash",  "application/x-shellscript"},
		{".csh",   "application/x-csh"},
		{".zsh",   "application/x-shellscript"},
		{".ps1",   "application/x-powershell"},
		{".py",    "text/x-python"},
		{".pyw",   "text/x-python"},
		{".pl",    "text/x-perl"},
		{".pm",    "text/x-perl"},
		{".rb",    "text/x-ruby"},
		{".php",   "application/x-php"},
		{".phps",  "application/x-php"},
		{".js",    "application/javascript"},
		{".mjs",   "application/javascript"},
		{".java",  "text/x-java-source"},
		{".c",     "text/x-csrc"},
		{".h",     "text/x-chdr"},
		{".cpp",   "text/x-c++src"},
		{".cc",    "text/x-c++src"},
		{".cxx",   "text/x-c++src"},
		{".hpp",   "text/x-c++hdr"},
		{".go",    "text/x-go"},
		{".rs",    "text/rust"},
		{".swift", "text/x-swift"},

		// Plain text / markup / data

		{".txt",   "text/plain"},
		{".md",    "text/markdown"},
		{".markdown","text/markdown"},
		{".tex",   "application/x-tex"},
		{".csv",   "text/csv"},
		{".tsv",   "text/tab-separated-values"},
		{".log",   "text/plain"},
		{".json",  "application/json"},
		{".yaml",  "text/yaml"},
		{".yml",   "text/yaml"},
		{".xml",   "application/xml"},
		{".html",  "text/html"},
		{".htm",   "text/html"},
		{".xhtml", "application/xhtml+xml"},
		{".ics",   "text/calendar"},

		// Office / documents

		{".pdf",   "application/pdf"},
		{".doc",   "application/msword"},
		{".docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
		{".xls",   "application/vnd.ms-excel"},
		{".xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
		{".ppt",   "application/vnd.ms-powerpoint"},
		{".pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
		{".odt",   "application/vnd.oasis.opendocument.text"},
		{".ods",   "application/vnd.oasis.opendocument.spreadsheet"},
		{".odp",   "application/vnd.oasis.opendocument.presentation"},
		{".epub",  "application/epub+zip"},
		{".rtf",   "application/rtf"},

		// Images

		{".jpg",   "image/jpeg"},
		{".jpeg",  "image/jpeg"},
		{".jpe",   "image/jpeg"},
		{".png",   "image/png"},
		{".gif",   "image/gif"},
		{".webp",  "image/webp"},
		{".svg",   "image/svg+xml"},
		{".ico",   "image/vnd.microsoft.icon"},
		{".bmp",   "image/bmp"},
		{".tif",   "image/tiff"},
		{".tiff",  "image/tiff"},
		{".heic",  "image/heic"},
		{".avif",  "image/avif"},
		{".apng",  "image/apng"},

		// Audio

		{".mp3",   "audio/mpeg"},
		{".m4a",   "audio/mp4"},
		{".aac",   "audio/aac"},
		{".ogg",   "audio/ogg"},
		{".oga",   "audio/ogg"},
		{".opus",  "audio/opus"},
		{".wav",   "audio/x-wav"},
		{".flac",  "audio/flac"},
		{".mid",   "audio/midi"},
		{".midi",  "audio/midi"},
		{".weba",  "audio/webm"},

		// Video

		{".mp4",   "video/mp4"},
		{".m4v",   "video/mp4"},
		{".mov",   "video/quicktime"},
		{".mkv",   "video/x-matroska"},
		{".webm",  "video/webm"},
		{".ogv",   "video/ogg"},
		{".avi",   "video/x-msvideo"},
		{".flv",   "video/x-flv"},
		{".wmv",   "video/x-ms-wmv"},
		{".3gp",   "video/3gpp"},
		{".3g2",   "video/3gpp2"},
		{".ts",    "video/mp2t"},

		// Archives / compressed

		{".zip",   "application/zip"},
		{".tar",   "application/x-tar"},
		{".gz",    "application/gzip"},
		{".tgz",   "application/gzip"},
		{".bz",    "application/x-bzip"},
		{".bz2",   "application/x-bzip2"},
		{".xz",    "application/x-xz"},
		{".7z",    "application/x-7z-compressed"},
		{".rar",   "application/vnd.rar"},
		{".jar",   "application/java-archive"},

		// Executables / binaries

		{".exe",   "application/x-ms-dos-executable"},
		{".dll",   "application/x-msdownload"},
		{".so",    "application/x-sharedlib"},
		{".elf",   "application/x-executable"},
		{".bin",   "application/octet-stream"},
		{".class", "application/java-vm"},

		// Fonts

		{".ttf",   "font/ttf"},
		{".otf",   "font/otf"},
		{".woff",  "font/woff"},
		{".woff2", "font/woff2"},
		{".eot",   "application/vnd.ms-fontobject"},

		// PostScript / vector

		{".ps",    "application/postscript"},
		{".eps",   "application/postscript"},
		{".ai",    "application/postscript"},

		// Disk images / containers

		{".iso",   "application/x-iso9660-image"},
		{".img",   "application/octet-stream"},
		{".dmg",   "application/x-apple-diskimage"},

		// Web / misc

		{".css",   "text/css"},
		{".map",   "application/json"},
		{".wasm",  "application/wasm"},
		{".jsonld","application/ld+json"},
		{".webmanifest","application/manifest+json"},

		// CAD / specialized

		{".dxf",   "image/vnd.dxf"},
		{".dwg",   "application/acad"},

		// Mail / office miscellany

		{".msg",   "application/vnd.ms-outlook"}
    };
    return map;
}

#endif