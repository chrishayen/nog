/**
 * @file emit_ffi.cpp
 * @brief FFI (Foreign Function Interface) code generation for Bishop.
 *
 * Handles generating extern "C" declarations for calling C libraries.
 */

#include "codegen.hpp"

using namespace std;

namespace codegen {

/**
 * Generates extern "C" declarations for FFI functions.
 */
string generate_extern_declarations(const unique_ptr<Program>& program) {
    if (program->externs.empty()) {
        return "";
    }

    string out = "extern \"C\" {\n";

    for (const auto& ext : program->externs) {
        string ret = map_type(ext->return_type);
        vector<string> params;

        for (const auto& p : ext->params) {
            params.push_back(map_type(p.type) + " " + p.name);
        }

        out += "\t" + ret + " " + ext->name + "(";

        for (size_t i = 0; i < params.size(); i++) {
            out += params[i];

            if (i < params.size() - 1) {
                out += ", ";
            }
        }

        out += ");\n";
    }

    out += "}\n\n";
    return out;
}

} // namespace codegen
