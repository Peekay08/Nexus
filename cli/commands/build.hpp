#ifndef BUILD_HPP
#define BUILD_HPP

#include "../../compiler/lexer/lexer.hpp"
#include "../../compiler/parser/parser.hpp"
#include "../../compiler/ir/ir.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

inline bool runBuildCommand(const std::string& filePath, const std::string& target = "nexus-ir") {
    std::cout << "[nxs] Starting build pipeline for " << filePath << "...\n";
    std::cout << "[nxs] Target architecture: " << target << "\n";

    // 1. Read input source
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
        // 2. Lexical analysis
        std::cout << "[nxs] [1/3] Running Lexical Analyzer...\n";
        Lexer lexer(source, filePath);
        std::vector<Token> tokens = lexer.tokenize();

        // Check for lexer errors
        for (const auto& token : tokens) {
            if (token.type == TokenType::TOK_ERROR) {
                std::cerr << token.value << "\n";
                return false;
            }
        }

        // 3. Syntax analysis (Parser)
        std::cout << "[nxs] [2/3] Constructing Abstract Syntax Tree (AST)...\n";
        Parser parser(tokens, filePath);
        std::unique_ptr<ProgramNode> ast = parser.parse();

        // 4. IR Generation
        std::cout << "[nxs] [3/3] Emitting NEXUS Intermediate Representation (IR)...\n";
        IRGenerator irGen;
        IRProgram irProg = irGen.generate(*ast);

        // 4.5. Optimize IR
        std::cout << "[nxs] Running IR Optimizer (Constant folding & Dead code elimination)...\n";
        IROptimizer optimizer;
        optimizer.optimize(irProg);

        // Get output filename
        size_t dotIndex = filePath.find_last_of(".");
        std::string baseName = (dotIndex == std::string::npos) ? filePath : filePath.substr(0, dotIndex);
        std::string outPath = baseName + ".nxs";

        // Save IR
        std::ofstream outFile(outPath);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open output file '" << outPath << "' for writing.\n";
            return false;
        }
        outFile << irProg.toString();
        outFile.close();

        std::cout << "=========================================================\n";
        std::cout << " 🎉 NEXUS IR generated successfully: " << outPath << "\n";
        std::cout << "=========================================================\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        std::cerr << "[nxs] Build failed due to compiler errors.\n";
        return false;
    }
}

#endif // BUILD_HPP
