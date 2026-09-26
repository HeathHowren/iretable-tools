#pragma once

#include "iretable/Model.h"
#include "iretable/Reader.h"

#include <string>
#include <vector>

namespace iretable {

// The outcome of linting a file. readable is false only for an unreadable file
// or a bad header, which is a failure in itself. problems are the lines a file
// should not contain: malformed records, bad hex, broken pointer chains,
// duplicate ids, a frozen entry with nothing to freeze. notes are informational
// and do not fail the lint, such as unrecognised records that were preserved.
struct LintResult {
    bool readable{};
    std::string error;
    std::vector<Issue> problems;
    std::vector<Issue> notes;

    [[nodiscard]] bool clean() const { return readable && problems.empty(); }
};

// Lint already-parsed load output. Kept separate from reading so the CLI parses
// once and both loads and lints from the same result.
LintResult lint(const LoadResult& loaded);

} // namespace iretable
