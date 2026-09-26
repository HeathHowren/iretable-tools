#pragma once

#include "iretable/Model.h"

#include <string>
#include <vector>

namespace iretable {

// A single field that differs between two matched entries.
struct FieldChange {
    std::string field;
    std::string before;
    std::string after;
};

// An entry present in both tables whose contents changed.
struct ChangedEntry {
    Entry before;
    Entry after;
    std::vector<FieldChange> changes;
};

// The result of comparing two tables at the entry level. Entries are matched by
// description first, preferring a match with the same address; a matched pair
// with differing fields is a change, an unmatched left entry is a removal, an
// unmatched right entry is an addition.
struct DiffResult {
    std::vector<Entry> added;
    std::vector<Entry> removed;
    std::vector<ChangedEntry> changed;

    [[nodiscard]] bool empty() const { return added.empty() && removed.empty() && changed.empty(); }
};

DiffResult diff(const Table& before, const Table& after);

// A one-line human-readable form of where an entry points: "fixed 0x1000",
// "helper.exe+0x3040" for a static base, or a chain with its offsets.
std::string locationString(const Entry& entry);

} // namespace iretable
