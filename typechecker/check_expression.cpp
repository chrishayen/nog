/**
 * @file check_expression.cpp
 * @brief Expression type inference dispatch for the Nog type checker.
 *
 * Dispatches to specialized check functions based on expression type.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of an expression. Dispatches to specialized check functions.
 */
TypeInfo infer_type(TypeCheckerState& state, const ASTNode& expr) {
    if (auto* num = dynamic_cast<const NumberLiteral*>(&expr)) {
        return check_number_literal(state, *num);
    }

    if (auto* flt = dynamic_cast<const FloatLiteral*>(&expr)) {
        return check_float_literal(state, *flt);
    }

    if (auto* str = dynamic_cast<const StringLiteral*>(&expr)) {
        return check_string_literal(state, *str);
    }

    if (auto* bl = dynamic_cast<const BoolLiteral*>(&expr)) {
        return check_bool_literal(state, *bl);
    }

    if (auto* none = dynamic_cast<const NoneLiteral*>(&expr)) {
        return check_none_literal(state, *none);
    }

    if (auto* ch = dynamic_cast<const CharLiteral*>(&expr)) {
        return check_char_literal(state, *ch);
    }

    if (auto* var = dynamic_cast<const VariableRef*>(&expr)) {
        return check_variable_ref(state, *var);
    }

    if (auto* fref = dynamic_cast<const FunctionRef*>(&expr)) {
        return check_function_ref(state, *fref);
    }

    if (auto* qref = dynamic_cast<const QualifiedRef*>(&expr)) {
        return check_qualified_ref(state, *qref);
    }

    if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        return check_binary_expr(state, *bin);
    }

    if (auto* is_none = dynamic_cast<const IsNone*>(&expr)) {
        return check_is_none(state, *is_none);
    }

    if (auto* not_expr = dynamic_cast<const NotExpr*>(&expr)) {
        return check_not_expr(state, *not_expr);
    }

    if (auto* await_expr = dynamic_cast<const AwaitExpr*>(&expr)) {
        return check_await_expr(state, *await_expr);
    }

    if (auto* paren = dynamic_cast<const ParenExpr*>(&expr)) {
        if (!paren->value) {
            return {"unknown", false, false};
        }

        return infer_type(state, *paren->value);
    }

    if (auto* channel = dynamic_cast<const ChannelCreate*>(&expr)) {
        return check_channel_create(state, *channel);
    }

    if (auto* list = dynamic_cast<const ListCreate*>(&expr)) {
        return check_list_create(state, *list);
    }

    if (auto* list = dynamic_cast<const ListLiteral*>(&expr)) {
        return check_list_literal(state, *list);
    }

    if (auto* call = dynamic_cast<const FunctionCall*>(&expr)) {
        return check_function_call(state, *call);
    }

    if (auto* mcall = dynamic_cast<const MethodCall*>(&expr)) {
        return check_method_call(state, *mcall);
    }

    if (auto* access = dynamic_cast<const FieldAccess*>(&expr)) {
        return check_field_access(state, *access);
    }

    if (auto* lit = dynamic_cast<const StructLiteral*>(&expr)) {
        return check_struct_literal(state, *lit);
    }

    return {"unknown", false, false};
}

} // namespace typechecker
