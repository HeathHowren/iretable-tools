#pragma once

#include "iretable/Model.h"

#include <cstdint>
#include <string>
#include <vector>

namespace iretable {

// A record of one address that moved during a rebase.
struct Shift {
    std::uint64_t id{};
    std::string description;
    std::uint64_t before{};
    std::uint64_t after{};
};

struct RebaseResult {
    std::vector<Shift> shifts;
    // What the shifts were: fixed-address entries, and the bases of pointer
    // chains that start at a fixed address.
    std::size_t fixedAddresses{};
    std::size_t chainBases{};
    // Module-rooted entries (chains and module+offset addresses) are already
    // relative to their module and survive a relocation on their own, so a base
    // delta does not touch them. Counted so the caller can say why some entries
    // were left alone.
    std::size_t moduleRootedSkipped{};
};

// Shift the fixed addresses in a table by (newBase - oldBase). A fixed-address
// entry has its address shifted; a chain with an absolute base (no module) has
// that base shifted; a module-rooted entry is left untouched. The table is
// modified in place. The delta wraps in 64 bits, so a move in either direction
// works.
RebaseResult rebase(Table& table, std::uint64_t oldBase, std::uint64_t newBase);

// The one-line summary `iretable rebase` prints to stderr, without a newline.
// It counts each kind of shift and signs the delta, as in "+0x1000" or
// "-0x1000".
std::string rebaseSummary(const RebaseResult& result, std::uint64_t oldBase, std::uint64_t newBase);

} // namespace iretable
