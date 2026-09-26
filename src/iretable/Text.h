#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Escaping, splitting and number parsing, matching Pointer Lab's ProjectStore
// byte for byte so the two tools read and write the same files.
namespace iretable {

// Escape a field: backslash, pipe, newline and carriage return become \\, \|,
// \n and \r. Everything else is passed through.
std::string escape(const std::string& text);

// Reverse of escape. A backslash before anything other than n or r is dropped
// and the following byte is taken literally; a trailing backslash is dropped.
std::string unescape(const std::string& text);

// Split a line on unescaped pipes, then unescape each field. Splitting happens
// before unescaping, so an escaped \| never ends a field.
std::vector<std::string> splitEscaped(const std::string& line);

// Parse an unsigned integer in the given base (10 or 16). Rejects empty input,
// input longer than 20 characters, and any non-digit for the base. This is the
// guard Pointer Lab puts on every attacker-controllable numeric field.
std::optional<std::uint64_t> parseUnsigned(const std::string& text, int base);

// Hex, no 0x, lower case, as addresses and offsets are written.
std::string toHexLower(std::uint64_t value);

// Uppercase hex byte string, e.g. {0x64,0,0,0} -> "64000000". With spaces off,
// this is the frozen-value form. Matches Pointer Lab's bytesToHex.
std::string bytesToHex(const std::vector<std::uint8_t>& bytes, bool spaces = false);

// Lenient hex-byte parse: non-hex characters are discarded and an odd digit
// count is left-padded with a zero. Matches Pointer Lab's parseHexBytes, which
// is what reads the frozen-value field.
std::vector<std::uint8_t> parseHexBytes(const std::string& text);

// Format a pointer-chain offset list as comma-separated lower-case hex of the
// unsigned bit pattern. Empty list -> empty string.
std::string formatOffsets(const std::vector<std::int64_t>& offsets);

// Parse a comma-separated offset list. nullopt if any element is empty or not
// whole-string hex. An empty string parses to an empty list (a static base).
std::optional<std::vector<std::int64_t>> parseOffsets(const std::string& text);

} // namespace iretable
