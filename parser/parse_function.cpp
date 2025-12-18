/**
 * @file parse_function.cpp
 * @brief Function and method parsing for the Nog parser.
 *
 * Handles function definitions, method definitions, and visibility annotations.
 * For extern declarations, see parse_ffi.cpp.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Parses visibility annotation: @private
 * Returns Private if found, Public otherwise.
 */
Visibility parse_visibility(ParserState& state) {
    if (check(state, TokenType::AT)) {
        advance(state);

        if (check(state, TokenType::PRIVATE)) {
            advance(state);
            return Visibility::Private;
        }
    }

    return Visibility::Public;
}

/**
 * @nog_syntax Function Declaration
 * @category Functions
 * @order 1
 * @description Declare a function with parameters and return type.
 * @syntax fn name(type param, ...) -> return_type { }
 * @example
 * fn add(int a, int b) -> int {
 *     return a + b;
 * }
 */
unique_ptr<FunctionDef> parse_function(ParserState& state, Visibility vis) {
    consume(state, TokenType::FN);
    Token name = consume(state, TokenType::IDENT);
    consume(state, TokenType::LPAREN);

    auto func = make_unique<FunctionDef>();
    func->name = name.value;
    func->line = name.line;
    func->visibility = vis;

    // Parse parameters: fn foo(int a, int b) or fn foo(Person p) or fn foo(fn(int) -> int callback)
    while (!check(state, TokenType::RPAREN) && !check(state, TokenType::EOF_TOKEN)) {
        FunctionParam param;
        param.type = parse_type(state);
        param.name = consume(state, TokenType::IDENT).value;
        func->params.push_back(param);

        if (check(state, TokenType::COMMA)) {
            advance(state);
        }
    }

    consume(state, TokenType::RPAREN);

    // Parse return type: -> int
    if (check(state, TokenType::ARROW)) {
        advance(state);
        func->return_type = parse_type(state);
    }

    consume(state, TokenType::LBRACE);

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        auto stmt = parse_statement(state);

        if (stmt) {
            func->body.push_back(move(stmt));
        }
    }

    consume(state, TokenType::RBRACE);
    return func;
}

/**
 * @nog_syntax Method Definition
 * @category Methods
 * @order 1
 * @description Define a method on a struct type.
 * @syntax Type :: name(self, params) -> return_type { }
 * @example
 * Person :: get_name(self) -> str {
 *     return self.name;
 * }
 */
unique_ptr<MethodDef> parse_method_def(ParserState& state, const string& struct_name, Visibility vis) {
    // We're past "Name ::", now at method_name
    Token method_name = consume(state, TokenType::IDENT);
    consume(state, TokenType::LPAREN);

    auto method = make_unique<MethodDef>();
    method->struct_name = struct_name;
    method->name = method_name.value;
    method->line = method_name.line;
    method->visibility = vis;

    // Parse parameters (first should be 'self')
    while (!check(state, TokenType::RPAREN) && !check(state, TokenType::EOF_TOKEN)) {
        FunctionParam param;

        // Check for 'self' (special case - type is the struct)
        if (current(state).value == "self") {
            param.type = struct_name;
            param.name = "self";
            advance(state);
        } else {
            param.type = parse_type(state);
            param.name = consume(state, TokenType::IDENT).value;
        }

        method->params.push_back(param);

        if (check(state, TokenType::COMMA)) {
            advance(state);
        }
    }

    consume(state, TokenType::RPAREN);

    // Parse return type: -> type
    if (check(state, TokenType::ARROW)) {
        advance(state);
        method->return_type = parse_type(state);
    }

    consume(state, TokenType::LBRACE);

    while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
        auto stmt = parse_statement(state);

        if (stmt) {
            method->body.push_back(move(stmt));
        }
    }

    consume(state, TokenType::RBRACE);
    return method;
}

} // namespace parser
