#include "symbol.hpp"
#include "utils.hpp"
#include <stdexcept>

SymbolId lookup_symbol(std::string const &symbol, SymbolMap const &global_scope, 
        SymbolMap const &enclosing_scope, SymbolMap const &current_scope) {
    SymbolMap::const_iterator iter;

    iter = current_scope.find(symbol);
    if (iter != current_scope.end()) {
        return iter->second;
    }
    iter = enclosing_scope.find(symbol);
    if (iter != enclosing_scope.end()) {
        return iter->second;
    }
    iter = global_scope.find(symbol);
    if (iter != global_scope.end()) {
        return iter->second;
    }
    throw std::runtime_error("Undeclared symbol: " + symbol);
}

void declare_symbol(std::string const &symbol, SymbolId id, SymbolMap &scope) {
    SymbolMap::const_iterator iter = scope.find(symbol);
    if (iter != scope.end()) {
        throw std::runtime_error("Redeclared symbol: " + symbol);
    }
    scope[symbol] = id;
}
