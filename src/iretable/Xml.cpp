#include "iretable/Xml.h"

#include <cctype>
#include <cstdint>

namespace iretable::xml {

const Node* Node::child(const std::string& childName) const {
    for (const auto& c : children) {
        if (c.name == childName) {
            return &c;
        }
    }
    return nullptr;
}

std::string Node::childText(const std::string& childName) const {
    const auto* c = child(childName);
    return c ? c->text : std::string{};
}

std::optional<std::string> Node::attribute(const std::string& attributeName) const {
    for (const auto& [key, value] : attributes) {
        if (key == attributeName) {
            return value;
        }
    }
    return std::nullopt;
}

std::vector<const Node*> Node::childrenNamed(const std::string& childName) const {
    std::vector<const Node*> out;
    for (const auto& c : children) {
        if (c.name == childName) {
            out.push_back(&c);
        }
    }
    return out;
}

namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    Document run() {
        Document doc;
        // A UTF-8 BOM is tolerated, as .CT files saved from Windows often carry
        // one.
        if (text_.compare(pos_, 3, "\xEF\xBB\xBF") == 0) {
            pos_ += 3;
        }
        skipMisc();
        if (pos_ >= text_.size() || text_[pos_] != '<') {
            doc.error = "No root element.";
            return doc;
        }
        if (!parseElement(doc.root)) {
            doc.error = error_.empty() ? "Malformed XML." : error_;
            return doc;
        }
        doc.ok = true;
        return doc;
    }

private:
    const std::string& text_;
    std::size_t pos_{};
    std::string error_;

    [[nodiscard]] bool atEnd() const { return pos_ >= text_.size(); }

    void skipWhitespace() {
        while (!atEnd() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool startsWith(const char* s) const { return text_.compare(pos_, std::string::traits_type::length(s), s) == 0; }

    // Skip declarations, comments, DOCTYPE and processing instructions between
    // top-level constructs.
    void skipMisc() {
        for (;;) {
            skipWhitespace();
            if (startsWith("<!--")) {
                const auto end = text_.find("-->", pos_ + 4);
                pos_ = (end == std::string::npos) ? text_.size() : end + 3;
            } else if (startsWith("<?")) {
                const auto end = text_.find("?>", pos_ + 2);
                pos_ = (end == std::string::npos) ? text_.size() : end + 2;
            } else if (startsWith("<!DOCTYPE") || startsWith("<!")) {
                const auto end = text_.find('>', pos_);
                pos_ = (end == std::string::npos) ? text_.size() : end + 1;
            } else {
                return;
            }
        }
    }

    bool fail(std::string message) {
        if (error_.empty()) {
            error_ = std::move(message);
        }
        return false;
    }

    static bool isNameChar(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' || c == ':';
    }

    std::string parseName() {
        const auto start = pos_;
        while (!atEnd() && isNameChar(text_[pos_])) {
            ++pos_;
        }
        return text_.substr(start, pos_ - start);
    }

    void appendEntity(std::string& out) {
        // pos_ is at '&'.
        const auto semicolon = text_.find(';', pos_);
        if (semicolon == std::string::npos) {
            out.push_back('&');
            ++pos_;
            return;
        }
        const std::string entity = text_.substr(pos_ + 1, semicolon - pos_ - 1);
        pos_ = semicolon + 1;
        if (entity == "amp") {
            out.push_back('&');
        } else if (entity == "lt") {
            out.push_back('<');
        } else if (entity == "gt") {
            out.push_back('>');
        } else if (entity == "quot") {
            out.push_back('"');
        } else if (entity == "apos") {
            out.push_back('\'');
        } else if (!entity.empty() && entity[0] == '#') {
            std::uint32_t code = 0;
            try {
                code = (entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X'))
                           ? static_cast<std::uint32_t>(std::stoul(entity.substr(2), nullptr, 16))
                           : static_cast<std::uint32_t>(std::stoul(entity.substr(1), nullptr, 10));
            } catch (...) {
                return;
            }
            appendUtf8(out, code);
        }
        // An unknown entity is dropped, which is the tolerant choice for a
        // format we only read.
    }

    static void appendUtf8(std::string& out, std::uint32_t code) {
        if (code < 0x80) {
            out.push_back(static_cast<char>(code));
        } else if (code < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else if (code < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (code >> 18)));
            out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }

    // Parse an element whose opening '<' is at pos_. Fills node.
    bool parseElement(Node& node) {
        if (atEnd() || text_[pos_] != '<') {
            return fail("Expected '<'.");
        }
        ++pos_;
        node.name = parseName();
        if (node.name.empty()) {
            return fail("Empty tag name.");
        }

        // Attributes.
        for (;;) {
            skipWhitespace();
            if (atEnd()) {
                return fail("Unterminated tag.");
            }
            if (text_[pos_] == '/') {
                ++pos_;
                if (atEnd() || text_[pos_] != '>') {
                    return fail("Malformed self-closing tag.");
                }
                ++pos_;
                return true; // self-closed, no children
            }
            if (text_[pos_] == '>') {
                ++pos_;
                break; // done with the open tag; parse content
            }
            std::string attrName = parseName();
            if (attrName.empty()) {
                return fail("Malformed attribute.");
            }
            skipWhitespace();
            std::string attrValue;
            if (!atEnd() && text_[pos_] == '=') {
                ++pos_;
                skipWhitespace();
                if (atEnd() || (text_[pos_] != '"' && text_[pos_] != '\'')) {
                    return fail("Unquoted attribute value.");
                }
                const char quote = text_[pos_++];
                while (!atEnd() && text_[pos_] != quote) {
                    if (text_[pos_] == '&') {
                        appendEntity(attrValue);
                    } else {
                        attrValue.push_back(text_[pos_++]);
                    }
                }
                if (atEnd()) {
                    return fail("Unterminated attribute value.");
                }
                ++pos_; // closing quote
            }
            node.attributes.emplace_back(std::move(attrName), std::move(attrValue));
        }

        // Content: text, children, until the matching close tag.
        for (;;) {
            if (atEnd()) {
                return fail("Unterminated element <" + node.name + ">.");
            }
            if (text_[pos_] == '<') {
                if (startsWith("<!--")) {
                    const auto end = text_.find("-->", pos_ + 4);
                    pos_ = (end == std::string::npos) ? text_.size() : end + 3;
                    continue;
                }
                if (startsWith("<![CDATA[")) {
                    const auto end = text_.find("]]>", pos_ + 9);
                    const auto stop = (end == std::string::npos) ? text_.size() : end;
                    node.text.append(text_, pos_ + 9, stop - (pos_ + 9));
                    pos_ = (end == std::string::npos) ? text_.size() : end + 3;
                    continue;
                }
                if (startsWith("<?")) {
                    const auto end = text_.find("?>", pos_ + 2);
                    pos_ = (end == std::string::npos) ? text_.size() : end + 2;
                    continue;
                }
                if (startsWith("</")) {
                    pos_ += 2;
                    const std::string closeName = parseName();
                    skipWhitespace();
                    if (atEnd() || text_[pos_] != '>') {
                        return fail("Malformed close tag.");
                    }
                    ++pos_;
                    if (closeName != node.name) {
                        return fail("Mismatched close tag </" + closeName + "> for <" + node.name + ">.");
                    }
                    return true;
                }
                Node childNode;
                if (!parseElement(childNode)) {
                    return false;
                }
                node.children.push_back(std::move(childNode));
            } else if (text_[pos_] == '&') {
                appendEntity(node.text);
            } else {
                node.text.push_back(text_[pos_++]);
            }
        }
    }
};

} // namespace

Document parse(const std::string& text) {
    return Parser(text).run();
}

} // namespace iretable::xml
