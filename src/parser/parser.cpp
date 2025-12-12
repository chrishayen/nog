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
        Token tok = current();
        string msg = "unexpected token";

        if (!tok.value.empty()) {
            msg += " '" + tok.value + "'";
        }

        msg += " at line " + to_string(tok.line);
        throw runtime_error(msg);
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
 * @nog_syntax import
 * @category Imports
 * @order 1
 * @description Import a module to use its types and functions.
 * @syntax import module_name;
 * @example
 * import http;
 * import myproject.utils;
 * @note Use dot notation for nested modules
 */

/**
 * @nog_syntax Qualified Access
 * @category Imports
 * @order 2
 * @description Access imported module members with dot notation.
 * @syntax module.member
 * @example
 * resp := http.text("Hello");
 * result := utils.helper();
 */
unique_ptr<ImportStmt> Parser::parse_import() {
    consume(TokenType::IMPORT);

    // Parse module path: math or utils.helpers
    string module_path = consume(TokenType::IDENT).value;

    while (check(TokenType::DOT)) {
        advance();
        module_path += "." + consume(TokenType::IDENT).value;
    }

    consume(TokenType::SEMICOLON);

    auto import = make_unique<ImportStmt>(module_path);
    imported_modules.push_back(import->alias);
    return import;
}

/**
 * Parses visibility annotation: @private
 * Returns Private if found, Public otherwise.
 */
Visibility Parser::parse_visibility() {
    if (check(TokenType::AT)) {
        advance();

        if (check(TokenType::PRIVATE)) {
            advance();
            return Visibility::Private;
        }
    }

    return Visibility::Public;
}

/**
 * Checks if the given name is an imported module alias.
 */
bool Parser::is_imported_module(const string& name) {
    for (const auto& m : imported_modules) {
        if (m == name) return true;
    }

    return false;
}

/**
 * Pre-scans the token stream to collect function and struct names.
 * This allows forward references to functions defined later in the file.
 */
void Parser::prescan_definitions() {
    size_t saved_pos = pos;

    while (!check(TokenType::EOF_TOKEN)) {
        // Skip async keyword
        if (check(TokenType::ASYNC)) {
            advance();
        }

        // Function: fn name(...)
        if (check(TokenType::FN)) {
            advance();

            if (check(TokenType::IDENT)) {
                function_names.push_back(current().value);
            }
        }

        // Struct: Name :: struct
        if (check(TokenType::IDENT)) {
            string name = current().value;
            advance();

            if (check(TokenType::DOUBLE_COLON)) {
                advance();

                if (check(TokenType::STRUCT)) {
                    struct_names.push_back(name);
                }
            }
        }

        advance();
    }

    pos = saved_pos;
}

/**
 * Main parsing entry point. Parses the complete token stream into a Program AST.
 * Parses imports first, then function definitions (fn), struct definitions (Name :: struct),
 * and method definitions (Name :: method_name).
 */
unique_ptr<Program> Parser::parse() {
    auto program = make_unique<Program>();

    // Pre-scan to collect all function and struct names for forward references
    prescan_definitions();

    // Parse imports first (must be at top of file)
    while (check(TokenType::IMPORT)) {
        program->imports.push_back(parse_import());
    }

    while (!check(TokenType::EOF_TOKEN)) {
        // Collect any doc comments before the definition
        string doc = collect_doc_comments();

        // Check for visibility annotation
        Visibility vis = parse_visibility();

        // Check for async keyword (can precede fn or method definition)
        bool is_async = false;

        if (check(TokenType::ASYNC)) {
            is_async = true;
            advance();
        }

        if (check(TokenType::FN)) {
            auto fn = parse_function(vis, is_async);
            fn->doc_comment = doc;
            program->functions.push_back(move(fn));
            continue;
        }

        if (!check(TokenType::IDENT)) {
            advance();
            continue;
        }

        // Check for struct definition: Name :: struct { ... }
        // or method definition: [async] Name :: method_name(...) -> type { ... }
        size_t saved_pos = pos;
        Token name_tok = current();
        string name = name_tok.value;
        advance();

        if (!check(TokenType::DOUBLE_COLON)) {
            pos = saved_pos;
            advance();
            continue;
        }

        advance();

        if (check(TokenType::STRUCT)) {
            auto s = parse_struct_def(name, vis);
            s->doc_comment = doc;
            program->structs.push_back(move(s));
            continue;
        }

        if (check(TokenType::IDENT)) {
            auto m = parse_method_def(name, name_tok.line, vis, is_async);
            m->doc_comment = doc;
            program->methods.push_back(move(m));
            continue;
        }

        pos = saved_pos;
        advance();
    }

    return program;
}

/**
 * @nog_syntax Struct Definition
 * @category Structs
 * @order 1
 * @description Define a custom data structure.
 * @syntax Name :: struct { field type, ... }
 * @example
 * Person :: struct {
 *     name str,
 *     age int
 * }
 */

/**
 * @nog_syntax Field Access
 * @category Structs
 * @order 3
 * @description Access or modify struct fields using dot notation.
 * @syntax instance.field
 * @example
 * name := p.name;
 * p.age = 33;
 */
unique_ptr<StructDef> Parser::parse_struct_def(const string& name, Visibility vis) {
    consume(TokenType::STRUCT);
    consume(TokenType::LBRACE);

    auto def = make_unique<StructDef>();
    def->name = name;
    def->visibility = vis;
    struct_names.push_back(name);

    // Parse fields: name type, name type (with optional doc comments)
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        // Collect doc comment for this field
        string field_doc = collect_doc_comments();

        StructField field;
        field.name = consume(TokenType::IDENT).value;
        field.doc_comment = field_doc;

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
 * Checks if the given name is a known function.
 */
bool Parser::is_function_name(const string& name) {
    for (const auto& f : function_names) {
        if (f == name) return true;
    }
    return false;
}

/**
 * Parses a type, including function types like fn(int, int) -> int.
 * Also handles qualified types like module.Type.
 * Returns the type as a string.
 */
string Parser::parse_type() {
    // Function type: fn(params) -> return_type
    if (check(TokenType::FN)) {
        advance();
        consume(TokenType::LPAREN);

        string fn_type = "fn(";
        bool first = true;

        while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
            if (!first) {
                fn_type += ", ";
            }
            first = false;

            // Parse parameter type (recursively for nested fn types)
            fn_type += parse_type();

            if (check(TokenType::COMMA)) {
                advance();
            }
        }

        consume(TokenType::RPAREN);
        fn_type += ")";

        // Parse optional return type
        if (check(TokenType::ARROW)) {
            advance();
            fn_type += " -> " + parse_type();
        }

        return fn_type;
    }

    // Primitive type
    if (is_type_token()) {
        string type = token_to_type(current().type);
        advance();
        return type;
    }

    // Custom type (struct name) or qualified type (module.Type)
    if (check(TokenType::IDENT)) {
        string type = current().value;
        advance();

        // Check for qualified type: module.Type
        if (check(TokenType::DOT)) {
            advance();
            type += "." + consume(TokenType::IDENT).value;
        }

        return type;
    }

    throw runtime_error("expected type at line " + to_string(current().line));
}

/**
 * @nog_syntax Doc Comments
 * @category Visibility
 * @order 2
 * @description Document functions, structs, and fields with /// comments.
 * @syntax /// description
 * @example
 * /// This is a doc comment for the function
 * fn add(int a, int b) -> int {
 *     return a + b;
 * }
 *
 * /// Doc comment for struct
 * Person :: struct {
 *     /// Doc comment for field
 *     name str
 * }
 */
string Parser::collect_doc_comments() {
    string doc;

    while (check(TokenType::DOC_COMMENT)) {
        if (!doc.empty()) {
            doc += "\n";
        }
        doc += current().value;
        advance();
    }

    return doc;
}

/**
 * @nog_syntax Method Definition
 * @category Methods
 * @order 1
 * @description Define a method on a struct type.
 * @syntax Type :: name(self, params) -> return_type { }
 * @example
 * Person :: get_name(self) -> str {
 *     return self.name;
 * }
 *
 * Person :: greet(self, str greeting) -> str {
 *     return greeting + ", " + self.name;
 * }
 */

/**
 * @nog_syntax Method Call
 * @category Methods
 * @order 2
 * @description Call a method on a struct instance.
 * @syntax instance.method(args)
 * @example
 * name := p.get_name();
 * msg := p.greet("Hello");
 */

/**
 * @nog_syntax async method
 * @category Async
 * @order 3
 * @description Declare an asynchronous method on a struct.
 * @syntax async Type :: name(self, params) -> return_type { }
 * @example
 * async Counter :: get_value(self) -> int {
 *     return self.value;
 * }
 */
unique_ptr<MethodDef> Parser::parse_method_def(const string& struct_name, int line, Visibility vis, bool is_async) {
    // We're past "Name ::", now at method_name
    Token method_name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);

    auto method = make_unique<MethodDef>();
    method->struct_name = struct_name;
    method->name = method_name.value;
    method->line = line;
    method->visibility = vis;
    method->is_async = is_async;

    // Parse parameters (first should be 'self')
    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        FunctionParam param;

        // Check for 'self' (special case - type is the struct)
        if (current().value == "self") {
            param.type = struct_name;
            param.name = "self";
            advance();
        } else {
            param.type = parse_type();
            param.name = consume(TokenType::IDENT).value;
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
        method->return_type = parse_type();
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
 * @nog_syntax Function Declaration
 * @category Functions
 * @order 1
 * @description Declare a function with parameters and return type.
 * @syntax fn name(type param, ...) -> return_type { }
 * @example
 * fn add(int a, int b) -> int {
 *     return a + b;
 * }
 */

/**
 * @nog_syntax Void Function
 * @category Functions
 * @order 2
 * @description Function with no return type (void).
 * @syntax fn name(params) { }
 * @example
 * fn greet(str name) {
 *     print("Hello, " + name);
 * }
 */

/**
 * @nog_syntax Function References
 * @category Functions
 * @order 3
 * @description Pass functions as arguments to other functions.
 * @syntax fn(param_types) -> return_type
 * @example
 * fn apply_op(int x, int y, fn(int, int) -> int op) -> int {
 *     return op(x, y);
 * }
 * result := apply_op(3, 4, add);
 */

/**
 * @nog_syntax async fn
 * @category Async
 * @order 1
 * @description Declare an asynchronous function.
 * @syntax async fn name(params) -> return_type { }
 * @example
 * async fn fetch_data() -> int {
 *     return 42;
 * }
 */

/**
 * @nog_syntax @private
 * @category Visibility
 * @order 1
 * @description Mark a function or struct as private to its module.
 * @syntax @private fn/struct
 * @example
 * @private fn internal_helper() { }
 * @private MyStruct :: struct { }
 * @note Private members are not exported when the module is imported
 */
unique_ptr<FunctionDef> Parser::parse_function(Visibility vis, bool is_async) {
    consume(TokenType::FN);
    Token name = consume(TokenType::IDENT);
    consume(TokenType::LPAREN);

    auto func = make_unique<FunctionDef>();
    func->name = name.value;
    func->visibility = vis;
    func->is_async = is_async;

    // Parse parameters: fn foo(int a, int b) or fn foo(Person p) or fn foo(fn(int) -> int callback)
    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        FunctionParam param;
        param.type = parse_type();
        param.name = consume(TokenType::IDENT).value;
        func->params.push_back(param);

        if (check(TokenType::COMMA)) {
            advance();
        }
    }

    consume(TokenType::RPAREN);

    // Parse return type: -> int
    if (check(TokenType::ARROW)) {
        advance();
        func->return_type = parse_type();
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

    // select statement
    if (check(TokenType::SELECT)) {
        return parse_select();
    }

    // await expression as statement: await expr;
    if (check(TokenType::AWAIT)) {
        auto expr = parse_expression();
        consume(TokenType::SEMICOLON);
        return expr;
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

        // Check for qualified reference: module.func() or module.Type var = ...
        if (check(TokenType::DOT) && is_imported_module(ident)) {
            advance();
            string member_name = consume(TokenType::IDENT).value;
            string qualified_name = ident + "." + member_name;

            // Qualified function call: module.func()
            if (check(TokenType::LPAREN)) {
                auto call = make_unique<FunctionCall>();
                call->name = qualified_name;
                call->line = current().line;

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

            // Qualified type declaration: module.Type var = ...
            if (check(TokenType::IDENT) || check(TokenType::OPTIONAL)) {
                auto decl = make_unique<VariableDecl>();
                decl->type = qualified_name;

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

            // Fallback
            pos = saved_pos;
            return parse_function_call();
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
 * @nog_syntax Explicit Declaration
 * @category Variables
 * @order 1
 * @description Declare a variable with an explicit type.
 * @syntax type name = expr;
 * @example
 * int x = 42;
 * str name = "Chris";
 * bool flag = true;
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
 * @nog_syntax Type Inference
 * @category Variables
 * @order 2
 * @description Declare a variable with inferred type using :=.
 * @syntax name := expr;
 * @example
 * x := 100;
 * name := "Hello";
 * pi := 3.14;
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
 * @nog_syntax if
 * @category Control Flow
 * @order 1
 * @description Conditional branching with if and optional else.
 * @syntax if condition { ... } else { ... }
 * @example
 * if x > 10 {
 *     print("big");
 * } else {
 *     print("small");
 * }
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
 * @nog_syntax while
 * @category Control Flow
 * @order 2
 * @description Loop while a condition is true.
 * @syntax while condition { ... }
 * @example
 * i := 0;
 * while i < 5 {
 *     print(i);
 *     i = i + 1;
 * }
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
 * @nog_syntax select
 * @category Channels
 * @order 4
 * @description Wait on multiple channel operations.
 * @syntax select { case val := ch.recv() { ... } }
 * @example
 * select {
 *     case val := ch1.recv() {
 *         x := val + 1;
 *     }
 *     case val := ch2.recv() {
 *         x := val + 2;
 *     }
 * }
 */
unique_ptr<SelectStmt> Parser::parse_select() {
    int start_line = current().line;
    consume(TokenType::SELECT);
    consume(TokenType::LBRACE);

    auto stmt = make_unique<SelectStmt>();
    stmt->line = start_line;

    while (check(TokenType::CASE)) {
        auto select_case = make_unique<SelectCase>();
        select_case->line = current().line;
        advance();  // consume 'case'

        // Parse case: either "val := ch.recv()" or "ch.send(value)"
        // First check for binding: val := ...
        if (check(TokenType::IDENT)) {
            size_t saved_pos = pos;
            string first_ident = current().value;
            advance();

            if (check(TokenType::COLON_ASSIGN)) {
                // This is a recv with binding: val := ch.recv()
                select_case->binding_name = first_ident;
                advance();  // consume :=

                // Now parse the channel.recv()
                select_case->channel = parse_primary();
                select_case->channel = parse_postfix(move(select_case->channel));

                // The channel expression should be a MethodCall with method_name "recv"
                if (auto* method_call = dynamic_cast<MethodCall*>(select_case->channel.get())) {
                    select_case->operation = method_call->method_name;

                    // For recv, we need to keep the channel object, not the method call
                    auto channel_obj = move(method_call->object);
                    select_case->channel = move(channel_obj);
                }
            } else if (check(TokenType::DOT)) {
                // This is either ch.send(value) or ch.recv() without binding
                pos = saved_pos;

                // Parse channel expression (identifier + method call)
                auto expr = parse_primary();
                expr = parse_postfix(move(expr));

                if (auto* method_call = dynamic_cast<MethodCall*>(expr.get())) {
                    select_case->operation = method_call->method_name;

                    if (method_call->method_name == "send" && !method_call->args.empty()) {
                        select_case->send_value = move(method_call->args[0]);
                    }

                    select_case->channel = move(method_call->object);
                }
            } else {
                pos = saved_pos;
            }
        }

        // Parse the case body: { statements }
        consume(TokenType::LBRACE);

        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
            auto s = parse_statement();

            if (s) {
                select_case->body.push_back(move(s));
            }
        }

        consume(TokenType::RBRACE);
        stmt->cases.push_back(move(select_case));
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
 * @nog_syntax await
 * @category Async
 * @order 3
 * @description Await an async operation.
 * @syntax await expr
 * @example
 * result := await fetch_data();
 * await ch.send(42);
 * val := await ch.recv();
 */

/**
 * @nog_syntax Channel Creation
 * @category Channels
 * @order 1
 * @description Create a typed channel for communication between async tasks.
 * @syntax Channel<type>()
 * @example
 * ch := Channel<int>();
 * ch_str := Channel<str>();
 */
unique_ptr<ASTNode> Parser::parse_primary() {
    // Handle await expression: await expr
    if (check(TokenType::AWAIT)) {
        advance();
        auto await_expr = make_unique<AwaitExpr>();
        await_expr->value = parse_primary();
        await_expr->value = parse_postfix(move(await_expr->value));
        return await_expr;
    }

    // Handle channel creation: Channel<int>()
    if (check(TokenType::CHANNEL)) {
        int start_line = current().line;
        advance();
        consume(TokenType::LT);

        string element_type;

        if (is_type_token()) {
            element_type = token_to_type(current().type);
            advance();
        } else if (check(TokenType::IDENT)) {
            element_type = current().value;
            advance();
        }

        consume(TokenType::GT);
        consume(TokenType::LPAREN);
        consume(TokenType::RPAREN);

        auto channel = make_unique<ChannelCreate>();
        channel->element_type = element_type;
        channel->line = start_line;
        return channel;
    }

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

        // Check for qualified reference: module.item (e.g., math.add)
        if (check(TokenType::DOT) && is_imported_module(tok.value)) {
            advance();
            Token item_tok = consume(TokenType::IDENT);
            string item_name = item_tok.value;

            // Check if it's a qualified function call: module.func(args)
            if (check(TokenType::LPAREN)) {
                auto call = make_unique<FunctionCall>();
                call->name = tok.value + "." + item_name;  // Store as "module.func"
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

            // Check if it's a qualified struct literal: module.Type { ... }
            if (check(TokenType::LBRACE)) {
                return parse_struct_literal(tok.value + "." + item_name);
            }

            // Check if it's a qualified function reference: module.func (without parens)
            // This is for passing module functions as arguments
            auto fref = make_unique<FunctionRef>(tok.value + "." + item_name);
            fref->line = tok.line;
            return fref;
        }

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

        // Check if it's a function reference (function name without parentheses)
        if (is_function_name(tok.value)) {
            auto ref = make_unique<FunctionRef>(tok.value);
            ref->line = tok.line;
            return ref;
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
 * @nog_syntax Struct Instantiation
 * @category Structs
 * @order 2
 * @description Create an instance of a struct.
 * @syntax TypeName { field: value, field: value }
 * @example
 * p := Person { name: "Chris", age: 32 };
 * req := http.Request { method: "GET", path: "/", body: "" };
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
