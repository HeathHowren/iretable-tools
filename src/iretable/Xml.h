#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

// A small read-only XML parser, just enough for Cheat Engine .CT tables: nested
// elements, attributes, text, CDATA, comments, the XML declaration, a DOCTYPE,
// self-closing tags, and the five predefined entities plus numeric character
// references. It builds a DOM and never executes anything. Hand-rolled so
// iretable-tools has no runtime dependency; a .CT file is small and its shape
// is simple, so a focused parser is cleaner than pulling in a library.
namespace iretable::xml {

struct Node {
    std::string name;
    std::vector<std::pair<std::string, std::string>> attributes;
    // Concatenated text and CDATA directly inside this element, entity-decoded.
    std::string text;
    std::vector<Node> children;

    // First direct child with this name, or nullptr.
    [[nodiscard]] const Node* child(const std::string& childName) const;
    // Text of the first direct child with this name, or "" if absent.
    [[nodiscard]] std::string childText(const std::string& childName) const;
    // Attribute value, or nullopt if absent.
    [[nodiscard]] std::optional<std::string> attribute(const std::string& attributeName) const;
    // Every direct child with this name, in order.
    [[nodiscard]] std::vector<const Node*> childrenNamed(const std::string& childName) const;
};

struct Document {
    bool ok{};
    std::string error;
    Node root;
};

Document parse(const std::string& text);

} // namespace iretable::xml
