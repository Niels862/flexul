#include "argparser.hpp"
#include <cctype>
#include <stdexcept>

Argument::Argument()
        : name(), alias(), value(), type(ArgType::Invalid) {}

Argument::Argument(std::string name, std::string alias, 
                   std::string value, ArgType type)
        : name(name), alias(alias), value(value), type(type) {}

Argument::operator bool() const {
    return !value.empty();
}

ArgParser::ArgParser()
        : m_positionals(), m_keywords(), m_keyword_map() {}

void ArgParser::add(std::string name) {
    m_positionals.push_back(Argument(name, "", "", ArgType::String));
}

void ArgParser::add(std::string const &name, std::string const &alias, 
        std::string const &default_value, ArgType type) {
    m_keyword_map[name] = m_keywords.size();
    if (!alias.empty()) {
        m_keyword_map[alias] = m_keywords.size();
    }
    m_keywords.push_back(Argument(name, alias, default_value, type));
}

Argument const &ArgParser::get(size_t i) const {
    if (i >= m_positionals.size()) {
        throw std::runtime_error("Undefined positional argument: " 
                                 + std::to_string(i));
    }
    return m_positionals[i];
}

Argument const &ArgParser::get(std::string const &name) const {
    return m_keywords[lookup_keyword(name)];
}

void ArgParser::parse(int argc, char *argv[]) {
    int i = 1;
    size_t argIndex;
    size_t positionalIndex = 0;
    for (i = 1; i < argc; i++) {
        char *str = argv[i];
        if (str[0] == '-') {
            if (str[1] == '-') { // long name: --argname
                argIndex = lookup_keyword(std::string(&str[2]));
            } else if (std::isalpha(str[1]) && str[2] == '\0') { // alias: -a
                argIndex = lookup_keyword(std::string(1, str[1]));
            } else {
                throw std::runtime_error("Expected argument value");
            }
            // Argument value found in next place for string argument
            if (m_keywords[argIndex].type == ArgType::String) {
                i++;
            }
            assign_arg(argc, argv, i, m_keywords[argIndex]);
        } else {
            if (positionalIndex >= m_positionals.size()) {
                throw std::runtime_error("Unexpected positional argument: " 
                                         + std::string(str));
            }
            assign_arg(argc, argv, i, m_positionals[positionalIndex]);
            positionalIndex++;
        }
    }

    for (Argument const &arg : m_positionals) {
        if (arg.value.empty()) {
            throw std::runtime_error("Positional argument has no value: " 
                                     + arg.name);
        }
    }
}

size_t ArgParser::lookup_keyword(std::string const &name) const {
    KeywordMap::const_iterator iter = m_keyword_map.find(name);
    if (iter == m_keyword_map.end()) {
        throw std::runtime_error("Undefined argument: " + name);
    }
    return iter->second;
}

void ArgParser::assign_arg(int argc, char *argv[], int i, Argument &arg) {
    if (arg.type == ArgType::Flag) {
        arg.value = "y";
        return;
    }
    if (arg.type != ArgType::String) {
        throw std::runtime_error("Unexpected argument type");
    }
    if (i >= argc || argv[i][0] == '-') {
        throw std::runtime_error("Expected argument value");
    }
    arg.value = std::string(argv[i]);
}
