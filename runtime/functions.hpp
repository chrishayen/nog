#pragma once
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include "types.hpp"

using namespace std;

namespace nog::runtime {

struct FunctionParam {
    string type;
    string name;
};

inline string function_def(const string& name, const vector<FunctionParam>& params,
                           const string& return_type, const vector<string>& body) {
    string rt = return_type.empty() ? "void" : map_type(return_type);

    vector<string> param_strs;
    for (const auto& p : params) {
        param_strs.push_back(fmt::format("{} {}", map_type(p.type), p.name));
    }

    string out = fmt::format("{} {}({}) {{\n", rt, name, fmt::join(param_strs, ", "));
    for (const auto& stmt : body) {
        out += fmt::format("\t{}\n", stmt);
    }
    out += "}\n";
    return out;
}

inline string program(const vector<string>& functions) {
    return fmt::format("{}", fmt::join(functions, ""));
}

}
