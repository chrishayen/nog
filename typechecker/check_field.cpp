/**
 * @file check_field.cpp
 * @brief Field access type inference for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a field access expression.
 */
TypeInfo check_field_access(TypeCheckerState& state, const FieldAccess& access) {
    TypeInfo obj_type = infer_type(state, *access.object);

    const StructDef* sdef = get_struct(state, obj_type.base_type);

    if (!sdef) {
        error(state, "cannot access field on non-struct type '" + obj_type.base_type + "'", access.line);
        return {"unknown", false, false};
    }

    for (const auto& field : sdef->fields) {
        if (field.name == access.field_name) {
            return {field.type, false, false};
        }
    }

    error(state, "struct '" + obj_type.base_type + "' has no field '" + access.field_name + "'", access.line);
    return {"unknown", false, false};
}

} // namespace typechecker
