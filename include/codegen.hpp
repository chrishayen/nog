/**
 * @file codegen.hpp
 * @brief C++ code generator for the Nog language.
 *
 * Transforms a type-checked AST into C++ source code. The generated code
 * uses the runtime helpers defined in runtime/*.hpp to produce idiomatic C++.
 *
 * Features:
 * - Structs become C++ structs with member functions for methods
 * - Optional types map to std::optional<T>
 * - Built-in print() maps to std::cout
 * - Test mode generates a test harness with assert_eq support
 */

#pragma once
#include <string>
#include <memory>
#include "ast.hpp"

using namespace std;

/**
 * @brief Generates C++ code from a Nog AST.
 *
 * The generated code includes necessary headers and uses runtime helper
 * functions to emit idiomatic C++. In test mode, generates a main() that
 * runs all test_* functions and tracks failures.
 *
 * @example
 *   CodeGen codegen;
 *   string cpp_code = codegen.generate(program, false);  // normal mode
 *   string test_code = codegen.generate(program, true);  // test mode
 */
class CodeGen {
public:
    /**
     * @brief Generates C++ code for the entire program.
     * @param program The type-checked AST
     * @param test_mode If true, generates test harness with assert_eq support
     * @return Complete C++ source code as a string
     */
    string generate(const unique_ptr<Program>& program, bool test_mode = false);

private:
    bool test_mode = false;                  ///< Whether generating test harness
    const Program* current_program = nullptr;  ///< Current program for method lookup

    /**
     * @brief Emits C++ for an expression node.
     * Handles literals, variables, binary expressions, function calls, etc.
     */
    string emit(const ASTNode& node);

    /**
     * @brief Generates a C++ function from a FunctionDef.
     * Maps Nog types to C++ types and handles main() specially.
     */
    string generate_function(const FunctionDef& fn);

    /**
     * @brief Generates a C++ struct with optional methods.
     * Methods become member functions with 'self' mapped to 'this'.
     */
    string generate_struct(const StructDef& def);

    /**
     * @brief Generates a C++ member function from a MethodDef.
     * Transforms self.field into this->field.
     */
    string generate_method(const MethodDef& method);

    /**
     * @brief Generates C++ for a statement node.
     * Handles if/while, function calls, assignments, etc.
     */
    string generate_statement(const ASTNode& node);

    /**
     * @brief Generates the test harness main() function.
     * Calls all test_* functions and returns failure count.
     */
    string generate_test_harness(const unique_ptr<Program>& program);
};
