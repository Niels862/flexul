#include "parser.hpp"
#include "serializer.hpp"
#include "program.hpp"
#include "argparse.hpp"
#include <iostream>
#include <fstream>

ArgParser get_args(int argc, char *argv[]) {
    ArgParser args;

    args.add("codefilename", ArgType::String);
    args.add_alias("c", "codefilename");
    args.add("binfilename", ArgType::String);
    args.add_alias("b", "binfilename");
    args.add("outfilename", ArgType::String);
    args.add_alias("o", "outfilename");
    args.add("tree", ArgType::Bool);
    args.add("stats", ArgType::Bool);
    args.add("dis", ArgType::Bool);
    args.add("symbols", ArgType::Bool);
    args.add("no-exec", ArgType::Bool);

    args.parse(argc, argv);

    return args;
}

std::vector<uint32_t> compile(ArgParser const &args) {
    std::string infilename = args.get("codefilename");
    if (infilename.empty()) {
        throw std::runtime_error("Code filename is not defined");
    }

    Parser parser(infilename);
    Serializer serializer;

    BaseNode *root = parser.parse();
    if (args.get_bool("tree")) {
        std::cerr << "Syntax Tree:" << std::endl;
        BaseNode::print(root);
    }
    serializer.serialize(root);
    if (args.get_bool("symbols")) {
        std::cerr << "Symbol Table:" << std::endl;
        serializer.symbol_table().dump();
    }
    if (args.get_bool("dis")) {
        std::cerr << "Assembly:" << std::endl;
        serializer.disassemble();
    }
    return serializer.assemble();
}

std::vector<uint32_t> read_bytecode_file(ArgParser const &args) {
    std::string infilename = args.get("binfilename");
    if (infilename.empty()) {
        throw std::runtime_error("Bin filename is not defined");
    }
    std::ifstream infile(infilename, std::ios::binary);
    if (!infile) {
        throw std::runtime_error("Could not open file: " + infilename);
    }
    std::vector<uint32_t> bytecode;
    uint32_t value = 0;
    while (infile.read(reinterpret_cast<char *>(&value), 4)) {
        bytecode.push_back(value);
        value = 0;
    }
    return bytecode;
}

void run_bytecode(ArgParser const &args, 
        std::vector<uint32_t> bytecode) {
    Program program = Program::load(bytecode);
    uint32_t exit_code = program.run();
    std::cout << "Program finished with exit code " 
            << exit_code << " (" 
            << static_cast<int32_t>(exit_code) << ")" << std::endl;
    if (args.get_bool("stats")) {
        program.analytics();
    }
}

void write_bytecode_file(ArgParser const &args, 
        std::vector<uint32_t> bytecode) {
    std::string infilename = args.get("outfilename");
    if (infilename.empty()) {
        throw std::runtime_error("Out filename is not defined");
    }
    std::ofstream outfile(infilename, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Could not open file: " + infilename);
    }
    for (uint32_t const bytecode_entry : bytecode) {
        outfile.write(reinterpret_cast<const char*>(&bytecode_entry), 4);
    }
}

int main(int argc, char *argv[]) {
    try {
        ArgParser args = get_args(argc, argv);
        
        std::vector<uint32_t> bytecode;

        if (args.get("codefilename").empty()) { // interpretation
            bytecode = read_bytecode_file(args);
        } else { // compilation (+interpretation if no outfile)
            bytecode = compile(args);
        }

        // Interpret bytecode
        if (args.get("outfilename").empty()) {
            if (!args.get_bool("no-exec")) {
                run_bytecode(args, bytecode);
            }
        } else { // Write bytecode to file
            write_bytecode_file(args, bytecode);
        }
    } catch (std::exception const &e) {
        std::cerr << "Error: " + std::string(e.what()) << std::endl;
        return 1;
    }
    return 0;
}
