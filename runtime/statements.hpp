#pragma once
#include <string>
#include <vector>
#include <fmt/format.h>
#include "types.hpp"

using namespace std;

namespace nog::runtime {

inline string variable_decl(const string& type, const string& name, const string& value) {
    string t = type.empty() ? "auto" : map_type(type);
    return fmt::format("{} {} = {};", t, name, value);
}

inline string return_stmt(const string& value) {
    return fmt::format("return {};", value);
}

inline string assignment(const string& name, const string& value) {
    return fmt::format("{} = {};", name, value);
}

inline string if_stmt(const string& condition, const vector<string>& then_body, const vector<string>& else_body) {
    string out = fmt::format("if ({}) {{\n", condition);
    for (const auto& stmt : then_body) {
        out += "\t" + stmt + "\n";
    }
    out += "}";
    if (!else_body.empty()) {
        out += " else {\n";
        for (const auto& stmt : else_body) {
            out += "\t" + stmt + "\n";
        }
        out += "}";
    }
    return out;
}

inline string while_stmt(const string& condition, const vector<string>& body) {
    string out = fmt::format("while ({}) {{\n", condition);
    for (const auto& stmt : body) {
        out += "\t" + stmt + "\n";
    }
    out += "}";
    return out;
}

}
