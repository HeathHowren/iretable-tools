# Changelog

All notable changes to iretable-tools are recorded here. This project follows
[Semantic Versioning](https://semver.org/).

## [1.0.2] - 2026-09-26

### Fixed

- **The `rebase` summary says what moved.** It said "static address(es)" and
  counted pointer chain bases among them. It now counts fixed addresses and
  pointer chain bases separately, and calls the entries it leaves alone
  module-rooted entries. A downward move prints a negative delta, such as
  `-0x10000000`, instead of the wrapped 64-bit value. A new test covers it.

## [1.0.1] - 2026-09-26

### Fixed

- **`show` escapes control characters in descriptions.** A description with a
  newline no longer breaks the row. A newline prints as `\n`, a carriage return
  as `\r`, a tab as `\t`, and any other control character as `\xNN`. A
  backslash prints as-is. `diff`, `lint` and the `lossy:` lines from `convert`
  and `export` follow the same rule. `--json` output is unchanged.
- **American spelling in messages.** `show`, `lint` and the reader now use
  American spelling, such as "unrecognized".
- **Docs on `--json` and `rebase`.** The README and `--help` now list `convert`
  among the commands that take `--json`. They also say what `rebase` shifts:
  fixed addresses and the base of any pointer chain that starts at a fixed
  address. Module-rooted entries are left alone.

## [1.0.0] - 2026-09-25

The first release.

### Added

- **A reader and writer for every `.iretable` version.** Headers 1, 2 and 3 all
  load, with the full escaping rules, the pid, process, bitness, symbol, script,
  struct and entry records, and pointer chains. The writer produces Pointer
  Lab's canonical form byte for byte, so the two tools read and write the same
  files. A UTF-8 byte-order mark before the header is tolerated.
- **Unknown records are preserved.** A record type this tool does not recognize
  is kept verbatim and written back, so a file from a newer Pointer Lab round-
  trips without losing anything. Pointer Lab itself only skips them.
- **`convert`,** a Cheat Engine `.CT` importer. Descriptions, value types,
  static addresses and pointer offset chains are carried across, groups become
  entry groups, and Cheat Engine's reversed offset order is corrected. Every
  field that has no `.iretable` counterpart is reported on stderr; nothing is
  dropped in silence.
- **`export`,** the reverse, writing a `.CT` where the records map and reporting
  the ones that do not (symbols, scripts, structures, freeze state, hotkeys).
- **`diff`,** an entry-level comparison: added, removed and changed rows, matched
  by description and location, with the changed fields named.
- **`rebase`,** which shifts static addresses by a base delta. Module-rooted
  chains are left alone, because they survive a relocation on their own.
- **`lint`,** which reports malformed lines, bad hex, unknown value types, broken
  pointer chains, duplicate ids and frozen entries with no value, and exits
  non-zero when a file has problems.
- **`--json`** on `show`, `diff` and `lint`, for scripting.
- No third-party runtime dependency: the `.CT` reader is a small hand-written
  XML parser, and the CLI is standard C++20 with a static C runtime.

[1.0.1]: https://github.com/HeathHowren/iretable-tools/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/HeathHowren/iretable-tools/releases/tag/v1.0.0
