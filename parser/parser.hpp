/**
 * @file parser.hpp
 * @brief Recursive descent parser for the Bishop language.
 *
 * Transforms a token stream into an Abstract Syntax Tree (AST). Uses
 * standalone functions in the parser namespace with explicit state passing.
 */

#pragma once
#include <vector>
#include <memory>
#include <string>
#include "lexer/token.hpp"
#include "ast.hpp"

/**
 * @brief Parser state passed to all parsing functions.
 *
 * Contains the token stream, current position, and lookup tables
 * for struct/function names and imported modules.
 */
struct ParserState {
    const std::vector<Token>& tokens;
    size_t pos = 0;
    std::vector<std::string> struct_names;
    std::vector<std::string> function_names;
    std::vector<std::string> imported_modules;

    explicit ParserState(const std::vector<Token>& toks) : tokens(toks) {}
};

namespace parser {

/**
 * @brief Parses all tokens into a complete Program AST.
 * @param state Parser state with token stream
 * @return The parsed program containing all definitions
 * @throws runtime_error on syntax errors
 */
std::unique_ptr<Program> parse(ParserState& state);

// Token navigation
Token current(const ParserState& state);
Token consume(ParserState& state, TokenType type);
bool check(const ParserState& state, TokenType type);
void advance(ParserState& state);

// Type utilities (parse_type.cpp)
bool is_type_token(const ParserState& state);
std::string token_to_type(TokenType type);
std::string parse_type(ParserState& state);

// Struct utilities (parse_struct.cpp)
bool is_struct_type(const ParserState& state, const std::string& name);
std::unique_ptr<StructDef> parse_struct_def(ParserState& state, const std::string& name, Visibility vis);
std::unique_ptr<StructLiteral> parse_struct_literal(ParserState& state, const std::string& name);

// Error utilities (parse_error.cpp)
std::unique_ptr<ErrorDef> parse_error_def(ParserState& state, const std::string& name, Visibility vis);

// Import utilities (parse_import.cpp)
bool is_imported_module(const ParserState& state, const std::string& name);
bool is_function_name(const ParserState& state, const std::string& name);
void prescan_definitions(ParserState& state);
std::string collect_doc_comments(ParserState& state);
std::unique_ptr<ImportStmt> parse_import(ParserState& state);

// Function parsing (parse_function.cpp)
Visibility parse_visibility(ParserState& state);
std::unique_ptr<FunctionDef> parse_function(ParserState& state, Visibility vis);
std::unique_ptr<ExternFunctionDef> parse_extern_function(ParserState& state, const std::string& library);
std::unique_ptr<MethodDef> parse_method_def(ParserState& state, const std::string& struct_name, Visibility vis);

// Go spawn parsing
std::unique_ptr<GoSpawn> parse_go_spawn(ParserState& state);

// Statement parsing (parse_statement.cpp)
std::unique_ptr<ASTNode> parse_statement(ParserState& state);
std::unique_ptr<FunctionCall> parse_function_call(ParserState& state);
std::unique_ptr<VariableDecl> parse_variable_decl(ParserState& state);
std::unique_ptr<VariableDecl> parse_inferred_decl(ParserState& state);
std::unique_ptr<ReturnStmt> parse_return(ParserState& state);
std::unique_ptr<FailStmt> parse_fail(ParserState& state);
std::unique_ptr<FailStmt> parse_fail_expr(ParserState& state);
std::unique_ptr<IfStmt> parse_if(ParserState& state);
std::unique_ptr<WhileStmt> parse_while(ParserState& state);
std::unique_ptr<ForStmt> parse_for(ParserState& state);
std::unique_ptr<SelectStmt> parse_select(ParserState& state);
std::unique_ptr<WithStmt> parse_with(ParserState& state);

// Expression parsing (parse_expression.cpp)
std::unique_ptr<ASTNode> parse_expression(ParserState& state);
std::unique_ptr<ASTNode> parse_or(ParserState& state);
std::unique_ptr<ASTNode> parse_default(ParserState& state);
std::unique_ptr<ASTNode> parse_comparison(ParserState& state);
std::unique_ptr<ASTNode> parse_additive(ParserState& state);
std::unique_ptr<ASTNode> parse_primary(ParserState& state);
std::unique_ptr<ASTNode> parse_postfix(ParserState& state, std::unique_ptr<ASTNode> left);

} // namespace parser
