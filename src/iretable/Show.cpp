#include "iretable/Show.h"

#include "iretable/Diff.h"
#include "iretable/Text.h"
#include "iretable/ValueType.h"

#include <sstream>

namespace iretable {

std::string formatTable(const Table& table) {
    std::ostringstream out;
    out << "IRETABLE version " << table.version << "\n";
    out << "process " << (table.lastProcessName.empty() ? "(none)" : escapeForDisplay(table.lastProcessName))
        << ", pid " << table.lastPid << ", " << (table.lastBitness == Bitness::X86 ? "x86" : "x64") << "\n";
    if (!table.symbols.empty()) {
        out << table.symbols.size() << " symbol(s), ";
    }
    if (!table.scripts.empty()) {
        out << table.scripts.size() << " script(s), ";
    }
    if (!table.structures.empty()) {
        out << table.structures.size() << " structure(s), ";
    }
    out << table.entries.size() << " entr" << (table.entries.size() == 1 ? "y" : "ies") << "\n\n";
    for (const auto& entry : table.entries) {
        out << "  #" << entry.id << "  " << valueTypeName(entry.type) << "  "
            << (entry.description.empty() ? "(no description)" : escapeForDisplay(entry.description));
        if (!entry.group.empty()) {
            out << "  [" << escapeForDisplay(entry.group) << "]";
        }
        if (entry.frozen) {
            out << "  frozen";
        }
        out << "\n      " << escapeForDisplay(locationString(entry)) << "\n";
    }
    if (!table.unknownRecords.empty()) {
        out << "\n" << table.unknownRecords.size() << " preserved unrecognized record(s)\n";
    }
    return out.str();
}

} // namespace iretable
