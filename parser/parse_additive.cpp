/**
 * @file parse_additive.cpp
 * @brief Additive/multiplicative expression parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses additive/multiplicative expressions: +, -, *, /
 * Parses postfix (field access, method calls) on each operand.
 */
unique_ptr<ASTNode> parse_additive(ParserState& state) {
    auto left = parse_primary(state);
    left = parse_postfix(state, move(left));

    while (check(state, TokenType::PLUS) || check(state, TokenType::MINUS) ||
           check(state, TokenType::STAR) || check(state, TokenType::SLASH)) {
        string op = current(state).value;
        advance(state);
        auto right = parse_primary(state);
        right = parse_postfix(state, move(right));

        auto binop = make_unique<BinaryExpr>();
        binop->op = op;
        binop->left = move(left);
        binop->right = move(right);
        left = move(binop);
    }

    return left;
}

} // namespace parser
