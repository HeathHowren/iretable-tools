#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The in-memory model of a Pointer Lab .iretable project file.
//
// The file format is documented in Pointer-Lab/docs/iretable-format.md and
// implemented in Pointer-Lab/src/storage/ProjectStore.cpp. This model matches
// that reference: the same records, the same field order, the same escaping.
// Text is carried as UTF-8 throughout, which is what the file is; the escaping
// works a byte at a time and never splits a multi-byte character, because none
// of the escaped bytes (\ | \n \r) can appear inside a UTF-8 sequence.
namespace iretable {

// The value types an address-list row can hold. Names on read are matched
// case-insensitively; Pointer Lab always writes lower case.
enum class ValueType {
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    Bytes,
    StringAscii,
    StringUtf16
};

// Pointer width of the process a table was built against. Absent in files
// written before the record existed, in which case x64 is assumed.
enum class Bitness {
    X86,
    X64
};

// A path from a static base to a value: find the module, add the module offset
// to get the chain root, read a pointer there, add the first offset, read a
// pointer, add the next, and so on. An empty moduleName means moduleOffset is
// an absolute base instead, which only manual entry produces and which does
// not survive a restart. Offsets may be empty: that is a static address written
// as module+offset, re-resolved every run with no dereferencing.
struct PointerChain {
    std::string moduleName;
    std::uint64_t moduleOffset{};
    std::vector<std::int64_t> offsets;

    [[nodiscard]] bool moduleRooted() const { return !moduleName.empty(); }
};

// One address-list row.
struct Entry {
    std::uint64_t id{};
    std::uint64_t address{};
    ValueType type{ValueType::Int32};
    bool frozen{};
    std::string description;
    std::string group;
    std::string hotkey;
    std::vector<std::uint8_t> frozenValue;
    std::optional<PointerChain> chain;
};

// A user-defined name, stored as the expression that produced it.
struct Symbol {
    std::string name;
    std::string expression;
};

// One auto-assembler script: a name and its source. The source is one field
// however long, because escaping turns each newline into \n.
struct Script {
    std::string name;
    std::string source;
};

// One member of a structure.
struct StructField {
    std::int64_t offset{};
    ValueType type{ValueType::Int32};
    std::uint64_t length{};
    std::string name;
};

// A structure definition: a header naming it and one field per member.
struct Structure {
    std::string name;
    std::vector<StructField> fields;
};

// A whole project file.
struct Table {
    // The header version, 1, 2 or 3. A fresh table is version 3; a loaded one
    // keeps the version it was read as, so a v1 or v2 file round-trips as
    // itself. Writing bumps to 3 automatically when an entry needs a pointer
    // chain, which versions 1 and 2 cannot express.
    int version{3};
    std::uint32_t lastPid{};
    std::string lastProcessName;
    Bitness lastBitness{Bitness::X64};
    std::vector<Symbol> symbols;
    std::vector<Script> scripts;
    std::vector<Structure> structures;
    std::vector<Entry> entries;
    // Records whose type this tool does not recognize, kept verbatim so a file
    // written by a newer Pointer Lab round-trips without losing them. Pointer
    // Lab itself only counts and skips these; iretable-tools preserves them.
    std::vector<std::string> unknownRecords;
};

} // namespace iretable
