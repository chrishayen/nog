/**
 * @file check_while_stmt.cpp
 * @brief While statement checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a while statement.
 */
void check_while_stmt(TypeCheckerState& state, const WhileStmt& while_stmt) {
    TypeInfo cond_type = infer_type(state, *while_stmt.condition);

    if (cond_type.base_type != "bool") {
        error(state, "while condition must be bool, got '" + format_type(cond_type) + "'", while_stmt.line);
    }

    // The loop body is a lexical scope; declarations inside must not be visible
    // after the while statement.
    push_scope(state);
    for (const auto& s : while_stmt.body) {
        check_statement(state, *s);
    }
    pop_scope(state);
}

} // namespace typechecker
