#pragma once
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../SHELL/WayToShell.h"

/*
	Port of f4's plugins/netfox/fishplus/session.go - the FISH+ wire protocol,
	version 1.

	One line per request, optionally followed by one line per path:

		<id> <cmd> [<short arg> ...]
		<path>

	Short arguments are bare tokens: never empty, never containing whitespace,
	which is why every path travels on a line of its own. A path therefore
	reaches the remote host byte for byte. Only a path a line based channel
	cannot carry - one containing a newline, or one starting with the escape
	marker itself - is base64 encoded and prefixed with '~'.

	The answer is zero or more payload lines followed by a terminator:

		.<token> <id> ok|err [message]

	The token is 64 bits of randomness generated per session and substituted
	into the helper before upload, which is what makes the terminator
	unambiguous: output of ls, stat or grep cannot accidentally look like one.

	Binary payload arrives in frames, each a header line "#<n>" followed by
	exactly n raw bytes. Frames are only recognized for commands issued as data
	commands, so a text payload line starting with '#' stays a text line.
*/

namespace FishPlus
{
	// Caps mirroring the Go client, so a confused remote cannot make the
	// broker allocate without bound.
	static const size_t MAX_FRAME_LEN = 64ull << 20;
	static const size_t MAX_READ_LEN = 16ull << 20;
	static const size_t MAX_WRITE_LEN = 16ull << 20;

	// How much travels in one request. Chunks are powers of two and aligned to
	// their own size, which lets the remote dd position itself with a single
	// lseek and read whole blocks even where GNU's iflag=skip_bytes is missing.
	static const size_t READ_CHUNK = 256 * 1024;
	static const size_t WRITE_CHUNK = 256 * 1024;
	// The base64 backend makes the remote shell read its payload one byte per
	// syscall, so it gets smaller chunks at the price of more round trips.
	static const size_t WRITE_CHUNK_B64 = 64 * 1024;

	struct Response
	{
		bool ok{false};
		std::string msg;
		std::vector<std::string> lines;
		std::vector<unsigned char> data;

		// True when the given line is present, used to tell a refusal that
		// drained its payload from one that could not.
		bool HasLine(const char *what) const;
	};

	class Features
	{
		std::set<std::string> _names;
		std::string _raw;

	public:
		// Parses the banner tail, i.e. everything after "FISHPLUS <version>".
		void Parse(const std::string &tail);

		bool Has(const char *name) const;
		const std::string &Raw() const { return _raw; }

		// Returns the value of a "<prefix><value>" feature, e.g. "read:dd"
		// queried with "read:" gives "dd". Empty when absent.
		std::string Valued(const char *prefix) const;

		std::string ListingMode() const { return Valued("mode:"); }
		std::string ReadMode() const { return Valued("read:"); }
		std::string WriteMode() const { return Valued("write:"); }
	};

	class Session
	{
		std::shared_ptr<WayToShell> _way;
		std::string _token;
		unsigned long long _seq{0};
		bool _broken{false};
		bool _raw_payload_safe{false};
		Features _feats;

		Response ReadResponse(unsigned long long id, bool binary);
		void SendRaw(const std::string &data);

		Response ExecFull(const char *cmd, const std::vector<std::string> &paths,
			const std::vector<std::string> &args,
			const void *payload, size_t payload_len, bool encoded, bool binary);

	public:
		Session(std::shared_ptr<WayToShell> way);
		~Session();

		// Uploads the helper and waits for its banner. helper_path is resolved
		// relative to the broker's working directory, like SHELL/remote.sh is.
		// tty_transport tells whether the stream is backed by a pseudo terminal,
		// which decides whether raw binary payload may be sent at all.
		void Handshake(const char *helper_path, bool tty_transport);

		const Features &Feats() const { return _feats; }
		const std::string &Token() const { return _token; }

		// False when the transport is a terminal the helper could not tame, in
		// which case binary payload must be base64 encoded.
		bool RawPayloadSafe() const { return _raw_payload_safe; }

		bool Broken() const { return _broken; }

		// Poisons the session after the caller found out, by means the session
		// itself cannot see, that the two sides no longer agree on how much of
		// the stream has been consumed.
		void MarkBroken() { _broken = true; }

		// Renders a path as one protocol line, escaping it only when a raw line
		// would not survive the round trip.
		static std::string EncodePathLine(const std::string &path);

		Response Exec(const char *cmd, const std::vector<std::string> &args = {});
		Response ExecPath(const char *cmd, const std::string &path,
			const std::vector<std::string> &args = {});
		Response ExecPaths(const char *cmd, const std::vector<std::string> &paths,
			const std::vector<std::string> &args = {});

		// Same, but binary frames are expected in the answer.
		Response ExecPathData(const char *cmd, const std::string &path,
			const std::vector<std::string> &args = {});

		// Runs a command carrying a payload of its own after the path lines. A
		// raw payload is exactly the announced number of bytes with nothing
		// around it; an encoded one is a single base64 line, which the remote
		// helper can consume with the shell alone and which therefore stays
		// exact on hosts whose dd cannot stop on a byte boundary.
		Response ExecPayload(const char *cmd, const std::vector<std::string> &paths,
			const std::vector<std::string> &args,
			const void *payload, size_t payload_len, bool encoded);

		// Throws ProtocolError built from the response when it is not ok.
		static void ThrowIfFailed(const Response &resp, const char *what, const std::string &path);
	};
}
