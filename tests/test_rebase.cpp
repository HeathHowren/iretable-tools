#include "iretable/Model.h"
#include "iretable/Rebase.h"

#include <catch2/catch_test_macros.hpp>

using namespace iretable;

TEST_CASE("rebase shifts fixed addresses and absolute-base chains, not module-rooted ones") {
    Table table;

    Entry fixed;
    fixed.id = 1;
    fixed.description = "Fixed";
    fixed.address = 0x140001000;
    table.entries.push_back(fixed);

    Entry absoluteChain;
    absoluteChain.id = 2;
    absoluteChain.description = "Absolute base chain";
    PointerChain abs;
    abs.moduleName = ""; // absolute base
    abs.moduleOffset = 0x140002000;
    abs.offsets = {0x10};
    absoluteChain.chain = abs;
    table.entries.push_back(absoluteChain);

    Entry moduleChain;
    moduleChain.id = 3;
    moduleChain.description = "Module chain";
    PointerChain mod;
    mod.moduleName = "game.exe";
    mod.moduleOffset = 0x3040;
    mod.offsets = {0x8};
    moduleChain.chain = mod;
    table.entries.push_back(moduleChain);

    const auto result = rebase(table, 0x140000000, 0x150000000);

    // delta = 0x10000000
    REQUIRE(table.entries[0].address == 0x150001000);
    REQUIRE(table.entries[1].chain->moduleOffset == 0x150002000);
    REQUIRE(table.entries[1].chain->offsets == std::vector<std::int64_t>{0x10}); // offsets untouched
    // The module-rooted chain is left alone; it survives a relocation on its own.
    REQUIRE(table.entries[2].chain->moduleOffset == 0x3040);

    REQUIRE(result.shifts.size() == 2);
    REQUIRE(result.moduleRootedSkipped == 1);
}

TEST_CASE("rebase works downward too") {
    Table table;
    Entry fixed;
    fixed.id = 1;
    fixed.address = 0x2000;
    table.entries.push_back(fixed);
    rebase(table, 0x1000, 0x0);
    REQUIRE(table.entries[0].address == 0x1000);
}

TEST_CASE("rebase summary counts each kind of shift and signs the delta") {
    Table table;

    Entry fixed;
    fixed.id = 1;
    fixed.address = 0x140001000;
    table.entries.push_back(fixed);

    for (std::uint64_t id : {2u, 3u}) {
        Entry absoluteChain;
        absoluteChain.id = id;
        PointerChain abs;
        abs.moduleOffset = 0x140002000;
        abs.offsets = {0x10};
        absoluteChain.chain = abs;
        table.entries.push_back(absoluteChain);
    }

    Entry moduleStatic;
    moduleStatic.id = 4;
    PointerChain mod;
    mod.moduleName = "game.exe";
    mod.moduleOffset = 0x3040; // no offsets: a static module+offset address
    moduleStatic.chain = mod;
    table.entries.push_back(moduleStatic);

    const auto up = rebase(table, 0x140000000, 0x150000000);
    CHECK(up.fixedAddresses == 1);
    CHECK(up.chainBases == 2);
    CHECK(up.moduleRootedSkipped == 1);
    CHECK(rebaseSummary(up, 0x140000000, 0x150000000) ==
          "Rebased 1 fixed address and 2 pointer chain bases by +0x10000000. "
          "Left 1 module-rooted entry alone. Module-rooted entries survive a relocation on their own.");

    // A downward move prints a negative delta, not the 64-bit wrapped value.
    const auto down = rebase(table, 0x150000000, 0x140000000);
    CHECK(rebaseSummary(down, 0x150000000, 0x140000000) ==
          "Rebased 1 fixed address and 2 pointer chain bases by -0x10000000. "
          "Left 1 module-rooted entry alone. Module-rooted entries survive a relocation on their own.");

    Table onlyFixed;
    onlyFixed.entries.push_back(fixed);
    const auto plain = rebase(onlyFixed, 0x1000, 0x3000);
    CHECK(rebaseSummary(plain, 0x1000, 0x3000) == "Rebased 1 fixed address and 0 pointer chain bases by +0x2000.");
}
