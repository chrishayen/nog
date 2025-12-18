/**
 * @file check_struct_literal.cpp
 * @brief Struct literal type inference for the Nog type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a struct literal expression.
 */
TypeInfo check_struct_literal(TypeCheckerState& state, const StructLiteral& lit) {
    const StructDef* sdef = get_struct(state, lit.struct_name);

    if (!sdef) {
        error(state, "unknown struct '" + lit.struct_name + "'", lit.line);
        return {"unknown", false, false};
    }

    for (const auto& [field_name, field_val] : lit.field_values) {
        string expected_type;

        for (const auto& f : sdef->fields) {
            if (f.name == field_name) {
                expected_type = f.type;
                break;
            }
        }

        if (expected_type.empty()) {
            error(state, "struct '" + lit.struct_name + "' has no field '" + field_name + "'", lit.line);
            continue;
        }

        TypeInfo val_type = infer_type(state, *field_val);
        TypeInfo exp_type = {expected_type, false, false};

        if (!types_compatible(exp_type, val_type)) {
            error(state, "field '" + field_name + "' expects '" + expected_type + "', got '" + format_type(val_type) + "'", lit.line);
        }
    }

    return {lit.struct_name, false, false};
}

} // namespace typechecker
