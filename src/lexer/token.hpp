/**
 * @file token.hpp
 * @brief Token definitions for the Nog lexer.
 *
 * Defines the TokenType enum representing all lexical tokens in the Nog language,
 * and the Token struct that pairs a token type with its string value and source location.
 */

// =============================================================================
// Nog Syntax Documentation: Types
// =============================================================================

/**
 * @nog_syntax int
 * @category Types
 * @order 1
 * @description Integer type for whole numbers.
 * @syntax int
 * @example int x = 42;
 */

/**
 * @nog_syntax str
 * @category Types
 * @order 2
 * @description String type for text.
 * @syntax str
 * @example str name = "Chris";
 */

/**
 * @nog_syntax bool
 * @category Types
 * @order 3
 * @description Boolean type with values true or false.
 * @syntax bool
 * @example bool flag = true;
 */

/**
 * @nog_syntax char
 * @category Types
 * @order 4
 * @description Single character type.
 * @syntax char
 * @example char c = 'a';
 */

/**
 * @nog_syntax f32
 * @category Types
 * @order 5
 * @description 32-bit floating point number.
 * @syntax f32
 * @example f32 pi = 3.14;
 */

/**
 * @nog_syntax f64
 * @category Types
 * @order 6
 * @description 64-bit floating point number.
 * @syntax f64
 * @example f64 precise = 3.14159265359;
 */

/**
 * @nog_syntax u32
 * @category Types
 * @order 7
 * @description Unsigned 32-bit integer.
 * @syntax u32
 * @example u32 count = 100;
 */

/**
 * @nog_syntax u64
 * @category Types
 * @order 8
 * @description Unsigned 64-bit integer.
 * @syntax u64
 * @example u64 big_num = 9999999999;
 */

/**
 * @nog_syntax Optional Types
 * @category Types
 * @order 9
 * @description Optional type that can hold a value or none.
 * @syntax T?
 * @example
 * int? maybe_num = none;
 * int? value = 42;
 * if value is none {
 *     print("No value");
 * }
 * @note Use `is none` to check for none, or use a truthy check
 */

// =============================================================================
// Nog Syntax Documentation: Operators
// =============================================================================

/**
 * @nog_syntax Arithmetic Operators
 * @category Operators
 * @order 1
 * @description Mathematical operations.
 * @syntax + - * /
 * @example
 * sum := a + b;
 * diff := a - b;
 * prod := a * b;
 * quot := a / b;
 */

/**
 * @nog_syntax Comparison Operators
 * @category Operators
 * @order 2
 * @description Compare values.
 * @syntax == != < > <= >=
 * @example
 * if x == y { }
 * if x != y { }
 * if x < y { }
 * if x >= y { }
 */

/**
 * @nog_syntax String Concatenation
 * @category Operators
 * @order 3
 * @description Join strings with the + operator.
 * @syntax str + str
 * @example msg := "Hello, " + name + "!";
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
    ASYNC,
    AWAIT,
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
    CHANNEL,
    SELECT,
    CASE,

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

    // Documentation
    DOC_COMMENT,  ///< /// doc comment text

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
