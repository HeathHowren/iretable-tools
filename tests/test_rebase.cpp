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
