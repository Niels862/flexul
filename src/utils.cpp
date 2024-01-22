#include "utils.hpp"

bool endswith(std::string const &string, std::string const &postfix) {
    return string.size() >= postfix.size() 
            && string.substr(
                string.size() - postfix.size(), postfix.size()) == postfix;
}
