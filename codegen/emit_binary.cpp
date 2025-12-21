/**
 * @file emit_binary.cpp
 * @brief Binary expression emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a binary expression: left op right.
 */
string binary_expr(const string& left, const string& op, const string& right) {
    return fmt::format("{} {} {}", left, op, right);
}

/**
 * Emits an "is none" check using std::optional::has_value().
 */
string is_none(const string& value) {
    return fmt::format("!{}.has_value()", value);
}

/**
 * Emits a not expression: !value.
 */
string emit_not_expr(CodeGenState& state, const NotExpr& expr) {
    return "!" + emit(state, *expr.value);
}

/**
 * Emits an address-of expression: &expr.
 */
string emit_address_of(CodeGenState& state, const AddressOf& addr) {
    return "&" + emit(state, *addr.value);
}

} // namespace codegen
