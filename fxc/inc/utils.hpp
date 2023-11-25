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

#endif
