#include "callable.hpp"
#include "serializer.hpp"

InlineFrames::InlineFrames()
        : m_params(), m_records() {}

void InlineFrames::open_call(
        std::vector<std::unique_ptr<ExpressionNode>> const &args, 
        std::vector<SymbolId> const &param_ids) {
    for (std::size_t i = 0; i < param_ids.size(); i++) {
        SymbolId id = param_ids[i];
        InlineParamMap::const_iterator iter = m_params.find(id);
        m_records.push({id, iter == m_params.end() ? nullptr : iter->second});
        m_params[id] = args[i].get();
    }
}

BaseNode *InlineFrames::get(SymbolId id) {
    InlineParamMap::const_iterator iter = m_params.find(id);
    if (iter == m_params.end()) {
        throw std::runtime_error("todo");
    }
    return iter->second;
}

void InlineFrames::close_call(std::vector<SymbolId> const &param_ids) {
    for (SymbolIdList::const_reverse_iterator riter = param_ids.rbegin(); 
            riter != param_ids.rend(); riter++) {
        InlineRecord record = m_records.top();
        m_params[record.id] = record.node;
        m_records.pop();
    }
}
