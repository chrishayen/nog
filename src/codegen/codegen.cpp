/**
 * @file codegen.cpp
 * @brief C++ code generation for the Nog language.
 *
 * Transforms a type-checked AST into C++ source code. Uses the runtime
 * helper functions (nog::runtime) to generate idiomatic C++ output.
 */

/**
 * @nog_fn print
 * @module builtins
 * @description Prints values to standard output, followed by a newline.
 * @param args ... - One or more values to print (separated by spaces)
 * @example
 * print("Hello, World!");
 * print("x =", x, "y =", y);
 */

/**
 * @nog_fn assert_eq
 * @module builtins
 * @description Asserts that two values are equal. Only available in test mode.
 * @param expected T - The expected value
 * @param actual T - The actual value to compare
 * @note Fails the test with line number and values if not equal
 * @example
 * assert_eq(5, add(2, 3));
 * assert_eq("hello", greet());
 */

/**
 * @nog_struct Channel
 * @module builtins
 * @description A typed channel for communication between async functions.
 * @example
 * ch := Channel<int>();
 * await ch.send(42);
 * val := await ch.recv();
 */

/**
 * @nog_method send
 * @type Channel
 * @async
 * @description Sends a value through the channel.
 * @param value T - The value to send
 * @example await ch.send(42);
 */

/**
 * @nog_method recv
 * @type Channel
 * @async
 * @description Receives a value from the channel.
 * @returns T - The received value
 * @example val := await ch.recv();
 */

#include "codegen.hpp"
#include "emit/runtime.hpp"
#include "stdlib/http.hpp"

using namespace std;
namespace rt = nog::runtime;

/**
 * Emits C++ code for an expression AST node. Handles all expression types:
 * literals, variable refs, binary expressions, function calls, method calls,
 * field access, struct literals, etc.
 */
string CodeGen::emit(const ASTNode& node) {
    if (auto* lit = dynamic_cast<const StringLiteral*>(&node)) {
        return rt::string_literal(lit->value);
    }
    if (auto* lit = dynamic_cast<const NumberLiteral*>(&node)) {
        return rt::number_literal(lit->value);
    }
    if (auto* lit = dynamic_cast<const FloatLiteral*>(&node)) {
        return rt::float_literal(lit->value);
    }
    if (auto* lit = dynamic_cast<const BoolLiteral*>(&node)) {
        return rt::bool_literal(lit->value);
    }
    if (dynamic_cast<const NoneLiteral*>(&node)) {
        return rt::none_literal();
    }
    if (auto* ref = dynamic_cast<const VariableRef*>(&node)) {
        return rt::variable_ref(ref->name);
    }
    if (auto* fref = dynamic_cast<const FunctionRef*>(&node)) {
        // Function reference - emit the function name (possibly qualified with ::)
        string func_name = fref->name;
        size_t dot_pos = func_name.find('.');

        if (dot_pos != string::npos) {
            func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
        }

        return func_name;
    }
    if (auto* expr = dynamic_cast<const BinaryExpr*>(&node)) {
        return rt::binary_expr(emit(*expr->left), expr->op, emit(*expr->right));
    }
    if (auto* expr = dynamic_cast<const IsNone*>(&node)) {
        return rt::is_none(emit(*expr->value));
    }
    if (auto* expr = dynamic_cast<const AwaitExpr*>(&node)) {
        // Special handling for channel recv: await ch.recv() -> std::get<1>(co_await ch.async_receive(...))
        if (auto* mcall = dynamic_cast<const MethodCall*>(expr->value.get())) {
            if (mcall->method_name == "recv") {
                return "std::get<1>(co_await " + emit(*mcall->object) +
                       ".async_receive(asio::as_tuple(asio::use_awaitable)))";
            }
        }

        // Wrap in parentheses since co_await has low precedence in C++
        return "(co_await " + emit(*expr->value) + ")";
    }
    if (auto* channel = dynamic_cast<const ChannelCreate*>(&node)) {
        // Channel<T>() -> asio::experimental::channel<void(asio::error_code, T)>(ex, 1)
        string cpp_type = rt::map_type(channel->element_type);
        return "asio::experimental::channel<void(asio::error_code, " + cpp_type + ")>(co_await asio::this_coro::executor, 1)";
    }
    if (auto* qref = dynamic_cast<const QualifiedRef*>(&node)) {
        // Qualified reference: module.name -> module::name
        return qref->module_name + "::" + qref->name;
    }

    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        vector<string> args;
        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }

        // Handle qualified function call: module.func -> module::func
        string func_name = call->name;
        size_t dot_pos = func_name.find('.');

        if (dot_pos != string::npos) {
            func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
        }

        return rt::function_call(func_name, args);
    }
    if (auto* decl = dynamic_cast<const VariableDecl*>(&node)) {
        return rt::variable_decl(decl->type, decl->name, emit(*decl->value), decl->is_optional);
    }
    if (auto* assign = dynamic_cast<const Assignment*>(&node)) {
        return rt::assignment(assign->name, emit(*assign->value));
    }
    if (auto* ret = dynamic_cast<const ReturnStmt*>(&node)) {
        string keyword = in_async_function ? "co_return" : "return";
        return keyword + " " + emit(*ret->value) + ";";
    }
    if (auto* lit = dynamic_cast<const StructLiteral*>(&node)) {
        vector<pair<string, string>> field_values;
        for (const auto& [name, value] : lit->field_values) {
            field_values.push_back({name, emit(*value)});
        }

        // Handle qualified struct name: module.Type -> module::Type
        string struct_name = lit->struct_name;
        size_t dot_pos = struct_name.find('.');

        if (dot_pos != string::npos) {
            struct_name = struct_name.substr(0, dot_pos) + "::" + struct_name.substr(dot_pos + 1);
        }

        return rt::struct_literal(struct_name, field_values);
    }
    if (auto* access = dynamic_cast<const FieldAccess*>(&node)) {
        // Handle self.field -> this->field in methods
        if (auto* ref = dynamic_cast<const VariableRef*>(access->object.get())) {
            if (ref->name == "self") {
                return "this->" + access->field_name;
            }
        }

        return rt::field_access(emit(*access->object), access->field_name);
    }

    if (auto* call = dynamic_cast<const MethodCall*>(&node)) {
        vector<string> args;

        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }

        // Handle self.method() -> this->method() in methods
        if (auto* ref = dynamic_cast<const VariableRef*>(call->object.get())) {
            if (ref->name == "self") {
                string args_str;

                for (size_t i = 0; i < args.size(); i++) {
                    if (i > 0) args_str += ", ";
                    args_str += args[i];
                }

                return "this->" + call->method_name + "(" + args_str + ")";
            }
        }

        // Handle channel methods using ASIO experimental channel
        if (call->method_name == "send") {
            string val = args.empty() ? "" : args[0];
            return emit(*call->object) + ".async_send(asio::error_code{}, " + val + ", asio::use_awaitable)";
        }

        if (call->method_name == "recv") {
            // Return raw async call - will be wrapped by AwaitExpr or used in select
            return emit(*call->object) + ".async_receive(asio::as_tuple(asio::use_awaitable))";
        }

        return rt::method_call(emit(*call->object), call->method_name, args);
    }

    if (auto* fa = dynamic_cast<const FieldAssignment*>(&node)) {
        // Handle self.field = value -> this->field = value in methods
        if (auto* ref = dynamic_cast<const VariableRef*>(fa->object.get())) {
            if (ref->name == "self") {
                return "this->" + fa->field_name + " = " + emit(*fa->value);
            }
        }

        return rt::field_assignment(emit(*fa->object), fa->field_name, emit(*fa->value));
    }

    return "";
}

/**
 * Checks if any function in the program is async.
 */
static bool has_async_functions(const Program& program) {
    for (const auto& fn : program.functions) {
        if (fn->is_async) {
            return true;
        }
    }

    return false;
}

/**
 * Checks if the program uses channels (simple heuristic: async functions assumed to use channels).
 */
static bool has_channels(const Program& program) {
    // For now, assume async functions may use channels
    return has_async_functions(program);
}

/**
 * Checks if any function has function type parameters (requires <functional>).
 */
static bool has_function_types(const Program& program) {
    for (const auto& fn : program.functions) {
        for (const auto& param : fn->params) {
            if (param.type.rfind("fn(", 0) == 0) {
                return true;
            }
        }
    }

    for (const auto& m : program.methods) {
        for (const auto& param : m->params) {
            if (param.type.rfind("fn(", 0) == 0) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Checks if the program imports the http module.
 */
static bool has_http_import(const map<string, const Module*>& imports) {
    return imports.find("http") != imports.end();
}

/**
 * Main code generation entry point. Generates complete C++ source with headers,
 * structs, and functions. In test mode, generates a test harness instead.
 */
string CodeGen::generate(const unique_ptr<Program>& program, bool test_mode) {
    this->test_mode = test_mode;
    this->current_program = program.get();
    this->imported_modules.clear();

    if (test_mode) {
        return generate_test_harness(program);
    }

    // std.hpp PCH includes iostream, string, optional, functional, memory, etc.
    string out = "#include <nog/std.hpp>\n";

    if (has_async_functions(*program)) {
        out += "#include <asio.hpp>\n#include <asio/awaitable.hpp>\n";
    }

    if (has_channels(*program)) {
        out += "#include <asio/experimental/channel.hpp>\n";
        out += "#include <asio/experimental/awaitable_operators.hpp>\n";
        out += "using namespace asio::experimental::awaitable_operators;\n";
    }

    out += "\n";

    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    for (const auto& fn : program->functions) {
        out += generate_function(*fn);
    }

    return out;
}

/**
 * Generates C++ code for a program with imported modules.
 * Imported modules are generated as C++ namespaces.
 */
string CodeGen::generate_with_imports(
    const unique_ptr<Program>& program,
    const map<string, const Module*>& imports,
    bool test_mode
) {
    this->test_mode = test_mode;
    this->current_program = program.get();
    this->imported_modules = imports;

    string out;

    // For precompiled header support, PCH must be included FIRST
    // GCC only uses PCH for the first #include in the translation unit
    if (has_http_import(imports)) {
        // http.hpp includes std.hpp and all ASIO headers
        out += "#include <nog/http.hpp>\n";
        out += "using namespace asio::experimental::awaitable_operators;\n";
        out += "\n";
    } else {
        // std.hpp includes iostream, string, optional, functional, memory, etc.
        out += "#include <nog/std.hpp>\n";

        if (has_async_functions(*program)) {
            out += "#include <asio.hpp>\n#include <asio/awaitable.hpp>\n";
        }

        if (has_channels(*program)) {
            out += "#include <asio/experimental/channel.hpp>\n";
            out += "#include <asio/experimental/awaitable_operators.hpp>\n";
            out += "using namespace asio::experimental::awaitable_operators;\n";
        }

        out += "\n";
    }

    // Generate test harness infrastructure if in test mode
    if (test_mode) {
        out += "int _failures = 0;\n\n";
        out += "template<typename T, typename U>\n";
        out += "void _assert_eq(T a, U b, int line) {\n";
        out += "\tif (a != b) {\n";
        out += "\t\tstd::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
        out += "\t\t_failures++;\n";
        out += "\t}\n";
        out += "}\n\n";
    }

    // Generate namespaces for imported modules
    for (const auto& [alias, mod] : imports) {
        out += generate_module_namespace(alias, *mod);
    }

    // Generate structs from the main program
    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    // Generate functions from the main program
    for (const auto& fn : program->functions) {
        out += generate_function(*fn);
    }

    // Generate test harness main if in test mode
    if (test_mode) {
        vector<string> test_names;

        for (const auto& fn : program->functions) {
            if (fn->name.rfind("test_", 0) == 0) {
                test_names.push_back(fn->name);
            }
        }

        out += "\nint main() {\n";

        for (const auto& name : test_names) {
            out += "\t" + name + "();\n";
        }

        out += "\treturn _failures;\n";
        out += "}\n";
    }

    return out;
}

/**
 * Generates a C++ namespace for an imported module.
 * Only includes public structs and functions.
 * Built-in modules (like http) use special runtime code.
 */
string CodeGen::generate_module_namespace(const string& name, const Module& module) {
    // Built-in http module has its own runtime implementation
    if (name == "http") {
        return nog::stdlib::generate_http_runtime();
    }

    string out = "namespace " + name + " {\n\n";

    // Save and set current program for method lookup
    const Program* saved_program = current_program;
    current_program = module.ast.get();

    // Generate public structs
    for (const auto* s : module.get_public_structs()) {
        out += generate_struct(*s) + "\n\n";
    }

    // Generate public functions
    for (const auto* f : module.get_public_functions()) {
        out += generate_function(*f);
    }

    // Restore current program
    current_program = saved_program;

    out += "} // namespace " + name + "\n\n";
    return out;
}

/**
 * Generates a C++ struct definition with fields and any associated methods.
 * Methods become member functions within the struct.
 */
string CodeGen::generate_struct(const StructDef& def) {
    vector<pair<string, string>> fields;

    for (const auto& f : def.fields) {
        fields.push_back({f.name, f.type});
    }

    // Collect methods for this struct
    vector<string> method_bodies;

    if (current_program) {
        for (const auto& m : current_program->methods) {
            if (m->struct_name == def.name) {
                method_bodies.push_back(generate_method(*m));
            }
        }
    }

    if (method_bodies.empty()) {
        return rt::struct_def(def.name, fields);
    }

    return rt::struct_def_with_methods(def.name, fields, method_bodies);
}

/**
 * Generates a C++ member function from a method definition.
 * Skips the 'self' parameter (becomes implicit 'this').
 */
string CodeGen::generate_method(const MethodDef& method) {
    // Track if we're in an async method for co_return generation
    in_async_function = method.is_async;

    // Params: skip 'self' since it becomes implicit 'this'
    vector<pair<string, string>> params;

    for (size_t i = 1; i < method.params.size(); i++) {
        params.push_back({method.params[i].type, method.params[i].name});
    }

    vector<string> body;

    for (const auto& stmt : method.body) {
        body.push_back(generate_statement(*stmt));
    }

    string result = rt::method_def(method.name, params, method.return_type, body, method.is_async);
    in_async_function = false;
    return result;
}

/**
 * Generates a C++ function. Handles 'main' specially (adds return 0 if no return type).
 * Maps Nog types to C++ types for parameters and return type.
 * Async functions generate asio::awaitable<T> return type and use co_return.
 * Async main is wrapped with io_context runner.
 */
string CodeGen::generate_function(const FunctionDef& fn) {
    bool is_main = (fn.name == "main" && !test_mode);
    bool is_async_main = is_main && fn.is_async;

    // Track if we're in an async function for co_return generation
    in_async_function = fn.is_async;

    vector<rt::FunctionParam> params;
    for (const auto& p : fn.params) {
        params.push_back({p.type, p.name});
    }

    vector<string> body;
    for (const auto& stmt : fn.body) {
        body.push_back(generate_statement(*stmt));
    }

    string out;

    if (is_async_main) {
        // Generate the async function as _async_main()
        string rt_type = fn.return_type.empty() ? "" : fn.return_type;
        out = rt::function_def("_async_main", params, rt_type, body, true);

        // Generate int main() that runs the coroutine
        out += "\nint main() {\n";
        out += "\tasio::io_context io_context;\n";
        out += "\tasio::co_spawn(io_context, _async_main(), asio::detached);\n";
        out += "\tio_context.run();\n";
        out += "\treturn 0;\n";
        out += "}\n";
    } else {
        string rt_type = is_main ? "int" : fn.return_type;
        out = rt::function_def(fn.name, params, rt_type, body, fn.is_async);

        // For main without explicit return, add return 0
        if (is_main && fn.return_type.empty()) {
            // Insert return 0 before closing brace
            auto pos = out.rfind("}\n");

            if (pos != string::npos) {
                out.insert(pos, "\treturn 0;\n");
            }
        }
    }

    in_async_function = false;
    return out;
}

/**
 * Generates C++ code for a statement. Handles print(), assert_eq() (in test mode),
 * if/while statements, method calls, field assignments, and other statements.
 */
string CodeGen::generate_statement(const ASTNode& node) {
    if (auto* call = dynamic_cast<const FunctionCall*>(&node)) {
        if (call->name == "print") {
            vector<string> args;
            for (const auto& arg : call->args) {
                args.push_back(emit(*arg));
            }
            return rt::print_multi(args) + ";";
        }

        if (call->name == "assert_eq" && test_mode && call->args.size() >= 2) {
            return rt::assert_eq(emit(*call->args[0]), emit(*call->args[1]), call->line) + ";";
        }

        vector<string> args;
        for (const auto& arg : call->args) {
            args.push_back(emit(*arg));
        }

        // Handle qualified function call: module.func -> module::func
        string func_name = call->name;
        size_t dot_pos = func_name.find('.');

        if (dot_pos != string::npos) {
            func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
        }

        return rt::function_call(func_name, args) + ";";
    }
    if (auto* stmt = dynamic_cast<const IfStmt*>(&node)) {
        vector<string> then_body;
        for (const auto& s : stmt->then_body) {
            then_body.push_back(generate_statement(*s));
        }
        vector<string> else_body;
        for (const auto& s : stmt->else_body) {
            else_body.push_back(generate_statement(*s));
        }
        return rt::if_stmt(emit(*stmt->condition), then_body, else_body);
    }
    if (auto* stmt = dynamic_cast<const WhileStmt*>(&node)) {
        vector<string> body;

        for (const auto& s : stmt->body) {
            body.push_back(generate_statement(*s));
        }

        return rt::while_stmt(emit(*stmt->condition), body);
    }

    if (auto* select_stmt = dynamic_cast<const SelectStmt*>(&node)) {
        return generate_select(*select_stmt);
    }

    if (auto* await_expr = dynamic_cast<const AwaitExpr*>(&node)) {
        return emit(node) + ";";
    }

    if (auto* call = dynamic_cast<const MethodCall*>(&node)) {
        return emit(node) + ";";
    }

    if (auto* fa = dynamic_cast<const FieldAssignment*>(&node)) {
        return emit(node) + ";";
    }

    return emit(node);
}

/**
 * Generates a test harness main() that runs all test_* functions and tracks
 * failures. Includes the _assert_eq template for test assertions.
 */
string CodeGen::generate_test_harness(const unique_ptr<Program>& program) {
    this->current_program = program.get();

    // std.hpp PCH includes iostream, string, optional, functional, memory, etc.
    string out = "#include <nog/std.hpp>\n";

    if (has_async_functions(*program)) {
        out += "#include <asio.hpp>\n#include <asio/awaitable.hpp>\n";
    }

    if (has_channels(*program)) {
        out += "#include <asio/experimental/channel.hpp>\n";
        out += "#include <asio/experimental/awaitable_operators.hpp>\n";
        out += "using namespace asio::experimental::awaitable_operators;\n";
    }

    out += "\n";

    out += "int _failures = 0;\n\n";
    out += "template<typename T, typename U>\n";
    out += "void _assert_eq(T a, U b, int line) {\n";
    out += "\tif (a != b) {\n";
    out += "\t\tstd::cerr << \"line \" << line << \": FAIL: \" << a << \" != \" << b << std::endl;\n";
    out += "\t\t_failures++;\n";
    out += "\t}\n";
    out += "}\n\n";

    for (const auto& s : program->structs) {
        out += generate_struct(*s) + "\n\n";
    }

    vector<pair<string, bool>> test_funcs;  // (name, is_async)
    for (const auto& fn : program->functions) {
        if (fn->name.rfind("test_", 0) == 0) {
            test_funcs.push_back({fn->name, fn->is_async});
        }
        out += generate_function(*fn);
    }

    out += "\nint main() {\n";

    for (const auto& [name, is_async] : test_funcs) {
        if (is_async) {
            // Async test - suppress nodiscard warning, test runs via co_spawn elsewhere
            out += "\t(void)" + name + "();\n";
        } else {
            out += "\t" + name + "();\n";
        }
    }

    out += "\treturn _failures;\n";
    out += "}\n";

    return out;
}

/**
 * Generates C++ for a select statement using ASIO awaitable operators.
 * Uses the || operator to select between multiple channel operations.
 */
string CodeGen::generate_select(const SelectStmt& stmt) {
    string out;

    // Build the operations for select using awaitable operator||
    vector<string> ops;

    for (const auto& c : stmt.cases) {
        string channel_code = emit(*c->channel);

        if (c->operation == "recv") {
            ops.push_back(channel_code + ".async_receive(asio::as_tuple(asio::use_awaitable))");
        } else if (c->operation == "send") {
            string send_val = c->send_value ? emit(*c->send_value) : "";
            ops.push_back(channel_code + ".async_send(asio::error_code{}, " + send_val + ", asio::use_awaitable)");
        }
    }

    // Generate the select using || operator from awaitable_operators
    out += "auto _sel_result = co_await (";

    for (size_t i = 0; i < ops.size(); i++) {
        out += ops[i];

        if (i < ops.size() - 1) {
            out += " || ";
        }
    }

    out += ");\n";

    // Generate the case handling using index checking on the variant
    size_t case_idx = 0;

    for (const auto& c : stmt.cases) {
        out += "if (_sel_result.index() == " + to_string(case_idx) + ") {\n";

        // For recv cases with binding, extract the value from the tuple
        if (c->operation == "recv" && !c->binding_name.empty()) {
            out += "\tauto " + c->binding_name + " = std::get<1>(std::get<" + to_string(case_idx) + ">(_sel_result));\n";
        }

        // Generate case body
        for (const auto& s : c->body) {
            out += "\t" + generate_statement(*s) + "\n";
        }

        out += "}\n";
        case_idx++;
    }

    return out;
}
