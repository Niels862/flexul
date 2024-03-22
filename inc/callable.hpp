#ifndef FLEXUL_CALLABLE_HPP
#define FLEXUL_CALLABLE_HPP

#include "symbol.hpp"
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>

class Serializer;

class BaseNode;

class CallableNode;

class ExpressionListNode;

class CallableEntry {
public:
    CallableEntry();

    void add_overload(CallableNode *overload);

    void call(Serializer &serializer, 
            std::unique_ptr<ExpressionListNode> const &params) const;
    void push_callable_addr(Serializer &serializer) const;
private:
    std::vector<CallableNode *> m_overloads;
};

using CallableMap = std::unordered_map<SymbolId, CallableEntry>;

struct InlineRecord {
    SymbolId id;
    BaseNode *node;
};

using InlineParamMap = std::unordered_map<SymbolId, BaseNode *>;

class InlineFrames {
public:
    InlineFrames();

    void open_call(std::unique_ptr<ExpressionListNode> const &params, 
            std::vector<SymbolId> const &param_ids);
    BaseNode *get(SymbolId id);
    void close_call(std::vector<SymbolId> const &param_ids);
private:
    InlineParamMap m_params;
    std::stack<InlineRecord> m_records;
};

#endif
