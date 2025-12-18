/**
 * @file parse_type.cpp
 * @brief Type parsing utilities for the Nog parser.
 *
 * Handles type tokens, type conversion, and complex type parsing
 * including function types and qualified types.
 */

#include "parser.hpp"
#include <stdexcept>

using namespace std;

namespace parser {

/**
 * Checks if current token is a primitive type keyword (int, str, bool, etc).
 */
bool is_type_token(const ParserState& state) {
    TokenType t = current(state).type;
    return t == TokenType::TYPE_INT || t == TokenType::TYPE_STR ||
           t == TokenType::TYPE_BOOL || t == TokenType::TYPE_CHAR ||
           t == TokenType::TYPE_F32 || t == TokenType::TYPE_F64 ||
           t == TokenType::TYPE_U32 || t == TokenType::TYPE_U64 ||
           t == TokenType::TYPE_CINT || t == TokenType::TYPE_CSTR ||
           t == TokenType::TYPE_VOID;
}

/**
 * Converts a type token to its string representation.
 */
string token_to_type(TokenType type) {
    switch (type) {
        case TokenType::TYPE_INT: return "int";
        case TokenType::TYPE_STR: return "str";
        case TokenType::TYPE_BOOL: return "bool";
        case TokenType::TYPE_CHAR: return "char";
        case TokenType::TYPE_F32: return "f32";
        case TokenType::TYPE_F64: return "f64";
        case TokenType::TYPE_U32: return "u32";
        case TokenType::TYPE_U64: return "u64";
        case TokenType::TYPE_CINT: return "cint";
        case TokenType::TYPE_CSTR: return "cstr";
        case TokenType::TYPE_VOID: return "void";
        default: return "";
    }
}

/**
 * Parses a type, including function types like fn(int, int) -> int.
 * Also handles qualified types like module.Type.
 * Returns the type as a string.
 */
string parse_type(ParserState& state) {
    // Function type: fn(params) -> return_type
    if (check(state, TokenType::FN)) {
        advance(state);
        consume(state, TokenType::LPAREN);

        string fn_type = "fn(";
        bool first = true;

        while (!check(state, TokenType::RPAREN) && !check(state, TokenType::EOF_TOKEN)) {
            if (!first) {
                fn_type += ", ";
            }
            first = false;

            // Parse parameter type (recursively for nested fn types)
            fn_type += parse_type(state);

            if (check(state, TokenType::COMMA)) {
                advance(state);
            }
        }

        consume(state, TokenType::RPAREN);
        fn_type += ")";

        // Parse optional return type
        if (check(state, TokenType::ARROW)) {
            advance(state);
            fn_type += " -> " + parse_type(state);
        }

        return fn_type;
    }

    // Primitive type
    if (is_type_token(state)) {
        string type = token_to_type(current(state).type);
        advance(state);
        return type;
    }

    // Channel<T> type
    if (check(state, TokenType::CHANNEL)) {
        advance(state);
        consume(state, TokenType::LT);
        string element_type = parse_type(state);
        consume(state, TokenType::GT);
        return "Channel<" + element_type + ">";
    }

    // List<T> type
    if (check(state, TokenType::LIST)) {
        advance(state);
        consume(state, TokenType::LT);
        string element_type = parse_type(state);
        consume(state, TokenType::GT);
        return "List<" + element_type + ">";
    }

    // Custom type (struct name), qualified type (module.Type), or generic (Type<T>)
    if (check(state, TokenType::IDENT)) {
        string type = current(state).value;
        advance(state);

        // Check for generic type parameters: Type<T>
        if (check(state, TokenType::LT)) {
            advance(state);
            type += "<" + parse_type(state) + ">";
            consume(state, TokenType::GT);
        }

        // Check for qualified type: module.Type
        if (check(state, TokenType::DOT)) {
            advance(state);
            type += "." + consume(state, TokenType::IDENT).value;
        }

        return type;
    }

    throw runtime_error("expected type at line " + to_string(current(state).line));
}

} // namespace parser
