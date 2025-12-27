/**
 * @file check_literals.cpp
 * @brief Literal type inference for the Bishop type checker.
 */

#include "typechecker.hpp"

using namespace std;

namespace typechecker {

/**
 * Infers the type of a number literal.
 */
TypeInfo check_number_literal(TypeCheckerState& state, const NumberLiteral& lit) {
    (void)state;
    (void)lit;
    return {"int", false, false};
}

/**
 * Infers the type of a float literal.
 */
TypeInfo check_float_literal(TypeCheckerState& state, const FloatLiteral& lit) {
    (void)state;
    (void)lit;
    return {"f64", false, false};
}

/**
 * Infers the type of a string literal.
 */
TypeInfo check_string_literal(TypeCheckerState& state, const StringLiteral& lit) {
    (void)state;
    (void)lit;
    return {"str", false, false};
}

/**
 * Infers the type of a bool literal.
 */
TypeInfo check_bool_literal(TypeCheckerState& state, const BoolLiteral& lit) {
    (void)state;
    (void)lit;
    return {"bool", false, false};
}

/**
 * Infers the type of a none literal.
 */
TypeInfo check_none_literal(TypeCheckerState& state, const NoneLiteral& lit) {
    (void)state;
    (void)lit;
    return {"none", true, false};
}

/**
 * Infers the type of a char literal.
 */
TypeInfo check_char_literal(TypeCheckerState& state, const CharLiteral& lit) {
    (void)state;
    (void)lit;
    return {"char", false, false};
}

} // namespace typechecker
