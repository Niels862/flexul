#ifndef FLEXUL_CALLABLE_HPP
#define FLEXUL_CALLABLE_HPP

#include "symbol.hpp"
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>

class BaseNode;

class ExpressionNode;

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
