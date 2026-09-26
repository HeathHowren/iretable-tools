#include "iretable/Writer.h"

#include "iretable/Text.h"
#include "iretable/ValueType.h"

#include <fstream>
#include <sstream>

namespace iretable {

namespace {

bool tableNeedsVersion3(const Table& table) {
    for (const auto& entry : table.entries) {
        if (entry.chain) {
            return true;
        }
    }
    return false;
}

} // namespace

std::string write(const Table& table) {
    const int version = (table.version < 3 && tableNeedsVersion3(table)) ? 3 : table.version;

    std::ostringstream out;
    out << "IRETABLE " << version << "\n";
    out << "pid|" << table.lastPid << "\n";
    out << "process|" << escape(table.lastProcessName) << "\n";
    out << "bitness|" << (table.lastBitness == Bitness::X86 ? "x86" : "x64") << "\n";

    for (const auto& symbol : table.symbols) {
        out << "symbol|" << escape(symbol.name) << '|' << escape(symbol.expression) << "\n";
    }
    for (const auto& script : table.scripts) {
        out << "script|" << escape(script.name) << '|' << escape(script.source) << "\n";
    }
    for (const auto& structure : table.structures) {
        out << "struct|" << escape(structure.name) << "\n";
        for (const auto& field : structure.fields) {
            out << "field|" << toHexLower(static_cast<std::uint64_t>(field.offset)) << '|'
                << valueTypeName(field.type) << '|' << field.length << '|' << escape(field.name) << "\n";
        }
    }
    for (const auto& entry : table.entries) {
        out << "entry|" << entry.id << '|' << toHexLower(entry.address) << '|' << valueTypeName(entry.type) << '|'
            << (entry.frozen ? 1 : 0) << '|' << escape(entry.description) << '|' << escape(entry.group) << '|'
            << escape(entry.hotkey) << '|' << bytesToHex(entry.frozenValue, false);
        if (version >= 3) {
            out << '|' << (entry.chain ? escape(entry.chain->moduleName) : "") << '|'
                << toHexLower(entry.chain ? entry.chain->moduleOffset : 0) << '|'
                << (entry.chain ? formatOffsets(entry.chain->offsets) : "");
        }
        out << "\n";
    }
    for (const auto& record : table.unknownRecords) {
        out << record << "\n";
    }
    return out.str();
}

bool writeFile(const std::filesystem::path& path, const Table& table) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file << write(table);
    file.flush();
    return static_cast<bool>(file);
}

} // namespace iretable
