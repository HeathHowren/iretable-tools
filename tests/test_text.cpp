#include "iretable/Text.h"

#include <catch2/catch_test_macros.hpp>

using namespace iretable;

TEST_CASE("escape turns the four special bytes into sequences") {
    REQUIRE(escape("a|b") == "a\\|b");
    REQUIRE(escape("c\\d") == "c\\\\d");
    REQUIRE(escape("line1\nline2") == "line1\\nline2");
    REQUIRE(escape("cr\rhere") == "cr\\rhere");
    REQUIRE(escape("plain text") == "plain text");
}

TEST_CASE("unescape reverses escape, dropping a stray backslash") {
    REQUIRE(unescape("a\\|b") == "a|b");
    REQUIRE(unescape("c\\\\d") == "c\\d");
    REQUIRE(unescape("line1\\nline2") == "line1\nline2");
    // A backslash before an ordinary character is dropped and the character
    // taken literally, matching Pointer Lab.
    REQUIRE(unescape("\\x") == "x");
    // A lone trailing backslash has no following byte to consume, so unescape
    // keeps it as-is (splitEscaped is where a trailing escape is dropped).
    REQUIRE(unescape("end\\") == "end\\");
}

TEST_CASE("escape then unescape is the identity for awkward fields") {
    for (const std::string sample : {"a|b|c", "back\\slash", "new\nline", "mix|\\\n\r", "", "|", "\\"}) {
        REQUIRE(unescape(escape(sample)) == sample);
    }
}

TEST_CASE("escapeForDisplay keeps text on one line") {
    REQUIRE(escapeForDisplay("line1\nline2") == "line1\\nline2");
    REQUIRE(escapeForDisplay("cr\rtab\t") == "cr\\rtab\\t");
    // Any other control byte becomes \xNN in lower-case hex.
    REQUIRE(escapeForDisplay(std::string("nul\0end", 7)) == "nul\\x00end");
    REQUIRE(escapeForDisplay("esc\x1b[0m") == "esc\\x1b[0m");
    REQUIRE(escapeForDisplay("del\x7f") == "del\\x7f");
    // A backslash and a pipe print as-is, and UTF-8 text passes through.
    REQUIRE(escapeForDisplay("C:\\Games|x") == "C:\\Games|x");
    REQUIRE(escapeForDisplay("caf\xc3\xa9") == "caf\xc3\xa9");
    REQUIRE(escapeForDisplay("plain text") == "plain text");
}

TEST_CASE("splitEscaped splits on unescaped pipes only") {
    const auto parts = splitEscaped("entry|a\\|b|c\\\\d");
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "entry");
    REQUIRE(parts[1] == "a|b"); // the escaped pipe did not end the field
    REQUIRE(parts[2] == "c\\d");
}

TEST_CASE("parseUnsigned guards the numeric fields") {
    REQUIRE(parseUnsigned("64", 16).value() == 0x64);
    REQUIRE(parseUnsigned("100", 10).value() == 100);
    REQUIRE_FALSE(parseUnsigned("", 10).has_value());
    REQUIRE_FALSE(parseUnsigned("zz", 16).has_value());
    REQUIRE_FALSE(parseUnsigned("12z", 16).has_value());
    REQUIRE_FALSE(parseUnsigned("999999999999999999999", 10).has_value()); // too long
}

TEST_CASE("hex byte round-trip matches Pointer Lab's uppercase, no-space form") {
    const std::vector<std::uint8_t> bytes{0x64, 0x00, 0x00, 0x00};
    REQUIRE(bytesToHex(bytes, false) == "64000000");
    REQUIRE(parseHexBytes("64000000") == bytes);
    // parseHexBytes is lenient, matching Pointer Lab: it keeps every hex digit,
    // discards separators, and left-pads an odd digit count with a zero.
    REQUIRE(parseHexBytes("64 00 FF") == std::vector<std::uint8_t>{0x64, 0x00, 0xFF});
    REQUIRE(parseHexBytes("abc") == std::vector<std::uint8_t>{0x0a, 0xbc});
}

TEST_CASE("offset lists round-trip, including an empty list and a negative offset") {
    REQUIRE(formatOffsets({0x10, 0x8}) == "10,8");
    REQUIRE(formatOffsets({}).empty());
    REQUIRE(parseOffsets("10,8").value() == std::vector<std::int64_t>{0x10, 0x8});
    REQUIRE(parseOffsets("").value().empty());
    REQUIRE_FALSE(parseOffsets("10,,8").has_value()); // empty element is malformed
    REQUIRE_FALSE(parseOffsets("10,zz").has_value()); // non-hex element is malformed

    // A negative offset is stored as its unsigned bit pattern and comes back
    // exactly.
    const auto text = formatOffsets({-8});
    const auto parsed = parseOffsets(text).value();
    REQUIRE(parsed.size() == 1);
    REQUIRE(parsed[0] == -8);
}
