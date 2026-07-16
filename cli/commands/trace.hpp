#ifndef TRACE_HPP
#define TRACE_HPP

#include "../../compiler/lexer/lexer.hpp"
#include "../../compiler/parser/parser.hpp"
#include "../../compiler/ir/ir.hpp"
#include "../../compiler/ir/ir_loader.hpp"
#include "../../runtime/interpreter/vm.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

inline bool runTraceCommand(const std::string& filePath) {
    std::string source;
    bool isPrecompiledIR = false;

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
            std::cout << "[nxs-trace] Loading NEXUS IR from '" << filePath << "'...\n";
            irProg = IRLoader::loadFromFile(filePath);
        } else {
            // Compile and optimize
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

            IROptimizer optimizer;
            optimizer.optimize(irProg);
        }

        std::cout << "[nxs-trace] Executing program with verbose tracing:\n";
        std::cout << "=========================================================\n";

        VM vm(irProg, filePath);
        // Execute with traceMode = true
        Value result = vm.execute("main", {}, true);

        std::cout << "=========================================================\n";
        std::cout << "[nxs-trace] Process returned: " << valueToString(result) << "\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[nxs-trace] VM Execution aborted:\n" << e.what() << "\n";
        return false;
    }
}

#endif // TRACE_HPP
