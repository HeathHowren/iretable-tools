#include "iretable/Json.h"

#include <cstdio>

namespace iretable::json {

std::string escape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    for (const unsigned char c : text) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                char buffer[7];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                out += buffer;
            } else {
                // Bytes >= 0x20, including UTF-8 continuation bytes, are emitted
                // as-is: the input is already valid UTF-8 from the file.
                out.push_back(static_cast<char>(c));
            }
        }
    }
    return out;
}

Value Value::str(std::string s) {
    Value v;
    v.kind_ = Kind::String;
    v.string_ = std::move(s);
    return v;
}

Value Value::number(std::int64_t n) {
    Value v;
    v.kind_ = Kind::Int;
    v.int_ = n;
    return v;
}

Value Value::number(std::uint64_t n) {
    Value v;
    v.kind_ = Kind::UInt;
    v.uint_ = n;
    return v;
}

Value Value::boolean(bool b) {
    Value v;
    v.kind_ = Kind::Bool;
    v.bool_ = b;
    return v;
}

Value Value::null() {
    return Value{};
}

Value Value::array() {
    Value v;
    v.kind_ = Kind::Array;
    return v;
}

Value Value::object() {
    Value v;
    v.kind_ = Kind::Object;
    return v;
}

Value& Value::push(Value v) {
    array_.push_back(std::move(v));
    return *this;
}

Value& Value::set(std::string key, Value v) {
    object_.emplace_back(std::move(key), std::move(v));
    return *this;
}

void Value::dumpTo(std::string& out, int indent, int depth) const {
    const auto newline = [&](int d) {
        if (indent > 0) {
            out.push_back('\n');
            out.append(static_cast<std::size_t>(indent) * static_cast<std::size_t>(d), ' ');
        }
    };
    switch (kind_) {
    case Kind::Null: out += "null"; break;
    case Kind::Bool: out += bool_ ? "true" : "false"; break;
    case Kind::Int: out += std::to_string(int_); break;
    case Kind::UInt: out += std::to_string(uint_); break;
    case Kind::String:
        out.push_back('"');
        out += escape(string_);
        out.push_back('"');
        break;
    case Kind::Array:
        if (array_.empty()) {
            out += "[]";
            break;
        }
        out.push_back('[');
        for (std::size_t i = 0; i < array_.size(); ++i) {
            if (i > 0) {
                out.push_back(',');
            }
            newline(depth + 1);
            array_[i].dumpTo(out, indent, depth + 1);
        }
        newline(depth);
        out.push_back(']');
        break;
    case Kind::Object:
        if (object_.empty()) {
            out += "{}";
            break;
        }
        out.push_back('{');
        for (std::size_t i = 0; i < object_.size(); ++i) {
            if (i > 0) {
                out.push_back(',');
            }
            newline(depth + 1);
            out.push_back('"');
            out += escape(object_[i].first);
            out.push_back('"');
            out.push_back(':');
            if (indent > 0) {
                out.push_back(' ');
            }
            object_[i].second.dumpTo(out, indent, depth + 1);
        }
        newline(depth);
        out.push_back('}');
        break;
    }
}

std::string Value::dump(int indent) const {
    std::string out;
    dumpTo(out, indent, 0);
    return out;
}

} // namespace iretable::json
