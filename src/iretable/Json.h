#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// A tiny JSON value for building --json output. Output only: iretable-tools
// never parses JSON, so there is no reader here. Keeps the CLI's scripting mode
// dependency-free and its output well-formed (proper string escaping, no hand
// concatenation).
namespace iretable::json {

class Value {
public:
    static Value str(std::string s);
    static Value number(std::int64_t n);
    static Value number(std::uint64_t n);
    static Value boolean(bool b);
    static Value null();
    static Value array();
    static Value object();

    // Append to an array.
    Value& push(Value v);
    // Set a key on an object. Insertion order is preserved for stable output.
    Value& set(std::string key, Value v);

    // Serialize. indent 0 means compact (one line); a positive indent
    // pretty-prints with that many spaces per level.
    [[nodiscard]] std::string dump(int indent = 2) const;

private:
    enum class Kind { Null, Bool, Int, UInt, String, Array, Object };
    Kind kind_{Kind::Null};
    bool bool_{};
    std::int64_t int_{};
    std::uint64_t uint_{};
    std::string string_;
    std::vector<Value> array_;
    std::vector<std::pair<std::string, Value>> object_;

    void dumpTo(std::string& out, int indent, int depth) const;
};

std::string escape(const std::string& text);

} // namespace iretable::json
