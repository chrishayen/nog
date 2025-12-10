#include "parser.hpp"
#include <stdexcept>

using namespace std;

Parser::Parser(const vector<Token>& tokens) : tokens(tokens) {}

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
        throw runtime_error("Unexpected token");
    }
    Token tok = current();
    advance();
    return tok;
}

bool Parser::is_type_token() {
    TokenType t = current().type;
    return t == TokenType::TYPE_INT || t == TokenType::TYPE_STR ||
           t == TokenType::TYPE_BOOL || t == TokenType::TYPE_CHAR ||
           t == TokenType::TYPE_F32 || t == TokenType::TYPE_F64 ||
           t == TokenType::TYPE_U32 || t == TokenType::TYPE_U64;
}

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

unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();

    while (!check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::FN)) {
            program->functions.push_back(parse_function());
        } else if (check(TokenType::IDENT)) {
            // Check for struct definition: Name :: struct { ... }
            size_t saved_pos = pos;
            string name = current().value;
            advance();
            if (check(TokenType::DOUBLE_COLON)) {
                advance();
                if (check(TokenType::STRUCT)) {
                    program->structs.push_back(parse_struct_def(name));
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

bool Parser::is_struct_type(const string& name) {
    for (const auto& s : struct_names) {
        if (s == name) return true;
    }
    return false;
}

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

    // inferred variable, assignment, or function call
    if (check(TokenType::IDENT)) {
        size_t saved_pos = pos;
        advance();

        if (check(TokenType::COLON_ASSIGN)) {
            pos = saved_pos;
            return parse_inferred_decl();
        }

        // assignment: x = expr
        if (check(TokenType::ASSIGN)) {
            pos = saved_pos;
            auto assign = make_unique<Assignment>();
            assign->name = consume(TokenType::IDENT).value;
            consume(TokenType::ASSIGN);
            assign->value = parse_expression();
            return assign;
        }

        pos = saved_pos;
        return parse_function_call();
    }

    advance();
    return nullptr;
}

unique_ptr<VariableDecl> Parser::parse_variable_decl() {
    auto decl = make_unique<VariableDecl>();
    decl->type = token_to_type(current().type);
    advance();
    decl->name = consume(TokenType::IDENT).value;
    consume(TokenType::ASSIGN);
    decl->value = parse_expression();
    return decl;
}

unique_ptr<VariableDecl> Parser::parse_inferred_decl() {
    auto decl = make_unique<VariableDecl>();
    decl->name = consume(TokenType::IDENT).value;
    consume(TokenType::COLON_ASSIGN);
    decl->value = parse_expression();
    return decl;
}

unique_ptr<ReturnStmt> Parser::parse_return() {
    consume(TokenType::RETURN);
    auto ret = make_unique<ReturnStmt>();
    ret->value = parse_expression();
    return ret;
}

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

unique_ptr<ASTNode> Parser::parse_expression() {
    return parse_comparison();
}

unique_ptr<ASTNode> Parser::parse_comparison() {
    auto left = parse_additive();

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
    return call;
}

unique_ptr<ASTNode> Parser::parse_postfix(unique_ptr<ASTNode> left) {
    while (check(TokenType::DOT)) {
        advance();
        string field_name = consume(TokenType::IDENT).value;

        auto access = make_unique<FieldAccess>();
        access->object = move(left);
        access->field_name = field_name;
        left = move(access);
    }
    return left;
}

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
