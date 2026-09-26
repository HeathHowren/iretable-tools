#pragma once

#include "iretable/Model.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace iretable {

// The short name written to the file, e.g. "i32". Lower case, as Pointer Lab
// always writes.
const char* valueTypeName(ValueType type);

// Width in bytes, or 0 for the variable-length types (Bytes and both strings),
// whose width comes from the value rather than the type.
std::size_t valueTypeSize(ValueType type);

bool isStringType(ValueType type);

// Parse a type name, case-insensitively. nullopt for an unknown name.
std::optional<ValueType> parseValueType(const std::string& text);

// Every type, in the order Pointer Lab lists them.
const std::vector<ValueType>& valueTypes();

} // namespace iretable
