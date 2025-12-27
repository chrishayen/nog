/**
 * @file project.cpp
 * @brief Project configuration and module resolution implementation.
 *
 * Implements project discovery (finding bishop.toml), configuration parsing,
 * and module path resolution for the Bishop module system.
 */

#include "project.hpp"
#include <toml++/toml.hpp>
#include <fstream>
#include <algorithm>

using namespace std;

/**
 * @brief Finds and loads project configuration.
 *
 * Walks up from the given path looking for bishop.toml file.
 * Returns nullopt if no project root is found.
 */
optional<ProjectConfig> find_project(const fs::path& start_path) {
    fs::path current = fs::absolute(start_path);

    // If start_path is a file, start from its parent directory
    if (fs::is_regular_file(current)) {
        current = current.parent_path();
    }

    // Walk up the directory tree looking for bishop.toml
    while (!current.empty()) {
        fs::path init_file = current / "bishop.toml";

        if (fs::exists(init_file)) {
            return parse_init_file(init_file);
        }

        fs::path parent = current.parent_path();

        if (parent == current) {
            break;  // Reached filesystem root
        }

        current = parent;
    }

    return nullopt;
}

/**
 * @brief Parses a bishop.toml file and returns the project configuration.
 *
 * Expects TOML format:
 *   [project]
 *   name = "projectname"
 */
optional<ProjectConfig> parse_init_file(const fs::path& init_file) {
    try {
        auto tbl = toml::parse_file(init_file.string());

        auto name = tbl["project"]["name"].value<string>();

        if (!name) {
            return nullopt;
        }

        ProjectConfig config;
        config.name = *name;
        config.init_file = init_file;
        config.root = init_file.parent_path();

        auto entry = tbl["project"]["entry"].value<string>();

        if (entry) {
            config.entry = *entry;
        }

        return config;
    } catch (const toml::parse_error&) {
        return nullopt;
    }
}

/**
 * @brief Resolves an import path to a directory of .b files.
 *
 * Given "math", looks for <project_root>/math/ directory.
 * Given "utils.helpers", looks for <project_root>/utils/helpers/ directory.
 * Returns nullopt if the module directory doesn't exist.
 */
optional<fs::path> resolve_module(const ProjectConfig& config, const string& import_path) {
    // Convert dots to directory separators
    string path_str = import_path;
    replace(path_str.begin(), path_str.end(), '.', '/');

    fs::path module_path = config.root / path_str;

    if (fs::is_directory(module_path)) {
        return module_path;
    }

    return nullopt;
}

/**
 * @brief Gets all .b files in a module directory.
 */
vector<fs::path> get_module_files(const fs::path& module_dir) {
    vector<fs::path> files;

    if (!fs::is_directory(module_dir)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(module_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".b") {
            files.push_back(entry.path());
        }
    }

    return files;
}
