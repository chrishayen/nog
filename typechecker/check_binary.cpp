/**
 * @file check_binary.cpp
 * @brief Binary expression type inference for the Bishop type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a binary expression.
 */
TypeInfo check_binary_expr(TypeCheckerState& state, const BinaryExpr& bin) {
    TypeInfo left_type = infer_type(state, *bin.left);
    TypeInfo right_type = infer_type(state, *bin.right);

    if (bin.op == "==" || bin.op == "!=" || bin.op == "<" ||
        bin.op == ">" || bin.op == "<=" || bin.op == ">=") {
        return {"bool", false, false};
    }

    if (bin.op == "+" && left_type.base_type == "str" && right_type.base_type == "str") {
        return {"str", false, false};
    }

    if (left_type.base_type != right_type.base_type) {
        error(state, "type mismatch in binary expression: '" + format_type(left_type) + "' " + bin.op +
              " '" + format_type(right_type) + "'", bin.line);
    }

    return left_type;
}

/**
 * Infers the type of an is_none expression.
 */
TypeInfo check_is_none(TypeCheckerState& state, const IsNone& expr) {
    infer_type(state, *expr.value);
    return {"bool", false, false};
}

/**
 * Infers the type of a not expression.
 */
TypeInfo check_not_expr(TypeCheckerState& state, const NotExpr& not_expr) {
    TypeInfo inner_type = infer_type(state, *not_expr.value);

    if (inner_type.base_type != "bool") {
        error(state, "'!' operator requires bool, got '" + format_type(inner_type) + "'", not_expr.line);
    }

    return {"bool", false, false};
}

/**
 * Infers the type of an address-of expression: &expr
 * Only allowed for struct types (not primitives).
 * Returns a pointer type: StructName -> StructName*
 */
TypeInfo check_address_of(TypeCheckerState& state, const AddressOf& addr) {
    if (!addr.value) {
        return {"unknown", false, false};
    }

    TypeInfo inner_type = infer_type(state, *addr.value);

    // Can only take address of lvalues (variables, field access)
    if (!dynamic_cast<const VariableRef*>(addr.value.get()) &&
        !dynamic_cast<const FieldAccess*>(addr.value.get())) {
        error(state, "cannot take address of this expression", addr.line);
        return {"unknown", false, false};
    }

    // Only allow pointers to struct types, not primitives
    if (is_primitive_type(inner_type.base_type)) {
        error(state, "cannot take address of primitive type '" + format_type(inner_type) +
              "'; pointers are only allowed for struct types", addr.line);
        return {"unknown", false, false};
    }

    return {inner_type.base_type + "*", false, false};
}

} // namespace typechecker
