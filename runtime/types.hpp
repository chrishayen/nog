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
    return t;
}

}
