/**
 * @file lexer.hpp
 * @brief Lexical analyzer for the Bishop language.
 *
 * The Lexer converts raw source code into a stream of tokens, handling
 * keywords, identifiers, literals, and operators.
 */

#pragma once
#include <string>
#include <vector>
#include "token.hpp"

using namespace std;

/**
 * @brief Converts Bishop source code into tokens.
 *
 * Scans through source character by character, recognizing keywords,
 * identifiers, numbers, strings, and operators. Tracks line numbers
 * for error reporting.
 *
 * @example
 *   Lexer lexer("fn main() { return 0; }");
 *   vector<Token> tokens = lexer.tokenize();
 */
class Lexer {
public:
    /**
     * @brief Constructs a lexer for the given source code.
     * @param source The complete Bishop source code to tokenize
     */
    explicit Lexer(const string& source);

    /**
     * @brief Tokenizes the entire source code.
     * @return Vector of tokens ending with EOF_TOKEN
     */
    vector<Token> tokenize();

private:
    string source;     ///< The source code being tokenized
    size_t pos = 0;    ///< Current position in source
    int line = 1;      ///< Current line number (1-indexed)

    char current();       ///< Returns current character or '\0' at end
    char peek();          ///< Returns next character or '\0' at end
    void advance();       ///< Moves to next character, tracking newlines
    void skip_whitespace();   ///< Skips spaces, tabs, newlines
    Token read_string();      ///< Reads a double-quoted string literal
    Token read_char();        ///< Reads a single-quoted character literal
    Token read_identifier();  ///< Reads identifier or keyword
    Token read_number();      ///< Reads integer or float literal
};
