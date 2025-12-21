/**
 * @file check_variable_stmt.cpp
 * @brief Variable declaration and assignment checking for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Type checks a variable declaration statement.
 * Handles struct pointer syntax: Person p = &bob creates a Person* variable.
 */
void check_variable_decl_stmt(TypeCheckerState& state, const VariableDecl& decl) {
    if (!decl.type.empty() && !is_valid_type(state, decl.type)) {
        error(state, "unknown type '" + decl.type + "'", decl.line);
    }

    if (decl.value) {
        TypeInfo init_type = infer_type(state, *decl.value);

        if (!decl.type.empty()) {
            // Allow "Person p = &bob" syntax: declared as Person, assigned Person*
            // The variable becomes a pointer type
            bool is_pointer_assignment = !init_type.base_type.empty() &&
                                         init_type.base_type.back() == '*' &&
                                         init_type.base_type == decl.type + "*";

            if (is_pointer_assignment) {
                // Variable is a pointer - update type for codegen and use the pointer type
                decl.type = init_type.base_type;
                declare_local(state, decl.name, init_type, decl.line);
                return;
            }

            TypeInfo expected = {decl.type, decl.is_optional, false};

            if (!types_compatible(expected, init_type)) {
                error(state, "cannot assign '" + format_type(init_type) + "' to variable of type '" + format_type(expected) + "'", decl.line);
            }

            declare_local(state, decl.name, {decl.type, decl.is_optional, false}, decl.line);
        } else {
            declare_local(state, decl.name, init_type, decl.line);
        }
    }
}

/**
 * Type checks an assignment statement.
 */
void check_assignment_stmt(TypeCheckerState& state, const Assignment& assign) {
    const TypeInfo* var = lookup_local(state, assign.name);

    if (!var) {
        error(state, "assignment to undefined variable '" + assign.name + "'", assign.line);
        return;
    }

    TypeInfo var_type = *var;
    TypeInfo val_type = infer_type(state, *assign.value);

    if (!types_compatible(var_type, val_type)) {
        error(state, "cannot assign '" + format_type(val_type) + "' to variable of type '" + format_type(var_type) + "'", assign.line);
    }
}

/**
 * Type checks a field assignment statement.
 * Auto-dereferences pointer types (like Go).
 */
void check_field_assignment_stmt(TypeCheckerState& state, const FieldAssignment& fa) {
    TypeInfo obj_type = infer_type(state, *fa.object);
    fa.object_type = obj_type.base_type;  // Store for codegen (includes pointer suffix)

    // Auto-dereference pointers for field assignment (like Go)
    string struct_type = obj_type.base_type;

    if (!struct_type.empty() && struct_type.back() == '*') {
        struct_type = struct_type.substr(0, struct_type.length() - 1);
    }

    if (state.structs.find(struct_type) == state.structs.end()) {
        error(state, "cannot access field on non-struct type '" + format_type(obj_type) + "'", fa.line);
        return;
    }

    string field_type = get_field_type(state, struct_type, fa.field_name);

    if (field_type.empty()) {
        error(state, "struct '" + struct_type + "' has no field '" + fa.field_name + "'", fa.line);
        return;
    }

    TypeInfo expected = {field_type, false, false};
    TypeInfo val_type = infer_type(state, *fa.value);

    if (!types_compatible(expected, val_type)) {
        error(state, "cannot assign '" + format_type(val_type) + "' to field of type '" + format_type(expected) + "'", fa.line);
    }
}

} // namespace typechecker
