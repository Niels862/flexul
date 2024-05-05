#include "callable.hpp"
#include "serializer.hpp"

InlineVariable::InlineVariable()
        : node(nullptr), used(false), writeback(false) {}

InlineVariable::InlineVariable(BaseNode *node, bool used, bool writeback)
        : node(node), used(used), writeback(writeback) {}

InlineRecord::InlineRecord(SymbolId id, InlineVariable var)
        : id(id), var(var) {}

InlineFrames::InlineFrames(Serializer &serializer)
        : m_serializer(serializer), m_params(), m_records() {}

void InlineFrames::open_call(
        std::vector<std::unique_ptr<ExpressionNode>> const &args, 
        std::vector<SymbolId> const &param_ids, bool writeback) {
    for (std::size_t i = 0; i < param_ids.size(); i++) {
        SymbolId id = param_ids[i];
        InlineParamMap::const_iterator iter = m_params.find(id);
        m_records.push(InlineRecord(
            id, iter == m_params.end() ? InlineVariable() : iter->second
        ));
        m_params[id] = InlineVariable(args[i].get(), false, false);
    }
    if (writeback) {
        args.front()->serialize_load_address(m_serializer);
        m_params[param_ids.front()].writeback = true;
    }
}

void InlineFrames::use(Serializer &serializer, SymbolId id) {
    InlineVariable var = get(id);
    if (var.writeback) {
        // for now, assume that address is at stack top
        // Future: either load by relative to stack top, or keep track of bp
        // Stack top is better as offset can often be optimized away.
        m_serializer.add_instr(OpCode::Dup);
        m_serializer.add_instr(OpCode::LoadAbs);
    } else {
        var.node->serialize(serializer);
    }
}

void InlineFrames::use_address(Serializer &serializer, SymbolId id) {
    InlineVariable var = get(id);
    if (var.writeback) {
        throw std::runtime_error("not implemented");
    } else {
        get(id).node->serialize_load_address(serializer);
    }
}

void InlineFrames::close_call(std::vector<SymbolId> const &param_ids) {
    bool writeback = false;
    for (SymbolIdList::const_reverse_iterator riter = param_ids.rbegin(); 
            riter != param_ids.rend(); riter++) {
        InlineRecord record = m_records.top();
        writeback = writeback || m_params[record.id].writeback;
        m_params[record.id] = record.var;
        m_records.pop();
    }
    if (writeback) {
        m_serializer.add_instr(OpCode::Binary, FuncCode::Assign);
    }
}

InlineVariable &InlineFrames::get(SymbolId id) {
    InlineParamMap::iterator iter = m_params.find(id);
    if (iter == m_params.end()) {
        throw std::runtime_error("todo");
    }
    if (iter->second.used) {
        throw std::runtime_error("Inline variable may only be used once");
    }
    iter->second.used = true;
    return iter->second;
}
