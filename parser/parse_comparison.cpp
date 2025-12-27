/**
 * @file parse_comparison.cpp
 * @brief Comparison expression parsing for the Bishop parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses comparison expressions: handles "is none" and comparison operators (==, !=, <, >, <=, >=).
 * Also chains multiple comparisons.
 */
unique_ptr<ASTNode> parse_comparison(ParserState& state) {
    auto left = parse_additive(state);

    // Handle "x is none"
    if (check(state, TokenType::IS)) {
        Token is_tok = current(state);
        advance(state);
        consume(state, TokenType::NONE);
        auto is_none = make_unique<IsNone>();
        is_none->line = is_tok.line;
        is_none->value = move(left);
        return is_none;
    }

    while (check(state, TokenType::EQ) || check(state, TokenType::NE) ||
           check(state, TokenType::LT) || check(state, TokenType::GT) ||
           check(state, TokenType::LE) || check(state, TokenType::GE)) {
        Token op_tok = current(state);
        string op = op_tok.value;
        advance(state);
        auto right = parse_additive(state);

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
