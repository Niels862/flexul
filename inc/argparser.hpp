#include <string>
#include <vector>
#include <unordered_map>

#ifndef FLEXUL_ARGPARSER_HPP
#define FLEXUL_ARGPARSER_HPP

enum class ArgType {
    String, Flag, Invalid
};

using KeywordMap = std::unordered_map<std::string, size_t>;

struct Argument {
    Argument();
    Argument(std::string name, std::string alias, 
             std::string value, ArgType type);
    
    operator bool() const;

    std::string name;
    std::string alias;
    std::string value;
    ArgType type;
};

class ArgParser {
public:
    ArgParser();

    void add(std::string name);
    void add(std::string const &name, std::string const &alias, 
                    std::string const &default_value, ArgType type);
    Argument const &get(size_t i) const;
    Argument const &get(std::string const &name) const;

    void parse(int argc, char *argv[]);
private:
    size_t lookup_keyword(std::string const &name) const;
    void assign_arg(int argc, char *argv[], int i, Argument &arg);

    std::vector<Argument> m_positionals;
    std::vector<Argument> m_keywords;
    KeywordMap m_keyword_map;
};

#endif
