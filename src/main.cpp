#include "parser.hpp"
#include "serializer.hpp"
#include "program.hpp"
#include "argparser.hpp"
#include <iostream>
#include <fstream>

ArgParser get_args(int argc, char *argv[]) {
    ArgParser args;

    args.add("codefilename");

    args.add("tree", "", "", ArgType::Flag);
    args.add("stats", "", "", ArgType::Flag);
    args.add("dis", "", "", ArgType::Flag);
    args.add("symbols", "", "", ArgType::Flag);
    args.add("no-exec", "", "", ArgType::Flag);

    args.parse(argc, argv);

    return args;
}

std::vector<uint32_t> compile(ArgParser const &args) {
    std::string infilename = args.get(0).value;

    Parser parser(infilename);
    Serializer serializer;

    BaseNode *root = parser.parse();
    if (args.get("tree")) {
        std::cerr << "Syntax Tree:" << std::endl;
        BaseNode::print(root);
    }
    serializer.serialize(root);
    if (args.get("symbols")) {
        std::cerr << "Symbol Table:" << std::endl;
        serializer.symbol_table().dump();
    }
    if (args.get("dis")) {
        std::cerr << "Assembly:" << std::endl;
        serializer.disassemble();
    }
    return serializer.assemble();
}

void run_bytecode(ArgParser const &args, 
        std::vector<uint32_t> bytecode) {
    Program program = Program::load(bytecode);
    uint32_t exit_code = program.run();
    std::cout << "Program finished with exit code " 
            << exit_code << " (" 
            << static_cast<int32_t>(exit_code) << ")" << std::endl;
    if (args.get("stats")) {
        program.analytics();
    }
}

int main(int argc, char *argv[]) {
    try {
        ArgParser args = get_args(argc, argv);
        
        std::vector<uint32_t> bytecode = compile(args);
        if (!args.get("no-exec")) {
            run_bytecode(args, bytecode);
        }
    } catch (std::exception const &e) {
        std::cerr << "Error: " + std::string(e.what()) << std::endl;
        return 1;
    }
    return 0;
}
