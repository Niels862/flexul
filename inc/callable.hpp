#ifndef FLEXUL_CALLABLE_HPP
#define FLEXUL_CALLABLE_HPP

#include "symbol.hpp"
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>

class BaseNode;

class ExpressionNode;

struct InlineVariable {
    InlineVariable();
    InlineVariable(BaseNode *node, bool used, bool writeback);

    BaseNode *node;
    bool used;
    bool writeback;
};

struct InlineRecord {
    InlineRecord(SymbolId id, InlineVariable var);

    SymbolId id;
    InlineVariable var;
};

using InlineParamMap = std::unordered_map<SymbolId, InlineVariable>;

class InlineFrames {
public:
    InlineFrames(Serializer &serializer);

    void open_call(std::vector<std::unique_ptr<ExpressionNode>> const &args, 
            std::vector<SymbolId> const &param_ids, bool writeback);
    void use(Serializer &serializer, SymbolId id);
    void use_address(Serializer &serializer, SymbolId id);
    void close_call(std::vector<SymbolId> const &param_ids);
private:
    InlineVariable &get(SymbolId id);

    Serializer &m_serializer;
    InlineParamMap m_params;
    std::stack<InlineRecord> m_records;
};

#endif
