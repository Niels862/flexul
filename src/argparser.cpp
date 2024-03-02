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
        : positionals(), keywords(), keywordMap() {}

void ArgParser::add(std::string name) {
    positionals.push_back(Argument(name, "", "", ArgType::String));
}

void ArgParser::add(std::string const &name, std::string const &alias, 
        std::string const &defaultValue, ArgType type) {
    keywordMap[name] = keywords.size();
    if (!alias.empty()) {
        keywordMap[alias] = keywords.size();
    }
    keywords.push_back(Argument(name, alias, defaultValue, type));
}

Argument const &ArgParser::get(size_t i) const {
    if (i >= positionals.size()) {
        throw std::runtime_error("Undefined positional argument: " 
                                 + std::to_string(i));
    }
    return positionals[i];
}

Argument const &ArgParser::get(std::string const &name) const {
    return keywords[lookupKeyword(name)];
}

void ArgParser::parse(int argc, char *argv[]) {
    int i = 1;
    size_t argIndex;
    size_t positionalIndex = 0;
    for (i = 1; i < argc; i++) {
        char *str = argv[i];
        if (str[0] == '-') {
            if (str[1] == '-') { // long name: --argname
                argIndex = lookupKeyword(std::string(&str[2]));
            } else if (std::isalpha(str[1]) && str[2] == '\0') { // alias: -a
                argIndex = lookupKeyword(std::string(1, str[1]));
            } else {
                throw std::runtime_error("Expected argument value");
            }
            // Argument value found in next place for string argument
            if (keywords[argIndex].type == ArgType::String) {
                i++;
            }
            assignArg(argc, argv, i, keywords[argIndex]);
        } else {
            if (positionalIndex >= positionals.size()) {
                throw std::runtime_error("Unexpected positional argument: " 
                                         + std::string(str));
            }
            assignArg(argc, argv, i, positionals[positionalIndex]);
            positionalIndex++;
        }
    }

    for (Argument const &arg : positionals) {
        if (arg.value.empty()) {
            throw std::runtime_error("Positional argument has no value: " 
                                     + arg.name);
        }
    }
}

size_t ArgParser::lookupKeyword(std::string const &name) const {
    KeywordMap::const_iterator iter = keywordMap.find(name);
    if (iter == keywordMap.end()) {
        throw std::runtime_error("Undefined argument: " + name);
    }
    return iter->second;
}

void ArgParser::assignArg(int argc, char *argv[], int i, Argument &arg) {
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
