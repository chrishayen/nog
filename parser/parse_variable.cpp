/**
 * @file parse_variable.cpp
 * @brief Variable declaration parsing for the Nog parser.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

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

} // namespace parser
