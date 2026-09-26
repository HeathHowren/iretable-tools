#pragma once

#include "iretable/Model.h"

#include <string>

namespace iretable {

// The text form of `iretable show`: a header, then one row per entry with its
// location on the line below, then a count of preserved records. Text from the
// file goes through escapeForDisplay, so a newline in a description or name
// cannot break a row.
std::string formatTable(const Table& table);

} // namespace iretable
