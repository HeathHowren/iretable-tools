#include "iretable/Rebase.h"

namespace iretable {

RebaseResult rebase(Table& table, std::uint64_t oldBase, std::uint64_t newBase) {
    RebaseResult result;
    const std::uint64_t delta = newBase - oldBase; // wraps; that is fine

    for (auto& entry : table.entries) {
        if (!entry.chain) {
            const std::uint64_t before = entry.address;
            entry.address = before + delta;
            result.shifts.push_back({entry.id, entry.description, before, entry.address});
        } else if (!entry.chain->moduleRooted()) {
            // A manually entered chain rooted at an absolute address. That base
            // is the thing a relocation moves; the offsets are relative and
            // stay put.
            const std::uint64_t before = entry.chain->moduleOffset;
            entry.chain->moduleOffset = before + delta;
            result.shifts.push_back({entry.id, entry.description, before, entry.chain->moduleOffset});
        } else {
            ++result.moduleRootedSkipped;
        }
    }

    return result;
}

} // namespace iretable
