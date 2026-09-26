#include "iretable/ValueType.h"

#include <algorithm>
#include <cctype>

namespace iretable {

namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

} // namespace

const char* valueTypeName(ValueType type) {
    switch (type) {
    case ValueType::Int8: return "i8";
    case ValueType::UInt8: return "u8";
    case ValueType::Int16: return "i16";
    case ValueType::UInt16: return "u16";
    case ValueType::Int32: return "i32";
    case ValueType::UInt32: return "u32";
    case ValueType::Int64: return "i64";
    case ValueType::UInt64: return "u64";
    case ValueType::Float: return "f32";
    case ValueType::Double: return "f64";
    case ValueType::Bytes: return "bytes";
    case ValueType::StringAscii: return "str";
    case ValueType::StringUtf16: return "wstr";
    }
    return "unknown";
}

std::size_t valueTypeSize(ValueType type) {
    switch (type) {
    case ValueType::Int8:
    case ValueType::UInt8: return 1;
    case ValueType::Int16:
    case ValueType::UInt16: return 2;
    case ValueType::Int32:
    case ValueType::UInt32:
    case ValueType::Float: return 4;
    case ValueType::Int64:
    case ValueType::UInt64:
    case ValueType::Double: return 8;
    case ValueType::Bytes:
    case ValueType::StringAscii:
    case ValueType::StringUtf16: return 0;
    }
    return 0;
}

bool isStringType(ValueType type) {
    return type == ValueType::StringAscii || type == ValueType::StringUtf16;
}

const std::vector<ValueType>& valueTypes() {
    static const std::vector<ValueType> all = {
        ValueType::Int8,  ValueType::UInt8,       ValueType::Int16,      ValueType::UInt16,
        ValueType::Int32, ValueType::UInt32,      ValueType::Int64,      ValueType::UInt64,
        ValueType::Float, ValueType::Double,      ValueType::Bytes,      ValueType::StringAscii,
        ValueType::StringUtf16};
    return all;
}

std::optional<ValueType> parseValueType(const std::string& text) {
    const auto value = lower(text);
    for (const auto type : valueTypes()) {
        if (value == valueTypeName(type)) {
            return type;
        }
    }
    return std::nullopt;
}

} // namespace iretable
