/**
 * @file check_statement.cpp
 * @brief Statement type checking dispatch for the Bishop type checker.
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
    } else if (auto* fail_stmt = dynamic_cast<const FailStmt*>(&stmt)) {
        check_fail_stmt(state, *fail_stmt);
    } else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        check_if_stmt(state, *if_stmt);
    } else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        check_while_stmt(state, *while_stmt);
    } else if (auto* for_stmt = dynamic_cast<const ForStmt*>(&stmt)) {
        check_for_stmt(state, *for_stmt);
    } else if (auto* select_stmt = dynamic_cast<const SelectStmt*>(&stmt)) {
        check_select_stmt(state, *select_stmt);
    } else if (auto* go_spawn = dynamic_cast<const GoSpawn*>(&stmt)) {
        check_go_spawn(state, *go_spawn);
    } else if (auto* with_stmt = dynamic_cast<const WithStmt*>(&stmt)) {
        check_with_stmt(state, *with_stmt);
    } else if (auto* call = dynamic_cast<const FunctionCall*>(&stmt)) {
        infer_type(state, *call);
    } else if (auto* mcall = dynamic_cast<const MethodCall*>(&stmt)) {
        infer_type(state, *mcall);
    }
}

/**
 * Validates a fail statement. Ensures it's used in a fallible function.
 */
void check_fail_stmt(TypeCheckerState& state, const FailStmt& fail) {
    if (!state.current_function_is_fallible) {
        error(state, "fail can only be used in fallible functions (use -> T or err)", fail.line);
    }
}

/**
 * Type checks a go spawn statement.
 */
void check_go_spawn(TypeCheckerState& state, const GoSpawn& spawn) {
    // Just type check the function call
    infer_type(state, *spawn.call);
}

/**
 * Type checks a with statement for resource management.
 * Infers the resource type, binds it to the name, and checks the body.
 */
void check_with_stmt(TypeCheckerState& state, const WithStmt& with_stmt) {
    // Infer the type of the resource expression
    TypeInfo resource_type = infer_type(state, *with_stmt.resource);

    if (resource_type.base_type.empty()) {
        error(state, "cannot determine type of resource expression", with_stmt.line);
        return;
    }

    // Push a new scope for the with block body
    push_scope(state);

    // Add the binding to the current scope
    declare_local(state, with_stmt.binding_name, resource_type, with_stmt.line);

    // Type check all body statements
    for (const auto& stmt : with_stmt.body) {
        check_statement(state, *stmt);
    }

    // Pop the scope
    pop_scope(state);
}

} // namespace typechecker
