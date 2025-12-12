/**
 * @nog_method length
 * @type str
 * @description Returns the number of characters in the string.
 * @returns int - The string length
 * @example
 * s := "hello";
 * len := s.length();  // 5
 */

/**
 * @nog_method empty
 * @type str
 * @description Returns true if the string has no characters.
 * @returns bool - True if empty, false otherwise
 * @example
 * s := "";
 * if s.empty() {
 *     print("String is empty");
 * }
 */

/**
 * @nog_method contains
 * @type str
 * @description Checks if the string contains the given substring.
 * @param substr str - The substring to search for
 * @returns bool - True if found, false otherwise
 * @example
 * s := "hello world";
 * if s.contains("world") {
 *     print("Found it!");
 * }
 */

/**
 * @nog_method starts_with
 * @type str
 * @description Checks if the string starts with the given prefix.
 * @param prefix str - The prefix to check
 * @returns bool - True if string starts with prefix
 * @example
 * path := "/api/users";
 * if path.starts_with("/api") {
 *     print("API route");
 * }
 */

/**
 * @nog_method ends_with
 * @type str
 * @description Checks if the string ends with the given suffix.
 * @param suffix str - The suffix to check
 * @returns bool - True if string ends with suffix
 * @example
 * file := "image.png";
 * if file.ends_with(".png") {
 *     print("PNG image");
 * }
 */

/**
 * @nog_method find
 * @type str
 * @description Returns the index of the first occurrence of a substring, or -1 if not found.
 * @param substr str - The substring to find
 * @returns int - Index of first occurrence, or -1
 * @example
 * s := "hello world";
 * idx := s.find("world");  // 6
 */

/**
 * @nog_method substr
 * @type str
 * @description Extracts a portion of the string.
 * @param start int - Starting index (0-based)
 * @param length int - Number of characters to extract
 * @returns str - The extracted substring
 * @example
 * s := "hello world";
 * sub := s.substr(0, 5);  // "hello"
 */

/**
 * @nog_method at
 * @type str
 * @description Returns the character at the specified index.
 * @param index int - The index (0-based)
 * @returns char - The character at that position
 * @example
 * s := "hello";
 * c := s.at(0);  // 'h'
 */

#include "strings.hpp"

#include <map>

namespace nog {

/**
 * Returns type information for built-in str methods.
 * Maps method names to their parameter types and return types.
 */
std::optional<StrMethodInfo> get_str_method_info(const std::string& method_name) {
    static const std::map<std::string, StrMethodInfo> str_methods = {
        {"length", {{}, "int"}},
        {"empty", {{}, "bool"}},
        {"contains", {{"str"}, "bool"}},
        {"starts_with", {{"str"}, "bool"}},
        {"ends_with", {{"str"}, "bool"}},
        {"find", {{"str"}, "int"}},
        {"substr", {{"int", "int"}, "str"}},
        {"at", {{"int"}, "char"}},
    };

    auto it = str_methods.find(method_name);

    if (it != str_methods.end()) {
        return it->second;
    }

    return std::nullopt;
}

}  // namespace nog
