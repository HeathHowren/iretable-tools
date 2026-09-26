#include "iretable/Text.h"

#include <cctype>
#include <cstdio>
#include <exception>
#include <sstream>

namespace iretable {

std::string escape(const std::string& text) {
    std::string out;
    for (const char c : text) {
        if (c == '\\' || c == '|' || c == '\n' || c == '\r') {
            out.push_back('\\');
            if (c == '\n') {
                out.push_back('n');
            } else if (c == '\r') {
                out.push_back('r');
            } else {
                out.push_back(c);
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

std::string unescape(const std::string& text) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            const char n = text[++i];
            if (n == 'n') {
                out.push_back('\n');
            } else if (n == 'r') {
                out.push_back('\r');
            } else {
                out.push_back(n);
            }
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

std::string escapeForDisplay(const std::string& text) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(text.size());
    for (const char ch : text) {
        const auto c = static_cast<unsigned char>(ch);
        if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (c < 0x20 || c == 0x7f) {
            out += "\\x";
            out.push_back(digits[c >> 4]);
            out.push_back(digits[c & 0xF]);
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::vector<std::string> splitEscaped(const std::string& line) {
    std::vector<std::string> parts;
    std::string current;
    bool escaped{};
    for (const char c : line) {
        if (escaped) {
            current.push_back('\\');
            current.push_back(c);
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '|') {
            parts.push_back(unescape(current));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(unescape(current));
    return parts;
}

std::optional<std::uint64_t> parseUnsigned(const std::string& text, int base) {
    if (text.empty() || text.size() > 20) {
        return std::nullopt;
    }
    const auto isValidDigit = [base](unsigned char c) {
        return base == 16 ? std::isxdigit(c) != 0 : std::isdigit(c) != 0;
    };
    for (const char c : text) {
        if (!isValidDigit(static_cast<unsigned char>(c))) {
            return std::nullopt;
        }
    }
    try {
        return std::stoull(text, nullptr, base);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string toHexLower(std::uint64_t value) {
    char buffer[sizeof(value) * 2 + 1];
    std::snprintf(buffer, sizeof(buffer), "%llx", static_cast<unsigned long long>(value));
    return buffer;
}

std::string bytesToHex(const std::vector<std::uint8_t>& bytes, bool spaces) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(bytes.size() * 3);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (spaces && i != 0) {
            out.push_back(' ');
        }
        out.push_back(digits[bytes[i] >> 4]);
        out.push_back(digits[bytes[i] & 0xF]);
    }
    return out;
}

std::vector<std::uint8_t> parseHexBytes(const std::string& text) {
    std::string hex;
    for (const char c : text) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            hex.push_back(c);
        }
    }
    if (hex.size() % 2 != 0) {
        hex.insert(hex.begin(), '0');
    }
    std::vector<std::uint8_t> bytes;
    const auto digit = [](char c) -> std::uint8_t {
        if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
        return static_cast<std::uint8_t>(c - 'A' + 10);
    };
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>((digit(hex[i]) << 4) | digit(hex[i + 1])));
    }
    return bytes;
}

std::string formatOffsets(const std::vector<std::int64_t>& offsets) {
    std::ostringstream out;
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        // The unsigned bit pattern, so a negative offset round-trips exactly
        // rather than through a minus sign that only works by accident.
        out << std::hex << static_cast<std::uint64_t>(offsets[i]);
    }
    return out.str();
}

std::optional<std::vector<std::int64_t>> parseOffsets(const std::string& text) {
    std::vector<std::int64_t> offsets;
    std::istringstream in(text);
    std::string field;
    while (std::getline(in, field, ',')) {
        if (field.empty()) {
            return std::nullopt;
        }
        std::size_t consumed{};
        try {
            const auto value = std::stoull(field, &consumed, 16);
            if (consumed != field.size()) {
                return std::nullopt;
            }
            offsets.push_back(static_cast<std::int64_t>(value));
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    return offsets;
}

} // namespace iretable
