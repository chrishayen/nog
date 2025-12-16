/**
 * @file parse_statement.cpp
 * @brief Statement parsing dispatch for the Nog parser.
 *
 * Main entry point for parsing statements. Individual statement types
 * are implemented in separate files.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses any statement. Dispatches based on the first token:
 * - return: parse return statement
 * - if: parse if/else
 * - while: parse while loop
 * - type keyword: parse typed variable declaration
 * - identifier: could be assignment, inferred decl, function call, or struct variable
 */
unique_ptr<ASTNode> parse_statement(ParserState& state) {
    // return statement
    if (check(state, TokenType::RETURN)) {
        return parse_return(state);
    }

    // if statement
    if (check(state, TokenType::IF)) {
        return parse_if(state);
    }

    // while loop
    if (check(state, TokenType::WHILE)) {
        return parse_while(state);
    }

    // for loop
    if (check(state, TokenType::FOR)) {
        return parse_for(state);
    }

    // select statement
    if (check(state, TokenType::SELECT)) {
        return parse_select(state);
    }

    // await expression as statement: await expr;
    if (check(state, TokenType::AWAIT)) {
        auto expr = parse_expression(state);
        consume(state, TokenType::SEMICOLON);
        return expr;
    }

    // typed variable: int x = 5
    if (is_type_token(state)) {
        return parse_variable_decl(state);
    }

    // List<T> variable declaration: List<int> nums = [1, 2, 3];
    if (check(state, TokenType::LIST)) {
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

        auto decl = make_unique<VariableDecl>();
        decl->type = "List<" + element_type + ">";
        decl->name = consume(state, TokenType::IDENT).value;
        consume(state, TokenType::ASSIGN);
        decl->value = parse_expression(state);
        consume(state, TokenType::SEMICOLON);
        return decl;
    }

    // inferred variable, assignment, function call, method call, field assignment, or struct-typed variable
    if (check(state, TokenType::IDENT)) {
        size_t saved_pos = state.pos;
        string ident = current(state).value;
        advance(state);

        if (check(state, TokenType::COLON_ASSIGN)) {
            state.pos = saved_pos;
            return parse_inferred_decl(state);
        }

        // struct-typed variable: Person p = ... or Person? p = ...
        if (is_struct_type(state, ident) && (check(state, TokenType::IDENT) || check(state, TokenType::OPTIONAL))) {
            auto decl = make_unique<VariableDecl>();
            decl->type = ident;

            if (check(state, TokenType::OPTIONAL)) {
                decl->is_optional = true;
                advance(state);
            }

            decl->name = consume(state, TokenType::IDENT).value;
            consume(state, TokenType::ASSIGN);
            decl->value = parse_expression(state);
            consume(state, TokenType::SEMICOLON);
            return decl;
        }

        // Check for qualified reference: module.func() or module.Type var = ...
        if (check(state, TokenType::DOT) && is_imported_module(state, ident)) {
            advance(state);
            string member_name = consume(state, TokenType::IDENT).value;
            string qualified_name = ident + "." + member_name;

            // Qualified function call: module.func()
            if (check(state, TokenType::LPAREN)) {
                auto call = make_unique<FunctionCall>();
                call->name = qualified_name;
                call->line = current(state).line;

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
                consume(state, TokenType::SEMICOLON);
                return call;
            }

            // Qualified type declaration: module.Type var = ...
            if (check(state, TokenType::IDENT) || check(state, TokenType::OPTIONAL)) {
                auto decl = make_unique<VariableDecl>();
                decl->type = qualified_name;

                if (check(state, TokenType::OPTIONAL)) {
                    decl->is_optional = true;
                    advance(state);
                }

                decl->name = consume(state, TokenType::IDENT).value;
                consume(state, TokenType::ASSIGN);
                decl->value = parse_expression(state);
                consume(state, TokenType::SEMICOLON);
                return decl;
            }

            // Fallback
            state.pos = saved_pos;
            return parse_function_call(state);
        }

        // field access: obj.field = value or obj.method()
        if (check(state, TokenType::DOT)) {
            advance(state);
            string field_name = consume(state, TokenType::IDENT).value;

            // Method call as statement: obj.method(args);
            if (check(state, TokenType::LPAREN)) {
                auto call = make_unique<MethodCall>();
                call->object = make_unique<VariableRef>(ident);
                call->method_name = field_name;

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
                consume(state, TokenType::SEMICOLON);
                return call;
            }

            // Field assignment: obj.field = value;
            if (check(state, TokenType::ASSIGN)) {
                consume(state, TokenType::ASSIGN);
                auto assign = make_unique<FieldAssignment>();
                assign->object = make_unique<VariableRef>(ident);
                assign->field_name = field_name;
                assign->value = parse_expression(state);
                consume(state, TokenType::SEMICOLON);
                return assign;
            }
        }

        // assignment: x = expr
        if (check(state, TokenType::ASSIGN)) {
            state.pos = saved_pos;
            auto assign = make_unique<Assignment>();
            assign->name = consume(state, TokenType::IDENT).value;
            consume(state, TokenType::ASSIGN);
            assign->value = parse_expression(state);
            consume(state, TokenType::SEMICOLON);
            return assign;
        }

        state.pos = saved_pos;
        return parse_function_call(state);
    }

    advance(state);
    return nullptr;
}

/**
 * Parses a function call statement: name(args);
 * Includes the trailing semicolon.
 */
unique_ptr<FunctionCall> parse_function_call(ParserState& state) {
    Token name = consume(state, TokenType::IDENT);
    consume(state, TokenType::LPAREN);

    auto call = make_unique<FunctionCall>();
    call->name = name.value;
    call->line = name.line;

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
    consume(state, TokenType::SEMICOLON);
    return call;
}

} // namespace parser
