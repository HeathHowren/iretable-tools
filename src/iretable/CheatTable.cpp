#include "iretable/CheatTable.h"

#include "iretable/Text.h"
#include "iretable/ValueType.h"
#include "iretable/Xml.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>

namespace iretable {

namespace {

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// Cheat Engine wraps a description in double quotes. Remove one surrounding
// pair if present.
std::string unquoteDescription(std::string text) {
    text = trim(text);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

// Map a Cheat Engine VariableType to a value type. wstr is chosen for a String
// whose Unicode flag is set. nullopt for a type with no counterpart.
std::optional<ValueType> mapVariableType(const std::string& variableType, bool unicode) {
    const auto name = lower(trim(variableType));
    if (name == "byte") return ValueType::Int8;
    if (name == "2 bytes") return ValueType::Int16;
    if (name == "4 bytes") return ValueType::Int32;
    if (name == "8 bytes") return ValueType::Int64;
    if (name == "float") return ValueType::Float;
    if (name == "double") return ValueType::Double;
    if (name == "string") return unicode ? ValueType::StringUtf16 : ValueType::StringAscii;
    if (name == "array of byte" || name == "array of bytes" || name == "aob") return ValueType::Bytes;
    if (name == "binary") return ValueType::Bytes; // bit length is lost; reported by the caller
    return std::nullopt;
}

// The Cheat Engine VariableType text for a value type, for export.
std::string cheatVariableType(ValueType type) {
    switch (type) {
    case ValueType::Int8:
    case ValueType::UInt8: return "Byte";
    case ValueType::Int16:
    case ValueType::UInt16: return "2 Bytes";
    case ValueType::Int32:
    case ValueType::UInt32: return "4 Bytes";
    case ValueType::Int64:
    case ValueType::UInt64: return "8 Bytes";
    case ValueType::Float: return "Float";
    case ValueType::Double: return "Double";
    case ValueType::Bytes: return "Array of Byte";
    case ValueType::StringAscii:
    case ValueType::StringUtf16: return "String";
    }
    return "4 Bytes";
}

// Parse a Cheat Engine address expression into an optional module name and a
// numeric offset/base. Handles "module.exe"+1AE30, module.exe+1AE30, and a bare
// hex address. Returns false if the numeric part is not hex.
struct ParsedAddress {
    std::string module; // empty for an absolute address
    std::uint64_t value{};
    bool ok{};
};

ParsedAddress parseCheatAddress(const std::string& raw) {
    ParsedAddress result;
    std::string text = trim(raw);
    if (text.empty()) {
        return result;
    }

    // A module-relative address: <module>+<hex>. The module may be quoted.
    const auto plus = text.find('+');
    if (plus != std::string::npos) {
        std::string module = trim(text.substr(0, plus));
        std::string offset = trim(text.substr(plus + 1));
        if (module.size() >= 2 && module.front() == '"' && module.back() == '"') {
            module = module.substr(1, module.size() - 2);
        }
        if (offset.rfind("0x", 0) == 0 || offset.rfind("0X", 0) == 0) {
            offset = offset.substr(2);
        }
        if (offset.empty() || offset.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
            return result;
        }
        try {
            result.value = std::stoull(offset, nullptr, 16);
        } catch (...) {
            return result;
        }
        result.module = module;
        result.ok = true;
        return result;
    }

    // A bare address, always hexadecimal.
    if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) {
        text = text.substr(2);
    }
    if (text.empty() || text.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        return result;
    }
    try {
        result.value = std::stoull(text, nullptr, 16);
    } catch (...) {
        return result;
    }
    result.ok = true;
    return result;
}

// Which Cheat Engine child elements carry data that .iretable cannot hold. Only
// these are reported per entry; display-only fields (colours, hex/signed
// toggles) are ignored quietly.
bool isReportedLossyField(const std::string& name) {
    return name == "Hotkeys" || name == "AssemblerScript";
}

void importEntry(const xml::Node& node, const std::string& group, std::vector<std::string>& lossy, Table& table);

void importChildren(const xml::Node& entriesNode, const std::string& group, std::vector<std::string>& lossy,
                    Table& table) {
    for (const auto* entry : entriesNode.childrenNamed("CheatEntry")) {
        importEntry(*entry, group, lossy, table);
    }
}

void importEntry(const xml::Node& node, const std::string& group, std::vector<std::string>& lossy, Table& table) {
    const std::string description = unquoteDescription(node.childText("Description"));
    const auto* nested = node.child("CheatEntries");
    const std::string variableType = trim(node.childText("VariableType"));

    // A group header: no value type of its own, but a bag of child entries. Its
    // description becomes (part of) the group name for everything beneath it.
    if (nested && variableType.empty()) {
        std::string childGroup = group.empty() ? description : (group + "/" + description);
        if (!group.empty() && !description.empty()) {
            lossy.push_back("group \"" + childGroup +
                            "\": Cheat Engine group nesting was flattened into one group name");
        }
        importChildren(*nested, childGroup, lossy, table);
        return;
    }

    const std::string addressText = node.childText("Address");
    const bool unicode = trim(node.childText("Unicode")) == "1";
    const auto type = mapVariableType(variableType, unicode);

    const std::string label = description.empty() ? "(no description)" : description;

    if (variableType.empty() && addressText.empty()) {
        // Neither a value nor an address: an empty header with no children.
        // Nothing to import, and nothing was lost.
        if (nested) {
            importChildren(*nested, group.empty() ? description : (group + "/" + description), lossy, table);
        }
        return;
    }

    if (!type) {
        lossy.push_back("entry \"" + label + "\": value type \"" + variableType +
                        "\" has no .iretable equivalent; entry skipped");
        return;
    }

    const auto parsed = parseCheatAddress(addressText);
    if (!parsed.ok) {
        lossy.push_back("entry \"" + label + "\": address \"" + trim(addressText) +
                        "\" is not a plain module+offset or hex address; entry skipped");
        return;
    }

    Entry entry;
    entry.id = table.entries.size() + 1;
    entry.type = *type;
    entry.description = description;
    entry.group = group;

    if (lower(trim(variableType)) == "binary") {
        lossy.push_back("entry \"" + label + "\": Binary type imported as a byte array; bit offset and length lost");
    }

    // Cheat Engine offsets are listed last-applied-first; .iretable stores them
    // base-first, so the list is reversed.
    std::vector<std::int64_t> offsets;
    if (const auto* offsetsNode = node.child("Offsets")) {
        for (const auto* offset : offsetsNode->childrenNamed("Offset")) {
            std::string value = trim(offset->text);
            if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0) {
                value = value.substr(2);
            }
            bool negative = false;
            if (!value.empty() && value.front() == '-') {
                negative = true;
                value = value.substr(1);
            }
            if (value.empty() || value.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
                lossy.push_back("entry \"" + label + "\": pointer offset \"" + trim(offset->text) +
                                "\" is not hex; offset dropped");
                continue;
            }
            std::uint64_t magnitude = 0;
            try {
                magnitude = std::stoull(value, nullptr, 16);
            } catch (...) {
                continue;
            }
            offsets.push_back(negative ? -static_cast<std::int64_t>(magnitude)
                                       : static_cast<std::int64_t>(magnitude));
        }
        std::reverse(offsets.begin(), offsets.end());
    }

    const bool isPointer = !offsets.empty();
    if (isPointer || !parsed.module.empty()) {
        // A chain: either a real pointer path, or a module-relative static
        // address written as a base with no offsets so it survives a restart.
        PointerChain chain;
        chain.moduleName = parsed.module;
        chain.moduleOffset = parsed.value;
        chain.offsets = offsets;
        entry.address = 0;
        entry.chain = std::move(chain);
    } else {
        // A bare absolute address with no offsets.
        entry.address = parsed.value;
    }

    // Report any data-bearing Cheat Engine field with no home in .iretable.
    for (const auto& child : node.children) {
        if (isReportedLossyField(child.name) && (!trim(child.text).empty() || !child.children.empty())) {
            lossy.push_back("entry \"" + label + "\": Cheat Engine field <" + child.name + "> not imported");
        }
    }

    table.entries.push_back(std::move(entry));

    // A group header can also carry its own value; import its children too.
    if (nested) {
        importChildren(*nested, group.empty() ? description : (group + "/" + description), lossy, table);
    }
}

} // namespace

ConvertResult convertFromCheatTable(const std::string& xmlText) {
    ConvertResult result;
    const auto document = xml::parse(xmlText);
    if (!document.ok) {
        result.ok = false;
        result.error = "Could not parse the .CT file as XML: " + document.error;
        return result;
    }
    if (document.root.name != "CheatTable") {
        result.ok = false;
        result.error = "This is not a Cheat Engine table: the root element is <" + document.root.name +
                       ">, not <CheatTable>.";
        return result;
    }

    result.table.version = 3;
    result.table.lastProcessName.clear();

    if (const auto* entries = document.root.child("CheatEntries")) {
        importChildren(*entries, "", result.lossy, result.table);
    }

    result.ok = true;
    return result;
}

namespace {

std::string xmlEscape(const std::string& text) {
    std::string out;
    for (const char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out.push_back(c);
        }
    }
    return out;
}

// Build the <Offsets> block for export, undoing the base-first ordering back to
// Cheat Engine's last-applied-first.
void appendOffsets(std::ostringstream& out, const std::vector<std::int64_t>& offsets, const std::string& indent) {
    if (offsets.empty()) {
        return;
    }
    out << indent << "<Offsets>\n";
    for (auto it = offsets.rbegin(); it != offsets.rend(); ++it) {
        out << indent << "  <Offset>" << toHexLower(static_cast<std::uint64_t>(*it)) << "</Offset>\n";
    }
    out << indent << "</Offsets>\n";
}

void appendEntry(std::ostringstream& out, const Entry& entry, int id, const std::string& indent) {
    out << indent << "<CheatEntry>\n";
    out << indent << "  <ID>" << id << "</ID>\n";
    // Description is XML-escaped and re-quoted in Cheat Engine's style.
    out << indent << "  <Description>\"" << xmlEscape(entry.description) << "\"</Description>\n";
    if (entry.type == ValueType::StringUtf16) {
        out << indent << "  <Unicode>1</Unicode>\n";
    }
    out << indent << "  <VariableType>" << cheatVariableType(entry.type) << "</VariableType>\n";

    if (!entry.chain) {
        out << indent << "  <Address>" << toHexLower(entry.address) << "</Address>\n";
    } else {
        const auto& chain = *entry.chain;
        out << indent << "  <Address>";
        if (chain.moduleRooted()) {
            out << '"' << chain.moduleName << "\"+" << toHexLower(chain.moduleOffset);
        } else {
            out << toHexLower(chain.moduleOffset);
        }
        out << "</Address>\n";
        appendOffsets(out, chain.offsets, indent + "  ");
    }
    out << indent << "</CheatEntry>\n";
}

} // namespace

ExportResult exportToCheatTable(const Table& table) {
    ExportResult result;

    // Report the records Cheat Engine cannot represent.
    if (!table.symbols.empty()) {
        result.lossy.push_back(std::to_string(table.symbols.size()) +
                               " symbol(s) not exported; Cheat Engine has no symbol-expression record");
    }
    if (!table.scripts.empty()) {
        result.lossy.push_back(std::to_string(table.scripts.size()) +
                               " script(s) not exported; auto-assembler scripts are not converted");
    }
    if (!table.structures.empty()) {
        result.lossy.push_back(std::to_string(table.structures.size()) +
                               " structure(s) not exported; Cheat Engine structures use a different format");
    }
    for (const auto& entry : table.entries) {
        const std::string label = entry.description.empty() ? "(no description)" : entry.description;
        if (entry.frozen || !entry.frozenValue.empty()) {
            result.lossy.push_back("entry \"" + label + "\": freeze state and frozen value not exported");
        }
        if (!entry.hotkey.empty()) {
            result.lossy.push_back("entry \"" + label + "\": hotkey \"" + entry.hotkey +
                                   "\" not exported; Cheat Engine hotkeys use a different record");
        }
    }

    // Group entries by their group string, preserving first-seen order. Entries
    // with no group sit at the top level.
    std::vector<std::string> groupOrder;
    std::vector<std::vector<const Entry*>> grouped;
    std::vector<const Entry*> ungrouped;
    for (const auto& entry : table.entries) {
        if (entry.group.empty()) {
            ungrouped.push_back(&entry);
            continue;
        }
        auto it = std::find(groupOrder.begin(), groupOrder.end(), entry.group);
        if (it == groupOrder.end()) {
            groupOrder.push_back(entry.group);
            grouped.emplace_back();
            grouped.back().push_back(&entry);
        } else {
            grouped[static_cast<std::size_t>(it - groupOrder.begin())].push_back(&entry);
        }
    }

    std::ostringstream out;
    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    out << "<CheatTable>\n";
    out << "  <CheatEntries>\n";

    int id = 1;
    for (const auto* entry : ungrouped) {
        appendEntry(out, *entry, id++, "    ");
    }
    for (std::size_t g = 0; g < groupOrder.size(); ++g) {
        out << "    <CheatEntry>\n";
        out << "      <ID>" << id++ << "</ID>\n";
        out << "      <Description>\"" << xmlEscape(groupOrder[g]) << "\"</Description>\n";
        out << "      <GroupHeader>1</GroupHeader>\n";
        out << "      <CheatEntries>\n";
        for (const auto* entry : grouped[g]) {
            appendEntry(out, *entry, id++, "        ");
        }
        out << "      </CheatEntries>\n";
        out << "    </CheatEntry>\n";
    }

    out << "  </CheatEntries>\n";
    out << "</CheatTable>\n";

    result.ct = out.str();
    result.ok = true;
    return result;
}

} // namespace iretable
