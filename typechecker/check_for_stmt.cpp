/**
 * @file check_for_stmt.cpp
 * @brief For statement checking for the Bishop type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a for statement.
 */
void check_for_stmt(TypeCheckerState& state, const ForStmt& for_stmt) {
    // The loop variable is scoped to the `for` statement, matching the generated C++:
    // - Range form:   for (int i = start; i < end; i++) { ... }
    // - Foreach form: for (auto& x : collection) { ... }
    //
    // We model this as a dedicated scope that contains the loop variable, with a
    // nested scope for the loop body block.
    TypeInfo loop_var_type = {"unknown", false, false};

    if (for_stmt.kind == ForLoopKind::Range) {
        TypeInfo start_type = infer_type(state, *for_stmt.range_start);
        TypeInfo end_type = infer_type(state, *for_stmt.range_end);

        if (start_type.base_type != "int") {
            error(state, "for range start must be int, got '" + format_type(start_type) + "'", for_stmt.line);
        }

        if (end_type.base_type != "int") {
            error(state, "for range end must be int, got '" + format_type(end_type) + "'", for_stmt.line);
        }

        loop_var_type = {"int", false, false};
    } else {
        TypeInfo iter_type = infer_type(state, *for_stmt.iterable);

        if (iter_type.base_type.rfind("List<", 0) != 0) {
            error(state, "for-each requires a List, got '" + format_type(iter_type) + "'", for_stmt.line);
        } else {
            size_t start = 5;
            size_t end = iter_type.base_type.find('>', start);
            string element_type = iter_type.base_type.substr(start, end - start);

            loop_var_type = {element_type, false, false};
        }
    }

    push_scope(state);  // for-statement scope (holds the loop variable)
    declare_local(state, for_stmt.loop_var, loop_var_type, for_stmt.line);

    push_scope(state);  // body block scope
    for (const auto& s : for_stmt.body) {
        check_statement(state, *s);
    }
    pop_scope(state);
    pop_scope(state);
}

} // namespace typechecker
