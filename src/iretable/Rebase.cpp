#include "iretable/Rebase.h"

#include "iretable/Text.h"

namespace iretable {

namespace {

std::string count(std::size_t n, const char* one, const char* many) {
    return std::to_string(n) + " " + (n == 1 ? one : many);
}

} // namespace

RebaseResult rebase(Table& table, std::uint64_t oldBase, std::uint64_t newBase) {
    RebaseResult result;
    const std::uint64_t delta = newBase - oldBase; // wraps; that is fine

    for (auto& entry : table.entries) {
        if (!entry.chain) {
            const std::uint64_t before = entry.address;
            entry.address = before + delta;
            result.shifts.push_back({entry.id, entry.description, before, entry.address});
            ++result.fixedAddresses;
        } else if (!entry.chain->moduleRooted()) {
            // A manually entered chain rooted at an absolute address. That base
            // is the thing a relocation moves; the offsets are relative and
            // stay put.
            const std::uint64_t before = entry.chain->moduleOffset;
            entry.chain->moduleOffset = before + delta;
            result.shifts.push_back({entry.id, entry.description, before, entry.chain->moduleOffset});
            ++result.chainBases;
        } else {
            ++result.moduleRootedSkipped;
        }
    }

    return result;
}

std::string rebaseSummary(const RebaseResult& result, std::uint64_t oldBase, std::uint64_t newBase) {
    const std::string delta =
        newBase >= oldBase ? "+0x" + toHexLower(newBase - oldBase) : "-0x" + toHexLower(oldBase - newBase);
    std::string out = "Rebased " + count(result.fixedAddresses, "fixed address", "fixed addresses") + " and " +
                      count(result.chainBases, "pointer chain base", "pointer chain bases") + " by " + delta + ".";
    if (result.moduleRootedSkipped > 0) {
        out += " Left " + count(result.moduleRootedSkipped, "module-rooted entry", "module-rooted entries") +
               " alone. Module-rooted entries survive a relocation on their own.";
    }
    return out;
}

} // namespace iretable
