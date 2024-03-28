#include "callable.hpp"
#include "serializer.hpp"

CallableEntry::CallableEntry() 
        : m_overloads() {}

void CallableEntry::add_overload(CallableNode *definition, SymbolId id) {
    m_overloads.push_back(OverloadEntry(definition, id));
}

void CallableEntry::call(Serializer &serializer, 
        std::vector<std::unique_ptr<ExpressionNode>> const &args) const {
    if (m_overloads.empty()) {
        throw std::runtime_error("No overloads declared for function");
    }
    OverloadEntry candidate;
    for (auto const &overload : m_overloads) {
        if (overload.definition->is_matching_call(args)) {
            if (candidate) {
                throw std::runtime_error("Multiple candidates for call");
            }
            candidate = overload;
        }
    }
    if (!candidate) {
        throw std::runtime_error("No suitable candidate for call");
    }

    serializer.add_function_implementation(candidate.id);
    candidate.definition->serialize_call(serializer, args);
}

void CallableEntry::push_callable_addr(Serializer &serializer) const {
    if (m_overloads.size() != 1) {
        throw std::runtime_error(
                "Can only load address of single implmementation");
    }
    serializer.add_function_implementation(m_overloads[0].id);
    serializer.add_instr(OpCode::Push, m_overloads[0].id, true);
}

CallableEntry::OverloadEntry::OverloadEntry()
        : definition(nullptr), id(0) {}

CallableEntry::OverloadEntry::OverloadEntry(
        CallableNode *definition, SymbolId id)
        : definition(definition), id(id) {}

CallableEntry::OverloadEntry::operator bool() const {
    return definition != nullptr;
}

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
