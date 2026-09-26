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
    // Module-rooted chains are already relative to their module and survive a
    // relocation on their own, so a base delta does not touch them. Counted so
    // the caller can say why some entries were left alone.
    std::size_t moduleRootedSkipped{};
};

// Shift the static addresses in a table by (newBase - oldBase). A fixed-address
// entry has its address shifted; a chain with an absolute base (no module) has
// that base shifted; a module-rooted chain is left untouched. The table is
// modified in place. The delta wraps in 64 bits, so a move in either direction
// works.
RebaseResult rebase(Table& table, std::uint64_t oldBase, std::uint64_t newBase);

} // namespace iretable
