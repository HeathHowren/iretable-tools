#include "iretable/Reader.h"

#include "iretable/Text.h"
#include "iretable/ValueType.h"

#include <fstream>
#include <sstream>

namespace iretable {

namespace {

void stripLineEnding(std::string& line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
}

bool isKnownRecordType(const std::string& type) {
    return type == "pid" || type == "process" || type == "bitness" || type == "symbol" ||
           type == "script" || type == "struct" || type == "field" || type == "entry";
}

// The width a field ends up with: from the type, or from the length for the
// variable-width types. Zero means the field has no width and is skipped.
std::uint64_t fieldWidth(ValueType type, std::uint64_t length) {
    const auto fixed = valueTypeSize(type);
    return fixed != 0 ? fixed : length;
}

} // namespace

std::size_t LoadResult::warningCount() const {
    std::size_t count = 0;
    for (const auto& issue : issues) {
        if (issue.severity == Severity::Warning) {
            ++count;
        }
    }
    return count;
}

LoadResult read(const std::string& text) {
    LoadResult result;
    std::istringstream in(text);

    std::string line;
    std::getline(in, line);
    // A UTF-8 BOM before the header is tolerated: text editors and PowerShell
    // add one without asking, and it would otherwise make the header compare
    // fail and the file look corrupt.
    if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }
    stripLineEnding(line);

    if (line == "IRETABLE 1") {
        result.table.version = 1;
    } else if (line == "IRETABLE 2") {
        result.table.version = 2;
    } else if (line == "IRETABLE 3") {
        result.table.version = 3;
    } else {
        result.ok = false;
        result.error = "This is not a Pointer Lab project file, or it was written by a newer version.";
        result.issues.push_back({0, Severity::Error, "bad-header", result.error});
        return result;
    }

    Table& table = result.table;
    std::size_t lineNumber = 1;

    const auto warn = [&result](std::size_t ln, const char* category, std::string message) {
        result.issues.push_back({ln, Severity::Warning, category, std::move(message)});
    };

    while (std::getline(in, line)) {
        ++lineNumber;
        stripLineEnding(line);
        if (line.empty()) {
            continue;
        }
        auto parts = splitEscaped(line);
        if (parts.empty()) {
            continue;
        }

        if (parts[0] == "pid" && parts.size() >= 2) {
            if (auto pid = parseUnsigned(parts[1], 10)) {
                table.lastPid = static_cast<std::uint32_t>(*pid);
            } else {
                warn(lineNumber, "bad-pid", "Unreadable process id; ignoring it.");
            }
        } else if (parts[0] == "process" && parts.size() >= 2) {
            table.lastProcessName = parts[1];
        } else if (parts[0] == "bitness" && parts.size() >= 2) {
            if (parts[1] == "x86") {
                table.lastBitness = Bitness::X86;
            } else if (parts[1] == "x64") {
                table.lastBitness = Bitness::X64;
            } else {
                warn(lineNumber, "bad-bitness", "Unrecognised target bitness \"" + parts[1] + "\"; assuming 64-bit.");
            }
        } else if (parts[0] == "symbol" && parts.size() >= 3) {
            if (parts[1].empty() || parts[2].empty()) {
                warn(lineNumber, "incomplete-symbol", "Incomplete symbol; ignoring it.");
            } else {
                table.symbols.push_back({parts[1], parts[2]});
            }
        } else if (parts[0] == "script" && parts.size() >= 3) {
            if (parts[1].empty() && parts[2].empty()) {
                warn(lineNumber, "empty-script", "Empty script; ignoring it.");
            } else {
                table.scripts.push_back({parts[1], parts[2]});
            }
        } else if (parts[0] == "struct" && parts.size() >= 2) {
            Structure structure;
            structure.name =
                parts[1].empty() ? "Structure " + std::to_string(table.structures.size() + 1) : parts[1];
            table.structures.push_back(std::move(structure));
        } else if (parts[0] == "field" && parts.size() >= 5) {
            if (table.structures.empty()) {
                warn(lineNumber, "orphan-field", "Field comes before any struct record; ignoring it.");
                continue;
            }
            const auto offset = parseUnsigned(parts[1], 16);
            const auto type = parseValueType(parts[2]);
            const auto length = parseUnsigned(parts[3], 10);
            if (!offset || !type || !length) {
                warn(lineNumber, "bad-field", "Unreadable field; skipping it.");
                continue;
            }
            StructField field;
            field.offset = static_cast<std::int64_t>(*offset);
            field.type = *type;
            field.length = *length;
            field.name = parts[4];
            if (fieldWidth(field.type, field.length) == 0) {
                warn(lineNumber, "zero-width-field", "Field has no width; skipping it.");
                continue;
            }
            table.structures.back().fields.push_back(std::move(field));
        } else if (parts[0] == "entry" && parts.size() >= 9) {
            const auto id = parseUnsigned(parts[1], 10);
            const auto address = parseUnsigned(parts[2], 16);
            if (!id || !address) {
                warn(lineNumber, "bad-hex", "Unreadable id or address; skipping that entry.");
                continue;
            }
            Entry entry;
            entry.id = *id;
            entry.address = *address;
            if (auto type = parseValueType(parts[3])) {
                entry.type = *type;
            } else {
                warn(lineNumber, "unknown-type", "Unknown value type \"" + parts[3] + "\"; skipping that entry.");
                continue;
            }
            entry.frozen = parts[4] == "1";
            entry.description = parts[5];
            entry.group = parts[6];
            entry.hotkey = parts[7];
            entry.frozenValue = parseHexBytes(parts[8]);

            // Pointer chain, present from version 3. A chain exists when the
            // module name is non-empty or the base is non-zero. A malformed
            // chain costs the chain, not the row: the entry is kept as a fixed
            // address and a warning logged.
            if (parts.size() >= 12 && (!parts[9].empty() || parseUnsigned(parts[10], 16).value_or(0) != 0)) {
                const auto moduleOffset = parseUnsigned(parts[10], 16);
                const auto offsets = parseOffsets(parts[11]);
                if (!moduleOffset || !offsets) {
                    warn(lineNumber, "malformed-chain",
                         "Malformed pointer chain; keeping the entry as a fixed address.");
                } else {
                    PointerChain chain;
                    chain.moduleName = parts[9];
                    chain.moduleOffset = *moduleOffset;
                    chain.offsets = *offsets;
                    entry.chain = std::move(chain);
                }
            }

            table.entries.push_back(std::move(entry));
        } else if (!parts.empty() && isKnownRecordType(parts[0])) {
            // A known record type with too few fields. Pointer Lab skips these;
            // so do we, but they are reported so lint can flag them.
            warn(lineNumber, "incomplete-record", "Incomplete \"" + parts[0] + "\" record; ignoring it.");
        } else {
            // An unrecognised record type. Pointer Lab counts and drops these;
            // iretable-tools keeps the raw line so it survives a round-trip.
            table.unknownRecords.push_back(line);
        }
    }

    result.ok = true;
    return result;
}

LoadResult readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        LoadResult result;
        result.ok = false;
        result.error = "Could not open project file.";
        result.issues.push_back({0, Severity::Error, "unreadable", result.error});
        return result;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return read(buffer.str());
}

} // namespace iretable
