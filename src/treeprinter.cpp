#include "treeprinter.hpp"
#include "tree.hpp"
#include <iostream>

TreePrinter::TreePrinter()
        : m_prefixes() {}

void TreePrinter::print_label(std::string const &label) {
    print_label_prefix();
    std::cout << label << std::endl;
}

void TreePrinter::next_child(BaseNode *next) {
    m_prefixes.push_back(PrefixRecord("├─", "│ "));
    print_child(next);
    m_prefixes.pop_back();
}

void TreePrinter::last_child(BaseNode *last) {
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

void TreePrinter::print_child(BaseNode *child) {
    if (child == nullptr) {
        print_label("(null)");
    } else {
        child->print(*this);
    }
}

TreePrinter::PrefixRecord::PrefixRecord(std::string label, std::string branch)
        : label(label), branch(branch) {}
