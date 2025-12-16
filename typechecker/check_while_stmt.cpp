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
        error(state, "while condition must be bool, got '" + cond_type.base_type + "'", while_stmt.line);
    }

    for (const auto& s : while_stmt.body) {
        check_statement(state, *s);
    }
}

} // namespace typechecker
