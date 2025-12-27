/**
 * @file emit_for.cpp
 * @brief For loop emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a range-based for loop: for (int var = start; var < end; var++)
 */
string for_range_stmt(const string& var, const string& start, const string& end, const vector<string>& body) {
    string out = fmt::format("for (int {} = {}; {} < {}; {}++) {{\n", var, start, var, end, var);

    for (const auto& stmt : body) {
        out += "\t" + stmt + "\n";
    }

    out += "}";
    return out;
}

/**
 * Emits a foreach loop: for (auto& var : collection)
 */
string for_each_stmt(const string& var, const string& collection, const vector<string>& body) {
    string out = fmt::format("for (auto& {} : {}) {{\n", var, collection);

    for (const auto& stmt : body) {
        out += "\t" + stmt + "\n";
    }

    out += "}";
    return out;
}

} // namespace codegen
