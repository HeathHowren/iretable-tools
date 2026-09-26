#include "iretable/Diff.h"

#include "iretable/Text.h"
#include "iretable/ValueType.h"

#include <cstddef>

namespace iretable {

std::string locationString(const Entry& entry) {
    if (!entry.chain) {
        return "fixed 0x" + toHexLower(entry.address);
    }
    const auto& chain = *entry.chain;
    std::string base = chain.moduleRooted() ? chain.moduleName + "+0x" + toHexLower(chain.moduleOffset)
                                             : "0x" + toHexLower(chain.moduleOffset);
    for (const auto offset : chain.offsets) {
        base += " -> 0x" + toHexLower(static_cast<std::uint64_t>(offset));
    }
    return base;
}

namespace {

void collectChanges(const Entry& a, const Entry& b, std::vector<FieldChange>& out) {
    if (locationString(a) != locationString(b)) {
        out.push_back({"location", locationString(a), locationString(b)});
    }
    if (a.type != b.type) {
        out.push_back({"type", valueTypeName(a.type), valueTypeName(b.type)});
    }
    if (a.frozen != b.frozen) {
        out.push_back({"frozen", a.frozen ? "true" : "false", b.frozen ? "true" : "false"});
    }
    if (a.frozenValue != b.frozenValue) {
        out.push_back({"frozen-value", bytesToHex(a.frozenValue), bytesToHex(b.frozenValue)});
    }
    if (a.group != b.group) {
        out.push_back({"group", a.group, b.group});
    }
    if (a.hotkey != b.hotkey) {
        out.push_back({"hotkey", a.hotkey, b.hotkey});
    }
}

} // namespace

DiffResult diff(const Table& before, const Table& after) {
    DiffResult result;

    // Right-hand entries still available to match. Matching consumes them so a
    // description that appears twice pairs one-to-one rather than many-to-one.
    std::vector<bool> matched(after.entries.size(), false);

    const auto findMatch = [&](const Entry& left) -> std::size_t {
        std::size_t sameDescription = after.entries.size();
        for (std::size_t i = 0; i < after.entries.size(); ++i) {
            if (matched[i] || after.entries[i].description != left.description) {
                continue;
            }
            // Prefer a match at the same location; it is almost certainly the
            // same row. Fall back to the first same-description row otherwise.
            if (locationString(after.entries[i]) == locationString(left)) {
                return i;
            }
            if (sameDescription == after.entries.size()) {
                sameDescription = i;
            }
        }
        return sameDescription;
    };

    for (const auto& left : before.entries) {
        const auto match = findMatch(left);
        if (match == after.entries.size()) {
            result.removed.push_back(left);
            continue;
        }
        matched[match] = true;
        const auto& right = after.entries[match];
        std::vector<FieldChange> changes;
        collectChanges(left, right, changes);
        if (!changes.empty()) {
            result.changed.push_back({left, right, std::move(changes)});
        }
    }

    for (std::size_t i = 0; i < after.entries.size(); ++i) {
        if (!matched[i]) {
            result.added.push_back(after.entries[i]);
        }
    }

    return result;
}

std::string formatDiff(const DiffResult& result) {
    if (result.empty()) {
        return "No differences.\n";
    }
    const auto label = [](const Entry& entry) {
        return entry.description.empty() ? std::string("(no description)") : escapeForDisplay(entry.description);
    };
    std::string out;
    for (const auto& entry : result.removed) {
        out += "- " + label(entry) + "  " + escapeForDisplay(locationString(entry)) + "\n";
    }
    for (const auto& entry : result.added) {
        out += "+ " + label(entry) + "  " + escapeForDisplay(locationString(entry)) + "\n";
    }
    for (const auto& change : result.changed) {
        out += "~ " + label(change.after) + "\n";
        for (const auto& field : change.changes) {
            out += "    " + field.field + ": " + escapeForDisplay(field.before) + " -> " +
                   escapeForDisplay(field.after) + "\n";
        }
    }
    return out;
}

} // namespace iretable
