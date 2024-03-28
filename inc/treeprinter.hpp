#ifndef FLEXUL_TREEPRINTER_HPP
#define FLEXUL_TREEPRINTER_HPP

#include <string>
#include <vector>

class BaseNode;

class TreePrinter {
public:
    TreePrinter(bool with_pointers, bool with_types, 
            bool with_symbol_ids);
    
    void print_node(BaseNode const *node);
    void next_child(BaseNode const *next);
    void last_child(BaseNode const *last);

private:
    void print_label_prefix();
    void print_child(BaseNode const *child);

    struct PrefixRecord {
        PrefixRecord(std::string label, std::string branch);

        std::string label;
        std::string branch;
    };

    std::vector<TreePrinter::PrefixRecord> m_prefixes;

    bool with_pointers;
    bool with_types;
    bool with_symbol_ids;
};

#endif
