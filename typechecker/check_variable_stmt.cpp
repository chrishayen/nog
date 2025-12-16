/**
 * @file check_variable_stmt.cpp
 * @brief Variable declaration and assignment checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a variable declaration statement.
 */
void check_variable_decl_stmt(TypeCheckerState& state, const VariableDecl& decl) {
    if (!decl.type.empty() && !is_valid_type(state, decl.type)) {
        error(state, "unknown type '" + decl.type + "'", decl.line);
    }

    if (decl.value) {
        TypeInfo init_type = infer_type(state, *decl.value);

        if (!decl.type.empty()) {
            TypeInfo expected = {decl.type, decl.is_optional, false};

            if (!types_compatible(expected, init_type)) {
                error(state, "cannot assign '" + init_type.base_type + "' to variable of type '" + decl.type + "'", decl.line);
            }
        }

        if (!decl.type.empty()) {
            state.locals[decl.name] = {decl.type, decl.is_optional, false};
        } else {
            state.locals[decl.name] = init_type;
        }
    }
}

/**
 * Type checks an assignment statement.
 */
void check_assignment_stmt(TypeCheckerState& state, const Assignment& assign) {
    if (state.locals.find(assign.name) == state.locals.end()) {
        error(state, "assignment to undefined variable '" + assign.name + "'", assign.line);
        return;
    }

    TypeInfo var_type = state.locals[assign.name];
    TypeInfo val_type = infer_type(state, *assign.value);

    if (!types_compatible(var_type, val_type)) {
        error(state, "cannot assign '" + val_type.base_type + "' to variable of type '" + var_type.base_type + "'", assign.line);
    }
}

/**
 * Type checks a field assignment statement.
 */
void check_field_assignment_stmt(TypeCheckerState& state, const FieldAssignment& fa) {
    TypeInfo obj_type = infer_type(state, *fa.object);

    if (state.structs.find(obj_type.base_type) == state.structs.end()) {
        error(state, "cannot access field on non-struct type '" + obj_type.base_type + "'", fa.line);
        return;
    }

    string field_type = get_field_type(state, obj_type.base_type, fa.field_name);

    if (field_type.empty()) {
        error(state, "struct '" + obj_type.base_type + "' has no field '" + fa.field_name + "'", fa.line);
        return;
    }

    TypeInfo expected = {field_type, false, false};
    TypeInfo val_type = infer_type(state, *fa.value);

    if (!types_compatible(expected, val_type)) {
        error(state, "cannot assign '" + val_type.base_type + "' to field of type '" + field_type + "'", fa.line);
    }
}

} // namespace typechecker
