# Where helper.sh and helper.ps1 come from, and how to update them

Both helpers in this directory are **byte-for-byte copies** from f4:

| File | Upstream | Path | Commit | License |
|---|---|---|---|---|
| `helper.sh`  | https://github.com/unxed/f4 | `plugins/netfox/fishplus/helper.sh`  | `9a03d417` (portable `chown` invocation) | BSD-3-Clause (see the f4 repository) |
| `helper.ps1` | https://github.com/unxed/f4 | `plugins/netfox/fishplus/helper.ps1` | `dcf89fe` (ffind as job)                | BSD-3-Clause (see the f4 repository) |

They are deliberately **not** edited here, not even to remove the parts NetRocks
does not call yet. f4 is where the protocol is developed, and every local edit
would have to be re-applied by hand on every refresh. Keeping the files
identical makes an update a copy plus a version bump.

`helper.sh` is uploaded to every peer whose login shell is POSIX; `helper.ps1`
is uploaded when the transport probes into a PowerShell host instead. Both
speak the same FISH+ wire; the only difference is how the bootstrap line
delivers them (see `../FishPlusScript.cpp`, `BootstrapLine()` vs
`BootstrapLinePwshB64()`).

The only thing done to either at runtime is what f4 does too: the literal
`__F4_TOKEN__` is replaced with the per-session token, and comments and blank
lines are stripped before upload (`FishPlusScript.cpp`, `Compact()`).

## Updating

```sh
# from the far2l source root
NetRocks/src/Protocol/FISHPLUS/Helpers/sync-from-f4.sh /path/to/f4
```

The script copies both files, verifies the token placeholder is still present
and prints the two upstream protocol version numbers so the table above can be
updated.

## What has to be checked after an update

The helpers and the C++ client are two halves of one protocol. These are the
places that are coupled, in the order they would break:

1. **`F4PROTO` in helper.sh (`$F4PROTO` in helper.ps1) vs `PROTOCOL_VERSION`
   in `FishPlusScript.h`.** A bump means the wire format changed; read f4's
   `FISH+.md` changelog before doing anything else. The handshake refuses a
   mismatch rather than guessing. Both helpers must report the same number.
2. **The bootstraps.** `BootstrapLine()` in `FishPlusScript.cpp` is a port of
   `BootstrapLine()` in f4's `script.go`, and `BootstrapLinePwshB64()` is a
   port of `Base64BootstrapLinePwsh()`. Each must agree with its Go peer on
   the ready marker (`F4RDY<token>`) and, for the sh path, the end marker
   (`F4EOF`).  If f4 changes how a helper is fed in, this is what changes
   with it.
3. **`Compact()`.** Same file, port of the Go function of the same name. Both
   helpers are written so that they survive comment stripping — no
   here-documents, no multi-line literals, no PowerShell here-strings. If
   upstream ever adds one, `Compact()` must learn about it or the upload will
   break in a way that looks like a hung session.
4. **Listing formats.** `FishPlusListing.cpp` ports f4's `fs.go` and `ls.go`.
   The formats are pinned by `F4FMT_FIND`, `F4FMT_STAT`, `F4FMT_BSD` and
   `F4LSOPT` in the helper; a new column anywhere means a new field count here.
5. **The `flavor:` feature tag.** The pwsh helper announces `flavor:pwsh` in
   its banner so the client can key path translation (Cygwin-shape `/c/foo`)
   on it. If a new flavor ever appears, `Features::Flavor()` and the C++
   callers that consume it have to grow with it.
6. **New commands.** They cost nothing: an unknown command is simply never
   sent. Adding support for one is additive on this side.

## Running the protocol tests after an update

f4's own `*AgainstLocalShell` and `*AgainstLocalPwsh` tests are the reference.
On this side the equivalent is to point a FISH+ site at `localhost` and walk
a directory tree, but the cheaper check is f4's, because both clients speak
the same wire protocol to the same scripts:

```sh
cd /path/to/f4 && go test ./plugins/netfox/fishplus/
```

Run it at least once with `/bin/sh` pointing at `dash`, and preferably also at
`busybox sh`. `bash` is forgiving in ways the shells on real hosts are not.
For the PowerShell helper, the equivalent tests are gated on `pwsh` being on
PATH.
