/**
 * @file module.cpp
 * @brief Module loading and management implementation.
 *
 * Implements module discovery, parsing, merging of multiple files,
 * and circular import detection.
 */

#include "module.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>

using namespace std;

/**
 * @brief Gets all public functions from this module.
 */
vector<const FunctionDef*> Module::get_public_functions() const {
    vector<const FunctionDef*> result;

    for (const auto& func : ast->functions) {
        if (func->visibility == Visibility::Public) {
            result.push_back(func.get());
        }
    }

    return result;
}

/**
 * @brief Gets all public structs from this module.
 */
vector<const StructDef*> Module::get_public_structs() const {
    vector<const StructDef*> result;

    for (const auto& s : ast->structs) {
        if (s->visibility == Visibility::Public) {
            result.push_back(s.get());
        }
    }

    return result;
}

/**
 * @brief Gets all public methods for a given struct.
 */
vector<const MethodDef*> Module::get_public_methods(const string& struct_name) const {
    vector<const MethodDef*> result;

    for (const auto& m : ast->methods) {
        if (m->struct_name == struct_name && m->visibility == Visibility::Public) {
            result.push_back(m.get());
        }
    }

    return result;
}

ModuleManager::ModuleManager(const ProjectConfig& config) : config(config) {}

/**
 * @brief Clears all cached modules and errors.
 */
void ModuleManager::clear() {
    modules.clear();
    loading.clear();
    errors.clear();
}

/**
 * @brief Loads a module and all its dependencies.
 */
const Module* ModuleManager::load_module(const string& module_path) {
    // Extract alias (last component)
    string alias = module_path;
    size_t last_dot = module_path.rfind('.');

    if (last_dot != string::npos) {
        alias = module_path.substr(last_dot + 1);
    }

    // Check cache
    auto it = modules.find(alias);

    if (it != modules.end()) {
        return it->second.get();
    }

    // Check for circular import
    if (loading.count(module_path) > 0) {
        errors.push_back("Circular import detected: " + module_path);
        return nullptr;
    }

    // Mark as loading
    loading.insert(module_path);

    // Load the module
    auto mod = load_module_internal(module_path);

    if (!mod) {
        loading.erase(module_path);
        return nullptr;
    }

    // Load dependencies
    for (const auto& dep : mod->dependencies) {
        if (!load_module(dep)) {
            loading.erase(module_path);
            return nullptr;
        }
    }

    // Cache and return
    loading.erase(module_path);
    const Module* result = mod.get();
    modules[alias] = move(mod);
    return result;
}

/**
 * @brief Gets a previously loaded module by its alias.
 */
const Module* ModuleManager::get_module(const string& alias) const {
    auto it = modules.find(alias);

    if (it != modules.end()) {
        return it->second.get();
    }

    return nullptr;
}

/**
 * @brief Reads a file into a string.
 */
static string read_file(const fs::path& path) {
    ifstream file(path);

    if (!file) {
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Loads a single module from disk.
 */
unique_ptr<Module> ModuleManager::load_module_internal(const string& module_path) {
    // Resolve module path to directory
    auto module_dir = resolve_module(config, module_path);

    if (!module_dir) {
        errors.push_back("Module not found: " + module_path);
        return nullptr;
    }

    // Get all .n files in the module directory
    auto files = get_module_files(*module_dir);

    if (files.empty()) {
        errors.push_back("No .n files found in module: " + module_path);
        return nullptr;
    }

    // Extract module name (last component)
    string name = module_path;
    size_t last_dot = module_path.rfind('.');

    if (last_dot != string::npos) {
        name = module_path.substr(last_dot + 1);
    }

    // Merge all files into a single AST
    auto merged = merge_files(files, name);

    if (!merged) {
        return nullptr;
    }

    auto mod = make_unique<Module>();
    mod->name = name;
    mod->full_path = module_path;
    mod->directory = *module_dir;
    mod->ast = move(merged);

    // Collect dependencies from imports
    for (const auto& imp : mod->ast->imports) {
        mod->dependencies.push_back(imp->module_path);
    }

    return mod;
}

/**
 * @brief Merges multiple .n files into a single Program AST.
 */
unique_ptr<Program> ModuleManager::merge_files(const vector<fs::path>& files, const string& module_name) {
    auto merged = make_unique<Program>();

    for (const auto& file : files) {
        string source = read_file(file);

        if (source.empty()) {
            errors.push_back("Could not read file: " + file.string());
            continue;
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();

        // Merge imports
        for (auto& imp : ast->imports) {
            merged->imports.push_back(move(imp));
        }

        // Merge structs
        for (auto& s : ast->structs) {
            merged->structs.push_back(move(s));
        }

        // Merge functions
        for (auto& f : ast->functions) {
            merged->functions.push_back(move(f));
        }

        // Merge methods
        for (auto& m : ast->methods) {
            merged->methods.push_back(move(m));
        }
    }

    return merged;
}
