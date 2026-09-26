#include "iretable/Diff.h"

#include <catch2/catch_test_macros.hpp>

using namespace iretable;

namespace {

Entry fixed(std::uint64_t id, const std::string& description, std::uint64_t address, ValueType type = ValueType::Int32) {
    Entry entry;
    entry.id = id;
    entry.description = description;
    entry.address = address;
    entry.type = type;
    return entry;
}

} // namespace

TEST_CASE("diff reports added, removed and changed entries") {
    Table before;
    before.entries.push_back(fixed(1, "Health", 0x1000));
    before.entries.push_back(fixed(2, "Score", 0x2000));
    before.entries.push_back(fixed(3, "Ammo", 0x3000));

    Table after;
    // Health unchanged.
    after.entries.push_back(fixed(1, "Health", 0x1000));
    // Score moved and changed type: matched by description, reported as change.
    after.entries.push_back(fixed(2, "Score", 0x2500, ValueType::Float));
    // Ammo removed; Shield added.
    after.entries.push_back(fixed(4, "Shield", 0x4000));

    const auto result = diff(before, after);

    REQUIRE(result.removed.size() == 1);
    REQUIRE(result.removed[0].description == "Ammo");

    REQUIRE(result.added.size() == 1);
    REQUIRE(result.added[0].description == "Shield");

    REQUIRE(result.changed.size() == 1);
    REQUIRE(result.changed[0].after.description == "Score");
    // Both the location and the type changed.
    bool sawLocation = false;
    bool sawType = false;
    for (const auto& change : result.changed[0].changes) {
        if (change.field == "location") sawLocation = true;
        if (change.field == "type") sawType = true;
    }
    REQUIRE(sawLocation);
    REQUIRE(sawType);
}

TEST_CASE("two identical tables diff to nothing") {
    Table table;
    table.entries.push_back(fixed(1, "Health", 0x1000));
    REQUIRE(diff(table, table).empty());
}

TEST_CASE("formatDiff keeps each entry on its own line") {
    Table before;
    before.entries.push_back(fixed(1, "old\nrow", 0x1000));
    Entry score = fixed(2, "Score", 0x2000);
    score.group = "a";
    before.entries.push_back(score);

    Table after;
    after.entries.push_back(fixed(3, "new\trow", 0x3000));
    score.group = "b\nc";
    after.entries.push_back(score);

    REQUIRE(formatDiff(diff(before, after)) == "- old\\nrow  fixed 0x1000\n"
                                               "+ new\\trow  fixed 0x3000\n"
                                               "~ Score\n"
                                               "    group: a -> b\\nc\n");
    REQUIRE(formatDiff(diff(before, before)) == "No differences.\n");
}

TEST_CASE("locationString describes fixed, static and chained entries") {
    Entry e = fixed(1, "X", 0x1234);
    REQUIRE(locationString(e) == "fixed 0x1234");

    Entry chained = fixed(2, "Y", 0);
    PointerChain chain;
    chain.moduleName = "game.exe";
    chain.moduleOffset = 0x3040;
    chain.offsets = {0x10, 0x8};
    chained.chain = chain;
    REQUIRE(locationString(chained) == "game.exe+0x3040 -> 0x10 -> 0x8");
}
