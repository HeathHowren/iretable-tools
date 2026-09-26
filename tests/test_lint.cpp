#include "iretable/Lint.h"
#include "iretable/Reader.h"

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

bool hasCategory(const std::vector<Issue>& issues, const std::string& category) {
    return std::any_of(issues.begin(), issues.end(),
                       [&](const Issue& issue) { return issue.category == category; });
}

} // namespace

TEST_CASE("lint catches each error class in the malformed fixture") {
    const auto result = lint(read(slurp("malformed.iretable")));
    REQUIRE(result.readable);
    REQUIRE_FALSE(result.clean());

    REQUIRE(hasCategory(result.problems, "orphan-field"));   // field before any struct
    REQUIRE(hasCategory(result.problems, "bad-hex"));        // "zzzz" address
    REQUIRE(hasCategory(result.problems, "unknown-type"));   // weirdtype
    REQUIRE(hasCategory(result.problems, "malformed-chain")); // "xyz" module offset
    REQUIRE(hasCategory(result.problems, "frozen-no-value")); // frozen with no value
    REQUIRE(hasCategory(result.problems, "duplicate-id"));   // two id 4 rows
}

TEST_CASE("lint passes a clean file") {
    const auto result = lint(read(slurp("canonical.iretable")));
    REQUIRE(result.readable);
    REQUIRE(result.clean());
    REQUIRE(result.problems.empty());
}

TEST_CASE("a preserved unknown record is a note, not a problem") {
    const auto result = lint(read(slurp("v3.iretable")));
    REQUIRE(result.readable);
    REQUIRE(result.clean()); // v3.iretable is otherwise well formed
    REQUIRE(hasCategory(result.notes, "unknown-record"));
    REQUIRE(result.notes.at(0).message == "Preserved unrecognized record: customrecord|keep me|verbatim\\|field");
}

TEST_CASE("a bad header fails the lint") {
    const auto result = lint(read("garbage\n"));
    REQUIRE_FALSE(result.readable);
    REQUIRE_FALSE(result.clean());
}
