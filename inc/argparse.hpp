#ifndef FLEXUL_ARGPARSE
#define FLEXUL_ARGPARSE

#include <vector>
#include <string>
#include <unordered_map>

enum class ArgType {
    Invalid, String, Bool, Number
};

class ArgEntry;

using ArgEntryMap = std::unordered_map<std::string, ArgEntry>;

using SynonymMap = std::unordered_map<std::string, std::string>;

class ArgEntry {
public:
    ArgEntry();
    ArgEntry(ArgType type, std::string const &default_value);

    void set_parsed_value(std::string const &name, int &i, 
            char *str_end, int argc, char *argv[]);
    void set_value(std::string const &value);
    std::string get_value() const;
private:
    ArgType type;
    std::string value;
};

class ArgParser {
public:
    ArgParser();
    void parse(int argc, char *argv[]);
    void add(std::string const &name, ArgType type,
            std::string const &default_value = "");
    void add_synonym(std::string const &synonym, 
            std::string const &defined_name);
    std::string get(std::string const &name) const;
private:
    std::string get_defined_name(std::string const &name) const;
    void set_parsed_value(std::string const &name, int &i, 
            char *str_end, int argc, char *argv[]);

    ArgEntryMap entries;
    SynonymMap synonyms;
};

#endif
