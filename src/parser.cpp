/**
 * @file parser.cpp
 * @brief Parser implementation for the Nog language.
 *
 * Implements a recursive descent parser that transforms a token stream
 * into an Abstract Syntax Tree. Uses precedence climbing for expressions.
 */

#include "parser.hpp"
#include <stdexcept>

using namespace std;

Parser::Parser(const vector<Token>& tokens) : tokens(tokens) {}

/**
 * Returns the current token, or EOF_TOKEN if past end.
 */
Token Parser::current() {
    if (pos >= tokens.size()) {
        return {TokenType::EOF_TOKEN, "", 0};
    }
    return tokens[pos];
}

/**
 * Checks if current token matches the given type.
 */
bool Parser::check(TokenType type) {
    return current().type == type;
}

/**
 * Advances to the next token.
 */
void Parser::advance() {
    pos++;
}

/**
 * Consumes and returns the current token if it matches type.
 * Throws runtime_error if the token doesn't match.
 */
Token Parser::consume(TokenType type) {
    if (!check(type)) {
        throw runtime_error("Unexpected token");
    }
    Token tok = current();
    advance();
    return tok;
}

/**
 * Checks if current token is a primitive type keyword (int, str, bool, etc).
 */
bool Parser::is_type_token() {
    TokenType t = current().type;
    return t == TokenType::TYPE_INT || t == TokenType::TYPE_STR ||
           t == TokenType::TYPE_BOOL || t == TokenType::TYPE_CHAR ||
           t == TokenType::TYPE_F32 || t == TokenType::TYPE_F64 ||
           t == TokenType::TYPE_U32 || t == TokenType::TYPE_U64;
}

/**
 * Converts a type token to its string representation.
 */
string Parser::token_to_type(TokenType type) {
    switch (type) {
        case TokenType::TYPE_INT: return "int";
        case TokenType::TYPE_STR: return "str";
        case TokenType::TYPE_BOOL: return "bool";
        case TokenType::TYPE_CHAR: return "char";
        case TokenType::TYPE_F32: return "f32";
        case TokenType::TYPE_F64: return "f64";
        case TokenType::TYPE_U32: return "u32";
        case TokenType::TYPE_U64: return "u64";
        default: return "";
    }
}

/**
 * Main parsing entry point. Parses the complete token stream into a Program AST.
 * Recognizes function definitions (fn), struct definitions (Name :: struct),
 * and method definitions (Name :: method_name).
 */
unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();

    while (!check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::FN)) {
            program->functions.push_back(parse_function());
        } else if (check(TokenType::IDENT)) {
            // Check for struct definition: Name :: struct { ... }
            // or method definition: Name :: method_name(...) -> type { ... }
            size_t saved_pos = pos;
            Token name_tok = current();
            string name = name_tok.value;
            advance();

            if (check(TokenType::DOUBLE_COLON)) {
                advance();

                if (check(TokenType::STRUCT)) {
                    program->structs.push_back(parse_struct_def(name));
                    continue;
                }

                if (check(TokenType::IDENT)) {
                    // Method definition: Name :: method_name(...) -> type { ... }
                    program->methods.push_back(parse_method_def(name, name_tok.line));
                    continue;
                }
            }

            pos = saved_pos;
            advance();
        } else {
            advance();
        }
    }

    return program;
}

/**
 * Parses a struct definition: Name :: struct { field type, ... }
 * Registers the struct name for later type resolution.
 */
unique_ptr<StructDef> Parser::parse_struct_def(const string& name) {
    consume(TokenType::STRUCT);
    consume(TokenType::LBRACE);

    auto def = make_unique<StructDef>();
    def->name = name;
    struct_names.push_back(name);

    // Parse fields: name type, name type
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        StructField field;
        field.name = consume(TokenType::IDENT).value;

        if (is_type_token()) {
            field.type = token_to_type(current().type);
            advance();
        } else if (check(TokenType::IDENT)) {
            // Custom type (another struct)
            field.type = current().value;
            advance();
        }

        def->fields.push_back(field);

        if (check(TokenType::COMMA)) {
            advance();
        }
    }

    consume(TokenType::RBRACE);
    return def;
}

/**
 * Checks if the given name is a known struct type.
 */
bool Parser::is_struct_type(const string& name) {
    for (const auto& s : struct_names) {
        if (s == name) return true;
    }
    return false;
}

/**
 * Parses a method definition: StructName :: method_name(self, params) -> type { body }
 * The first parameter must be 'self', which becomes implicit 'this' in generated C++.
 */
unique_ptr<MethodDef> Parser::parse_method_def(const string& struct_name, int line) {
    // We're past "Name ::", now at method_name
    Token method_name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);

    auto method = make_unique<MethodDef>();
    method->struct_name = struct_name;
    method->name = method_name.value;
    method->line = line;

    // Parse parameters (first should be 'self')
    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        FunctionParam param;

        // Check for 'self' (special case - type is the struct)
        if (current().value == "self") {
            param.type = struct_name;
            param.name = "self";
            advance();
        } else if (is_type_token()) {
            param.type = token_to_type(current().type);
            advance();
            param.name = consume(TokenType::IDENT).value;
        } else if (check(TokenType::IDENT) && is_struct_type(current().value)) {
            // Struct type parameter
            param.type = current().value;
            advance();
            param.name = consume(TokenType::IDENT).value;
        } else {
            advance();
            continue;
        }

        method->params.push_back(param);

        if (check(TokenType::COMMA)) {
            advance();
        }
    }

    consume(TokenType::RPAREN);

    // Parse return type: -> type
    if (check(TokenType::ARROW)) {
        advance();

        if (is_type_token()) {
            method->return_type = token_to_type(current().type);
            advance();
        } else if (check(TokenType::IDENT)) {
            method->return_type = current().value;
            advance();
        }
    }

    consume(TokenType::LBRACE);

    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        auto stmt = parse_statement();

        if (stmt) {
            method->body.push_back(move(stmt));
        }
    }

    consume(TokenType::RBRACE);
    return method;
}

/**
 * Parses a function definition: fn name(type param, ...) -> return_type { body }
 */
unique_ptr<FunctionDef> Parser::parse_function() {
    consume(TokenType::FN);
    Token name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);

    auto func = make_unique<FunctionDef>();
    func->name = name.value;

    // Parse parameters: fn foo(int a, int b)
    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        if (is_type_token()) {
            FunctionParam param;
            param.type = token_to_type(current().type);
            advance();
            param.name = consume(TokenType::IDENT).value;
            func->params.push_back(param);

            if (check(TokenType::COMMA)) {
                advance();
            }
        } else {
            advance();
        }
    }

    consume(TokenType::RPAREN);

    // Parse return type: -> int
    if (check(TokenType::ARROW)) {
        advance();
        if (is_type_token()) {
            func->return_type = token_to_type(current().type);
            advance();
        }
    }

    consume(TokenType::LBRACE);

    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        auto stmt = parse_statement();
        if (stmt) {
            func->body.push_back(move(stmt));
        }
    }

    consume(TokenType::RBRACE);
    return func;
}

/**
 * Parses any statement. Dispatches based on the first token:
 * - return: parse return statement
 * - if: parse if/else
 * - while: parse while loop
 * - type keyword: parse typed variable declaration
 * - identifier: could be assignment, inferred decl, function call, or struct variable
 */
unique_ptr<ASTNode> Parser::parse_statement() {
    // return statement
    if (check(TokenType::RETURN)) {
        return parse_return();
    }

    // if statement
    if (check(TokenType::IF)) {
        return parse_if();
    }

    // while loop
    if (check(TokenType::WHILE)) {
        return parse_while();
    }

    // typed variable: int x = 5
    if (is_type_token()) {
        return parse_variable_decl();
    }

    // inferred variable, assignment, function call, method call, field assignment, or struct-typed variable
    if (check(TokenType::IDENT)) {
        size_t saved_pos = pos;
        string ident = current().value;
        advance();

        if (check(TokenType::COLON_ASSIGN)) {
            pos = saved_pos;
            return parse_inferred_decl();
        }

        // struct-typed variable: Person p = ... or Person? p = ...
        if (is_struct_type(ident) && (check(TokenType::IDENT) || check(TokenType::OPTIONAL))) {
            auto decl = make_unique<VariableDecl>();
            decl->type = ident;

            if (check(TokenType::OPTIONAL)) {
                decl->is_optional = true;
                advance();
            }

            decl->name = consume(TokenType::IDENT).value;
            consume(TokenType::ASSIGN);
            decl->value = parse_expression();
            consume(TokenType::SEMICOLON);
            return decl;
        }

        // field access: obj.field = value or obj.method()
        if (check(TokenType::DOT)) {
            advance();
            string field_name = consume(TokenType::IDENT).value;

            // Method call as statement: obj.method(args);
            if (check(TokenType::LPAREN)) {
                auto call = make_unique<MethodCall>();
                call->object = make_unique<VariableRef>(ident);
                call->method_name = field_name;

                consume(TokenType::LPAREN);

                while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
                    auto arg = parse_expression();

                    if (arg) {
                        call->args.push_back(move(arg));
                    }

                    if (check(TokenType::COMMA)) {
                        advance();
                    }
                }

                consume(TokenType::RPAREN);
                consume(TokenType::SEMICOLON);
                return call;
            }

            // Field assignment: obj.field = value;
            if (check(TokenType::ASSIGN)) {
                consume(TokenType::ASSIGN);
                auto assign = make_unique<FieldAssignment>();
                assign->object = make_unique<VariableRef>(ident);
                assign->field_name = field_name;
                assign->value = parse_expression();
                consume(TokenType::SEMICOLON);
                return assign;
            }
        }

        // assignment: x = expr
        if (check(TokenType::ASSIGN)) {
            pos = saved_pos;
            auto assign = make_unique<Assignment>();
            assign->name = consume(TokenType::IDENT).value;
            consume(TokenType::ASSIGN);
            assign->value = parse_expression();
            consume(TokenType::SEMICOLON);
            return assign;
        }

        pos = saved_pos;
        return parse_function_call();
    }

    advance();
    return nullptr;
}

/**
 * Parses a typed variable declaration: type name = expr; or type? name = expr;
 */
unique_ptr<VariableDecl> Parser::parse_variable_decl() {
    auto decl = make_unique<VariableDecl>();
    decl->type = token_to_type(current().type);
    advance();
    if (check(TokenType::OPTIONAL)) {
        decl->is_optional = true;
        advance();
    }
    decl->name = consume(TokenType::IDENT).value;
    consume(TokenType::ASSIGN);
    decl->value = parse_expression();
    consume(TokenType::SEMICOLON);
    return decl;
}

/**
 * Parses an inferred variable declaration: name := expr;
 * Type is inferred from the initializer expression.
 */
unique_ptr<VariableDecl> Parser::parse_inferred_decl() {
    auto decl = make_unique<VariableDecl>();
    decl->name = consume(TokenType::IDENT).value;
    consume(TokenType::COLON_ASSIGN);
    decl->value = parse_expression();
    consume(TokenType::SEMICOLON);
    return decl;
}

/**
 * Parses a return statement: return expr;
 */
unique_ptr<ReturnStmt> Parser::parse_return() {
    consume(TokenType::RETURN);
    auto ret = make_unique<ReturnStmt>();
    ret->value = parse_expression();
    consume(TokenType::SEMICOLON);
    return ret;
}

/**
 * Parses an if statement with optional else: if condition { then } else { else }
 */
unique_ptr<IfStmt> Parser::parse_if() {
    consume(TokenType::IF);
    auto stmt = make_unique<IfStmt>();
    stmt->condition = parse_expression();
    consume(TokenType::LBRACE);

    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        auto s = parse_statement();
        if (s) {
            stmt->then_body.push_back(move(s));
        }
    }
    consume(TokenType::RBRACE);

    if (check(TokenType::ELSE)) {
        advance();
        consume(TokenType::LBRACE);
        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
            auto s = parse_statement();
            if (s) {
                stmt->else_body.push_back(move(s));
            }
        }
        consume(TokenType::RBRACE);
    }

    return stmt;
}

/**
 * Parses a while loop: while condition { body }
 */
unique_ptr<WhileStmt> Parser::parse_while() {
    consume(TokenType::WHILE);
    auto stmt = make_unique<WhileStmt>();
    stmt->condition = parse_expression();
    consume(TokenType::LBRACE);

    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        auto s = parse_statement();
        if (s) {
            stmt->body.push_back(move(s));
        }
    }
    consume(TokenType::RBRACE);

    return stmt;
}

/**
 * Entry point for expression parsing. Delegates to comparison (lowest precedence).
 */
unique_ptr<ASTNode> Parser::parse_expression() {
    return parse_comparison();
}

/**
 * Parses comparison expressions: handles "is none" and comparison operators (==, !=, <, >, <=, >=).
 * Also chains multiple comparisons.
 */
unique_ptr<ASTNode> Parser::parse_comparison() {
    auto left = parse_additive();

    // Handle "x is none"
    if (check(TokenType::IS)) {
        advance();
        consume(TokenType::NONE);
        auto is_none = make_unique<IsNone>();
        is_none->value = move(left);
        return is_none;
    }

    while (check(TokenType::EQ) || check(TokenType::NE) ||
           check(TokenType::LT) || check(TokenType::GT) ||
           check(TokenType::LE) || check(TokenType::GE)) {
        string op = current().value;
        advance();
        auto right = parse_additive();

        auto binop = make_unique<BinaryExpr>();
        binop->op = op;
        binop->left = move(left);
        binop->right = move(right);
        left = move(binop);
    }

    return left;
}

/**
 * Parses additive/multiplicative expressions: +, -, *, /
 * Parses postfix (field access, method calls) on each operand.
 */
unique_ptr<ASTNode> Parser::parse_additive() {
    auto left = parse_primary();
    left = parse_postfix(move(left));

    while (check(TokenType::PLUS) || check(TokenType::MINUS) ||
           check(TokenType::STAR) || check(TokenType::SLASH)) {
        string op = current().value;
        advance();
        auto right = parse_primary();
        right = parse_postfix(move(right));

        auto binop = make_unique<BinaryExpr>();
        binop->op = op;
        binop->left = move(left);
        binop->right = move(right);
        left = move(binop);
    }

    return left;
}

/**
 * Parses primary expressions: literals, identifiers, function calls, struct literals.
 */
unique_ptr<ASTNode> Parser::parse_primary() {
    if (check(TokenType::NUMBER)) {
        Token tok = current();
        advance();
        return make_unique<NumberLiteral>(tok.value);
    }

    if (check(TokenType::FLOAT)) {
        Token tok = current();
        advance();
        return make_unique<FloatLiteral>(tok.value);
    }

    if (check(TokenType::STRING)) {
        Token tok = current();
        advance();
        return make_unique<StringLiteral>(tok.value);
    }

    if (check(TokenType::TRUE)) {
        advance();
        return make_unique<BoolLiteral>(true);
    }

    if (check(TokenType::FALSE)) {
        advance();
        return make_unique<BoolLiteral>(false);
    }

    if (check(TokenType::NONE)) {
        advance();
        return make_unique<NoneLiteral>();
    }

    if (check(TokenType::IDENT)) {
        Token tok = current();
        advance();
        // Check if it's a struct literal: TypeName { field: value, ... }
        if (check(TokenType::LBRACE) && is_struct_type(tok.value)) {
            return parse_struct_literal(tok.value);
        }
        // Check if it's a function call
        if (check(TokenType::LPAREN)) {
            auto call = make_unique<FunctionCall>();
            call->name = tok.value;
            call->line = tok.line;
            consume(TokenType::LPAREN);
            while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
                auto arg = parse_expression();
                if (arg) {
                    call->args.push_back(move(arg));
                }
                if (check(TokenType::COMMA)) {
                    advance();
                }
            }
            consume(TokenType::RPAREN);
            return call;
        }
        return make_unique<VariableRef>(tok.value);
    }

    advance();
    return nullptr;
}

/**
 * Parses a function call statement: name(args);
 * Includes the trailing semicolon.
 */
unique_ptr<FunctionCall> Parser::parse_function_call() {
    Token name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);

    auto call = make_unique<FunctionCall>();
    call->name = name.value;
    call->line = name.line;

    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        auto arg = parse_expression();
        if (arg) {
            call->args.push_back(move(arg));
        }
        if (check(TokenType::COMMA)) {
            advance();
        }
    }

    consume(TokenType::RPAREN);
    consume(TokenType::SEMICOLON);
    return call;
}

/**
 * Parses postfix operations: field access (obj.field) and method calls (obj.method()).
 * Chains multiple accesses like a.b.c() or a.b.c.d.
 */
unique_ptr<ASTNode> Parser::parse_postfix(unique_ptr<ASTNode> left) {
    while (check(TokenType::DOT)) {
        advance();
        Token member_tok = consume(TokenType::IDENT);
        string member_name = member_tok.value;

        // Check if it's a method call: obj.method(args)
        if (check(TokenType::LPAREN)) {
            auto call = make_unique<MethodCall>();
            call->object = move(left);
            call->method_name = member_name;
            call->line = member_tok.line;

            consume(TokenType::LPAREN);

            while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
                auto arg = parse_expression();

                if (arg) {
                    call->args.push_back(move(arg));
                }

                if (check(TokenType::COMMA)) {
                    advance();
                }
            }

            consume(TokenType::RPAREN);
            left = move(call);
        } else {
            // Field access: obj.field
            auto access = make_unique<FieldAccess>();
            access->object = move(left);
            access->field_name = member_name;
            left = move(access);
        }
    }

    return left;
}

/**
 * Parses a struct literal: TypeName { field: value, field: value }
 */
unique_ptr<StructLiteral> Parser::parse_struct_literal(const string& name) {
    consume(TokenType::LBRACE);

    auto lit = make_unique<StructLiteral>();
    lit->struct_name = name;

    // Parse field values: field: value, field: value
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        string field_name = consume(TokenType::IDENT).value;
        consume(TokenType::COLON);
        auto value = parse_expression();

        lit->field_values.push_back({field_name, move(value)});

        if (check(TokenType::COMMA)) {
            advance();
        }
    }

    consume(TokenType::RBRACE);
    return lit;
}
