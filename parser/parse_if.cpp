/**
 * @file parse_if.cpp
 * @brief If statement parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @nog_syntax if
 * @category Control Flow
 * @order 1
 * @description Conditional branching with if and optional else.
 * @syntax if condition { ... } else { ... }
 * @example
 * if x > 10 {
 *     print("big");
 * } else {
 *     print("small");
 * }
 */
unique_ptr<IfStmt> parse_if(ParserState& state) {
    consume(state, TokenType::IF);
    auto stmt = make_unique<IfStmt>();
    stmt->condition = parse_expression(state);
    consume(state, TokenType::LBRACE);

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        auto s = parse_statement(state);

        if (s) {
            stmt->then_body.push_back(move(s));
        }
    }

    consume(state, TokenType::RBRACE);

    if (check(state, TokenType::ELSE)) {
        advance(state);
        consume(state, TokenType::LBRACE);

        while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
            auto s = parse_statement(state);

            if (s) {
                stmt->else_body.push_back(move(s));
            }
        }

        consume(state, TokenType::RBRACE);
    }

    return stmt;
}

} // namespace parser
