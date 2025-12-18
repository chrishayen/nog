/**
 * @file check_return_stmt.cpp
 * @brief Return statement checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a return statement.
 */
void check_return_stmt(TypeCheckerState& state, const ReturnStmt& ret) {
    TypeInfo ret_type = {"void", false, true};

    if (ret.value) {
        ret_type = infer_type(state, *ret.value);
    }

    if (!types_compatible(state.current_return, ret_type)) {
        error(state, "return type '" + format_type(ret_type) + "' does not match declared type '" + format_type(state.current_return) + "'", ret.line);
    }
}

} // namespace typechecker
