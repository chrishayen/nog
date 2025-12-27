/**
 * @file parse_postfix.cpp
 * @brief Postfix expression parsing for the Bishop parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses postfix operations: field access (obj.field) and method calls (obj.method()).
 * Chains multiple accesses like a.b.c() or a.b.c.d.
 */
unique_ptr<ASTNode> parse_postfix(ParserState& state, unique_ptr<ASTNode> left) {
    while (check(state, TokenType::DOT)) {
        advance(state);
        Token member_tok = consume(state, TokenType::IDENT);
        string member_name = member_tok.value;

        // Check if it's a method call: obj.method(args)
        if (check(state, TokenType::LPAREN)) {
            auto call = make_unique<MethodCall>();
            call->object = move(left);
            call->method_name = member_name;
            call->line = member_tok.line;

            consume(state, TokenType::LPAREN);

            while (!check(state, TokenType::RPAREN) && !check(state, TokenType::EOF_TOKEN)) {
                auto arg = parse_expression(state);

                if (arg) {
                    call->args.push_back(move(arg));
                }

                if (check(state, TokenType::COMMA)) {
                    advance(state);
                }
            }

            consume(state, TokenType::RPAREN);
            left = move(call);
        } else {
            // Field access: obj.field
            auto access = make_unique<FieldAccess>();
            access->object = move(left);
            access->field_name = member_name;
            access->line = member_tok.line;
            left = move(access);
        }
    }

    return left;
}

} // namespace parser
