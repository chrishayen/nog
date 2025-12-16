/**
 * @file check_if_stmt.cpp
 * @brief If statement checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks an if statement.
 */
void check_if_stmt(TypeCheckerState& state, const IfStmt& if_stmt) {
    TypeInfo cond_type = infer_type(state, *if_stmt.condition);

    if (cond_type.base_type != "bool" && !cond_type.is_optional) {
        error(state, "if condition must be bool or optional type, got '" + cond_type.base_type + "'", if_stmt.line);
    }

    for (const auto& s : if_stmt.then_body) {
        check_statement(state, *s);
    }

    for (const auto& s : if_stmt.else_body) {
        check_statement(state, *s);
    }
}

} // namespace typechecker
