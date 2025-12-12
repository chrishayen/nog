/**
 * @file fs.cpp
 * @brief Built-in fs (filesystem) module implementation.
 *
 * Creates the AST definitions for the fs module.
 * The actual runtime is in src/runtime/fs/fs.hpp and included as a header.
 */

/**
 * @nog_fn read_file
 * @module fs
 * @description Reads the entire contents of a file as a string.
 * @param path str - Path to the file to read
 * @returns str - File contents, or empty string if file cannot be read
 * @example
 * import fs;
 * content := fs.read_file("config.txt");
 */

/**
 * @nog_fn exists
 * @module fs
 * @description Checks if a file or directory exists at the given path.
 * @param path str - Path to check
 * @returns bool - True if path exists, false otherwise
 * @example
 * import fs;
 * if fs.exists("data.json") {
 *     content := fs.read_file("data.json");
 * }
 */

/**
 * @nog_fn is_dir
 * @module fs
 * @description Checks if the given path is a directory.
 * @param path str - Path to check
 * @returns bool - True if path is a directory, false otherwise
 * @example
 * import fs;
 * if fs.is_dir("./uploads") {
 *     files := fs.read_dir("./uploads");
 * }
 */

/**
 * @nog_fn read_dir
 * @module fs
 * @description Lists all entries in a directory, separated by newlines.
 * @param path str - Path to the directory
 * @returns str - Newline-separated list of filenames
 * @example
 * import fs;
 * files := fs.read_dir("./data");
 */

#include "fs.hpp"

using namespace std;

namespace nog::stdlib {

/**
 * Creates the AST for the built-in fs module.
 */
unique_ptr<Program> create_fs_module() {
    auto program = make_unique<Program>();

    // fn read_file(str path) -> str
    auto read_file_fn = make_unique<FunctionDef>();
    read_file_fn->name = "read_file";
    read_file_fn->visibility = Visibility::Public;
    read_file_fn->params.push_back({"str", "path"});
    read_file_fn->return_type = "str";
    program->functions.push_back(move(read_file_fn));

    // fn exists(str path) -> bool
    auto exists_fn = make_unique<FunctionDef>();
    exists_fn->name = "exists";
    exists_fn->visibility = Visibility::Public;
    exists_fn->params.push_back({"str", "path"});
    exists_fn->return_type = "bool";
    program->functions.push_back(move(exists_fn));

    // fn is_dir(str path) -> bool
    auto is_dir_fn = make_unique<FunctionDef>();
    is_dir_fn->name = "is_dir";
    is_dir_fn->visibility = Visibility::Public;
    is_dir_fn->params.push_back({"str", "path"});
    is_dir_fn->return_type = "bool";
    program->functions.push_back(move(is_dir_fn));

    // fn read_dir(str path) -> str
    auto read_dir_fn = make_unique<FunctionDef>();
    read_dir_fn->name = "read_dir";
    read_dir_fn->visibility = Visibility::Public;
    read_dir_fn->params.push_back({"str", "path"});
    read_dir_fn->return_type = "str";
    program->functions.push_back(move(read_dir_fn));

    return program;
}

/**
 * Returns empty - fs.hpp is included at the top of generated code
 * for precompiled header support.
 */
string generate_fs_runtime() {
    return "";
}

}  // namespace nog::stdlib
