#pragma once

#include "iretable/Model.h"

#include <filesystem>
#include <string>

namespace iretable {

// Serialize a table to .iretable text, LF line endings, no BOM. Records are
// written in Pointer Lab's canonical order (pid, process, bitness, symbols,
// scripts, structures, entries). Preserved unknown records follow the entries.
//
// The header version written is table.version, except that a table holding an
// entry with a pointer chain is written as version 3 regardless, because
// versions 1 and 2 cannot express a chain. For a version 1 or 2 table, entry
// lines stop after the frozen-value field, as those versions wrote them.
std::string write(const Table& table);

// Write to disk. Returns false if the file cannot be opened or the write does
// not flush cleanly.
bool writeFile(const std::filesystem::path& path, const Table& table);

} // namespace iretable
