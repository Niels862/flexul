#include "parser.hpp"
#include "serializer.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[]) {
    std::string infilename, outfilename;
    if (argc < 2) {
        infilename = "in.fx";
    } else {
        infilename = argv[1];
    }
    if (argc < 3) {
        outfilename = "out.bin";
    } else {
        outfilename = argv[2];
    }
    
    std::ifstream infile(infilename);
    if (!infile) {
        std::cout << "Failed to open " << infilename << std::endl;
        return 1;
    }
    std::ofstream outfile(outfilename);
    if (!outfile) {
        std::cout << "Failed to open " << outfilename << std::endl;
        return 1;
    }

    Parser parser(infile);
    Serializer serializer;
    
    serializer.add_instr(OpCode::Push)
              .add_data(0);
    serializer.add_instr(OpCode::Push)
              .add_data().with_label(100);
    serializer.add_instr(OpCode::Call);
    serializer.add_instr(OpCode::SysCall, FuncCode::Exit);
    serializer.add_instr(OpCode::Push).with_label(100)
              .add_data(1);
    serializer.add_instr(OpCode::Unary, FuncCode::Neg);
    serializer.add_instr(OpCode::Ret);

    serializer.assemble(outfile);

    // try {
    //     BaseNode *node = parser.parse();
    //     BaseNode::print(node);
    //     return 1;
    // } catch (std::exception const &e) {
    //     std::cerr << e.what() << std::endl;
    //     return 1;
    // }
    return 0;
}
