#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "../../compiler/lexer/lexer.hpp"
#include "../../compiler/parser/parser.hpp"
#include "../../compiler/ir/ir.hpp"
#include "../../compiler/ir/ir_loader.hpp"
#include "../../runtime/interpreter/vm.hpp"
#include "../../runtime/interpreter/frame.hpp"
#include "../../runtime/interpreter/builtins.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

class DebugVM {
public:
    IRProgram program;
    std::string sourceName;
    std::map<std::string, IRFunction> functionTable;
    std::vector<StackFrame> callStack;
    std::map<int, FILE*> fileHandles;
    int nextFileHandle = 1;
    Value lastReturnValue = 0;

    DebugVM(const IRProgram& program, const std::string& sourceName) : program(program), sourceName(sourceName) {
        for (const auto& func : program.functions) {
            functionTable[func.name] = func;
        }
    }

    ~DebugVM() {
        for (auto const& [handle, f] : fileHandles) {
            if (f) std::fclose(f);
        }
    }

    bool isTrue(const Value& val) {
        if (std::holds_alternative<int>(val)) {
            return std::get<int>(val) != 0;
        } else if (std::holds_alternative<float>(val)) {
            return std::get<float>(val) != 0.0f;
        } else {
            return !std::get<std::string>(val).empty();
        }
    }

    void setupLabels(IRFunction& func, StackFrame& frame) {
        for (size_t i = 0; i < func.instructions.size(); ++i) {
            if (func.instructions[i].op == IROp::LABEL) {
                frame.labels[func.instructions[i].dest] = static_cast<int>(i);
            }
        }
    }

    void init(const std::string& entryFunction) {
        auto it = functionTable.find(entryFunction);
        if (it == functionTable.end()) {
            throw std::runtime_error("Entry function '" + entryFunction + "' not found.");
        }
        IRFunction currentFunc = it->second;
        StackFrame entryFrame;
        entryFrame.funcName = entryFunction;
        entryFrame.ip = 0;
        setupLabels(currentFunc, entryFrame);
        callStack.push_back(entryFrame);
    }

    bool stepOneInstruction(bool verbose = true) {
        if (callStack.empty()) return false;

        StackFrame& frame = callStack.back();
        IRFunction& func = functionTable[frame.funcName];

        if (frame.ip >= static_cast<int>(func.instructions.size())) {
            callStack.pop_back();
            if (verbose) std::cout << "[debug] Completed function " << frame.funcName << "\n";
            return !callStack.empty();
        }

        const IRInstruction& inst = func.instructions[frame.ip];
        int currentIp = frame.ip;
        frame.ip++;

        if (verbose) {
            std::cout << "[debug-step] " << frame.funcName << ":" << currentIp << " -> " << inst.toString() << "\n";
        }

        switch (inst.op) {
            case IROp::ALLOCA: {
                frame.variables[inst.dest] = 0;
                break;
            }
            case IROp::STORE: {
                Value val = resolveValue(inst.dest, frame, currentIp, sourceName);
                frame.variables[inst.arg1] = val;
                break;
            }
            case IROp::LOAD: {
                Value val = resolveValue(inst.arg1, frame, currentIp, sourceName);
                frame.registers[inst.dest] = val;
                break;
            }
            case IROp::CONST: {
                Value val = resolveValue(inst.arg1, frame, currentIp, sourceName);
                frame.registers[inst.dest] = val;
                break;
            }
            case IROp::ADD: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp, sourceName);
                Value rhs = resolveValue(inst.arg2, frame, currentIp, sourceName);
                if (std::holds_alternative<std::string>(lhs) || std::holds_alternative<std::string>(rhs)) {
                    frame.registers[inst.dest] = valueToString(lhs) + valueToString(rhs);
                } else if (std::holds_alternative<float>(lhs) || std::holds_alternative<float>(rhs)) {
                    float l = std::holds_alternative<float>(lhs) ? std::get<float>(lhs) : static_cast<float>(std::get<int>(lhs));
                    float r = std::holds_alternative<float>(rhs) ? std::get<float>(rhs) : static_cast<float>(std::get<int>(rhs));
                    frame.registers[inst.dest] = l + r;
                } else {
                    frame.registers[inst.dest] = std::get<int>(lhs) + std::get<int>(rhs);
                }
                break;
            }
            case IROp::SUB: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp, sourceName);
                Value rhs = resolveValue(inst.arg2, frame, currentIp, sourceName);
                if (std::holds_alternative<float>(lhs) || std::holds_alternative<float>(rhs)) {
                    float l = std::holds_alternative<float>(lhs) ? std::get<float>(lhs) : static_cast<float>(std::get<int>(lhs));
                    float r = std::holds_alternative<float>(rhs) ? std::get<float>(rhs) : static_cast<float>(std::get<int>(rhs));
                    frame.registers[inst.dest] = l - r;
                } else {
                    frame.registers[inst.dest] = std::get<int>(lhs) - std::get<int>(rhs);
                }
                break;
            }
            case IROp::EQ:
            case IROp::NE:
            case IROp::LT:
            case IROp::LE:
            case IROp::GT:
            case IROp::GE: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp, sourceName);
                Value rhs = resolveValue(inst.arg2, frame, currentIp, sourceName);
                bool res = false;
                if (std::holds_alternative<std::string>(lhs) || std::holds_alternative<std::string>(rhs)) {
                    std::string l = valueToString(lhs);
                    std::string r = valueToString(rhs);
                    if (inst.op == IROp::EQ) res = (l == r);
                    else if (inst.op == IROp::NE) res = (l != r);
                    else if (inst.op == IROp::LT) res = (l < r);
                    else if (inst.op == IROp::LE) res = (l <= r);
                    else if (inst.op == IROp::GT) res = (l > r);
                    else if (inst.op == IROp::GE) res = (l >= r);
                } else if (std::holds_alternative<float>(lhs) || std::holds_alternative<float>(rhs)) {
                    float l = std::holds_alternative<float>(lhs) ? std::get<float>(lhs) : static_cast<float>(std::get<int>(lhs));
                    float r = std::holds_alternative<float>(rhs) ? std::get<float>(rhs) : static_cast<float>(std::get<int>(rhs));
                    if (inst.op == IROp::EQ) res = (l == r);
                    else if (inst.op == IROp::NE) res = (l != r);
                    else if (inst.op == IROp::LT) res = (l < r);
                    else if (inst.op == IROp::LE) res = (l <= r);
                    else if (inst.op == IROp::GT) res = (l > r);
                    else if (inst.op == IROp::GE) res = (l >= r);
                } else {
                    int l = std::get<int>(lhs);
                    int r = std::get<int>(rhs);
                    if (inst.op == IROp::EQ) res = (l == r);
                    else if (inst.op == IROp::NE) res = (l != r);
                    else if (inst.op == IROp::LT) res = (l < r);
                    else if (inst.op == IROp::LE) res = (l <= r);
                    else if (inst.op == IROp::GT) res = (l > r);
                    else if (inst.op == IROp::GE) res = (l >= r);
                }
                frame.registers[inst.dest] = res ? 1 : 0;
                break;
            }
            case IROp::MUL: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp, sourceName);
                Value rhs = resolveValue(inst.arg2, frame, currentIp, sourceName);
                if (std::holds_alternative<float>(lhs) || std::holds_alternative<float>(rhs)) {
                    float l = std::holds_alternative<float>(lhs) ? std::get<float>(lhs) : static_cast<float>(std::get<int>(lhs));
                    float r = std::holds_alternative<float>(rhs) ? std::get<float>(rhs) : static_cast<float>(std::get<int>(rhs));
                    frame.registers[inst.dest] = l * r;
                } else {
                    frame.registers[inst.dest] = std::get<int>(lhs) * std::get<int>(rhs);
                }
                break;
            }
            case IROp::DIV: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp, sourceName);
                Value rhs = resolveValue(inst.arg2, frame, currentIp, sourceName);
                if (std::holds_alternative<float>(lhs) || std::holds_alternative<float>(rhs)) {
                    float l = std::holds_alternative<float>(lhs) ? std::get<float>(lhs) : static_cast<float>(std::get<int>(lhs));
                    float r = std::holds_alternative<float>(rhs) ? std::get<float>(rhs) : static_cast<float>(std::get<int>(rhs));
                    if (r == 0.0f) {
                        throw std::runtime_error("Division by zero (float)");
                    }
                    frame.registers[inst.dest] = l / r;
                } else {
                    int l = std::get<int>(lhs);
                    int r = std::get<int>(rhs);
                    if (r == 0) {
                        throw std::runtime_error("Division by zero (integer)");
                    }
                    frame.registers[inst.dest] = l / r;
                }
                break;
            }
            case IROp::MOD: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp, sourceName);
                Value rhs = resolveValue(inst.arg2, frame, currentIp, sourceName);
                if (std::holds_alternative<int>(lhs) && std::holds_alternative<int>(rhs)) {
                    int r = std::get<int>(rhs);
                    if (r == 0) {
                        throw std::runtime_error("Modulo division by zero (integer)");
                    }
                    frame.registers[inst.dest] = std::get<int>(lhs) % r;
                } else {
                    throw std::runtime_error("Modulo operator requires integer operands.");
                }
                break;
            }
            case IROp::LABEL: {
                break;
            }
            case IROp::BR: {
                auto itL = frame.labels.find(inst.dest);
                if (itL != frame.labels.end()) {
                    frame.ip = itL->second;
                } else {
                    throw std::runtime_error("Branch target label '" + inst.dest + "' not found.");
                }
                break;
            }
            case IROp::CBR: {
                Value cond = resolveValue(inst.dest, frame, currentIp, sourceName);
                std::string targetLabel = isTrue(cond) ? inst.arg1 : inst.arg2;
                auto itL = frame.labels.find(targetLabel);
                if (itL != frame.labels.end()) {
                    frame.ip = itL->second;
                } else {
                    throw std::runtime_error("Branch target label '" + targetLabel + "' not found.");
                }
                break;
            }
            case IROp::CALL: {
                std::string calleeName = inst.arg1;
                Value builtinResult = 0;
                
                if (executeBuiltin(calleeName, inst.callArgs, frame, currentIp, sourceName, fileHandles, nextFileHandle, builtinResult)) {
                    if (!inst.dest.empty()) {
                        frame.registers[inst.dest] = builtinResult;
                    }
                    break;
                }
 
                auto itC = functionTable.find(calleeName);
                if (itC == functionTable.end()) {
                    throw std::runtime_error("Callee function '" + calleeName + "' not found.");
                }
 
                IRFunction calleeFunc = itC->second;
                StackFrame calleeFrame;
                calleeFrame.funcName = calleeName;
                calleeFrame.ip = 0;
                setupLabels(calleeFunc, calleeFrame);
 
                for (size_t i = 0; i < calleeFunc.paramNames.size() && i < inst.callArgs.size(); ++i) {
                    Value val = resolveValue(inst.callArgs[i], frame, currentIp, sourceName);
                    std::string regName = "%" + calleeFunc.paramNames[i];
                    calleeFrame.registers[regName] = val;
                }
 
                if (callStack.size() >= 1000) {
                    throw std::runtime_error("Stack overflow: call stack limit exceeded");
                }
                callStack.push_back(calleeFrame);
                break;
            }
            case IROp::RET: {
                Value retVal = resolveValue(inst.dest, frame, currentIp, sourceName);
                lastReturnValue = retVal;
                
                if (callStack.empty()) {
                    throw std::runtime_error("Stack underflow: ret called on empty call stack");
                }
                callStack.pop_back();
 
                if (!callStack.empty()) {
                    StackFrame& callerFrame = callStack.back();
                    IRFunction& callerFunc = functionTable[callerFrame.funcName];
                    int callIdx = callerFrame.ip - 1;
                    if (callIdx >= 0 && callIdx < static_cast<int>(callerFunc.instructions.size())) {
                        const IRInstruction& callInst = callerFunc.instructions[callIdx];
                        if (callInst.op == IROp::CALL && !callInst.dest.empty()) {
                            callerFrame.registers[callInst.dest] = retVal;
                        }
                    }
                }
                break;
            }
            case IROp::PRINT: {
                Value val = resolveValue(inst.arg1, frame, currentIp, sourceName);
                std::cout << valueToString(val);
                break;
            }
        }

        if (verbose && !inst.dest.empty() && inst.dest[0] == '%' && inst.op != IROp::STORE) {
            try {
                std::cout << "  ↓\n";
                std::cout << "  " << inst.dest << " = " << valueToString(frame.registers[inst.dest]) << "\n";
            } catch (...) {}
        }

        return !callStack.empty();
    }
};

inline bool runDebugCommand(const std::string& filePath) {
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
            irProg = IRLoader::loadFromFile(filePath);
        } else {
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

        std::cout << "=========================================================\n";
        std::cout << " 🛠️ NEXUS Interactive Step Debugger\n";
        std::cout << "=========================================================\n";
        std::cout << "Commands:\n";
        std::cout << "  step (s)      - Step one instruction (or press Enter)\n";
        std::cout << "  regs (r)      - Print active registers in current frame\n";
        std::cout << "  vars (v)      - Print variables in current frame\n";
        std::cout << "  stack (st)    - Print call stack trace\n";
        std::cout << "  continue (c)  - Run program to completion\n";
        std::cout << "  quit (q)      - Quit debugger\n\n";

        DebugVM dbg(irProg, filePath);
        dbg.init("main");

        std::string cmd;
        bool running = true;
        while (running && !dbg.callStack.empty()) {
            std::cout << "nxs-dbg> ";
            if (!std::getline(std::cin, cmd)) break;
            
            if (cmd == "step" || cmd == "s" || cmd.empty()) {
                running = dbg.stepOneInstruction(true);
            } else if (cmd == "regs" || cmd == "r") {
                const auto& frame = dbg.callStack.back();
                std::cout << "Registers in " << frame.funcName << ":\n";
                if (frame.registers.empty()) std::cout << "  (none)\n";
                for (const auto& [name, val] : frame.registers) {
                    std::cout << "  " << name << " = " << valueToString(val) << "\n";
                }
            } else if (cmd == "vars" || cmd == "v") {
                const auto& frame = dbg.callStack.back();
                std::cout << "Variables in " << frame.funcName << ":\n";
                if (frame.variables.empty()) std::cout << "  (none)\n";
                for (const auto& [name, val] : frame.variables) {
                    std::cout << "  " << name << " = " << valueToString(val) << "\n";
                }
            } else if (cmd == "stack" || cmd == "st") {
                std::cout << "Call Stack Trace:\n";
                for (int i = static_cast<int>(dbg.callStack.size()) - 1; i >= 0; --i) {
                    std::cout << "  [" << i << "] " << dbg.callStack[i].funcName << " (ip: " << dbg.callStack[i].ip << ")\n";
                }
            } else if (cmd == "continue" || cmd == "c") {
                std::cout << "[debug] Running to completion...\n";
                while (dbg.stepOneInstruction(false)) {}
                running = false;
            } else if (cmd == "quit" || cmd == "q") {
                std::cout << "[debug] Quitting debugger.\n";
                break;
            } else {
                std::cout << "Unknown command. Try: step (s), regs (r), vars (v), stack (st), continue (c), quit (q).\n";
            }
        }

        std::cout << "[debug] Execution finished. Result: " << valueToString(dbg.lastReturnValue) << "\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[debug] Debugging session failed: " << e.what() << "\n";
        return false;
    }
}

#endif // DEBUG_HPP
