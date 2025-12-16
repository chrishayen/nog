/**
 * @file check_statement.cpp
 * @brief Statement type checking dispatch for the Nog type checker.
 *
 * Dispatches to specialized check functions based on statement type.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Validates a single statement. Dispatches to specialized check functions.
 */
void check_statement(TypeCheckerState& state, const ASTNode& stmt) {
    if (auto* decl = dynamic_cast<const VariableDecl*>(&stmt)) {
        check_variable_decl_stmt(state, *decl);
    } else if (auto* assign = dynamic_cast<const Assignment*>(&stmt)) {
        check_assignment_stmt(state, *assign);
    } else if (auto* fa = dynamic_cast<const FieldAssignment*>(&stmt)) {
        check_field_assignment_stmt(state, *fa);
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt)) {
        check_return_stmt(state, *ret);
    } else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        check_if_stmt(state, *if_stmt);
    } else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        check_while_stmt(state, *while_stmt);
    } else if (auto* for_stmt = dynamic_cast<const ForStmt*>(&stmt)) {
        check_for_stmt(state, *for_stmt);
    } else if (auto* select_stmt = dynamic_cast<const SelectStmt*>(&stmt)) {
        check_select_stmt(state, *select_stmt);
    } else if (auto* call = dynamic_cast<const FunctionCall*>(&stmt)) {
        infer_type(state, *call);
    } else if (auto* mcall = dynamic_cast<const MethodCall*>(&stmt)) {
        infer_type(state, *mcall);
    } else if (auto* await_expr = dynamic_cast<const AwaitExpr*>(&stmt)) {
        infer_type(state, *await_expr);
    }
}

} // namespace typechecker
