#include "iretable/Lint.h"

#include <set>

namespace iretable {

LintResult lint(const LoadResult& loaded) {
    LintResult result;
    result.readable = loaded.ok;
    result.error = loaded.error;

    if (!loaded.ok) {
        // A bad header or unreadable file is the whole problem; report it and
        // stop, there is nothing parsed to check further.
        for (const auto& issue : loaded.issues) {
            result.problems.push_back(issue);
        }
        return result;
    }

    // Every degradation the reader logged is a lint problem: a skipped row, a
    // downgraded chain, an orphan field, an unreadable number.
    for (const auto& issue : loaded.issues) {
        result.problems.push_back(issue);
    }

    // Duplicate ids: the format says an id is stable within a file, so two rows
    // sharing one is a malformation a hand-edit can easily introduce.
    std::set<std::uint64_t> seen;
    for (const auto& entry : loaded.table.entries) {
        if (!seen.insert(entry.id).second) {
            result.problems.push_back(
                {0, Severity::Warning, "duplicate-id", "Duplicate entry id " + std::to_string(entry.id) + "."});
        }
    }

    // A frozen entry with no frozen value freezes nothing, which is almost
    // always a mistake in a hand-written file.
    for (const auto& entry : loaded.table.entries) {
        if (entry.frozen && entry.frozenValue.empty()) {
            result.problems.push_back({0, Severity::Warning, "frozen-no-value",
                                       "Entry " + std::to_string(entry.id) + " is frozen but has no frozen value."});
        }
    }

    // Preserved unknown records are not a failure; the format is deliberately
    // extensible. They are noted so the user knows they are there.
    for (const auto& record : loaded.table.unknownRecords) {
        result.notes.push_back({0, Severity::Warning, "unknown-record", "Preserved unrecognized record: " + record});
    }

    return result;
}

} // namespace iretable
