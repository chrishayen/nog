/**
 * @file typechecker.hpp
 * @brief Static type checker for the Nog language.
 *
 * Performs semantic analysis on the AST before code generation. Validates:
 * - All types are defined (primitives or structs)
 * - Variable assignments match declared types
 * - Function/method calls have correct argument types and counts
 * - Return types match function declarations
 * - Field accesses are valid for struct types
 * - Optional types are handled correctly (none assignment, is none checks)
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include "parser/ast.hpp"
#include "project/module.hpp"

using namespace std;

/**
 * @brief A type error found during checking.
 * Contains the error message and source location for reporting.
 */
struct TypeError {
    string message;    ///< Human-readable error description
    int line;          ///< Source line number
    string filename;   ///< Source filename
};

/**
 * @brief Represents the type of an expression or variable.
 * Tracks the base type name and whether it's optional (?).
 */
struct TypeInfo {
    string base_type;        ///< Type name (e.g., "int", "str", "MyStruct")
    bool is_optional = false;  ///< True if declared with ? (e.g., int?)
    bool is_void = false;      ///< True for void return types

    bool operator==(const TypeInfo& other) const {
        return base_type == other.base_type && is_optional == other.is_optional;
    }

    bool operator!=(const TypeInfo& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Performs static type checking on a Nog program.
 *
 * Two-pass algorithm:
 * 1. Collect all struct, method, and function declarations into symbol tables
 * 2. Validate function/method bodies: check types, calls, and expressions
 *
 * @example
 *   TypeChecker checker;
 *   if (!checker.check(*program, "source.n")) {
 *       for (const auto& err : checker.get_errors()) {
 *           cerr << err.filename << ":" << err.line << ": " << err.message << endl;
 *       }
 *   }
 */
class TypeChecker {
public:
    /**
     * @brief Type-checks a complete program.
     * @param program The parsed AST to check
     * @param filename Source filename for error messages
     * @return true if no type errors, false otherwise
     */
    bool check(const Program& program, const string& filename);

    /**
     * @brief Registers an imported module for cross-module type checking.
     * @param alias The local alias for the module (e.g., "math" or "helpers")
     * @param module The loaded module containing exported symbols
     */
    void register_module(const string& alias, const Module& module);

    /**
     * @brief Returns all type errors found.
     * @return Vector of TypeError structs
     */
    const vector<TypeError>& get_errors() const { return errors; }

private:
    // Symbol tables
    map<string, const StructDef*> structs;          ///< Struct name -> definition
    map<string, vector<const MethodDef*>> methods;  ///< Struct name -> its methods
    map<string, const FunctionDef*> functions;      ///< Function name -> definition
    map<string, TypeInfo> locals;                   ///< Local variable types in current scope
    map<string, const Module*> imported_modules;    ///< Module alias -> module

    // Current context during checking
    string current_struct;     ///< Struct name when checking a method
    TypeInfo current_return;   ///< Expected return type of current function/method
    string filename;           ///< Current source filename
    bool in_async_context = false;  ///< True when checking async function/method body

    vector<TypeError> errors;  ///< Accumulated type errors

    // First pass: collect declarations into symbol tables
    void collect_structs(const Program& program);   ///< Registers all struct definitions
    void collect_methods(const Program& program);   ///< Registers all method definitions
    void collect_functions(const Program& program); ///< Registers all function definitions

    // Second pass: validate bodies
    void check_method(const MethodDef& method);     ///< Validates method body
    void check_function(const FunctionDef& func);   ///< Validates function body
    void check_statement(const ASTNode& stmt);      ///< Validates a single statement

    // Expression type inference
    TypeInfo infer_type(const ASTNode& expr);       ///< Computes type of an expression

    // Helper functions
    bool is_primitive_type(const string& type) const;  ///< Checks if type is built-in
    bool is_valid_type(const string& type) const;      ///< Checks if type exists
    bool types_compatible(const TypeInfo& expected, const TypeInfo& actual) const;  ///< Assignment compatibility
    const StructDef* get_struct(const string& name) const;  ///< Looks up struct by name
    const MethodDef* get_method(const string& struct_name, const string& method_name) const;  ///< Looks up method
    string get_field_type(const string& struct_name, const string& field_name) const;  ///< Gets type of struct field
    void error(const string& msg, int line);  ///< Records a type error

    // Module-aware lookups
    const FunctionDef* get_qualified_function(const string& module, const string& name) const;  ///< Looks up module.func
    const StructDef* get_qualified_struct(const string& module, const string& name) const;      ///< Looks up module.Type
    const MethodDef* get_qualified_method(const string& module, const string& struct_name, const string& method_name) const;  ///< Looks up module method

    // Return statement analysis
    bool has_return(const vector<unique_ptr<ASTNode>>& stmts) const;  ///< Checks if statements contain a return
};
