/**
 * @file emit_field.cpp
 * @brief Field access and assignment emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a field access AST node with self handling.
 */
string emit_field_access(CodeGenState& state, const FieldAccess& access) {
    // Handle self.field -> this->field in methods
    if (auto* ref = dynamic_cast<const VariableRef*>(access.object.get())) {
        if (ref->name == "self") {
            return "this->" + access.field_name;
        }
    }

    return field_access(emit(state, *access.object), access.field_name);
}

/**
 * Emits a field assignment AST node with self handling.
 */
string emit_field_assignment(CodeGenState& state, const FieldAssignment& fa) {
    // Handle self.field = value -> this->field = value in methods
    if (auto* ref = dynamic_cast<const VariableRef*>(fa.object.get())) {
        if (ref->name == "self") {
            return "this->" + fa.field_name + " = " + emit(state, *fa.value);
        }
    }

    return field_assignment(emit(state, *fa.object), fa.field_name, emit(state, *fa.value));
}

} // namespace codegen
