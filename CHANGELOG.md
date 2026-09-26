# Changelog

All notable changes to iretable-tools are recorded here. This project follows
[Semantic Versioning](https://semver.org/).

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
