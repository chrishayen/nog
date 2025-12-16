/**
 * @file parse_return.cpp
 * @brief Return statement parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses a return statement: return expr;
 */
unique_ptr<ReturnStmt> parse_return(ParserState& state) {
    consume(state, TokenType::RETURN);
    auto ret = make_unique<ReturnStmt>();
    ret->value = parse_expression(state);
    consume(state, TokenType::SEMICOLON);
    return ret;
}

} // namespace parser
