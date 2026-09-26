#include "iretable/CheatTable.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

using namespace iretable;

namespace {

std::string slurp(const char* name) {
    std::ifstream in(std::string(IRETABLE_FIXTURE_DIR) + "/" + name, std::ios::binary);
    REQUIRE(in.good());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

bool anyContains(const std::vector<std::string>& lines, const std::string& needle) {
    return std::any_of(lines.begin(), lines.end(),
                       [&](const std::string& line) { return line.find(needle) != std::string::npos; });
}

} // namespace

TEST_CASE("a Cheat Engine table imports with chains, groups and types") {
    const auto result = convertFromCheatTable(slurp("pointer.CT"));
    REQUIRE(result.ok);
    REQUIRE(result.table.entries.size() == 4);

    const auto& health = result.table.entries.at(0);
    REQUIRE(health.description == "Health");
    REQUIRE(health.type == ValueType::Int32);
    REQUIRE(health.chain.has_value());
    REQUIRE(health.chain->moduleName == "Tutorial.exe");
    REQUIRE(health.chain->moduleOffset == 0x2E5A0);
    // Cheat Engine stores 18,0,10 (last-applied first); .iretable stores it
    // base-first, so the list is reversed on import.
    REQUIRE(health.chain->offsets == std::vector<std::int64_t>{0x10, 0x0, 0x18});

    const auto& ammo = result.table.entries.at(1);
    REQUIRE(ammo.description == "Ammo & clips"); // &amp; decoded
    REQUIRE(ammo.type == ValueType::Int16);
    REQUIRE(ammo.chain.has_value());
    REQUIRE(ammo.chain->offsets.empty()); // module+offset static, no dereference

    const auto& speed = result.table.entries.at(2);
    REQUIRE(speed.description == "Speed");
    REQUIRE(speed.type == ValueType::Float);
    REQUIRE(speed.group == "Player group");
    REQUIRE_FALSE(speed.chain.has_value());
    REQUIRE(speed.address == 0x13FA40000ULL);

    const auto& name = result.table.entries.at(3);
    REQUIRE(name.description == "Name");
    REQUIRE(name.type == ValueType::StringUtf16); // Unicode flag
    REQUIRE(name.group == "Player group");

    // The freeze hotkey has no home in .iretable and must be reported.
    REQUIRE(anyContains(result.lossy, "Hotkeys"));
}

TEST_CASE("export then re-import preserves the pointer chain") {
    const auto imported = convertFromCheatTable(slurp("pointer.CT"));
    REQUIRE(imported.ok);

    const auto exported = exportToCheatTable(imported.table);
    REQUIRE(exported.ok);

    const auto reimported = convertFromCheatTable(exported.ct);
    REQUIRE(reimported.ok);
    REQUIRE(reimported.table.entries.size() == imported.table.entries.size());

    const auto& before = imported.table.entries.at(0);
    const auto& after = reimported.table.entries.at(0);
    REQUIRE(after.chain.has_value());
    REQUIRE(after.chain->moduleName == before.chain->moduleName);
    REQUIRE(after.chain->moduleOffset == before.chain->moduleOffset);
    // The reverse on export undoes the reverse on import, so the offsets match.
    REQUIRE(after.chain->offsets == before.chain->offsets);

    // The group survived the round trip as a Cheat Engine group header.
    REQUIRE(reimported.table.entries.at(2).group == "Player group");
}

TEST_CASE("export reports records Cheat Engine cannot hold") {
    Table table;
    table.symbols.push_back({"health", "game.exe+0x10"});
    Entry entry;
    entry.id = 1;
    entry.description = "Frozen";
    entry.frozen = true;
    entry.frozenValue = {0x01};
    entry.hotkey = "F5";
    entry.address = 0x1000;
    table.entries.push_back(entry);

    const auto exported = exportToCheatTable(table);
    REQUIRE(exported.ok);
    REQUIRE(anyContains(exported.lossy, "symbol"));
    REQUIRE(anyContains(exported.lossy, "freeze"));
    REQUIRE(anyContains(exported.lossy, "hotkey"));
}

TEST_CASE("a non-Cheat-Engine XML file is refused") {
    const auto result = convertFromCheatTable("<something><else/></something>");
    REQUIRE_FALSE(result.ok);
}
