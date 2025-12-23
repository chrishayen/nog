/**
 * @file lexer.cpp
 * @brief Lexer implementation for the Nog language.
 *
 * Implements the tokenization logic that converts raw source text into
 * a stream of tokens. Recognizes all Nog keywords, operators, and literals.
 */

#include "lexer.hpp"
#include <unordered_map>
#include <stdexcept>

using namespace std;

/**
 * @brief Mapping of keyword strings to their token types.
 * Used during identifier parsing to distinguish keywords from user identifiers.
 */
static unordered_map<string, TokenType> keywords = {
    {"fn", TokenType::FN},
    {"go", TokenType::GO},
    {"return", TokenType::RETURN},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"struct", TokenType::STRUCT},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"none", TokenType::NONE},
    {"is", TokenType::IS},
    {"import", TokenType::IMPORT},
    {"private", TokenType::PRIVATE},
    {"Channel", TokenType::CHANNEL},
    {"List", TokenType::LIST},
    {"select", TokenType::SELECT},
    {"case", TokenType::CASE},
    {"extern", TokenType::EXTERN},
    {"err", TokenType::ERR},
    {"fail", TokenType::FAIL},
    {"or", TokenType::OR},
    {"default", TokenType::DEFAULT},
    {"match", TokenType::MATCH},
    {"int", TokenType::TYPE_INT},
    {"str", TokenType::TYPE_STR},
    {"bool", TokenType::TYPE_BOOL},
    {"char", TokenType::TYPE_CHAR},
    {"f32", TokenType::TYPE_F32},
    {"f64", TokenType::TYPE_F64},
    {"u32", TokenType::TYPE_U32},
    {"u64", TokenType::TYPE_U64},
    {"cint", TokenType::TYPE_CINT},
    {"cstr", TokenType::TYPE_CSTR},
    {"void", TokenType::TYPE_VOID},
};

Lexer::Lexer(const string& source) : source(source) {}

/**
 * Returns the character at the current position, or '\0' if at end of source.
 */
char Lexer::current() {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

/**
 * Returns the next character (lookahead), or '\0' if at end.
 */
char Lexer::peek() {
    if (pos + 1 >= source.length()) return '\0';
    return source[pos + 1];
}

/**
 * Advances to the next character, incrementing line count on newlines.
 */
void Lexer::advance() {
    if (current() == '\n') line++;
    pos++;
}

/**
 * Skips over whitespace characters (space, tab, newline, carriage return).
 */
void Lexer::skip_whitespace() {
    while (current() == ' ' || current() == '\n' || current() == '\t' || current() == '\r') {
        advance();
    }
}

/**
 * Reads a double-quoted string literal. Assumes current char is '"'.
 * Consumes characters until the closing quote or end of input.
 */
Token Lexer::read_string() {
    int start_line = line;
    advance();  // skip opening quote
    string value;

    while (current() != '"' && current() != '\0') {
        value += current();
        advance();
    }

    advance();  // skip closing quote
    return {TokenType::STRING, value, start_line};
}

/**
 * Reads a single-quoted character literal. Assumes current char is '\''.
 * Format: 'c' where c is a single character.
 */
Token Lexer::read_char() {
    int start_line = line;
    advance();  // skip opening quote

    if (current() == '\0' || current() == '\'') {
        throw runtime_error("Empty character literal at line " + to_string(line));
    }

    string value(1, current());
    advance();  // consume the character

    if (current() != '\'') {
        throw runtime_error("Unterminated character literal at line " + to_string(line));
    }

    advance();  // skip closing quote
    return {TokenType::CHAR_LITERAL, value, start_line};
}

/**
 * Reads an identifier or keyword. Identifiers start with a letter or underscore
 * and contain letters, digits, or underscores. Checks against keyword table.
 */
Token Lexer::read_identifier() {
    int start_line = line;
    string value;
    while (isalnum(current()) || current() == '_') {
        value += current();
        advance();
    }

    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return {it->second, value, start_line};
    }
    return {TokenType::IDENT, value, start_line};
}

/**
 * Reads a numeric literal (integer or float).
 * Floats are detected by the presence of a decimal point.
 */
Token Lexer::read_number() {
    int start_line = line;
    string value;
    bool is_float = false;

    while (isdigit(current()) || current() == '.') {
        if (current() == '.') {
            if (is_float) break;  // second dot ends the number
            if (peek() == '.') break;  // .. is range operator, not float
            is_float = true;
        }
        value += current();
        advance();
    }

    return {is_float ? TokenType::FLOAT : TokenType::NUMBER, value, start_line};
}

/**
 * Main tokenization loop. Processes the entire source and returns all tokens.
 * Handles single and multi-character operators, literals, and keywords.
 */
vector<Token> Lexer::tokenize() {
    vector<Token> tokens;

    while (pos < source.length()) {
        skip_whitespace();
        if (current() == '\0') break;

        int start_line = line;

        if (current() == '(') {
            tokens.push_back({TokenType::LPAREN, "(", start_line});
            advance();
        } else if (current() == ')') {
            tokens.push_back({TokenType::RPAREN, ")", start_line});
            advance();
        } else if (current() == '{') {
            tokens.push_back({TokenType::LBRACE, "{", start_line});
            advance();
        } else if (current() == '}') {
            tokens.push_back({TokenType::RBRACE, "}", start_line});
            advance();
        } else if (current() == '[') {
            tokens.push_back({TokenType::LBRACKET, "[", start_line});
            advance();
        } else if (current() == ']') {
            tokens.push_back({TokenType::RBRACKET, "]", start_line});
            advance();
        } else if (current() == ',') {
            tokens.push_back({TokenType::COMMA, ",", start_line});
            advance();
        } else if (current() == '+') {
            tokens.push_back({TokenType::PLUS, "+", start_line});
            advance();
        } else if (current() == '*') {
            tokens.push_back({TokenType::STAR, "*", start_line});
            advance();
        } else if (current() == '/') {
            if (peek() == '/') {
                advance();  // skip first /
                advance();  // skip second /

                if (current() == '/') {
                    // Doc comment ///
                    advance();  // skip third /

                    // Skip leading space if present
                    if (current() == ' ') {
                        advance();
                    }

                    string doc_text;
                    while (current() != '\n' && current() != '\0') {
                        doc_text += current();
                        advance();
                    }

                    tokens.push_back({TokenType::DOC_COMMENT, doc_text, start_line});
                } else {
                    // Regular comment // - skip until newline
                    while (current() != '\n' && current() != '\0') {
                        advance();
                    }
                }
            } else {
                tokens.push_back({TokenType::SLASH, "/", start_line});
                advance();
            }
        } else if (current() == '-') {
            if (peek() == '>') {
                tokens.push_back({TokenType::ARROW, "->", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::MINUS, "-", start_line});
                advance();
            }
        } else if (current() == '.') {
            if (peek() == '.') {
                tokens.push_back({TokenType::DOTDOT, "..", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::DOT, ".", start_line});
                advance();
            }
        } else if (current() == ':') {
            if (peek() == '=') {
                tokens.push_back({TokenType::COLON_ASSIGN, ":=", start_line});
                advance();
                advance();
            } else if (peek() == ':') {
                tokens.push_back({TokenType::DOUBLE_COLON, "::", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::COLON, ":", start_line});
                advance();
            }
        } else if (current() == '=') {
            if (peek() == '=') {
                tokens.push_back({TokenType::EQ, "==", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::ASSIGN, "=", start_line});
                advance();
            }
        } else if (current() == '!') {
            if (peek() == '=') {
                tokens.push_back({TokenType::NE, "!=", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::NOT, "!", start_line});
                advance();
            }
        } else if (current() == '<') {
            if (peek() == '=') {
                tokens.push_back({TokenType::LE, "<=", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::LT, "<", start_line});
                advance();
            }
        } else if (current() == '>') {
            if (peek() == '=') {
                tokens.push_back({TokenType::GE, ">=", start_line});
                advance();
                advance();
            } else {
                tokens.push_back({TokenType::GT, ">", start_line});
                advance();
            }
        } else if (current() == '?') {
            tokens.push_back({TokenType::OPTIONAL, "?", start_line});
            advance();
        } else if (current() == ';') {
            tokens.push_back({TokenType::SEMICOLON, ";", start_line});
            advance();
        } else if (current() == '@') {
            tokens.push_back({TokenType::AT, "@", start_line});
            advance();
        } else if (current() == '&') {
            tokens.push_back({TokenType::AMPERSAND, "&", start_line});
            advance();
        } else if (current() == '"') {
            tokens.push_back(read_string());
        } else if (current() == '\'') {
            tokens.push_back(read_char());
        } else if (isdigit(current())) {
            tokens.push_back(read_number());
        } else if (isalpha(current()) || current() == '_') {
            tokens.push_back(read_identifier());
        } else {
            advance();
        }
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line});
    return tokens;
}
