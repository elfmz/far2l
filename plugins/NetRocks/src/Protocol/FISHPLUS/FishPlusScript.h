#pragma once
#include <string>

/*
	Port of f4's plugins/netfox/fishplus/script.go

	Everything in this file is coupled to Helpers/helper.sh and must be kept
	in sync with the upstream Go implementation whenever that file is
	refreshed. See Helpers/UPSTREAM.md.
*/

namespace FishPlus
{
	// Wire protocol revision implemented by both this code and helper.sh.
	// The remote reports its own in the handshake banner; a mismatch is fatal.
	static const int PROTOCOL_VERSION = 1;

	// Substituted by a per-session random token before the helper is uploaded.
	extern const char TOKEN_PLACEHOLDER[];

	// The line that follows the helper on the wire. It cannot occur inside the
	// script, which is ours to write.
	extern const char HELPER_END_MARKER[];

	// 64 bits of randomness rendered as lowercase hex. Hex only: the token
	// ends up inside MatchWildcard patterns, so it must not contain '*' or '?'.
	std::string NewToken();

	// What the bootstrap prints once the remote shell has parsed it and
	// started executing.
	std::string ReadyMarker(const std::string &token);

	// Strips comments, blank lines and leading indentation. The helper travels
	// over the wire on every connect, so shaving it down is worth it. This is
	// why helper.sh must not rely on here-documents or multi-line literals.
	std::string Compact(const std::string &src);

	// Reads helper.sh, substitutes the token and compacts the result.
	// Throws ProtocolError when the file cannot be read.
	std::string LoadHelperScript(const char *path, const std::string &token);

	// The single line that has to reach the remote shell before the helper does.
	//
	// A shell reads its script from the same stream the requests arrive on, and
	// it does not read it a byte at a time: dash fills a parse buffer, and
	// whatever lands in that buffer past the end of the script is parsed as
	// part of it. Send the script and a request together and the request gets
	// executed as a shell command while the client waits forever for an answer.
	// bash reads byte by byte and never shows this, which is why it survived so
	// long unnoticed on machines whose /bin/sh is bash.
	//
	// So the script is not parsed off the stream at all. What the shell parses
	// is this one line, which prints a marker and then reads the script in
	// through the shell's own read builtin - which takes its bytes from the
	// file descriptor and cannot run ahead of itself.
	//
	// The marker is printed in two pieces, F4R"DY", so that a terminal echoing
	// the line back cannot be mistaken for the shell answering.
	std::string BootstrapLine(const std::string &token);
}
