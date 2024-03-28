#include "treeprinter.hpp"
#include "tree.hpp"
#include <iostream>

TreePrinter::TreePrinter(bool with_pointers, bool with_types, 
        bool with_symbol_ids)
        : m_prefixes(), with_pointers(with_pointers), with_types(with_types), 
        with_symbol_ids(with_symbol_ids) {}

void TreePrinter::print_node(BaseNode const *node) {
    print_label_prefix();
    std::cout << node->label();
    if (with_pointers) {
        std::cout << " [p=" << node << "]";
    }
    if (with_types && 0) {
        std::cout << " [type=" << "todo" << "]";
    }
    if (with_symbol_ids && node->id()) {
        std::cout << " [id=" << node->id() << "]";
    }
    std::cout << std::endl;
}

void TreePrinter::next_child(BaseNode const *next) {
    m_prefixes.push_back(PrefixRecord("├─", "│ "));
    print_child(next);
    m_prefixes.pop_back();
}

void TreePrinter::last_child(BaseNode const *last) {
    m_prefixes.push_back(PrefixRecord("╰─", "  "));
    print_child(last);
    m_prefixes.pop_back();
}

void TreePrinter::print_label_prefix() {
    for (std::size_t i = 0; i < m_prefixes.size(); i++) {
        if (i == m_prefixes.size() - 1) {
            std::cout << m_prefixes[i].label;
        } else {
            std::cout << m_prefixes[i].branch;
        }
    }
}

void TreePrinter::print_child(BaseNode const *child) {
    if (child == nullptr) {
        print_label_prefix();
        std::cout << "(null)" << std::endl;
    } else {
        child->print(*this);
    }
}

TreePrinter::PrefixRecord::PrefixRecord(std::string label, std::string branch)
        : label(label), branch(branch) {}
