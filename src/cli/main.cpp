// The `iretable` command-line tool: read, convert, diff, rebase and lint
// Pointer Lab .iretable project files.

#include "iretable/CheatTable.h"
#include "iretable/Diff.h"
#include "iretable/Json.h"
#include "iretable/Lint.h"
#include "iretable/Model.h"
#include "iretable/Reader.h"
#include "iretable/Rebase.h"
#include "iretable/Show.h"
#include "iretable/Text.h"
#include "iretable/ValueType.h"
#include "iretable/Writer.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace iretable;

constexpr const char* kVersion = "1.0.0";

int usage(std::ostream& out) {
    out << "iretable " << kVersion << " - a toolkit for Pointer Lab .iretable project files\n"
        << "\n"
        << "Usage:\n"
        << "  iretable show    <in.iretable> [--json]\n"
        << "  iretable convert <in.CT> -o <out.iretable> [--json]\n"
        << "  iretable export  <in.iretable> --ct <out.CT>\n"
        << "  iretable diff    <a.iretable> <b.iretable> [--json]\n"
        << "  iretable rebase  <in.iretable> --old-base <hex> --new-base <hex> [-o <out.iretable>]\n"
        << "  iretable lint    <in.iretable> [--json]\n"
        << "\n"
        << "  --json    machine-readable output (show, convert, diff, lint)\n"
        << "  --version print the version\n"
        << "  --help    print this help\n"
        << "\n"
        << "convert --json still writes the file, then prints the entry count and the\n"
        << "lossy notes as JSON.\n"
        << "\n"
        << "rebase adds (new base - old base) to every fixed address and to the base of\n"
        << "every pointer chain that starts at a fixed address. It leaves module-rooted\n"
        << "entries, chain offsets, symbols, scripts and structures alone.\n"
        << "\n"
        << "Exit codes: diff and lint return 1 when there are differences or problems,\n"
        << "2 on a usage or read error. Other commands return 0 on success, 2 on error.\n";
    return 2;
}

bool readWholeFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

std::optional<std::uint64_t> parseHexArg(const std::string& text) {
    std::string value = text;
    if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0) {
        value = value.substr(2);
    }
    if (value.empty() || value.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos || value.size() > 16) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(std::stoull(value, nullptr, 16));
}

const char* bitnessName(Bitness bitness) {
    return bitness == Bitness::X86 ? "x86" : "x64";
}

// --- JSON builders -------------------------------------------------------

json::Value chainJson(const PointerChain& chain) {
    auto obj = json::Value::object();
    obj.set("module", json::Value::str(chain.moduleName));
    obj.set("module_offset", json::Value::str(toHexLower(chain.moduleOffset)));
    auto offsets = json::Value::array();
    for (const auto offset : chain.offsets) {
        offsets.push(json::Value::str(toHexLower(static_cast<std::uint64_t>(offset))));
    }
    obj.set("offsets", std::move(offsets));
    return obj;
}

json::Value entryJson(const Entry& entry) {
    auto obj = json::Value::object();
    obj.set("id", json::Value::number(entry.id));
    obj.set("address", json::Value::str(toHexLower(entry.address)));
    obj.set("type", json::Value::str(valueTypeName(entry.type)));
    obj.set("frozen", json::Value::boolean(entry.frozen));
    obj.set("description", json::Value::str(entry.description));
    obj.set("group", json::Value::str(entry.group));
    obj.set("hotkey", json::Value::str(entry.hotkey));
    if (!entry.frozenValue.empty()) {
        obj.set("frozen_value", json::Value::str(bytesToHex(entry.frozenValue)));
    }
    if (entry.chain) {
        obj.set("chain", chainJson(*entry.chain));
    }
    obj.set("location", json::Value::str(locationString(entry)));
    return obj;
}

json::Value tableJson(const Table& table) {
    auto obj = json::Value::object();
    obj.set("version", json::Value::number(static_cast<std::int64_t>(table.version)));
    obj.set("pid", json::Value::number(static_cast<std::uint64_t>(table.lastPid)));
    obj.set("process", json::Value::str(table.lastProcessName));
    obj.set("bitness", json::Value::str(bitnessName(table.lastBitness)));

    auto symbols = json::Value::array();
    for (const auto& symbol : table.symbols) {
        auto s = json::Value::object();
        s.set("name", json::Value::str(symbol.name));
        s.set("expression", json::Value::str(symbol.expression));
        symbols.push(std::move(s));
    }
    obj.set("symbols", std::move(symbols));

    auto scripts = json::Value::array();
    for (const auto& script : table.scripts) {
        auto s = json::Value::object();
        s.set("name", json::Value::str(script.name));
        s.set("source", json::Value::str(script.source));
        scripts.push(std::move(s));
    }
    obj.set("scripts", std::move(scripts));

    auto structures = json::Value::array();
    for (const auto& structure : table.structures) {
        auto s = json::Value::object();
        s.set("name", json::Value::str(structure.name));
        auto fields = json::Value::array();
        for (const auto& field : structure.fields) {
            auto f = json::Value::object();
            f.set("offset", json::Value::str(toHexLower(static_cast<std::uint64_t>(field.offset))));
            f.set("type", json::Value::str(valueTypeName(field.type)));
            f.set("length", json::Value::number(field.length));
            f.set("name", json::Value::str(field.name));
            fields.push(std::move(f));
        }
        s.set("fields", std::move(fields));
        structures.push(std::move(s));
    }
    obj.set("structures", std::move(structures));

    auto entries = json::Value::array();
    for (const auto& entry : table.entries) {
        entries.push(entryJson(entry));
    }
    obj.set("entries", std::move(entries));

    auto unknown = json::Value::array();
    for (const auto& record : table.unknownRecords) {
        unknown.push(json::Value::str(record));
    }
    obj.set("unknown_records", std::move(unknown));
    return obj;
}

json::Value issuesJson(const std::vector<Issue>& issues) {
    auto array = json::Value::array();
    for (const auto& issue : issues) {
        auto obj = json::Value::object();
        obj.set("line", json::Value::number(static_cast<std::uint64_t>(issue.line)));
        obj.set("category", json::Value::str(issue.category));
        obj.set("message", json::Value::str(issue.message));
        array.push(std::move(obj));
    }
    return array;
}

// --- commands ------------------------------------------------------------

int cmdShow(const std::vector<std::string>& args) {
    std::string path;
    bool asJson = false;
    for (const auto& arg : args) {
        if (arg == "--json") {
            asJson = true;
        } else if (path.empty()) {
            path = arg;
        } else {
            std::cerr << "show: unexpected argument \"" << arg << "\"\n";
            return 2;
        }
    }
    if (path.empty()) {
        std::cerr << "show: expected a file\n";
        return 2;
    }
    const auto loaded = readFile(path);
    if (!loaded.ok) {
        std::cerr << "show: " << loaded.error << "\n";
        return 2;
    }

    if (asJson) {
        auto obj = json::Value::object();
        obj.set("table", tableJson(loaded.table));
        obj.set("warnings", issuesJson(loaded.issues));
        std::cout << obj.dump() << "\n";
        return 0;
    }

    std::cout << formatTable(loaded.table);
    for (const auto& issue : loaded.issues) {
        std::cerr << "warning: line " << issue.line << ": " << escapeForDisplay(issue.message) << "\n";
    }
    return 0;
}

int cmdConvert(const std::vector<std::string>& args) {
    std::string input;
    std::string output;
    bool asJson = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-o" || args[i] == "--out") {
            if (i + 1 >= args.size()) {
                std::cerr << "convert: " << args[i] << " needs a file\n";
                return 2;
            }
            output = args[++i];
        } else if (args[i] == "--json") {
            asJson = true;
        } else if (input.empty()) {
            input = args[i];
        } else {
            std::cerr << "convert: unexpected argument \"" << args[i] << "\"\n";
            return 2;
        }
    }
    if (input.empty() || output.empty()) {
        std::cerr << "convert: expected <in.CT> -o <out.iretable>\n";
        return 2;
    }

    std::string xml;
    if (!readWholeFile(input, xml)) {
        std::cerr << "convert: could not read " << input << "\n";
        return 2;
    }
    const auto converted = convertFromCheatTable(xml);
    if (!converted.ok) {
        std::cerr << "convert: " << converted.error << "\n";
        return 2;
    }
    if (!writeFile(output, converted.table)) {
        std::cerr << "convert: could not write " << output << "\n";
        return 2;
    }

    if (asJson) {
        auto obj = json::Value::object();
        obj.set("entries", json::Value::number(static_cast<std::uint64_t>(converted.table.entries.size())));
        auto lossy = json::Value::array();
        for (const auto& note : converted.lossy) {
            lossy.push(json::Value::str(note));
        }
        obj.set("lossy", std::move(lossy));
        std::cout << obj.dump() << "\n";
    } else {
        for (const auto& note : converted.lossy) {
            std::cerr << "lossy: " << escapeForDisplay(note) << "\n";
        }
        std::cout << "Imported " << converted.table.entries.size() << " entr"
                  << (converted.table.entries.size() == 1 ? "y" : "ies") << " to " << output;
        if (!converted.lossy.empty()) {
            std::cout << " (" << converted.lossy.size() << " field(s) did not map; see stderr)";
        }
        std::cout << "\n";
    }
    return 0;
}

int cmdExport(const std::vector<std::string>& args) {
    std::string input;
    std::string output;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--ct") {
            if (i + 1 >= args.size()) {
                std::cerr << "export: --ct needs a file\n";
                return 2;
            }
            output = args[++i];
        } else if (input.empty()) {
            input = args[i];
        } else {
            std::cerr << "export: unexpected argument \"" << args[i] << "\"\n";
            return 2;
        }
    }
    if (input.empty() || output.empty()) {
        std::cerr << "export: expected <in.iretable> --ct <out.CT>\n";
        return 2;
    }

    const auto loaded = readFile(input);
    if (!loaded.ok) {
        std::cerr << "export: " << loaded.error << "\n";
        return 2;
    }
    const auto exported = exportToCheatTable(loaded.table);
    if (!exported.ok) {
        std::cerr << "export: " << exported.error << "\n";
        return 2;
    }
    std::ofstream out(output, std::ios::binary);
    if (!out) {
        std::cerr << "export: could not write " << output << "\n";
        return 2;
    }
    out << exported.ct;
    out.flush();
    if (!out) {
        std::cerr << "export: could not write " << output << "\n";
        return 2;
    }
    for (const auto& note : exported.lossy) {
        std::cerr << "lossy: " << escapeForDisplay(note) << "\n";
    }
    std::cout << "Exported " << loaded.table.entries.size() << " entr"
              << (loaded.table.entries.size() == 1 ? "y" : "ies") << " to " << output;
    if (!exported.lossy.empty()) {
        std::cout << " (" << exported.lossy.size() << " field(s) did not map; see stderr)";
    }
    std::cout << "\n";
    return 0;
}

int cmdDiff(const std::vector<std::string>& args) {
    std::vector<std::string> files;
    bool asJson = false;
    for (const auto& arg : args) {
        if (arg == "--json") {
            asJson = true;
        } else {
            files.push_back(arg);
        }
    }
    if (files.size() != 2) {
        std::cerr << "diff: expected two files\n";
        return 2;
    }
    const auto a = readFile(files[0]);
    const auto b = readFile(files[1]);
    if (!a.ok) {
        std::cerr << "diff: " << files[0] << ": " << a.error << "\n";
        return 2;
    }
    if (!b.ok) {
        std::cerr << "diff: " << files[1] << ": " << b.error << "\n";
        return 2;
    }
    const auto result = diff(a.table, b.table);

    if (asJson) {
        auto obj = json::Value::object();
        auto added = json::Value::array();
        for (const auto& entry : result.added) {
            added.push(entryJson(entry));
        }
        auto removed = json::Value::array();
        for (const auto& entry : result.removed) {
            removed.push(entryJson(entry));
        }
        auto changed = json::Value::array();
        for (const auto& change : result.changed) {
            auto c = json::Value::object();
            c.set("description", json::Value::str(change.after.description));
            auto fields = json::Value::array();
            for (const auto& field : change.changes) {
                auto f = json::Value::object();
                f.set("field", json::Value::str(field.field));
                f.set("before", json::Value::str(field.before));
                f.set("after", json::Value::str(field.after));
                fields.push(std::move(f));
            }
            c.set("changes", std::move(fields));
            changed.push(std::move(c));
        }
        obj.set("added", std::move(added));
        obj.set("removed", std::move(removed));
        obj.set("changed", std::move(changed));
        std::cout << obj.dump() << "\n";
        return result.empty() ? 0 : 1;
    }

    std::cout << formatDiff(result);
    return result.empty() ? 0 : 1;
}

int cmdRebase(const std::vector<std::string>& args) {
    std::string input;
    std::string output;
    std::optional<std::uint64_t> oldBase;
    std::optional<std::uint64_t> newBase;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--old-base") {
            if (i + 1 >= args.size() || !(oldBase = parseHexArg(args[++i]))) {
                std::cerr << "rebase: --old-base needs a hex value\n";
                return 2;
            }
        } else if (args[i] == "--new-base") {
            if (i + 1 >= args.size() || !(newBase = parseHexArg(args[++i]))) {
                std::cerr << "rebase: --new-base needs a hex value\n";
                return 2;
            }
        } else if (args[i] == "-o" || args[i] == "--out") {
            if (i + 1 >= args.size()) {
                std::cerr << "rebase: " << args[i] << " needs a file\n";
                return 2;
            }
            output = args[++i];
        } else if (input.empty()) {
            input = args[i];
        } else {
            std::cerr << "rebase: unexpected argument \"" << args[i] << "\"\n";
            return 2;
        }
    }
    if (input.empty() || !oldBase || !newBase) {
        std::cerr << "rebase: expected <in.iretable> --old-base <hex> --new-base <hex>\n";
        return 2;
    }

    auto loaded = readFile(input);
    if (!loaded.ok) {
        std::cerr << "rebase: " << loaded.error << "\n";
        return 2;
    }
    const auto result = rebase(loaded.table, *oldBase, *newBase);

    if (output.empty()) {
        std::cout << write(loaded.table);
    } else {
        if (!writeFile(output, loaded.table)) {
            std::cerr << "rebase: could not write " << output << "\n";
            return 2;
        }
    }
    std::cerr << "Rebased " << result.shifts.size() << " static address(es) by 0x"
              << toHexLower(*newBase - *oldBase) << ".";
    if (result.moduleRootedSkipped > 0) {
        std::cerr << " Left " << result.moduleRootedSkipped
                  << " module-rooted chain(s) untouched; they survive a relocation on their own.";
    }
    std::cerr << "\n";
    return 0;
}

int cmdLint(const std::vector<std::string>& args) {
    std::string path;
    bool asJson = false;
    for (const auto& arg : args) {
        if (arg == "--json") {
            asJson = true;
        } else if (path.empty()) {
            path = arg;
        } else {
            std::cerr << "lint: unexpected argument \"" << arg << "\"\n";
            return 2;
        }
    }
    if (path.empty()) {
        std::cerr << "lint: expected a file\n";
        return 2;
    }
    const auto loaded = readFile(path);
    const auto result = lint(loaded);

    if (asJson) {
        auto obj = json::Value::object();
        obj.set("readable", json::Value::boolean(result.readable));
        if (!result.readable) {
            obj.set("error", json::Value::str(result.error));
        }
        obj.set("problems", issuesJson(result.problems));
        obj.set("notes", issuesJson(result.notes));
        obj.set("clean", json::Value::boolean(result.clean()));
        std::cout << obj.dump() << "\n";
        return result.clean() ? 0 : (result.readable ? 1 : 2);
    }

    if (!result.readable) {
        std::cerr << "lint: " << result.error << "\n";
        return 2;
    }
    for (const auto& problem : result.problems) {
        if (problem.line > 0) {
            std::cerr << "line " << problem.line << ": ";
        }
        std::cerr << problem.category << ": " << escapeForDisplay(problem.message) << "\n";
    }
    for (const auto& note : result.notes) {
        std::cout << "note: " << escapeForDisplay(note.message) << "\n";
    }
    if (result.problems.empty()) {
        std::cout << "Clean: no problems found.\n";
        return 0;
    }
    std::cerr << result.problems.size() << " problem(s) found.\n";
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + (argc > 0 ? 1 : 0), argv + argc);
    if (args.empty()) {
        return usage(std::cerr);
    }
    const std::string command = args.front();
    const std::vector<std::string> rest(args.begin() + 1, args.end());

    if (command == "--version" || command == "-v") {
        std::cout << kVersion << "\n";
        return 0;
    }
    if (command == "--help" || command == "-h" || command == "help") {
        usage(std::cout);
        return 0;
    }
    if (command == "show") return cmdShow(rest);
    if (command == "convert") return cmdConvert(rest);
    if (command == "export") return cmdExport(rest);
    if (command == "diff") return cmdDiff(rest);
    if (command == "rebase") return cmdRebase(rest);
    if (command == "lint") return cmdLint(rest);

    std::cerr << "Unknown command \"" << command << "\".\n\n";
    return usage(std::cerr);
}
