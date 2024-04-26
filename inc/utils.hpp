#ifndef FLEXUL_UTILS_HPP
#define FLEXUL_UTILS_HPP

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <optional>

template <typename T>
void print_map(T const &map, size_t width = 16) {
    std::cerr << "{" << std::endl;
    for (auto const &kv_pair : map) {
        std::cerr << std::setw(width) << kv_pair.first << ": " 
                << kv_pair.second << std::endl;
    }
    std::cerr << "}" << std::endl;
}

template <typename T>
void print_iterable(T const &iterable) {
    std::cerr << "{" << std::endl;
    for (auto const &value : iterable) {
        std::cerr << "    " << value << std::endl;
    }
    std::cerr << "}" << std::endl;
}

template <typename T>
T assert_equal(T const &x, T const &y, std::string const &err) {
    if (x == y) {
        return x;
    }
    throw std::runtime_error(err);
}

bool endswith(std::string const &string, std::string const &postfix);

#endif
