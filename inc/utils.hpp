#ifndef FLEXUL_UTILS_HPP
#define FLEXUL_UTILS_HPP

#include <iostream>
#include <iomanip>

template <typename T>
void print_map(T const &map, size_t width = 16) {
    for (auto const &kv_pair : map) {
        std::cerr << std::setw(width) << kv_pair.first << ": " 
                << kv_pair.second << std::endl;
    }
}

template <typename T>
void print_iterable(T const &iterable) {
    std::cerr << "{" << std::endl;
    for (auto const &value : iterable) {
        std::cerr << "    " << value << std::endl;
    }
    std::cerr << "}" << std::endl;
}

#endif
