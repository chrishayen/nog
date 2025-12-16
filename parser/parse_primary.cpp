/**
 * @file parse_primary.cpp
 * @brief Primary expression parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @nog_syntax await
 * @category Async
 * @order 3
 * @description Await an async operation.
 * @syntax await expr
 * @example
 * result := await fetch_data();
 * await ch.send(42);
 * val := await ch.recv();
 */

/**
 * @nog_syntax Channel Creation
 * @category Channels
 * @order 1
 * @description Create a typed channel for communication between async tasks.
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

    // Handle await expression: await expr
    if (check(state, TokenType::AWAIT)) {
        advance(state);
        auto await_expr = make_unique<AwaitExpr>();
        await_expr->value = parse_primary(state);
        await_expr->value = parse_postfix(state, move(await_expr->value));
        return await_expr;
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
        return make_unique<NumberLiteral>(tok.value);
    }

    if (check(state, TokenType::FLOAT)) {
        Token tok = current(state);
        advance(state);
        return make_unique<FloatLiteral>(tok.value);
    }

    if (check(state, TokenType::STRING)) {
        Token tok = current(state);
        advance(state);
        return make_unique<StringLiteral>(tok.value);
    }

    if (check(state, TokenType::TRUE)) {
        advance(state);
        return make_unique<BoolLiteral>(true);
    }

    if (check(state, TokenType::FALSE)) {
        advance(state);
        return make_unique<BoolLiteral>(false);
    }

    if (check(state, TokenType::NONE)) {
        advance(state);
        return make_unique<NoneLiteral>();
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

        return make_unique<VariableRef>(tok.value);
    }

    advance(state);
    return nullptr;
}

} // namespace parser
