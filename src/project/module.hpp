/**
 * @file module.hpp
 * @brief Module loading and management for multi-file nog programs.
 *
 * Handles loading modules from disk, caching parsed ASTs, detecting
 * circular imports, and merging symbol tables across modules.
 */

#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <optional>
#include "parser/ast.hpp"
#include "project.hpp"

/**
 * @brief Represents a loaded module with its exported symbols.
 */
struct Module {
    std::string name;                             ///< Module name (last path component)
    std::string full_path;                        ///< Full module path (e.g., "utils.helpers")
    fs::path directory;                           ///< Path to module directory
    std::unique_ptr<Program> ast;                 ///< Parsed and merged AST of all files
    std::vector<std::string> dependencies;        ///< Modules this module imports

    /**
     * @brief Gets all public functions from this module.
     */
    std::vector<const FunctionDef*> get_public_functions() const;

    /**
     * @brief Gets all public structs from this module.
     */
    std::vector<const StructDef*> get_public_structs() const;

    /**
     * @brief Gets all public methods for a given struct.
     */
    std::vector<const MethodDef*> get_public_methods(const std::string& struct_name) const;
};

/**
 * @brief Manages module loading, caching, and dependency resolution.
 */
class ModuleManager {
public:
    /**
     * @brief Constructs a module manager for the given project.
     */
    explicit ModuleManager(const ProjectConfig& config);

    /**
     * @brief Loads a module and all its dependencies.
     * @param module_path The module to load (e.g., "math" or "utils.helpers")
     * @return Pointer to loaded module, or nullptr on error
     */
    const Module* load_module(const std::string& module_path);

    /**
     * @brief Gets a previously loaded module by its alias (last path component).
     */
    const Module* get_module(const std::string& alias) const;

    /**
     * @brief Returns any errors encountered during loading.
     */
    const std::vector<std::string>& get_errors() const { return errors; }

    /**
     * @brief Clears all cached modules and errors.
     */
    void clear();

private:
    ProjectConfig config;
    std::map<std::string, std::unique_ptr<Module>> modules;  ///< Cached modules by alias
    std::set<std::string> loading;                            ///< Currently loading (cycle detection)
    std::vector<std::string> errors;

    /**
     * @brief Loads a single module from disk.
     */
    std::unique_ptr<Module> load_module_internal(const std::string& module_path);

    /**
     * @brief Creates a built-in stdlib module.
     */
    std::unique_ptr<Module> create_builtin_module(const std::string& name);

    /**
     * @brief Merges multiple .n files into a single Program AST.
     */
    std::unique_ptr<Program> merge_files(const std::vector<fs::path>& files, const std::string& module_name);
};
