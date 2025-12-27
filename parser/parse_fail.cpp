/**
 * @file parse_fail.cpp
 * @brief Fail statement parsing for the Bishop parser.
 *
 * Handles fail statements for error propagation.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parse a fail expression without consuming semicolon.
 * Used in match arms where semicolons aren't expected.
 */
unique_ptr<FailStmt> parse_fail_expr(ParserState& state) {
    Token fail_tok = consume(state, TokenType::FAIL);
    auto stmt = make_unique<FailStmt>();
    stmt->line = fail_tok.line;

    // Special case: 'fail err' where err refers to the current error
    if (check(state, TokenType::ERR)) {
        advance(state);
        auto err_ref = make_unique<VariableRef>("err");
        err_ref->line = fail_tok.line;
        stmt->value = move(err_ref);
    } else {
        stmt->value = parse_comparison(state);
    }

    return stmt;
}

/**
 * @bishop_syntax Fail Statement
 * @category Error Handling
 * @order 2
 * @description Return an error from a fallible function.
 * @syntax fail "message"; or fail ErrorType { fields };
 * @example
 * fail "file not found";
 * fail IOError { code: 404, path: path };
 */
unique_ptr<FailStmt> parse_fail(ParserState& state) {
    Token fail_tok = consume(state, TokenType::FAIL);
    auto stmt = make_unique<FailStmt>();
    stmt->line = fail_tok.line;
    stmt->value = parse_expression(state);
    consume(state, TokenType::SEMICOLON);
    return stmt;
}

} // namespace parser
