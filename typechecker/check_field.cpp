/**
 * @file check_field.cpp
 * @brief Field access type inference for the Bishop type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a field access expression.
 * Auto-dereferences pointer types (like Go).
 */
TypeInfo check_field_access(TypeCheckerState& state, const FieldAccess& access) {
    TypeInfo obj_type = infer_type(state, *access.object);
    access.object_type = obj_type.base_type;  // Store for codegen (includes pointer suffix)

    // Auto-dereference pointers for field access (like Go)
    string struct_type = obj_type.base_type;

    if (!struct_type.empty() && struct_type.back() == '*') {
        struct_type = struct_type.substr(0, struct_type.length() - 1);
    }

    const StructDef* sdef = get_struct(state, struct_type);

    if (!sdef) {
        error(state, "cannot access field on non-struct type '" + format_type(obj_type) + "'", access.line);
        return {"unknown", false, false};
    }

    for (const auto& field : sdef->fields) {
        if (field.name == access.field_name) {
            return {field.type, false, false};
        }
    }

    error(state, "struct '" + struct_type + "' has no field '" + access.field_name + "'", access.line);
    return {"unknown", false, false};
}

} // namespace typechecker
