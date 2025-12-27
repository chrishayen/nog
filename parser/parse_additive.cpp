/**
 * @file parse_additive.cpp
 * @brief Additive/multiplicative expression parsing for the Bishop parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses multiplicative expressions: *, /
 * Parses postfix (field access, method calls) on each operand.
 */
static unique_ptr<ASTNode> parse_multiplicative(ParserState& state) {
    auto left = parse_primary(state);
    left = parse_postfix(state, move(left));

    while (check(state, TokenType::STAR) || check(state, TokenType::SLASH)) {
        Token op_tok = current(state);
        string op = op_tok.value;
        advance(state);

        auto right = parse_primary(state);
        right = parse_postfix(state, move(right));

        auto binop = make_unique<BinaryExpr>();
        binop->op = op;
        binop->line = op_tok.line;
        binop->left = move(left);
        binop->right = move(right);
        left = move(binop);
    }

    return left;
}

/**
 * Parses additive expressions: +, -
 */
unique_ptr<ASTNode> parse_additive(ParserState& state) {
    auto left = parse_multiplicative(state);

    while (check(state, TokenType::PLUS) || check(state, TokenType::MINUS)) {
        Token op_tok = current(state);
        string op = op_tok.value;
        advance(state);
        auto right = parse_multiplicative(state);

        auto binop = make_unique<BinaryExpr>();
        binop->op = op;
        binop->line = op_tok.line;
        binop->left = move(left);
        binop->right = move(right);
        left = move(binop);
    }

    return left;
}

} // namespace parser
