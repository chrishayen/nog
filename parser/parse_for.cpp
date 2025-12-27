/**
 * @file parse_for_loop.cpp
 * @brief For loop parsing for the Bishop parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @bishop_syntax for
 * @category Control Flow
 * @order 3
 * @description Iterate over ranges or collections.
 * @syntax for var in start..end { ... }
 * @syntax for var in collection { ... }
 * @example
 * for i in 0..5 {
 *     print(i);
 * }
 *
 * nums := [1, 2, 3];
 * for n in nums {
 *     print(n);
 * }
 */
unique_ptr<ForStmt> parse_for(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::FOR);

    auto stmt = make_unique<ForStmt>();
    stmt->line = start_line;
    stmt->loop_var = consume(state, TokenType::IDENT).value;

    consume(state, TokenType::IN);

    // Parse first expression (could be range start or collection)
    auto first_expr = parse_primary(state);

    // Check if this is a range: expr..expr
    if (check(state, TokenType::DOTDOT)) {
        advance(state);
        stmt->kind = ForLoopKind::Range;
        stmt->range_start = move(first_expr);
        stmt->range_end = parse_additive(state);
    } else {
        // Foreach over a collection - may have postfix (e.g., obj.list)
        stmt->kind = ForLoopKind::Foreach;
        stmt->iterable = parse_postfix(state, move(first_expr));
    }

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
