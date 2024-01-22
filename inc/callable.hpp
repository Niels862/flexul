#ifndef FLEXUL_CALLABLE_HPP
#define FLEXUL_CALLABLE_HPP

#include "symbol.hpp"
#include <vector>
#include <unordered_map>

class Serializer;

class BaseNode;

class CallableNode;

class CallableEntry {
public:
    CallableEntry();

    void add_overload(CallableNode *overload);

    void call(Serializer &serializer, BaseNode *params) const;
private:
    std::vector<CallableNode *> overloads;
};

using CallableMap = std::unordered_map<SymbolId, CallableEntry>;

#endif
