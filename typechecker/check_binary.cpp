/**
 * @file check_binary.cpp
 * @brief Binary expression type inference for the Nog type checker.
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

    if (bin.op == "+" && left_type.base_type == "str") {
        return {"str", false, false};
    }

    if (left_type.base_type != right_type.base_type) {
        error(state, "type mismatch in binary expression: '" + left_type.base_type + "' " + bin.op + " '" + right_type.base_type + "'", bin.line);
    }

    return left_type;
}

/**
 * Infers the type of an is_none expression.
 */
TypeInfo check_is_none(TypeCheckerState& state, const IsNone& expr) {
    (void)state;
    (void)expr;
    return {"bool", false, false};
}

/**
 * Infers the type of a not expression.
 */
TypeInfo check_not_expr(TypeCheckerState& state, const NotExpr& not_expr) {
    TypeInfo inner_type = infer_type(state, *not_expr.value);

    if (inner_type.base_type != "bool") {
        error(state, "'!' operator requires bool, got '" + inner_type.base_type + "'", not_expr.line);
    }

    return {"bool", false, false};
}

/**
 * Infers the type of an await expression.
 */
TypeInfo check_await_expr(TypeCheckerState& state, const AwaitExpr& await_expr) {
    if (!state.in_async_context) {
        error(state, "'await' can only be used inside async functions", await_expr.line);
    }

    return infer_type(state, *await_expr.value);
}

} // namespace typechecker
