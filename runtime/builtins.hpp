#pragma once
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace nog::runtime {

inline std::string print(const std::string& arg) {
    return fmt::format("std::cout << {} << std::endl", arg);
}

inline std::string print_multi(const std::vector<std::string>& args) {
    return fmt::format("std::cout << {} << std::endl", fmt::join(args, " << "));
}

inline std::string assert_eq(const std::string& a, const std::string& b, int line) {
    return fmt::format("_assert_eq({}, {}, {})", a, b, line);
}

}
