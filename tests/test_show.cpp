#include "iretable/Model.h"
#include "iretable/Reader.h"
#include "iretable/Show.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using namespace iretable;

namespace {

std::string slurp(const char* name) {
    std::ifstream in(std::string(IRETABLE_FIXTURE_DIR) + "/" + name, std::ios::binary);
    REQUIRE(in.good());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        out.push_back(line);
    }
    return out;
}

} // namespace

TEST_CASE("show keeps an entry whose description has a newline on one row") {
    const auto loaded = read(slurp("v3.iretable"));
    REQUIRE(loaded.ok);
    // Entry #3's description holds a real newline, plus a backslash and a pipe.
    REQUIRE(loaded.table.entries.at(2).description == "weird|desc\\path\nline2");

    const auto rows = lines(formatTable(loaded.table));

    // The newline prints as \n; the backslash and the pipe print as-is.
    const auto row = std::find(rows.begin(), rows.end(), "  #3  i32  weird|desc\\path\\nline2  [Stats]");
    REQUIRE(row != rows.end());
    // The location follows on the next line, so the row was not split.
    REQUIRE(std::next(row) != rows.end());
    REQUIRE(*std::next(row) == "      helper.exe+0x3040");
    // No line starts with the text that came after the newline.
    for (const auto& line : rows) {
        REQUIRE(line.rfind("line2", 0) != 0);
    }
    REQUIRE(rows.back() == "1 preserved unrecognized record(s)");
}

TEST_CASE("show escapes control characters in every text field") {
    Table table;
    table.lastProcessName = "game\r.exe";
    Entry entry;
    entry.id = 7;
    entry.description = "tab\there";
    entry.group = "g\x01";
    PointerChain chain;
    chain.moduleName = "mod\n.dll";
    chain.moduleOffset = 0x10;
    entry.chain = chain;
    table.entries.push_back(entry);

    REQUIRE(formatTable(table) == "IRETABLE version 3\n"
                                  "process game\\r.exe, pid 0, x64\n"
                                  "1 entry\n"
                                  "\n"
                                  "  #7  i32  tab\\there  [g\\x01]\n"
                                  "      mod\\n.dll+0x10\n");
}
