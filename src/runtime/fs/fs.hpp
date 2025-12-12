/**
 * @file fs.hpp
 * @brief Nog filesystem runtime library.
 *
 * Provides filesystem operations for Nog programs.
 * This header is included when programs import the fs module.
 */

#pragma once

#include <nog/std.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs {

/**
 * Reads the entire contents of a file.
 * Returns empty string if file cannot be read.
 */
inline std::string read_file(const std::string& path) {
    std::ifstream file(path);

    if (!file) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Checks if a file or directory exists.
 */
inline bool exists(const std::string& path) {
    return std::filesystem::exists(path);
}

/**
 * Checks if a path is a directory.
 */
inline bool is_dir(const std::string& path) {
    return std::filesystem::is_directory(path);
}

/**
 * Lists all entries in a directory, separated by newlines.
 */
inline std::string read_dir(const std::string& path) {
    std::string result;

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!result.empty()) {
            result += "\n";
        }

        result += entry.path().filename().string();
    }

    return result;
}

}  // namespace fs
