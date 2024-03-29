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

class ExpressionNode;

class CallableEntry {
public:
    CallableEntry();

    void add_overload(CallableNode *definition, SymbolId id);

    void call(Serializer &serializer, 
            std::vector<std::unique_ptr<ExpressionNode>> const &args) const;
    void push_callable_addr(Serializer &serializer) const;
private:
    struct OverloadEntry {
        OverloadEntry();
        OverloadEntry(CallableNode *definition, SymbolId id);

        operator bool() const;

        CallableNode *definition;
        SymbolId id;
    };

    std::vector<CallableEntry::OverloadEntry> m_overloads;
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

    void open_call(std::vector<std::unique_ptr<ExpressionNode>> const &args, 
            std::vector<SymbolId> const &param_ids);
    BaseNode *get(SymbolId id);
    void close_call(std::vector<SymbolId> const &param_ids);
private:
    InlineParamMap m_params;
    std::stack<InlineRecord> m_records;
};

#endif
