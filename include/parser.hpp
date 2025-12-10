#pragma once
#include <vector>
#include <memory>
#include "token.hpp"
#include "ast.hpp"

using namespace std;

class Parser {
public:
    explicit Parser(const vector<Token>& tokens);
    unique_ptr<Program> parse();

private:
    vector<Token> tokens;
    size_t pos = 0;
    vector<string> struct_names;

    Token current();
    Token consume(TokenType type);
    bool check(TokenType type);
    bool is_type_token();
    void advance();

    unique_ptr<FunctionDef> parse_function();
    unique_ptr<StructDef> parse_struct_def(const string& name);
    unique_ptr<ASTNode> parse_statement();
    unique_ptr<FunctionCall> parse_function_call();
    unique_ptr<VariableDecl> parse_variable_decl();
    unique_ptr<VariableDecl> parse_inferred_decl();
    unique_ptr<ReturnStmt> parse_return();
    unique_ptr<ASTNode> parse_expression();
    unique_ptr<ASTNode> parse_primary();
    unique_ptr<ASTNode> parse_postfix(unique_ptr<ASTNode> left);
    unique_ptr<StructLiteral> parse_struct_literal(const string& name);
    string token_to_type(TokenType type);
    bool is_struct_type(const string& name);
};
