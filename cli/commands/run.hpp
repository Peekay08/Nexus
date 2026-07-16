#ifndef RUN_HPP
#define RUN_HPP

#include "../../compiler/lexer/lexer.hpp"
#include "../../compiler/parser/parser.hpp"
#include "../../compiler/ir/ir.hpp"
#include "../../compiler/ir/ir_loader.hpp"
#include "../../runtime/interpreter/vm.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

inline bool runRunCommand(const std::string& filePath) {
    std::string source;
    bool isPrecompiledIR = false;

    // Check if precompiled IR
    if (filePath.length() > 4 && filePath.substr(filePath.length() - 4) == ".nxs") {
        isPrecompiledIR = true;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filePath << "'\n";
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    source = ss.str();
    file.close();

    try {
        IRProgram irProg;

        if (isPrecompiledIR) {
            std::cout << "[nxs-vm] Parsing and loading NEXUS IR from '" << filePath << "'...\n";
            irProg = IRLoader::loadFromFile(filePath);
        }

        if (!isPrecompiledIR) {
            // Compile to IR in-memory
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
            irProg = irGen.generate(*ast);

            // Run IR Optimizer
            IROptimizer optimizer;
            optimizer.optimize(irProg);
        }

        // Run on VM
        std::cout << "[nxs-vm] Booting NEXUS execution environment...\n";
        std::cout << "[nxs-vm] ------------------ EXECUTION START ------------------\n";

        VM vm(irProg, filePath);
        Value result = vm.execute("main");

        std::cout << "\n[nxs-vm] ------------------- EXECUTION END -------------------\n";
        std::cout << "[nxs-vm] Process returned: " << valueToString(result) << "\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[nxs-vm] VM Execution aborted:\n" << e.what() << "\n";
        return false;
    }
}

#endif // RUN_HPP
