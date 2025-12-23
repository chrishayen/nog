/**
 * @file parse_or.cpp
 * @brief Or expression parsing for the Nog parser.
 *
 * Handles parsing of error handling expressions:
 * - expr or return
 * - expr or return value
 * - expr or fail err
 * - expr or { block }
 * - expr or match err { ... }
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parse an or-return handler: or return [value]
 */
unique_ptr<OrReturn> parse_or_return(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::RETURN);

    auto handler = make_unique<OrReturn>();
    handler->line = start_line;

    // Check for optional return value
    if (!check(state, TokenType::SEMICOLON) && !check(state, TokenType::RBRACE)) {
        handler->value = parse_comparison(state);
    }

    return handler;
}

/**
 * Parse an or-fail handler: or fail error_expr
 */
unique_ptr<OrFail> parse_or_fail(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::FAIL);

    auto handler = make_unique<OrFail>();
    handler->line = start_line;

    // Special case: 'or fail err' where err refers to the current error
    if (check(state, TokenType::ERR)) {
        advance(state);
        auto err_ref = make_unique<VariableRef>("err");
        err_ref->line = start_line;
        handler->error_expr = move(err_ref);
    } else {
        handler->error_expr = parse_comparison(state);
    }

    return handler;
}

/**
 * Parse an or-block handler: or { statements }
 */
unique_ptr<OrBlock> parse_or_block(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::LBRACE);

    auto handler = make_unique<OrBlock>();
    handler->line = start_line;

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        auto stmt = parse_statement(state);

        if (stmt) {
            handler->body.push_back(move(stmt));
        }
    }

    consume(state, TokenType::RBRACE);
    return handler;
}

/**
 * Parse an or-match handler: or match err { arms }
 */
unique_ptr<OrMatch> parse_or_match(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::MATCH);
    consume(state, TokenType::ERR);
    consume(state, TokenType::LBRACE);

    auto handler = make_unique<OrMatch>();
    handler->line = start_line;

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        MatchArm arm;

        // Parse error type or wildcard _
        if (current(state).value == "_") {
            arm.error_type = "_";
            advance(state);
        } else {
            arm.error_type = consume(state, TokenType::IDENT).value;
        }

        // Consume =>
        consume(state, TokenType::ASSIGN);

        if (check(state, TokenType::GT)) {
            advance(state);  // skip > for =>
        }

        // Parse body - either expression or fail statement
        if (check(state, TokenType::FAIL)) {
            arm.body = parse_fail_expr(state);
        } else {
            arm.body = parse_comparison(state);
        }

        handler->arms.push_back(move(arm));

        if (check(state, TokenType::COMMA)) {
            advance(state);
        }
    }

    consume(state, TokenType::RBRACE);
    return handler;
}

/**
 * Parse or expression: expr or handler
 */
unique_ptr<ASTNode> parse_or(ParserState& state) {
    auto expr = parse_default(state);

    if (!check(state, TokenType::OR)) {
        return expr;
    }

    int or_line = current(state).line;
    advance(state);  // consume 'or'

    auto or_expr = make_unique<OrExpr>();
    or_expr->line = or_line;
    or_expr->expr = move(expr);

    // Parse handler based on next token
    if (check(state, TokenType::RETURN)) {
        or_expr->handler = parse_or_return(state);
    } else if (check(state, TokenType::FAIL)) {
        or_expr->handler = parse_or_fail(state);
    } else if (check(state, TokenType::MATCH)) {
        or_expr->handler = parse_or_match(state);
    } else if (check(state, TokenType::LBRACE)) {
        or_expr->handler = parse_or_block(state);
    }

    return or_expr;
}

/**
 * Parse default expression: expr default fallback
 */
unique_ptr<ASTNode> parse_default(ParserState& state) {
    auto expr = parse_comparison(state);

    if (!check(state, TokenType::DEFAULT)) {
        return expr;
    }

    int default_line = current(state).line;
    advance(state);  // consume 'default'

    auto default_expr = make_unique<DefaultExpr>();
    default_expr->line = default_line;
    default_expr->expr = move(expr);
    default_expr->fallback = parse_comparison(state);

    return default_expr;
}

} // namespace parser
