#ifndef STATS_HPP
#define STATS_HPP

#include "../../compiler/lexer/lexer.hpp"
#include "../../compiler/parser/parser.hpp"
#include "../../compiler/ir/ir.hpp"
#include "../../runtime/interpreter/vm.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#undef CONST
#include <psapi.h>
inline size_t getPeakMemory() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize;
    }
    return 0;
}
#else
inline size_t getPeakMemory() { return 0; }
#endif

inline bool runStatsCommand(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filePath << "'\n";
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();
    file.close();

    std::cout << "[nxs-stats] Benchmarking execution for " << filePath << "...\n\n";

    try {
        // Measure compilation
        auto compStart = std::chrono::high_resolution_clock::now();

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

        auto compEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> compDuration = compEnd - compStart;

        // Calculate metrics
        size_t instructionCount = 0;
        for (const auto& f : irProg.functions) {
            instructionCount += f.instructions.size();
        }

        // Measure execution
        VM vm(irProg, filePath);
        
        auto execStart = std::chrono::high_resolution_clock::now();
        Value result = vm.execute("main");
        auto execEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> execDuration = execEnd - execStart;

        size_t peakMem = getPeakMemory();

        std::cout << "=========================================\n";
        std::cout << " 📊 NEXUS Performance Metrics Summary\n";
        std::cout << "=========================================\n";
        std::cout << "  Compile Time:          " << compDuration.count() << " ms\n";
        std::cout << "  VM Execution Time:     " << execDuration.count() << " ms\n";
        std::cout << "  Optimized Instruction Count: " << instructionCount << "\n";
        if (peakMem > 0) {
            std::cout << "  Peak Memory Usage:     " << (peakMem / 1024) << " KB\n";
        } else {
            std::cout << "  Peak Memory Usage:     N/A\n";
        }
        std::cout << "  VM Return Value:       " << valueToString(result) << "\n";
        std::cout << "-----------------------------------------\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Stats Benchmarking aborted: " << e.what() << "\n";
        return false;
    }
}

#endif // STATS_HPP
