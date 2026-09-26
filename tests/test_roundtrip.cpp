#include "iretable/Model.h"
#include "iretable/Reader.h"
#include "iretable/Writer.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <sstream>
#include <string>

using namespace iretable;

namespace {

std::string fixturePath(const char* name) {
    return std::string(IRETABLE_FIXTURE_DIR) + "/" + name;
}

std::string slurp(const char* name) {
    std::ifstream in(fixturePath(name), std::ios::binary);
    REQUIRE(in.good());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

} // namespace

TEST_CASE("a table built in code survives write then read unchanged") {
    Table table;
    table.version = 3;
    table.lastPid = 4812;
    table.lastProcessName = "helper.exe";
    table.lastBitness = Bitness::X64;
    table.symbols.push_back({"health", "helper.exe+0x4a2c10"});
    table.scripts.push_back({"note", "[ENABLE]\ndo stuff"});
    Structure player;
    player.name = "Player";
    player.fields.push_back({0, ValueType::UInt64, 0, "vtable"});
    player.fields.push_back({8, ValueType::Float, 0, "health"});
    table.structures.push_back(player);

    Entry fixed;
    fixed.id = 1;
    fixed.address = 0x7ff6a1c02040;
    fixed.type = ValueType::Int32;
    fixed.frozen = true;
    fixed.description = "weird|desc\\path\nline2"; // exercises all escaping
    fixed.group = "Stats";
    fixed.hotkey = "F1";
    fixed.frozenValue = {0x64, 0x00, 0x00, 0x00};
    table.entries.push_back(fixed);

    Entry chained;
    chained.id = 2;
    chained.type = ValueType::Int32;
    chained.description = "Score";
    chained.group = "Stats";
    PointerChain chain;
    chain.moduleName = "helper.exe";
    chain.moduleOffset = 0x3040;
    chain.offsets = {0x10, 0x8};
    chained.chain = chain;
    table.entries.push_back(chained);

    const auto text = write(table);
    const auto loaded = read(text);
    REQUIRE(loaded.ok);
    // Writing the reparsed table must reproduce the same bytes.
    REQUIRE(write(loaded.table) == text);

    const auto& e0 = loaded.table.entries.at(0);
    REQUIRE(e0.description == "weird|desc\\path\nline2");
    REQUIRE(e0.frozen);
    REQUIRE(e0.frozenValue == std::vector<std::uint8_t>{0x64, 0x00, 0x00, 0x00});
    const auto& e1 = loaded.table.entries.at(1);
    REQUIRE(e1.chain.has_value());
    REQUIRE(e1.chain->moduleName == "helper.exe");
    REQUIRE(e1.chain->moduleOffset == 0x3040);
    REQUIRE(e1.chain->offsets == std::vector<std::int64_t>{0x10, 0x8});
}

TEST_CASE("version 1 files read as fixed addresses") {
    const auto loaded = read(slurp("v1.iretable"));
    REQUIRE(loaded.ok);
    REQUIRE(loaded.table.version == 1);
    REQUIRE(loaded.table.entries.size() == 2);
    REQUIRE_FALSE(loaded.table.entries[0].chain.has_value());
    REQUIRE(loaded.table.entries[0].address == 0x400000);
    REQUIRE(loaded.table.entries[1].type == ValueType::Float);
    REQUIRE(loaded.table.entries[1].frozen);
    // A version 1 table writes without the trailing chain fields.
    const auto out = write(loaded.table);
    REQUIRE(out.find("IRETABLE 1") == 0);
    REQUIRE(out.find("entry|1|400000|i32|0|Score|Main||\n") != std::string::npos);
}

TEST_CASE("version 2 files tolerate an extra trailing field") {
    const auto loaded = read(slurp("v2.iretable"));
    REQUIRE(loaded.ok);
    REQUIRE(loaded.table.version == 2);
    REQUIRE(loaded.table.entries.size() == 2);
    REQUIRE(loaded.table.entries[1].type == ValueType::UInt16);
    REQUIRE_FALSE(loaded.table.entries[1].chain.has_value());
}

TEST_CASE("version 3 files parse chains, escaping and preserve unknown records") {
    const auto loaded = read(slurp("v3.iretable"));
    REQUIRE(loaded.ok);
    REQUIRE(loaded.table.version == 3);
    REQUIRE(loaded.table.symbols.size() == 1);
    REQUIRE(loaded.table.scripts.size() == 1);
    REQUIRE(loaded.table.scripts[0].source == "[ENABLE]\ndo stuff");
    REQUIRE(loaded.table.structures.size() == 1);
    REQUIRE(loaded.table.structures[0].fields.size() == 2);

    // The escaped description round-trips to its literal form.
    REQUIRE(loaded.table.entries.at(2).description == "weird|desc\\path\nline2");

    // A static module-rooted address is a chain with an empty offset list.
    const auto& ammo = loaded.table.entries.at(2);
    REQUIRE(ammo.chain.has_value());
    REQUIRE(ammo.chain->moduleName == "helper.exe");
    REQUIRE(ammo.chain->offsets.empty());

    // The unrecognized record is preserved verbatim.
    REQUIRE(loaded.table.unknownRecords.size() == 1);
    REQUIRE(loaded.table.unknownRecords[0] == "customrecord|keep me|verbatim\\|field");

    // On write the unknown record is still there.
    REQUIRE(write(loaded.table).find("customrecord|keep me|verbatim\\|field") != std::string::npos);
}

TEST_CASE("a canonical file round-trips byte for byte") {
    const auto original = slurp("canonical.iretable");
    const auto loaded = read(original);
    REQUIRE(loaded.ok);
    REQUIRE(write(loaded.table) == original);
}

TEST_CASE("a UTF-8 BOM before the header is tolerated") {
    const auto loaded = read(slurp("bom.iretable"));
    REQUIRE(loaded.ok);
    REQUIRE(loaded.table.entries.size() == 1);
    REQUIRE(loaded.table.lastProcessName == "bom.exe");
}

TEST_CASE("a bad header is refused") {
    const auto loaded = read("NOTATABLE 9\nentry|1|1000|i32|0|X|G||\n");
    REQUIRE_FALSE(loaded.ok);
    REQUIRE_FALSE(loaded.error.empty());
}
