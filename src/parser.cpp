#include "parser.hpp"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

Token Parser::current() {
    if (pos >= tokens.size()) {
        return {TokenType::EOF_TOKEN, "", 0};
    }
    return tokens[pos];
}

bool Parser::check(TokenType type) {
    return current().type == type;
}

void Parser::advance() {
    pos++;
}

Token Parser::consume(TokenType type) {
    if (!check(type)) {
        throw std::runtime_error("Unexpected token");
    }
    Token tok = current();
    advance();
    return tok;
}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();

    while (!check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::FN)) {
            program->functions.push_back(parse_function());
        } else {
            advance();
        }
    }

    return program;
}

std::unique_ptr<FunctionDef> Parser::parse_function() {
    consume(TokenType::FN);
    Token name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);
    consume(TokenType::RPAREN);
    consume(TokenType::LBRACE);

    auto func = std::make_unique<FunctionDef>();
    func->name = name.value;

    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        func->body.push_back(parse_statement());
    }

    consume(TokenType::RBRACE);
    return func;
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
    if (check(TokenType::IDENT)) {
        return parse_function_call();
    }
    advance();
    return nullptr;
}

std::unique_ptr<FunctionCall> Parser::parse_function_call() {
    Token name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);

    auto call = std::make_unique<FunctionCall>();
    call->name = name.value;
    call->line = name.line;

    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::STRING)) {
            Token str = consume(TokenType::STRING);
            call->args.push_back(std::make_unique<StringLiteral>(str.value));
        } else if (check(TokenType::COMMA)) {
            advance();
        } else {
            advance();
        }
    }

    consume(TokenType::RPAREN);
    return call;
}
