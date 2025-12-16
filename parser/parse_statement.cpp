/**
 * @file parse_statement.cpp
 * @brief Statement parsing for the Nog parser.
 *
 * Handles all statement types: variable declarations, assignments,
 * return, if/else, while, select, and function/method calls.
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
 * @nog_syntax Explicit Declaration
 * @category Variables
 * @order 1
 * @description Declare a variable with an explicit type.
 * @syntax type name = expr;
 * @example
 * int x = 42;
 * str name = "Chris";
 * bool flag = true;
 */
unique_ptr<VariableDecl> parse_variable_decl(ParserState& state) {
    auto decl = make_unique<VariableDecl>();
    decl->type = token_to_type(current(state).type);
    advance(state);

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

/**
 * @nog_syntax Type Inference
 * @category Variables
 * @order 2
 * @description Declare a variable with inferred type using :=.
 * @syntax name := expr;
 * @example
 * x := 100;
 * name := "Hello";
 * pi := 3.14;
 */
unique_ptr<VariableDecl> parse_inferred_decl(ParserState& state) {
    auto decl = make_unique<VariableDecl>();
    decl->name = consume(state, TokenType::IDENT).value;
    consume(state, TokenType::COLON_ASSIGN);
    decl->value = parse_expression(state);
    consume(state, TokenType::SEMICOLON);
    return decl;
}

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

/**
 * @nog_syntax if
 * @category Control Flow
 * @order 1
 * @description Conditional branching with if and optional else.
 * @syntax if condition { ... } else { ... }
 * @example
 * if x > 10 {
 *     print("big");
 * } else {
 *     print("small");
 * }
 */
unique_ptr<IfStmt> parse_if(ParserState& state) {
    consume(state, TokenType::IF);
    auto stmt = make_unique<IfStmt>();
    stmt->condition = parse_expression(state);
    consume(state, TokenType::LBRACE);

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        auto s = parse_statement(state);

        if (s) {
            stmt->then_body.push_back(move(s));
        }
    }

    consume(state, TokenType::RBRACE);

    if (check(state, TokenType::ELSE)) {
        advance(state);
        consume(state, TokenType::LBRACE);

        while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
            auto s = parse_statement(state);

            if (s) {
                stmt->else_body.push_back(move(s));
            }
        }

        consume(state, TokenType::RBRACE);
    }

    return stmt;
}

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

/**
 * @nog_syntax for
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

/**
 * @nog_syntax select
 * @category Channels
 * @order 4
 * @description Wait on multiple channel operations.
 * @syntax select { case val := ch.recv() { ... } }
 * @example
 * select {
 *     case val := ch1.recv() {
 *         x := val + 1;
 *     }
 *     case val := ch2.recv() {
 *         x := val + 2;
 *     }
 * }
 */
unique_ptr<SelectStmt> parse_select(ParserState& state) {
    int start_line = current(state).line;
    consume(state, TokenType::SELECT);
    consume(state, TokenType::LBRACE);

    auto stmt = make_unique<SelectStmt>();
    stmt->line = start_line;

    while (check(state, TokenType::CASE)) {
        auto select_case = make_unique<SelectCase>();
        select_case->line = current(state).line;
        advance(state);  // consume 'case'

        // Parse case: either "val := ch.recv()" or "ch.send(value)"
        // First check for binding: val := ...
        if (check(state, TokenType::IDENT)) {
            size_t saved_pos = state.pos;
            string first_ident = current(state).value;
            advance(state);

            if (check(state, TokenType::COLON_ASSIGN)) {
                // This is a recv with binding: val := ch.recv()
                select_case->binding_name = first_ident;
                advance(state);  // consume :=

                // Now parse the channel.recv()
                select_case->channel = parse_primary(state);
                select_case->channel = parse_postfix(state, move(select_case->channel));

                // The channel expression should be a MethodCall with method_name "recv"
                if (auto* method_call = dynamic_cast<MethodCall*>(select_case->channel.get())) {
                    select_case->operation = method_call->method_name;

                    // For recv, we need to keep the channel object, not the method call
                    auto channel_obj = move(method_call->object);
                    select_case->channel = move(channel_obj);
                }
            } else if (check(state, TokenType::DOT)) {
                // This is either ch.send(value) or ch.recv() without binding
                state.pos = saved_pos;

                // Parse channel expression (identifier + method call)
                auto expr = parse_primary(state);
                expr = parse_postfix(state, move(expr));

                if (auto* method_call = dynamic_cast<MethodCall*>(expr.get())) {
                    select_case->operation = method_call->method_name;

                    if (method_call->method_name == "send" && !method_call->args.empty()) {
                        select_case->send_value = move(method_call->args[0]);
                    }

                    select_case->channel = move(method_call->object);
                }
            } else {
                state.pos = saved_pos;
            }
        }

        // Parse the case body: { statements }
        consume(state, TokenType::LBRACE);

        while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
            auto s = parse_statement(state);

            if (s) {
                select_case->body.push_back(move(s));
            }
        }

        consume(state, TokenType::RBRACE);
        stmt->cases.push_back(move(select_case));
    }

    consume(state, TokenType::RBRACE);
    return stmt;
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
