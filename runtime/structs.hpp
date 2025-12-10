#pragma once
#include <string>
#include <vector>
#include <utility>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include "types.hpp"

using namespace std;

namespace nog::runtime {

inline string struct_def(const string& name, const vector<pair<string, string>>& fields) {
    string body;
    for (const auto& [field_name, field_type] : fields) {
        string cpp_type = map_type(field_type);
        if (cpp_type == "void") {
            cpp_type = field_type;
        }
        body += fmt::format("\t{} {};\n", cpp_type, field_name);
    }
    return fmt::format("struct {} {{\n{}}};", name, body);
}

inline string struct_literal(const string& name, const vector<pair<string, string>>& field_values) {
    vector<string> inits;
    for (const auto& [field_name, value] : field_values) {
        inits.push_back(fmt::format(".{} = {}", field_name, value));
    }
    return fmt::format("{} {{ {} }}", name, fmt::join(inits, ", "));
}

inline string field_access(const string& object, const string& field) {
    return fmt::format("{}.{}", object, field);
}

}
