#pragma once

#include "iretable/Model.h"

#include <string>
#include <vector>

// Conversion between Cheat Engine .CT tables and .iretable. Descriptions,
// value types, static addresses and pointer offset chains are carried across;
// anything that has no counterpart is reported rather than dropped in silence.
namespace iretable {

// The result of a conversion. lossy holds one human-readable line per field
// that did not map, meant for stderr. On import, ok is false only when the file
// is not a readable Cheat Engine table.
struct ConvertResult {
    bool ok{};
    std::string error;
    Table table;
    std::vector<std::string> lossy;
};

// The result of exporting a table back to .CT. ct is the XML text.
struct ExportResult {
    bool ok{};
    std::string error;
    std::string ct;
    std::vector<std::string> lossy;
};

// Import a Cheat Engine .CT (XML) into a table. Cheat Engine stores pointer
// offsets in the reverse of the order they are applied; they are reversed here
// so the resulting chain reads base-first, as .iretable stores it.
ConvertResult convertFromCheatTable(const std::string& xmlText);

// Export a table to a Cheat Engine .CT. The reverse of the offset ordering is
// undone, groups become Cheat Engine group headers, and records with no Cheat
// Engine counterpart (symbols, scripts, structures, freeze state, hotkeys) are
// reported as lossy.
ExportResult exportToCheatTable(const Table& table);

} // namespace iretable
