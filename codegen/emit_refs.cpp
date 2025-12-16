/**
 * @file emit_refs.cpp
 * @brief Variable and function reference emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a variable reference (just the name).
 */
string variable_ref(const string& name) {
    return name;
}

/**
 * Emits a function reference, possibly qualified with ::.
 */
string emit_function_ref(const FunctionRef& ref) {
    string func_name = ref.name;
    size_t dot_pos = func_name.find('.');

    if (dot_pos != string::npos) {
        func_name = func_name.substr(0, dot_pos) + "::" + func_name.substr(dot_pos + 1);
    }

    return func_name;
}

/**
 * Emits a qualified reference: module.name -> module::name.
 */
string emit_qualified_ref(const QualifiedRef& ref) {
    return ref.module_name + "::" + ref.name;
}

} // namespace codegen
