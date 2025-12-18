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
        if (left_type.is_awaitable || right_type.is_awaitable) {
            error(state, "type mismatch in binary expression: '" + format_type(left_type) + "' " + bin.op +
                  " '" + format_type(right_type) + "' (did you forget 'await'?)", bin.line);
        }

        return {"bool", false, false};
    }

    if (left_type.is_awaitable || right_type.is_awaitable) {
        error(state, "type mismatch in binary expression: '" + format_type(left_type) + "' " + bin.op +
              " '" + format_type(right_type) + "' (did you forget 'await'?)", bin.line);
        return {"unknown", false, false};
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
    TypeInfo inner_type = infer_type(state, *expr.value);

    if (inner_type.is_awaitable) {
        error(state, "cannot use 'is none' on '" + format_type(inner_type) + "' (did you forget 'await'?)", expr.line);
    }

    return {"bool", false, false};
}

/**
 * Infers the type of a not expression.
 */
TypeInfo check_not_expr(TypeCheckerState& state, const NotExpr& not_expr) {
    TypeInfo inner_type = infer_type(state, *not_expr.value);

    if (inner_type.is_awaitable || inner_type.base_type != "bool") {
        error(state, "'!' operator requires bool, got '" + format_type(inner_type) + "'", not_expr.line);
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

    TypeInfo awaited_type = infer_type(state, *await_expr.value);

    if (awaited_type.base_type == "unknown") {
        return awaited_type;
    }

    if (!awaited_type.is_awaitable) {
        error(state, "cannot await non-async value of type '" + format_type(awaited_type) + "'", await_expr.line);
        return awaited_type;
    }

    awaited_type.is_awaitable = false;
    return awaited_type;
}

} // namespace typechecker
