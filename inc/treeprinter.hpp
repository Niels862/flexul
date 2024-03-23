#ifndef FLEXUL_TREEPRINTER_HPP
#define FLEXUL_TREEPRINTER_HPP

#include <string>
#include <vector>
#include <memory>

class BaseNode;

class TreePrinter {
public:
    TreePrinter();
    
    void print_label(std::string const &label);
    void next_child(BaseNode *next);
    void last_child(BaseNode *last);

private:
    void print_label_prefix();
    void print_child(BaseNode *child);

    struct PrefixRecord {
        PrefixRecord(std::string label, std::string branch);

        std::string label;
        std::string branch;
    };

    std::vector<TreePrinter::PrefixRecord> m_prefixes;
};

#endif
