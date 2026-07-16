#ifndef DUMP_IR_HPP
#define DUMP_IR_HPP

#include "../../compiler/lexer/lexer.hpp"
#include "../../compiler/parser/parser.hpp"
#include "../../compiler/ir/ir.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

inline bool runDumpIRCommand(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filePath << "'\n";
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();
    file.close();

    try {
        // Compile steps
        Lexer lexer(source, filePath);
        std::vector<Token> tokens = lexer.tokenize();

        for (const auto& token : tokens) {
            if (token.type == TokenType::TOK_ERROR) {
                std::cerr << token.value << "\n";
                return false;
            }
        }

        Parser parser(tokens, filePath);
        std::unique_ptr<ProgramNode> ast = parser.parse();

        IRGenerator irGen;
        IRProgram irProg = irGen.generate(*ast);

        IROptimizer optimizer;
        optimizer.optimize(irProg);

        // Output IR
        std::cout << irProg.toString();
        return true;

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
}

#endif // DUMP_IR_HPP
