#pragma once
#include <string>

using namespace std;

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

    EOF_TOKEN
};

struct Token {
    TokenType type;
    string value;
    int line = 1;
};
