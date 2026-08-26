# Where helper.sh comes from, and how to update it

`helper.sh` in this directory is a **byte-for-byte copy** from f4:

| | |
|---|---|
| Upstream | https://github.com/unxed/f4 |
| Path | `plugins/netfox/fishplus/helper.sh` |
| Commit | `9a03d417` (portable `chown` invocation) |
| License | BSD-3-Clause (see the f4 repository) |

It is deliberately **not** edited here, not even to remove the parts NetRocks
does not call yet. f4 is where the protocol is developed, and every local edit
would have to be re-applied by hand on every refresh. Keeping the file
identical makes an update a copy plus a version bump.

The only thing done to it at runtime is what f4 does too: the literal
`__F4_TOKEN__` is replaced with the per-session token, and comments and blank
lines are stripped before upload (`FishPlusScript.cpp`, `Compact()`).

## Updating

```sh
# from the far2l source root
NetRocks/src/Protocol/FISHPLUS/Helpers/sync-from-f4.sh /path/to/f4
```

The script copies the file, verifies the token placeholder is still present and
prints the new upstream commit so this table can be updated.

## What has to be checked after an update

The helper and the C++ client are two halves of one protocol. These are the
places that are coupled, in the order they would break:

1. **`F4PROTO` in helper.sh vs `PROTOCOL_VERSION` in `FishPlusScript.h`.**
   A bump means the wire format changed; read f4's `FISH+.md` changelog before
   doing anything else. The handshake refuses a mismatch rather than guessing.
2. **The bootstrap.** `BootstrapLine()` in `FishPlusScript.cpp` is a port of
   `BootstrapLine()` in f4's `script.go`, and the two must agree on the marker
   (`F4RDY<token>`) and the end marker (`F4EOF`). If f4 changes how the helper
   is fed in, this is what changes with it.
3. **`Compact()`.** Same file, port of the Go function of the same name. The
   helper is written so that it survives comment stripping - no here-documents,
   no multi-line literals. If upstream ever adds one, `Compact()` must learn
   about it or the upload will break in a way that looks like a hung session.
4. **Listing formats.** `FishPlusListing.cpp` ports f4's `fs.go` and `ls.go`.
   The formats are pinned by `F4FMT_FIND`, `F4FMT_STAT`, `F4FMT_BSD` and
   `F4LSOPT` in the helper; a new column anywhere means a new field count here.
5. **New commands.** They cost nothing: an unknown command is simply never
   sent. Adding support for one is additive on this side.

## Running the protocol tests after an update

f4's own `*AgainstLocalShell` tests are the reference. On this side the
equivalent is to point a FISH+ site at `localhost` and walk a directory tree,
but the cheaper check is f4's, because both clients speak the same wire
protocol to the same script:

```sh
cd /path/to/f4 && go test ./plugins/netfox/fishplus/
```

Run it at least once with `/bin/sh` pointing at `dash`, and preferably also at
`busybox sh`. `bash` is forgiving in ways the shells on real hosts are not.
