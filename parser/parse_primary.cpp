/**
 * @file parse_primary.cpp
 * @brief Primary expression parsing for the Nog parser.
 */

#include "parser.hpp"
#include <stdexcept>

using namespace std;

namespace parser {

/**
 * @nog_syntax Channel Creation
 * @category Channels
 * @order 1
 * @description Create a typed channel for communication between goroutines.
 * @syntax Channel<type>()
 * @example
 * ch := Channel<int>();
 * ch_str := Channel<str>();
 */
unique_ptr<ASTNode> parse_primary(ParserState& state) {
    // Handle NOT expression: !expr
    if (check(state, TokenType::NOT)) {
        int start_line = current(state).line;
        advance(state);
        auto not_expr = make_unique<NotExpr>();
        not_expr->value = parse_primary(state);
        not_expr->line = start_line;
        return not_expr;
    }

    // Handle address-of expression: &expr
    if (check(state, TokenType::AMPERSAND)) {
        int start_line = current(state).line;
        advance(state);
        auto addr = make_unique<AddressOf>();
        addr->value = parse_primary(state);
        addr->line = start_line;
        return addr;
    }

    // Parenthesized expression: (expr)
    if (check(state, TokenType::LPAREN)) {
        Token lparen = consume(state, TokenType::LPAREN);

        // Disallow empty parentheses to avoid consuming ')' as part of an invalid expression.
        if (check(state, TokenType::RPAREN)) {
            throw runtime_error("expected expression after '(' at line " + to_string(lparen.line));
        }

        auto group = make_unique<ParenExpr>();
        group->line = lparen.line;
        group->value = parse_expression(state);
        consume(state, TokenType::RPAREN);
        return group;
    }

    // Handle channel creation: Channel<int>()
    if (check(state, TokenType::CHANNEL)) {
        int start_line = current(state).line;
        advance(state);
        consume(state, TokenType::LT);

        string element_type;

        if (is_type_token(state)) {
            element_type = token_to_type(current(state).type);
            advance(state);
        } else if (check(state, TokenType::IDENT)) {
            element_type = current(state).value;
            advance(state);
        }

        consume(state, TokenType::GT);
        consume(state, TokenType::LPAREN);
        consume(state, TokenType::RPAREN);

        auto channel = make_unique<ChannelCreate>();
        channel->element_type = element_type;
        channel->line = start_line;
        return channel;
    }

    // Handle list creation: List<int>()
    if (check(state, TokenType::LIST)) {
        int start_line = current(state).line;
        advance(state);
        consume(state, TokenType::LT);

        string element_type;

        if (is_type_token(state)) {
            element_type = token_to_type(current(state).type);
            advance(state);
        } else if (check(state, TokenType::IDENT)) {
            element_type = current(state).value;
            advance(state);
        }

        consume(state, TokenType::GT);
        consume(state, TokenType::LPAREN);
        consume(state, TokenType::RPAREN);

        auto list = make_unique<ListCreate>();
        list->element_type = element_type;
        list->line = start_line;
        return list;
    }

    // Handle list literal: [expr, expr, ...]
    if (check(state, TokenType::LBRACKET)) {
        int start_line = current(state).line;
        advance(state);

        auto list = make_unique<ListLiteral>();
        list->line = start_line;

        while (!check(state, TokenType::RBRACKET) && !check(state, TokenType::EOF_TOKEN)) {
            auto elem = parse_expression(state);

            if (elem) {
                list->elements.push_back(move(elem));
            }

            if (check(state, TokenType::COMMA)) {
                advance(state);
            }
        }

        consume(state, TokenType::RBRACKET);
        return list;
    }

    if (check(state, TokenType::NUMBER)) {
        Token tok = current(state);
        advance(state);
        auto lit = make_unique<NumberLiteral>(tok.value);
        lit->line = tok.line;
        return lit;
    }

    if (check(state, TokenType::FLOAT)) {
        Token tok = current(state);
        advance(state);
        auto lit = make_unique<FloatLiteral>(tok.value);
        lit->line = tok.line;
        return lit;
    }

    if (check(state, TokenType::STRING)) {
        Token tok = current(state);
        advance(state);
        auto lit = make_unique<StringLiteral>(tok.value);
        lit->line = tok.line;
        return lit;
    }

    if (check(state, TokenType::CHAR_LITERAL)) {
        Token tok = current(state);
        advance(state);
        return make_unique<CharLiteral>(tok.value[0], tok.line);
    }

    if (check(state, TokenType::TRUE)) {
        int start_line = current(state).line;
        advance(state);
        auto lit = make_unique<BoolLiteral>(true);
        lit->line = start_line;
        return lit;
    }

    if (check(state, TokenType::FALSE)) {
        int start_line = current(state).line;
        advance(state);
        auto lit = make_unique<BoolLiteral>(false);
        lit->line = start_line;
        return lit;
    }

    if (check(state, TokenType::NONE)) {
        int start_line = current(state).line;
        advance(state);
        auto lit = make_unique<NoneLiteral>();
        lit->line = start_line;
        return lit;
    }

    if (check(state, TokenType::IDENT)) {
        Token tok = current(state);
        advance(state);

        // Check for qualified reference: module.item (e.g., math.add)
        if (check(state, TokenType::DOT) && is_imported_module(state, tok.value)) {
            advance(state);
            Token item_tok = consume(state, TokenType::IDENT);
            string item_name = item_tok.value;

            // Check if it's a qualified function call: module.func(args)
            if (check(state, TokenType::LPAREN)) {
                auto call = make_unique<FunctionCall>();
                call->name = tok.value + "." + item_name;  // Store as "module.func"
                call->line = tok.line;
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
                return call;
            }

            // Check if it's a qualified struct literal: module.Type { ... }
            if (check(state, TokenType::LBRACE)) {
                return parse_struct_literal(state, tok.value + "." + item_name);
            }

            // Check if it's a qualified function reference: module.func (without parens)
            // This is for passing module functions as arguments
            auto fref = make_unique<FunctionRef>(tok.value + "." + item_name);
            fref->line = tok.line;
            return fref;
        }

        // Check if it's a struct literal: TypeName { field: value, ... }
        if (check(state, TokenType::LBRACE) && is_struct_type(state, tok.value)) {
            return parse_struct_literal(state, tok.value);
        }

        // Check if it's a function call
        if (check(state, TokenType::LPAREN)) {
            auto call = make_unique<FunctionCall>();
            call->name = tok.value;
            call->line = tok.line;
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
            return call;
        }

        // Check if it's a function reference (function name without parentheses)
        if (is_function_name(state, tok.value)) {
            auto ref = make_unique<FunctionRef>(tok.value);
            ref->line = tok.line;
            return ref;
        }

        auto ref = make_unique<VariableRef>(tok.value);
        ref->line = tok.line;
        return ref;
    }

    advance(state);
    return nullptr;
}

} // namespace parser
