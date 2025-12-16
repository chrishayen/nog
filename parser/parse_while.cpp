/**
 * @file parse_while.cpp
 * @brief While statement parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @nog_syntax while
 * @category Control Flow
 * @order 2
 * @description Loop while a condition is true.
 * @syntax while condition { ... }
 * @example
 * i := 0;
 * while i < 5 {
 *     print(i);
 *     i = i + 1;
 * }
 */
unique_ptr<WhileStmt> parse_while(ParserState& state) {
    consume(state, TokenType::WHILE);
    auto stmt = make_unique<WhileStmt>();
    stmt->condition = parse_expression(state);
    consume(state, TokenType::LBRACE);

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        auto s = parse_statement(state);

        if (s) {
            stmt->body.push_back(move(s));
        }
    }

    consume(state, TokenType::RBRACE);

    return stmt;
}

} // namespace parser
