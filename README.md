<p align="center">
  <img src="docs/logo.svg" width="96" alt="iretable-tools logo">
</p>

# iretable-tools

A command-line toolkit for Pointer Lab `.iretable` project files.

[![CI](https://github.com/HeathHowren/iretable-tools/actions/workflows/ci.yml/badge.svg)](https://github.com/HeathHowren/iretable-tools/actions/workflows/ci.yml)

`iretable` reads and writes the project files Pointer Lab saves: the address
list, its pointer chains, the value types, structures and scripts. From the
command line it converts a Cheat Engine `.CT` table into one, exports back where
the fields map, diffs two tables, shifts fixed addresses after a rebase, and
lints a file for problems. It touches files on disk only. It never opens a
process.

iretable-tools is written by Heath Howren
([Cyborg Elf](https://www.youtube.com/cyborgelf)) of
[Game Reversal Club](https://gamereversal.club) as a command-line companion to
[Pointer Lab](https://gamereversal.club/tools/pointer-lab/), whose `.iretable`
format it implements from the published
[format specification](https://github.com/HeathHowren/Pointer-Lab/blob/master/docs/iretable-format.md).

```
$ iretable lint broken.iretable
line 2: orphan-field: Field comes before any struct record; ignoring it.
line 3: bad-hex: Unreadable id or address; skipping that entry.
line 4: unknown-type: Unknown value type "weirdtype"; skipping that entry.
line 5: malformed-chain: Malformed pointer chain; keeping the entry as a fixed address.
duplicate-id: Duplicate entry id 4.
frozen-no-value: Entry 4 is frozen but has no frozen value.
6 problem(s) found.
$ echo $?
1
```

*Real output from `iretable lint` on `tests/fixtures/malformed.iretable`, a
deliberately broken table used by the tests.*

## What it does

- **Reads and writes every `.iretable` version.** Headers 1, 2 and 3 all load,
  with the full escaping rules and every record type. The writer produces
  Pointer Lab's canonical form byte for byte, so a file this tool writes is a
  file Pointer Lab reads without complaint. A UTF-8 byte-order mark before the
  header is tolerated.
- **Preserves records it does not recognize.** A record type from a newer
  Pointer Lab is kept verbatim and written back, so nothing is lost on a
  round-trip. Pointer Lab itself only skips these.
- **Imports Cheat Engine tables.** `convert` reads a `.CT` address list, keeping
  descriptions, value types, static addresses and pointer offset chains, and
  turning Cheat Engine groups into entry groups. Cheat Engine stores pointer
  offsets in reverse; `convert` corrects the order. Every field with no
  `.iretable` counterpart is reported on stderr, so a lossy import is loud, not
  silent.
- **Exports back to Cheat Engine.** `export` writes a `.CT` where the records
  map and reports the ones that do not: symbols, scripts, structures, freeze
  state and hotkeys.
- **Diffs two tables** at the entry level: added, removed and changed rows,
  matched by description and location, with the changed fields named.
- **Rebases fixed addresses** by a base delta. It shifts each fixed-address
  entry and the base of each pointer chain that starts at a fixed address.
  Module-rooted entries are left alone, because they are stored relative to
  their module and survive a relocation on their own.
- **Lints a file** for malformed lines, bad hex, unknown value types, broken
  pointer chains, duplicate ids and frozen entries with no value, and exits
  non-zero when it finds any.
- **Speaks JSON.** `--json` on `show`, `convert`, `diff` and `lint` gives
  scriptable output.
- **No runtime dependencies.** The `.CT` reader is a small hand-written XML
  parser, and the CLI is standard C++20 with a static C runtime, so the released
  binary runs on a clean Windows machine.

## Download

Get the latest zip from
[Releases](https://github.com/HeathHowren/iretable-tools/releases). It contains:

```
iretable.exe             the command-line tool
LICENSE, README.md, CHANGELOG.md, THIRD_PARTY_NOTICES.md
```

`iretable.exe` needs nothing else installed. The binary is unsigned, so
antivirus software or SmartScreen may warn on first run; build from source if
you would rather not take a binary on trust.

## Quick start

```powershell
# Import a Cheat Engine table. Anything that cannot be represented is listed.
iretable convert table.CT -o table.iretable

# Look at what loaded.
iretable show table.iretable

# Check a hand-edited file before opening it in Pointer Lab.
iretable lint table.iretable

# After a game update moved the main module, shift the fixed addresses.
iretable rebase table.iretable --old-base 140000000 --new-base 150000000 -o moved.iretable

# See what changed between two tables.
iretable diff table.iretable moved.iretable
```

## Usage

```
iretable show    <in.iretable> [--json]
iretable convert <in.CT> -o <out.iretable> [--json]
iretable export  <in.iretable> --ct <out.CT>
iretable diff    <a.iretable> <b.iretable> [--json]
iretable rebase  <in.iretable> --old-base <hex> --new-base <hex> [-o <out.iretable>]
iretable lint    <in.iretable> [--json]
```

- `show` prints the table. With `--json` it prints the whole model, including
  pointer chains and any preserved unknown records.
- `convert` writes an `.iretable`. Fields with no home in the format are printed
  to stderr as `lossy:` lines; the exit code is still 0, because the import
  succeeded. Cheat Engine hotkeys, freeze state and auto-assembler scripts are
  the usual ones reported. With `--json` it still writes the file. It then
  prints one JSON object to standard output with the entry count and the lossy
  notes. This replaces the summary line and the `lossy:` lines.
- `export` writes a `.CT`. Symbols, scripts, structures, freeze state and
  hotkeys have no Cheat Engine counterpart and are reported.
- `diff` and `lint` return `1` when there are differences or problems and `2` on
  a read error, so they drop straight into a script or a pre-commit check.
- `rebase` adds the delta (`--new-base` minus `--old-base`) to every
  fixed-address entry. It adds the same delta to the base of every pointer
  chain that starts at a fixed address. The chain's offsets stay the same. It
  does not check whether an address is inside the module, so every fixed
  address moves. Module-rooted entries are left alone, both pointer chains and
  static addresses like `Tutorial.exe+0x2e5a0`. Symbols, scripts and structures
  are not changed. The result goes to `-o`, or to standard output when `-o` is
  absent. A one-line summary goes to stderr.

Pointer chains are shown base-first, the way `.iretable` stores them:
`Tutorial.exe+0x2e5a0 -> 0x10 -> 0x0 -> 0x18`. A static module address with no
dereferencing shows as `Tutorial.exe+0x2e5a0`, and a fixed address as
`fixed 0x140a40000`.

### What it does not do

- It does not attach to, read or modify any process. It is a file tool.
- It does not resolve a pointer chain to an address; that needs the live target,
  which is Pointer Lab's job.
- Cheat Engine tables carry more than an address list. `convert` takes the
  address list and its pointer chains; it does not convert auto-assembler
  scripts, hotkey bindings or Lua, and it says so for each entry.

## Build

**Requirements:** Visual Studio 2022 with the C++ workload (MSVC v143) and CMake
3.28 or newer. The CMake that ships with Visual Studio is recent enough.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For a 32-bit build, configure with `-A Win32` into a separate directory. The
first configure downloads Catch2, pinned by tag, for the tests only; the tool
itself has no dependency to fetch. To produce the release zip:

```powershell
cpack --config build/CPackConfig.cmake -C Release -B build/package
```

## License

MIT; see [LICENSE](LICENSE). The released tool bundles no third-party code. The
tests use Catch2 (Boost Software License 1.0), which is not linked into the tool
and does not ship in the release zip. Details are in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
