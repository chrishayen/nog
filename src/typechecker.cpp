/**
 * @file typechecker.cpp
 * @brief Type checker implementation for the Nog language.
 *
 * Performs semantic analysis to validate types before code generation.
 * Uses a two-pass algorithm: first collecting declarations, then validating bodies.
 */

#include "typechecker.hpp"
#include <iostream>

using namespace std;

/**
 * Main entry point for type checking. Collects all declarations into symbol
 * tables, then validates each function and method body.
 */
bool TypeChecker::check(const Program& program, const string& fname) {
    this->filename = fname;
    errors.clear();

    // First pass: collect all declarations
    collect_structs(program);
    collect_methods(program);
    collect_functions(program);

    // Second pass: validate method definitions
    for (const auto& method : program.methods) {
        check_method(*method);
    }

    // Validate function definitions
    for (const auto& func : program.functions) {
        check_function(*func);
    }

    return errors.empty();
}

/**
 * Collects all struct definitions into the structs symbol table.
 */
void TypeChecker::collect_structs(const Program& program) {
    for (const auto& s : program.structs) {
        structs[s->name] = s.get();
    }
}

/**
 * Collects all method definitions. Validates that the struct exists
 * and checks for duplicate method definitions.
 */
void TypeChecker::collect_methods(const Program& program) {
    for (const auto& m : program.methods) {
        // Validate struct exists
        if (structs.find(m->struct_name) == structs.end()) {
            error("method '" + m->name + "' defined on unknown struct '" + m->struct_name + "'", m->line);
            continue;
        }

        // Check for duplicate methods
        auto& struct_methods = methods[m->struct_name];
        for (const auto* existing : struct_methods) {
            if (existing->name == m->name) {
                error("duplicate method '" + m->name + "' on struct '" + m->struct_name + "'", m->line);
                break;
            }
        }

        struct_methods.push_back(m.get());
    }
}

/**
 * Collects all function definitions into the functions symbol table.
 */
void TypeChecker::collect_functions(const Program& program) {
    for (const auto& f : program.functions) {
        functions[f->name] = f.get();
    }
}

/**
 * Validates a method definition. Ensures 'self' is the first parameter,
 * validates parameter types, and checks the method body.
 */
void TypeChecker::check_method(const MethodDef& method) {
    locals.clear();
    current_struct = method.struct_name;

    // Set up expected return type
    if (method.return_type.empty()) {
        current_return = {"void", false, true};
    } else {
        current_return = {method.return_type, false, false};
    }

    // Validate first param is self
    if (method.params.empty() || method.params[0].name != "self") {
        error("method '" + method.name + "' must have 'self' as first parameter", method.line);
        return;
    }

    // Add parameters to locals
    for (const auto& param : method.params) {
        if (!is_valid_type(param.type)) {
            error("unknown type '" + param.type + "' for parameter '" + param.name + "'", method.line);
        }

        locals[param.name] = {param.type, false, false};
    }

    // Check body statements
    for (const auto& stmt : method.body) {
        check_statement(*stmt);
    }

    current_struct.clear();
}

/**
 * Validates a function definition. Checks parameter types and validates the body.
 */
void TypeChecker::check_function(const FunctionDef& func) {
    locals.clear();
    current_struct.clear();

    // Set up expected return type
    if (func.return_type.empty()) {
        current_return = {"void", false, true};
    } else {
        current_return = {func.return_type, false, false};
    }

    // Add parameters to locals
    for (const auto& param : func.params) {
        if (!is_valid_type(param.type)) {
            error("unknown type '" + param.type + "' for parameter '" + param.name + "'", func.line);
        }

        locals[param.name] = {param.type, false, false};
    }

    // Check body statements
    for (const auto& stmt : func.body) {
        check_statement(*stmt);
    }
}

/**
 * Validates a single statement. Handles variable declarations, assignments,
 * field assignments, return statements, if/while statements, and function/method calls.
 */
void TypeChecker::check_statement(const ASTNode& stmt) {
    if (auto* decl = dynamic_cast<const VariableDecl*>(&stmt)) {
        // Validate declared type if specified
        if (!decl->type.empty() && !is_valid_type(decl->type)) {
            error("unknown type '" + decl->type + "'", decl->line);
        }

        // Infer type from initializer
        if (decl->value) {
            TypeInfo init_type = infer_type(*decl->value);

            if (!decl->type.empty()) {
                TypeInfo expected = {decl->type, decl->is_optional, false};

                if (!types_compatible(expected, init_type)) {
                    error("cannot assign '" + init_type.base_type + "' to variable of type '" + decl->type + "'", decl->line);
                }
            }

            // Add to locals
            if (!decl->type.empty()) {
                locals[decl->name] = {decl->type, decl->is_optional, false};
            } else {
                locals[decl->name] = init_type;
            }
        }
    } else if (auto* assign = dynamic_cast<const Assignment*>(&stmt)) {
        // Check variable exists
        if (locals.find(assign->name) == locals.end()) {
            error("assignment to undefined variable '" + assign->name + "'", assign->line);
            return;
        }

        TypeInfo var_type = locals[assign->name];
        TypeInfo val_type = infer_type(*assign->value);

        if (!types_compatible(var_type, val_type)) {
            error("cannot assign '" + val_type.base_type + "' to variable of type '" + var_type.base_type + "'", assign->line);
        }
    } else if (auto* fa = dynamic_cast<const FieldAssignment*>(&stmt)) {
        TypeInfo obj_type = infer_type(*fa->object);

        // Must be a struct type
        if (structs.find(obj_type.base_type) == structs.end()) {
            error("cannot access field on non-struct type '" + obj_type.base_type + "'", fa->line);
            return;
        }

        string field_type = get_field_type(obj_type.base_type, fa->field_name);

        if (field_type.empty()) {
            error("struct '" + obj_type.base_type + "' has no field '" + fa->field_name + "'", fa->line);
            return;
        }

        TypeInfo expected = {field_type, false, false};
        TypeInfo val_type = infer_type(*fa->value);

        if (!types_compatible(expected, val_type)) {
            error("cannot assign '" + val_type.base_type + "' to field of type '" + field_type + "'", fa->line);
        }
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt)) {
        TypeInfo ret_type = {"void", false, true};

        if (ret->value) {
            ret_type = infer_type(*ret->value);
        }

        if (!types_compatible(current_return, ret_type)) {
            error("return type '" + ret_type.base_type + "' does not match declared type '" + current_return.base_type + "'", ret->line);
        }
    } else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        TypeInfo cond_type = infer_type(*if_stmt->condition);

        // Allow bool, optional types (truthy check), and IsNone expressions
        if (cond_type.base_type != "bool" && !cond_type.is_optional) {
            error("if condition must be bool or optional type, got '" + cond_type.base_type + "'", if_stmt->line);
        }

        for (const auto& s : if_stmt->then_body) {
            check_statement(*s);
        }

        for (const auto& s : if_stmt->else_body) {
            check_statement(*s);
        }
    } else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        TypeInfo cond_type = infer_type(*while_stmt->condition);

        if (cond_type.base_type != "bool") {
            error("while condition must be bool, got '" + cond_type.base_type + "'", while_stmt->line);
        }

        for (const auto& s : while_stmt->body) {
            check_statement(*s);
        }
    } else if (auto* call = dynamic_cast<const FunctionCall*>(&stmt)) {
        infer_type(*call);
    } else if (auto* mcall = dynamic_cast<const MethodCall*>(&stmt)) {
        infer_type(*mcall);
    }
}

/**
 * Infers the type of an expression. Recursively handles all expression types:
 * literals, variables, binary ops, function calls, method calls, field access, etc.
 */
TypeInfo TypeChecker::infer_type(const ASTNode& expr) {
    if (auto* num = dynamic_cast<const NumberLiteral*>(&expr)) {
        (void)num;
        return {"int", false, false};
    }

    if (auto* flt = dynamic_cast<const FloatLiteral*>(&expr)) {
        (void)flt;
        return {"f64", false, false};
    }

    if (auto* str = dynamic_cast<const StringLiteral*>(&expr)) {
        (void)str;
        return {"str", false, false};
    }

    if (auto* bl = dynamic_cast<const BoolLiteral*>(&expr)) {
        (void)bl;
        return {"bool", false, false};
    }

    if (auto* none = dynamic_cast<const NoneLiteral*>(&expr)) {
        (void)none;
        return {"none", true, false};
    }

    if (auto* var = dynamic_cast<const VariableRef*>(&expr)) {
        if (locals.find(var->name) != locals.end()) {
            return locals[var->name];
        }

        error("undefined variable '" + var->name + "'", var->line);
        return {"unknown", false, false};
    }

    if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        TypeInfo left_type = infer_type(*bin->left);
        TypeInfo right_type = infer_type(*bin->right);

        // Comparison operators return bool
        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" ||
            bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
            return {"bool", false, false};
        }

        // String concatenation
        if (bin->op == "+" && left_type.base_type == "str") {
            return {"str", false, false};
        }

        // Arithmetic
        if (left_type.base_type != right_type.base_type) {
            error("type mismatch in binary expression: '" + left_type.base_type + "' " + bin->op + " '" + right_type.base_type + "'", bin->line);
        }

        return left_type;
    }

    if (auto* is_none = dynamic_cast<const IsNone*>(&expr)) {
        (void)is_none;
        return {"bool", false, false};
    }

    if (auto* call = dynamic_cast<const FunctionCall*>(&expr)) {
        // Built-in functions
        if (call->name == "assert_eq" || call->name == "print") {
            return {"void", false, true};
        }

        if (functions.find(call->name) == functions.end()) {
            error("undefined function '" + call->name + "'", call->line);
            return {"unknown", false, false};
        }

        const FunctionDef* func = functions[call->name];

        // Check argument count
        if (call->args.size() != func->params.size()) {
            error("function '" + call->name + "' expects " + to_string(func->params.size()) + " arguments, got " + to_string(call->args.size()), call->line);
        }

        // Check argument types
        for (size_t i = 0; i < call->args.size() && i < func->params.size(); i++) {
            TypeInfo arg_type = infer_type(*call->args[i]);
            TypeInfo param_type = {func->params[i].type, false, false};

            if (!types_compatible(param_type, arg_type)) {
                error("argument " + to_string(i + 1) + " of function '" + call->name + "' expects '" + param_type.base_type + "', got '" + arg_type.base_type + "'", call->line);
            }
        }

        if (func->return_type.empty()) {
            return {"void", false, true};
        }

        return {func->return_type, false, false};
    }

    if (auto* mcall = dynamic_cast<const MethodCall*>(&expr)) {
        TypeInfo obj_type = infer_type(*mcall->object);

        // Check object is a struct
        if (structs.find(obj_type.base_type) == structs.end()) {
            error("cannot call method on non-struct type '" + obj_type.base_type + "'", mcall->line);
            return {"unknown", false, false};
        }

        // Find the method
        const MethodDef* method = get_method(obj_type.base_type, mcall->method_name);

        if (!method) {
            error("method '" + mcall->method_name + "' not found on struct '" + obj_type.base_type + "'", mcall->line);
            return {"unknown", false, false};
        }

        // Check argument count (excluding self)
        size_t expected_args = method->params.size() - 1;

        if (mcall->args.size() != expected_args) {
            error("method '" + mcall->method_name + "' expects " + to_string(expected_args) + " arguments, got " + to_string(mcall->args.size()), mcall->line);
        }

        // Check argument types (starting from param index 1, skipping self)
        for (size_t i = 0; i < mcall->args.size() && i + 1 < method->params.size(); i++) {
            TypeInfo arg_type = infer_type(*mcall->args[i]);
            TypeInfo param_type = {method->params[i + 1].type, false, false};

            if (!types_compatible(param_type, arg_type)) {
                error("argument " + to_string(i + 1) + " of method '" + mcall->method_name + "' expects '" + param_type.base_type + "', got '" + arg_type.base_type + "'", mcall->line);
            }
        }

        if (method->return_type.empty()) {
            return {"void", false, true};
        }

        return {method->return_type, false, false};
    }

    if (auto* access = dynamic_cast<const FieldAccess*>(&expr)) {
        TypeInfo obj_type = infer_type(*access->object);

        if (structs.find(obj_type.base_type) == structs.end()) {
            error("cannot access field on non-struct type '" + obj_type.base_type + "'", access->line);
            return {"unknown", false, false};
        }

        string field_type = get_field_type(obj_type.base_type, access->field_name);

        if (field_type.empty()) {
            error("struct '" + obj_type.base_type + "' has no field '" + access->field_name + "'", access->line);
            return {"unknown", false, false};
        }

        return {field_type, false, false};
    }

    if (auto* lit = dynamic_cast<const StructLiteral*>(&expr)) {
        if (structs.find(lit->struct_name) == structs.end()) {
            error("unknown struct '" + lit->struct_name + "'", lit->line);
            return {"unknown", false, false};
        }

        // Validate field values
        const StructDef* sdef = structs[lit->struct_name];

        for (const auto& [field_name, field_val] : lit->field_values) {
            string expected_type = get_field_type(lit->struct_name, field_name);

            if (expected_type.empty()) {
                error("struct '" + lit->struct_name + "' has no field '" + field_name + "'", lit->line);
                continue;
            }

            TypeInfo val_type = infer_type(*field_val);
            TypeInfo exp_type = {expected_type, false, false};

            if (!types_compatible(exp_type, val_type)) {
                error("field '" + field_name + "' expects '" + expected_type + "', got '" + val_type.base_type + "'", lit->line);
            }
        }

        return {lit->struct_name, false, false};
    }

    return {"unknown", false, false};
}

/**
 * Checks if a type is a built-in primitive (int, str, bool, char, f32, f64, u32, u64).
 */
bool TypeChecker::is_primitive_type(const string& type) const {
    return type == "int" || type == "str" || type == "bool" ||
           type == "char" || type == "f32" || type == "f64" ||
           type == "u32" || type == "u64";
}

/**
 * Checks if a type is valid (either a primitive or a known struct).
 */
bool TypeChecker::is_valid_type(const string& type) const {
    return is_primitive_type(type) || structs.find(type) != structs.end();
}

/**
 * Checks if actual type can be assigned to expected type.
 * Special case: none can be assigned to optional types.
 */
bool TypeChecker::types_compatible(const TypeInfo& expected, const TypeInfo& actual) const {
    // none can be assigned to optional types
    if (actual.base_type == "none" && expected.is_optional) {
        return true;
    }

    // Allow unknown types to pass (error already reported)
    if (actual.base_type == "unknown" || expected.base_type == "unknown") {
        return true;
    }

    return expected.base_type == actual.base_type;
}

/**
 * Looks up a struct definition by name. Returns nullptr if not found.
 */
const StructDef* TypeChecker::get_struct(const string& name) const {
    auto it = structs.find(name);

    if (it != structs.end()) {
        return it->second;
    }

    return nullptr;
}

/**
 * Looks up a method on a struct. Returns nullptr if not found.
 */
const MethodDef* TypeChecker::get_method(const string& struct_name, const string& method_name) const {
    auto it = methods.find(struct_name);

    if (it == methods.end()) {
        return nullptr;
    }

    for (const auto* m : it->second) {
        if (m->name == method_name) {
            return m;
        }
    }

    return nullptr;
}

/**
 * Gets the type of a field on a struct. Returns empty string if not found.
 */
string TypeChecker::get_field_type(const string& struct_name, const string& field_name) const {
    const StructDef* sdef = get_struct(struct_name);

    if (!sdef) {
        return "";
    }

    for (const auto& field : sdef->fields) {
        if (field.name == field_name) {
            return field.type;
        }
    }

    return "";
}

/**
 * Records a type error with the given message and line number.
 */
void TypeChecker::error(const string& msg, int line) {
    errors.push_back({msg, line, filename});
}
