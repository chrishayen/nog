/**
 * @file parse_return.cpp
 * @brief Return statement parsing for the Bishop parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses a return statement: return; or return expr;
 */
unique_ptr<ReturnStmt> parse_return(ParserState& state) {
    Token return_tok = consume(state, TokenType::RETURN);
    auto ret = make_unique<ReturnStmt>();
    ret->line = return_tok.line;

    // Check for void return: return;
    if (check(state, TokenType::SEMICOLON)) {
        ret->value = nullptr;
    } else {
        ret->value = parse_expression(state);
    }

    consume(state, TokenType::SEMICOLON);
    return ret;
}

} // namespace parser
