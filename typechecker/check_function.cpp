/**
 * @file check_function.cpp
 * @brief Function and method type checking for the Nog type checker.
 *
 * Handles validating function and method definitions, including
 * parameter types, return types, and body statements.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Checks if a list of statements contains a return or fail statement.
 * Recursively checks if/else branches for guaranteed returns.
 */
bool has_return(const vector<unique_ptr<ASTNode>>& stmts) {
    for (const auto& stmt : stmts) {
        if (dynamic_cast<const ReturnStmt*>(stmt.get())) {
            return true;
        }

        // fail is a valid return path for fallible functions
        if (dynamic_cast<const FailStmt*>(stmt.get())) {
            return true;
        }

        if (auto* if_stmt = dynamic_cast<const IfStmt*>(stmt.get())) {
            if (!if_stmt->else_body.empty() &&
                has_return(if_stmt->then_body) &&
                has_return(if_stmt->else_body)) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Validates a method definition. Ensures 'self' is the first parameter,
 * validates parameter types, and checks the method body.
 */
void check_method(TypeCheckerState& state, const MethodDef& method) {
    state.local_scopes.clear();
    push_scope(state);  // method scope (parameters + body)
    state.current_struct = method.struct_name;
    state.current_function_is_fallible = !method.error_type.empty();

    if (method.return_type.empty()) {
        state.current_return = {"void", false, true};
    } else {
        state.current_return = {method.return_type, false, false};
    }

    if (method.params.empty() || method.params[0].name != "self") {
        error(state, "method '" + method.name + "' must have 'self' as first parameter", method.line);
        return;
    }

    for (const auto& param : method.params) {
        if (!is_valid_type(state, param.type)) {
            error(state, "unknown type '" + param.type + "' for parameter '" + param.name + "'", method.line);
        }

        declare_local(state, param.name, {param.type, false, false}, method.line);
    }

    for (const auto& stmt : method.body) {
        check_statement(state, *stmt);
    }

    if (!method.return_type.empty() && !has_return(method.body)) {
        error(state, "method '" + method.name + "' must return a value of type '" + method.return_type + "'", method.line);
    }

    state.current_struct.clear();
}

/**
 * Validates a function definition. Checks parameter types and validates the body.
 */
void check_function(TypeCheckerState& state, const FunctionDef& func) {
    state.local_scopes.clear();
    push_scope(state);  // function scope (parameters + body)
    state.current_struct.clear();
    state.current_function_is_fallible = !func.error_type.empty();

    if (func.return_type.empty()) {
        state.current_return = {"void", false, true};
    } else {
        state.current_return = {func.return_type, false, false};
    }

    for (const auto& param : func.params) {
        if (!is_valid_type(state, param.type)) {
            error(state, "unknown type '" + param.type + "' for parameter '" + param.name + "'", func.line);
        }

        declare_local(state, param.name, {param.type, false, false}, func.line);
    }

    for (const auto& stmt : func.body) {
        check_statement(state, *stmt);
    }

    if (!func.return_type.empty() && !has_return(func.body)) {
        error(state, "function '" + func.name + "' must return a value of type '" + func.return_type + "'", func.line);
    }
}

} // namespace typechecker
