/**
 * @file parser.hpp
 * @brief Recursive descent parser for the Nog language.
 *
 * Transforms a token stream into an Abstract Syntax Tree (AST). Handles
 * all Nog constructs including functions, structs, methods, control flow,
 * and expressions.
 */

#pragma once
#include <vector>
#include <memory>
#include "token.hpp"
#include "ast.hpp"

using namespace std;

/**
 * @brief Parses Nog tokens into an AST.
 *
 * Uses recursive descent parsing with separate methods for each grammar rule.
 * Expression parsing uses precedence climbing for operators.
 *
 * Grammar overview:
 *   program     -> import* (function | struct | method)*
 *   import      -> "import" module_path ";"
 *   function    -> visibility? "fn" IDENT "(" params ")" ("->" type)? "{" statements "}"
 *   struct      -> visibility? IDENT "::" "struct" "{" fields "}"
 *   method      -> visibility? IDENT "::" IDENT "(" params ")" ("->" type)? "{" statements "}"
 *   visibility  -> "@private"
 *   statement   -> var_decl | assignment | return | if | while | call
 *   expression  -> comparison
 *   comparison  -> additive (("==" | "!=" | ...) additive)*
 *   additive    -> primary (("+" | "-" | "*" | "/") primary)*
 *   primary     -> literal | IDENT | qualified_ref | call | struct_literal | "(" expression ")"
 */
class Parser {
public:
    /**
     * @brief Constructs a parser for the given token stream.
     * @param tokens Vector of tokens from the lexer
     */
    explicit Parser(const vector<Token>& tokens);

    /**
     * @brief Parses all tokens into a complete Program AST.
     * @return The parsed program containing all definitions
     * @throws runtime_error on syntax errors
     */
    unique_ptr<Program> parse();

private:
    vector<Token> tokens;          ///< Token stream to parse
    size_t pos = 0;                ///< Current position in tokens
    vector<string> struct_names;   ///< Known struct names for type resolution
    vector<string> imported_modules;  ///< Imported module aliases for qualified reference resolution

    // Token navigation
    Token current();               ///< Returns current token
    Token consume(TokenType type); ///< Consumes expected token or throws
    bool check(TokenType type);    ///< Checks if current token matches
    bool is_type_token();          ///< Checks if current is a type keyword
    void advance();                ///< Moves to next token

    // Definition parsing
    unique_ptr<ImportStmt> parse_import();                                 ///< Parses import statement
    Visibility parse_visibility();                                         ///< Parses @private annotation
    unique_ptr<FunctionDef> parse_function(Visibility vis);                ///< Parses fn definition
    unique_ptr<StructDef> parse_struct_def(const string& name, Visibility vis);  ///< Parses struct definition
    unique_ptr<MethodDef> parse_method_def(const string& struct_name, int line, Visibility vis);  ///< Parses method definition
    bool is_imported_module(const string& name);                           ///< Checks if name is an imported module

    // Statement parsing
    unique_ptr<ASTNode> parse_statement();         ///< Parses any statement
    unique_ptr<FunctionCall> parse_function_call();   ///< Parses function call statement
    unique_ptr<VariableDecl> parse_variable_decl();   ///< Parses typed variable declaration
    unique_ptr<VariableDecl> parse_inferred_decl();   ///< Parses := inference declaration
    unique_ptr<ReturnStmt> parse_return();            ///< Parses return statement
    unique_ptr<IfStmt> parse_if();                    ///< Parses if/else statement
    unique_ptr<WhileStmt> parse_while();              ///< Parses while loop

    // Expression parsing (precedence climbing)
    unique_ptr<ASTNode> parse_expression();        ///< Entry point for expressions
    unique_ptr<ASTNode> parse_comparison();        ///< Handles comparison operators
    unique_ptr<ASTNode> parse_additive();          ///< Handles +, -, *, /
    unique_ptr<ASTNode> parse_primary();           ///< Handles literals, identifiers
    unique_ptr<ASTNode> parse_postfix(unique_ptr<ASTNode> left);  ///< Handles . access/calls
    unique_ptr<StructLiteral> parse_struct_literal(const string& name);  ///< Parses { field: value }

    // Helpers
    string token_to_type(TokenType type);          ///< Converts type token to string
    bool is_struct_type(const string& name);       ///< Checks if name is a known struct
};
