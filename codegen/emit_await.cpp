/**
 * @file emit_await.cpp
 * @brief Await expression emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits an await expression.
 */
string emit_await(CodeGenState& state, const AwaitExpr& expr) {
    // Wrap in parentheses since co_await has low precedence in C++
    return "(co_await " + emit(state, *expr.value) + ")";
}

} // namespace codegen
