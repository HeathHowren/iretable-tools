#pragma once

#include "iretable/Model.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace iretable {

// Where a parse problem sits on the severity scale. Warnings are lines Pointer
// Lab degrades over (a skipped row, a chain downgraded to a fixed address);
// the load still succeeds. An Error is a bad header or an unreadable file, and
// stops the load.
enum class Severity {
    Warning,
    Error
};

// One problem found while reading. line is 1-based; 0 means the file as a whole.
struct Issue {
    std::size_t line{};
    Severity severity{Severity::Warning};
    // A short machine-readable class, e.g. "bad-hex", "unknown-type",
    // "malformed-chain", used by lint and the --json output.
    std::string category;
    std::string message;
};

// The result of reading a file: the table, every issue found, and whether the
// load succeeded at all. A load fails only for an unreadable file or a bad
// header; everything else degrades to a warning, exactly as Pointer Lab does.
struct LoadResult {
    bool ok{};
    std::string error; // set when ok is false
    Table table;
    std::vector<Issue> issues;

    [[nodiscard]] std::size_t warningCount() const;
};

// Parse .iretable text. The path is used only for messages.
LoadResult read(const std::string& text);

// Read a file from disk. Returns ok=false with an error if it cannot be opened.
LoadResult readFile(const std::filesystem::path& path);

} // namespace iretable
