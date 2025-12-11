/**
 * @file project.hpp
 * @brief Project configuration and module resolution for Nog.
 *
 * Handles finding the project root (where nog.toml lives), parsing the
 * configuration file, and resolving import paths to source files.
 */

#pragma once
#include <string>
#include <optional>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Project configuration loaded from nog.toml
 */
struct ProjectConfig {
    std::string name;      ///< Project name from [project] section
    fs::path root;         ///< Absolute path to project root directory
    fs::path init_file;    ///< Path to the nog.toml file
};

/**
 * @brief Finds and loads project configuration.
 *
 * Walks up from the given path looking for nog.toml file.
 * Returns nullopt if no project root is found.
 */
std::optional<ProjectConfig> find_project(const fs::path& start_path);

/**
 * @brief Parses a nog.toml file and returns the project configuration.
 *
 * Expects TOML-like format:
 *   [project]
 *   name = "projectname"
 */
std::optional<ProjectConfig> parse_init_file(const fs::path& init_file);

/**
 * @brief Resolves an import path to a directory of .n files.
 *
 * Given "math", looks for <project_root>/math/ directory.
 * Given "utils.helpers", looks for <project_root>/utils/helpers/ directory.
 * Returns nullopt if the module directory doesn't exist.
 */
std::optional<fs::path> resolve_module(const ProjectConfig& config, const std::string& import_path);

/**
 * @brief Gets all .n files in a module directory.
 */
std::vector<fs::path> get_module_files(const fs::path& module_dir);

/**
 * @brief Creates a new nog.toml file in the specified directory.
 *
 * Uses the directory name as the project name.
 * Returns true on success, false if file already exists or on write error.
 */
bool create_init_file(const fs::path& directory);
