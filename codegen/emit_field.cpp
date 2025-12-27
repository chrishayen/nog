/**
 * @file emit_field.cpp
 * @brief Field access and assignment emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a field access AST node with self handling.
 * Auto-dereferences pointer types (like Go).
 */
string emit_field_access(CodeGenState& state, const FieldAccess& access) {
    // Handle self.field -> this->field in methods
    if (auto* ref = dynamic_cast<const VariableRef*>(access.object.get())) {
        if (ref->name == "self") {
            return "this->" + access.field_name;
        }
    }

    string obj_str = emit(state, *access.object);

    // Use -> for pointer types (auto-deref like Go)
    if (!access.object_type.empty() && access.object_type.back() == '*') {
        return obj_str + "->" + access.field_name;
    }

    return field_access(obj_str, access.field_name);
}

/**
 * Emits a field assignment AST node with self handling.
 * Auto-dereferences pointer types (like Go).
 */
string emit_field_assignment(CodeGenState& state, const FieldAssignment& fa) {
    // Handle self.field = value -> this->field = value in methods
    if (auto* ref = dynamic_cast<const VariableRef*>(fa.object.get())) {
        if (ref->name == "self") {
            return "this->" + fa.field_name + " = " + emit(state, *fa.value);
        }
    }

    string obj_str = emit(state, *fa.object);
    string val_str = emit(state, *fa.value);

    // Use -> for pointer types (auto-deref like Go)
    if (!fa.object_type.empty() && fa.object_type.back() == '*') {
        return obj_str + "->" + fa.field_name + " = " + val_str;
    }

    return field_assignment(obj_str, fa.field_name, val_str);
}

} // namespace codegen
