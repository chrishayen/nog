/**
 * @file emit_assert.cpp
 * @brief Assert statement emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a call to the _assert_eq test helper with line number for errors.
 */
string assert_eq(const string& a, const string& b, int line) {
    return fmt::format("_assert_eq({}, {}, {})", a, b, line);
}

} // namespace codegen
