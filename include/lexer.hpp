#pragma once
#include <string>
#include <vector>
#include "token.hpp"

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos = 0;
    int line = 1;

    char current();
    char peek();
    void advance();
    void skip_whitespace();
    Token read_string();
    Token read_identifier();
};
