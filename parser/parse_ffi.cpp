/**
 * @file parse_ffi.cpp
 * @brief FFI (Foreign Function Interface) parsing for the Bishop parser.
 *
 * Handles parsing of @extern function declarations for calling C libraries.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @bishop_syntax @extern
 * @category FFI
 * @order 1
 * @description Declare an external C function to call from Bishop.
 * @syntax @extern("lib") fn name(params) -> type;
 * @example
 * @extern("c") fn puts(cstr s) -> cint;
 * @extern("m") fn sqrt(f64 x) -> f64;
 * @note Use C-compatible types: cint, cstr, void
 */
unique_ptr<ExternFunctionDef> parse_extern_function(ParserState& state, const string& library) {
    consume(state, TokenType::FN);
    Token name = consume(state, TokenType::IDENT);
    consume(state, TokenType::LPAREN);

    auto func = make_unique<ExternFunctionDef>();
    func->name = name.value;
    func->library = library;
    func->line = name.line;

    // Parse parameters
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

    // Parse return type: -> cint
    if (check(state, TokenType::ARROW)) {
        advance(state);
        func->return_type = parse_type(state);
    }

    // Extern functions end with semicolon, no body
    consume(state, TokenType::SEMICOLON);

    return func;
}

} // namespace parser
