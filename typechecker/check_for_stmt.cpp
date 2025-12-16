/**
 * @file check_for_stmt.cpp
 * @brief For statement checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a for statement.
 */
void check_for_stmt(TypeCheckerState& state, const ForStmt& for_stmt) {
    if (for_stmt.kind == ForLoopKind::Range) {
        TypeInfo start_type = infer_type(state, *for_stmt.range_start);
        TypeInfo end_type = infer_type(state, *for_stmt.range_end);

        if (start_type.base_type != "int") {
            error(state, "for range start must be int, got '" + start_type.base_type + "'", for_stmt.line);
        }

        if (end_type.base_type != "int") {
            error(state, "for range end must be int, got '" + end_type.base_type + "'", for_stmt.line);
        }

        state.locals[for_stmt.loop_var] = {"int", false, false};
    } else {
        TypeInfo iter_type = infer_type(state, *for_stmt.iterable);

        if (iter_type.base_type.rfind("List<", 0) != 0) {
            error(state, "for-each requires a List, got '" + iter_type.base_type + "'", for_stmt.line);
        } else {
            size_t start = 5;
            size_t end = iter_type.base_type.find('>', start);
            string element_type = iter_type.base_type.substr(start, end - start);

            state.locals[for_stmt.loop_var] = {element_type, false, false};
        }
    }

    for (const auto& s : for_stmt.body) {
        check_statement(state, *s);
    }
}

} // namespace typechecker
