#include "callable.hpp"
#include "serializer.hpp"

CallableEntry::CallableEntry() 
        : overloads() {}

void CallableEntry::add_overload(CallableNode *overload) {
    overloads.push_back(overload);
}

void CallableEntry::call(Serializer &serializer, BaseNode *params) const {
    if (overloads.empty()) {
        throw std::runtime_error("No overloads declared for function");
    }
    CallableNode *overload = nullptr;
    if (params == nullptr) {
        if (overloads.size() != 1) {
            throw std::runtime_error("Overloads present for generic call");
        }
        overload = overloads[0];
    } else {
        for (CallableNode * const callable : overloads) {
            if (callable->is_matching_call(params)) {
                if (overload != nullptr) {
                    throw std::runtime_error("Multiple candidates for call");
                }
                overload = callable;
            }
        }
        if (overload == nullptr) {
            throw std::runtime_error("No suitable candidate for call");
        }
    }
    overload->serialize_call(serializer, params);
}

void CallableEntry::push_callable_addr(Serializer &serializer) const {
    if (overloads.size() != 1) {
        throw std::runtime_error(
                "Can only load address of single implmementation");
    }
    serializer.add_instr(OpCode::Push, overloads[0]->get_id(), true);
}
