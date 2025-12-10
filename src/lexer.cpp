#include "lexer.hpp"

Lexer::Lexer(const std::string& source) : source(source) {}

char Lexer::current() {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::peek() {
    if (pos + 1 >= source.length()) return '\0';
    return source[pos + 1];
}

void Lexer::advance() {
    if (current() == '\n') line++;
    pos++;
}

void Lexer::skip_whitespace() {
    while (current() == ' ' || current() == '\n' || current() == '\t' || current() == '\r') {
        advance();
    }
}

Token Lexer::read_string() {
    int start_line = line;
    advance(); // skip opening quote
    std::string value;
    while (current() != '"' && current() != '\0') {
        value += current();
        advance();
    }
    advance(); // skip closing quote
    return {TokenType::STRING, value, start_line};
}

Token Lexer::read_identifier() {
    int start_line = line;
    std::string value;
    while (std::isalnum(current()) || current() == '_') {
        value += current();
        advance();
    }

    if (value == "fn") {
        return {TokenType::FN, value, start_line};
    }
    return {TokenType::IDENT, value, start_line};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < source.length()) {
        skip_whitespace();

        if (current() == '\0') break;

        if (current() == '(') {
            tokens.push_back({TokenType::LPAREN, "(", line});
            advance();
        } else if (current() == ')') {
            tokens.push_back({TokenType::RPAREN, ")", line});
            advance();
        } else if (current() == '{') {
            tokens.push_back({TokenType::LBRACE, "{", line});
            advance();
        } else if (current() == '}') {
            tokens.push_back({TokenType::RBRACE, "}", line});
            advance();
        } else if (current() == ',') {
            tokens.push_back({TokenType::COMMA, ",", line});
            advance();
        } else if (current() == '"') {
            tokens.push_back(read_string());
        } else if (std::isalpha(current()) || current() == '_') {
            tokens.push_back(read_identifier());
        } else {
            advance();
        }
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line});
    return tokens;
}
