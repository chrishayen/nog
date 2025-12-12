/**
 * @file types.hpp
 * @brief Nog-to-C++ type mapping.
 *
 * Maps Nog primitive types to their C++ equivalents.
 */

#pragma once
#include <string>

using namespace std;

namespace nog::runtime {

/**
 * Maps a Nog type name to its C++ equivalent.
 * Returns the input unchanged if it's a user-defined type (struct).
 * Handles qualified types (module.Type -> module::Type).
 * Handles function types (fn(int, int) -> int -> std::function<int(int, int)>).
 */
inline string map_type(const string& t) {
    if (t == "int") return "int";
    if (t == "str") return "std::string";
    if (t == "bool") return "bool";
    if (t == "char") return "char";
    if (t == "f32") return "float";
    if (t == "f64") return "double";
    if (t == "u32") return "uint32_t";
    if (t == "u64") return "uint64_t";
    if (t.empty()) return "void";

    // Handle function types: fn(int, str) -> bool -> std::function<bool(int, std::string)>
    if (t.rfind("fn(", 0) == 0) {
        // Find the closing paren and extract param types
        size_t paren_start = 3;  // After "fn("
        int paren_depth = 1;
        size_t paren_end = paren_start;

        while (paren_end < t.size() && paren_depth > 0) {
            if (t[paren_end] == '(') paren_depth++;
            else if (t[paren_end] == ')') paren_depth--;
            paren_end++;
        }

        paren_end--;  // Point to the closing paren
        string params_str = t.substr(paren_start, paren_end - paren_start);

        // Parse the return type (after " -> ")
        string ret_type = "void";
        size_t arrow_pos = t.find(" -> ", paren_end);

        if (arrow_pos != string::npos) {
            ret_type = map_type(t.substr(arrow_pos + 4));
        }

        // Parse parameter types (comma-separated, but handle nested fn types)
        string cpp_params;
        size_t pos = 0;
        int depth = 0;
        size_t param_start = 0;

        while (pos <= params_str.size()) {
            char c = (pos < params_str.size()) ? params_str[pos] : ',';

            if (c == '(') depth++;
            else if (c == ')') depth--;
            else if ((c == ',' || pos == params_str.size()) && depth == 0) {
                string param = params_str.substr(param_start, pos - param_start);

                // Trim whitespace
                size_t start = param.find_first_not_of(" ");
                size_t end = param.find_last_not_of(" ");

                if (start != string::npos) {
                    param = param.substr(start, end - start + 1);

                    if (!cpp_params.empty()) {
                        cpp_params += ", ";
                    }

                    cpp_params += map_type(param);
                }

                param_start = pos + 1;
            }

            pos++;
        }

        return "std::function<" + ret_type + "(" + cpp_params + ")>";
    }

    // Handle qualified types: module.Type -> module::Type
    size_t dot_pos = t.find('.');

    if (dot_pos != string::npos) {
        return t.substr(0, dot_pos) + "::" + t.substr(dot_pos + 1);
    }

    return t;
}

}
