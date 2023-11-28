#include "argparse.hpp"
#include <stdexcept>
#include <cstring>

ArgEntry::ArgEntry()
        : type(ArgType::Invalid), value() {} 

ArgEntry::ArgEntry(ArgType type, std::string const &default_value)
        : type(type), value(default_value) {
    if (type == ArgType::Bool && default_value.empty()) {
        value = "F";
    }
}

void ArgEntry::set_parsed_value(std::string const &name, int &i, 
        char *str_end, int argc, char *argv[]) {
    if (type == ArgType::Bool) {
        if (*str_end == '\0') {
            value = "T";
            return;
        }
    }
    if (*str_end != '\0') {
        value = std::string(str_end);
    } else if (i + 1 < argc && argv[i + 1][0] != '-') {
        value = argv[i + 1];
        i++;
    } else {
        throw std::runtime_error("No value for argument: " + name);
    }
}

void ArgEntry::set_value(std::string const &value) {
    this->value = value;
}

std::string ArgEntry::get_value() const {
    return value;
}

ArgParser::ArgParser()
        : entries() {}

void ArgParser::parse(int argc, char *argv[]) {
    int i;
    for (i = 1; i < argc; i++) {
        std::string name;
        std::string value;
        ArgEntry entry;
        char *str_end;
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                name = std::string(argv[i] + 2);
                str_end = argv[i] + std::strlen(argv[i]);
            } else {
                name = std::string(1, argv[i][1]);
                str_end = argv[i] + 2;
            }
        } else {
            throw std::runtime_error("Unexpected argument: "
                    + std::string(argv[i]));
        }
        set_parsed_value(name, i, str_end, argc, argv);
    }
}

void ArgParser::add(std::string const &name, ArgType type,
        std::string const &default_value) {
    entries[name] = ArgEntry(type, default_value);
}

void ArgParser::add_synonym(std::string const &synonym, 
        std::string const &defined_name) {
    synonyms[synonym] = defined_name;
}

std::string ArgParser::get(std::string const &name) const {
    ArgEntryMap::const_iterator iter = entries.find(get_defined_name(name));
    if (iter == entries.end()) {
        throw std::runtime_error("Argument not found: " + name);
    }
    return iter->second.get_value();
}

std::string ArgParser::get_defined_name(std::string const &name) const {
    SynonymMap::const_iterator synonym_iter = synonyms.find(name);
    if (synonym_iter == synonyms.end()) {
        return name;
    }
    return synonym_iter->second;
}

void ArgParser::set_parsed_value(std::string const &name, int &i, 
        char *str_end, int argc, char *argv[]) {
    std::string defined_name = get_defined_name(name);
    ArgEntryMap::iterator iter = entries.find(defined_name);
    if (iter == entries.end()) {
        throw std::runtime_error("Unrecognized argument: " + defined_name);
    }
    iter->second.set_parsed_value(name, i, str_end, argc, argv);
}
