/**
 * @file lists.cpp
 * @brief List method type definitions for the Bishop type checker.
 *
 * Defines type signatures for all built-in List<T> methods.
 * Uses "T" as a placeholder for the element type, which is
 * substituted with the actual type at type check time.
 */

/**
 * @nog_method length
 * @type List<T>
 * @description Returns the number of elements in the list.
 * @returns int - The list length
 * @example
 * nums := [1, 2, 3];
 * len := nums.length();  // 3
 */

/**
 * @nog_method is_empty
 * @type List<T>
 * @description Returns true if the list has no elements.
 * @returns bool - True if empty, false otherwise
 * @example
 * nums := List<int>();
 * if nums.is_empty() {
 *     print("List is empty");
 * }
 */

/**
 * @nog_method contains
 * @type List<T>
 * @description Checks if the list contains the given element.
 * @param elem T - The element to search for
 * @returns bool - True if found, false otherwise
 * @example
 * nums := [1, 2, 3];
 * if nums.contains(2) {
 *     print("Found it!");
 * }
 */

/**
 * @nog_method get
 * @type List<T>
 * @description Returns the element at the specified index (bounds-checked).
 * @param index int - The index (0-based)
 * @returns T - The element at that position
 * @example
 * nums := [10, 20, 30];
 * val := nums.get(1);  // 20
 */

/**
 * @nog_method set
 * @type List<T>
 * @description Replaces the element at the specified index.
 * @param index int - The index (0-based)
 * @param value T - The new value
 * @example
 * nums := [1, 2, 3];
 * nums.set(1, 99);  // nums is now [1, 99, 3]
 */

/**
 * @nog_method append
 * @type List<T>
 * @description Adds an element to the end of the list.
 * @param elem T - The element to add
 * @example
 * nums := [1, 2];
 * nums.append(3);  // nums is now [1, 2, 3]
 */

/**
 * @nog_method pop
 * @type List<T>
 * @description Removes the last element from the list.
 * @example
 * nums := [1, 2, 3];
 * nums.pop();  // nums is now [1, 2]
 */

/**
 * @nog_method clear
 * @type List<T>
 * @description Removes all elements from the list.
 * @example
 * nums := [1, 2, 3];
 * nums.clear();  // nums is now empty
 */

/**
 * @nog_method first
 * @type List<T>
 * @description Returns the first element in the list.
 * @returns T - The first element
 * @example
 * nums := [10, 20, 30];
 * val := nums.first();  // 10
 */

/**
 * @nog_method last
 * @type List<T>
 * @description Returns the last element in the list.
 * @returns T - The last element
 * @example
 * nums := [10, 20, 30];
 * val := nums.last();  // 30
 */

/**
 * @nog_method insert
 * @type List<T>
 * @description Inserts an element at the specified index.
 * @param index int - The index to insert at
 * @param value T - The value to insert
 * @example
 * nums := [1, 3];
 * nums.insert(1, 2);  // nums is now [1, 2, 3]
 */

/**
 * @nog_method remove
 * @type List<T>
 * @description Removes the element at the specified index.
 * @param index int - The index to remove
 * @example
 * nums := [1, 2, 3];
 * nums.remove(1);  // nums is now [1, 3]
 */

#include "lists.hpp"

#include <map>

namespace nog {

std::optional<ListMethodInfo> get_list_method_info(const std::string& method_name) {
    // "T" is placeholder for element type, substituted at type check time
    static const std::map<std::string, ListMethodInfo> list_methods = {
        // Query methods
        {"length", {{}, "int"}},
        {"is_empty", {{}, "bool"}},
        {"contains", {{"T"}, "bool"}},

        // Access methods
        {"get", {{"int"}, "T"}},
        {"first", {{}, "T"}},
        {"last", {{}, "T"}},

        // Modification methods
        {"append", {{"T"}, "void"}},
        {"pop", {{}, "void"}},
        {"set", {{"int", "T"}, "void"}},
        {"clear", {{}, "void"}},
        {"insert", {{"int", "T"}, "void"}},
        {"remove", {{"int"}, "void"}},
    };

    auto it = list_methods.find(method_name);

    if (it != list_methods.end()) {
        return it->second;
    }

    return std::nullopt;
}

}  // namespace nog
