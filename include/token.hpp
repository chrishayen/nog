/**
 * @file token.hpp
 * @brief Token definitions for the Nog lexer.
 *
 * Defines the TokenType enum representing all lexical tokens in the Nog language,
 * and the Token struct that pairs a token type with its string value and source location.
 */

#pragma once
#include <string>

using namespace std;

/**
 * @brief All possible token types in the Nog language.
 *
 * Organized into categories: keywords, primitive types, literals,
 * punctuation/operators, and comparison operators.
 */
enum class TokenType {
    // Keywords
    FN,
    RETURN,
    TRUE,
    FALSE,
    STRUCT,
    IF,
    ELSE,
    WHILE,
    NONE,
    IS,
    IMPORT,
    PRIVATE,

    // Annotations
    AT,

    // Types
    TYPE_INT,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_F32,
    TYPE_F64,
    TYPE_U32,
    TYPE_U64,

    // Literals
    IDENT,
    STRING,
    NUMBER,
    FLOAT,

    // Punctuation
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,
    COLON,
    DOUBLE_COLON,
    DOT,
    ASSIGN,
    COLON_ASSIGN,
    ARROW,
    PLUS,
    MINUS,
    STAR,
    SLASH,

    // Comparison
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,

    OPTIONAL,
    SEMICOLON,

    EOF_TOKEN  ///< End of file marker
};

/**
 * @brief A single token from the source code.
 *
 * Produced by the Lexer during tokenization. Contains the token type,
 * the original string value from source, and the line number for error reporting.
 */
struct Token {
    TokenType type;   ///< The category/type of this token
    string value;     ///< The literal string value from source
    int line = 1;     ///< Source line number (1-indexed)
};
