#include "vm.hpp"
#include "compiler/diagnostics.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <thread>
#include <chrono>

std::string valueToString(const Value& val) {
    if (std::holds_alternative<int>(val)) {
        return std::to_string(std::get<int>(val));
    } else if (std::holds_alternative<float>(val)) {
        std::string s = std::to_string(std::get<float>(val));
        // Trim trailing zeros for a clean float representation
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    } else {
        return std::get<std::string>(val);
    }
}

VM::VM(const IRProgram& program, const std::string& sourceName) : program(program), sourceName(sourceName) {
    for (const auto& func : program.functions) {
        functionTable[func.name] = func;
    }
}

VM::~VM() {
    for (auto const& [handle, f] : fileHandles) {
        if (f) {
            std::fclose(f);
        }
    }
}

Value VM::resolveValue(const std::string& valStr, const StackFrame& frame, int ip) {
    if (valStr.empty()) return 0;

    // Check if it's a register or stack address
    if (valStr[0] == '%') {
        auto itReg = frame.registers.find(valStr);
        if (itReg != frame.registers.end()) {
            return itReg->second;
        }
        auto itVar = frame.variables.find(valStr);
        if (itVar != frame.variables.end()) {
            return itVar->second;
        }
        
        // Uninitialized register
        throw VMException(formatRuntimeDiagnostic("NX-402", "Access to uninitialized register or variable: " + valStr, sourceName, frame.funcName, ip));
    }

    // Check if it's a string literal
    if (valStr[0] == '"' && valStr.back() == '"') {
        std::string raw = valStr.substr(1, valStr.length() - 2);
        // Translate escapes
        std::string resolved = "";
        for (size_t i = 0; i < raw.length(); ++i) {
            if (raw[i] == '\\' && i + 1 < raw.length()) {
                if (raw[i + 1] == 'n') resolved += '\n';
                else if (raw[i + 1] == 't') resolved += '\t';
                else resolved += raw[i + 1];
                i++;
            } else {
                resolved += raw[i];
            }
        }
        return resolved;
    }

    // Check if float
    if (valStr.find('.') != std::string::npos) {
        try {
            return std::stof(valStr);
        } catch (...) {
            return 0.0f;
        }
    }

    // Default to integer
    try {
        return std::stoi(valStr);
    } catch (...) {
        return 0;
    }
}

bool VM::isTrue(const Value& val) {
    if (std::holds_alternative<int>(val)) {
        return std::get<int>(val) != 0;
    } else if (std::holds_alternative<float>(val)) {
        return std::get<float>(val) != 0.0f;
    } else {
        return !std::get<std::string>(val).empty();
    }
}

void VM::setupLabels(IRFunction& func, StackFrame& frame) {
    for (size_t i = 0; i < func.instructions.size(); ++i) {
        if (func.instructions[i].op == IROp::LABEL) {
            frame.labels[func.instructions[i].dest] = static_cast<int>(i);
        }
    }
}

Value VM::execute(const std::string& entryFunction, const std::vector<Value>& args) {
    auto it = functionTable.find(entryFunction);
    if (it == functionTable.end()) {
        throw VMException(formatRuntimeDiagnostic("NX-404", "Entry function '" + entryFunction + "' not found.", sourceName, "unknown", 0));
    }

    IRFunction currentFunc = it->second;
    StackFrame entryFrame;
    entryFrame.funcName = entryFunction;
    entryFrame.ip = 0;
    setupLabels(currentFunc, entryFrame);

    // Map entry function parameters to values
    for (size_t i = 0; i < currentFunc.paramNames.size() && i < args.size(); ++i) {
        std::string regName = "%" + currentFunc.paramNames[i];
        entryFrame.registers[regName] = args[i];
    }

    callStack.clear();
    callStack.push_back(entryFrame);
    Value lastReturnValue = 0;

    while (!callStack.empty()) {
        StackFrame& frame = callStack.back();
        IRFunction& func = functionTable[frame.funcName];

        if (frame.ip >= static_cast<int>(func.instructions.size())) {
            // Function completed without return instruction
            callStack.pop_back();
            continue;
        }

        const IRInstruction& inst = func.instructions[frame.ip];
        int currentIp = frame.ip;
        frame.ip++; // advance

        switch (inst.op) {
            case IROp::ALLOCA: {
                frame.variables[inst.dest] = 0; // initialize
                break;
            }
            case IROp::STORE: {
                Value val = resolveValue(inst.dest, frame, currentIp);
                frame.variables[inst.arg1] = val;
                break;
            }
            case IROp::LOAD: {
                Value val = resolveValue(inst.arg1, frame, currentIp);
                frame.registers[inst.dest] = val;
                break;
            }
            case IROp::CONST: {
                Value val = resolveValue(inst.arg1, frame, currentIp);
                frame.registers[inst.dest] = val;
                break;
            }
            case IROp::ADD: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp);
                Value rhs = resolveValue(inst.arg2, frame, currentIp);
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
                Value lhs = resolveValue(inst.arg1, frame, currentIp);
                Value rhs = resolveValue(inst.arg2, frame, currentIp);
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
                Value lhs = resolveValue(inst.arg1, frame, currentIp);
                Value rhs = resolveValue(inst.arg2, frame, currentIp);
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
                Value lhs = resolveValue(inst.arg1, frame, currentIp);
                Value rhs = resolveValue(inst.arg2, frame, currentIp);
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
                Value lhs = resolveValue(inst.arg1, frame, currentIp);
                Value rhs = resolveValue(inst.arg2, frame, currentIp);
                if (std::holds_alternative<float>(lhs) || std::holds_alternative<float>(rhs)) {
                    float l = std::holds_alternative<float>(lhs) ? std::get<float>(lhs) : static_cast<float>(std::get<int>(lhs));
                    float r = std::holds_alternative<float>(rhs) ? std::get<float>(rhs) : static_cast<float>(std::get<int>(rhs));
                    if (r == 0.0f) {
                        throw VMException(formatRuntimeDiagnostic("NX-401", "Division by zero (float)", sourceName, frame.funcName, currentIp));
                    }
                    frame.registers[inst.dest] = l / r;
                } else {
                    int l = std::get<int>(lhs);
                    int r = std::get<int>(rhs);
                    if (r == 0) {
                        throw VMException(formatRuntimeDiagnostic("NX-401", "Division by zero (integer)", sourceName, frame.funcName, currentIp));
                    }
                    frame.registers[inst.dest] = l / r;
                }
                break;
            }
            case IROp::MOD: {
                Value lhs = resolveValue(inst.arg1, frame, currentIp);
                Value rhs = resolveValue(inst.arg2, frame, currentIp);
                if (std::holds_alternative<int>(lhs) && std::holds_alternative<int>(rhs)) {
                    int r = std::get<int>(rhs);
                    if (r == 0) {
                        throw VMException(formatRuntimeDiagnostic("NX-401", "Modulo division by zero (integer)", sourceName, frame.funcName, currentIp));
                    }
                    frame.registers[inst.dest] = std::get<int>(lhs) % r;
                } else {
                    throw VMException(formatRuntimeDiagnostic("NX-402", "Modulo operator requires integer operands.", sourceName, frame.funcName, currentIp));
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
                    throw VMException(formatRuntimeDiagnostic("NX-403", "Branch target label '" + inst.dest + "' not found.", sourceName, frame.funcName, currentIp));
                }
                break;
            }
            case IROp::CBR: {
                Value cond = resolveValue(inst.dest, frame, currentIp);
                std::string targetLabel = isTrue(cond) ? inst.arg1 : inst.arg2;
                auto itL = frame.labels.find(targetLabel);
                if (itL != frame.labels.end()) {
                    frame.ip = itL->second;
                } else {
                    throw VMException(formatRuntimeDiagnostic("NX-403", "Branch target label '" + targetLabel + "' not found.", sourceName, frame.funcName, currentIp));
                }
                break;
            }
            case IROp::CALL: {
                std::string calleeName = inst.arg1;
                
                // Intercept standard library built-ins
                if (calleeName == "sqrt" || calleeName == "pow" || calleeName == "abs" ||
                    calleeName == "sin" || calleeName == "cos" || calleeName == "system" ||
                    calleeName == "sleep" || calleeName == "file_open" || calleeName == "file_close" ||
                    calleeName == "file_write" || calleeName == "file_read") {
                    
                    Value res = 0;
                    if (calleeName == "sqrt") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: sqrt expects 1 argument.");
                        Value arg = resolveValue(inst.callArgs[0], frame, currentIp);
                        float val = std::holds_alternative<float>(arg) ? std::get<float>(arg) : static_cast<float>(std::get<int>(arg));
                        res = std::sqrt(val);
                    } else if (calleeName == "pow") {
                        if (inst.callArgs.size() != 2) throw std::runtime_error("VM Error: pow expects 2 arguments.");
                        Value base = resolveValue(inst.callArgs[0], frame, currentIp);
                        Value exp = resolveValue(inst.callArgs[1], frame, currentIp);
                        float b = std::holds_alternative<float>(base) ? std::get<float>(base) : static_cast<float>(std::get<int>(base));
                        float e = std::holds_alternative<float>(exp) ? std::get<float>(exp) : static_cast<float>(std::get<int>(exp));
                        res = std::pow(b, e);
                    } else if (calleeName == "abs") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: abs expects 1 argument.");
                        Value arg = resolveValue(inst.callArgs[0], frame, currentIp);
                        if (std::holds_alternative<float>(arg)) {
                            res = std::abs(std::get<float>(arg));
                        } else {
                            res = std::abs(std::get<int>(arg));
                        }
                    } else if (calleeName == "sin") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: sin expects 1 argument.");
                        Value arg = resolveValue(inst.callArgs[0], frame, currentIp);
                        float val = std::holds_alternative<float>(arg) ? std::get<float>(arg) : static_cast<float>(std::get<int>(arg));
                        res = std::sin(val);
                    } else if (calleeName == "cos") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: cos expects 1 argument.");
                        Value arg = resolveValue(inst.callArgs[0], frame, currentIp);
                        float val = std::holds_alternative<float>(arg) ? std::get<float>(arg) : static_cast<float>(std::get<int>(arg));
                        res = std::cos(val);
                    } else if (calleeName == "system") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: system expects 1 argument.");
                        Value arg = resolveValue(inst.callArgs[0], frame, currentIp);
                        std::string cmd = valueToString(arg);
                        res = std::system(cmd.c_str());
                    } else if (calleeName == "sleep") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: sleep expects 1 argument.");
                        Value arg = resolveValue(inst.callArgs[0], frame, currentIp);
                        int ms = std::holds_alternative<int>(arg) ? std::get<int>(arg) : static_cast<int>(std::get<float>(arg));
                        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                        res = 0;
                    } else if (calleeName == "file_open") {
                        if (inst.callArgs.size() != 2) throw std::runtime_error("VM Error: file_open expects 2 arguments.");
                        std::string filename = valueToString(resolveValue(inst.callArgs[0], frame, currentIp));
                        std::string mode = valueToString(resolveValue(inst.callArgs[1], frame, currentIp));
                        FILE* f = std::fopen(filename.c_str(), mode.c_str());
                        if (f) {
                            int handle = nextFileHandle++;
                            fileHandles[handle] = f;
                            res = handle;
                        } else {
                            res = -1;
                        }
                    } else if (calleeName == "file_close") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: file_close expects 1 argument.");
                        Value hVal = resolveValue(inst.callArgs[0], frame, currentIp);
                        int handle = std::holds_alternative<int>(hVal) ? std::get<int>(hVal) : static_cast<int>(std::get<float>(hVal));
                        auto itF = fileHandles.find(handle);
                        if (itF != fileHandles.end()) {
                            if (itF->second) std::fclose(itF->second);
                            fileHandles.erase(itF);
                            res = 0;
                        } else {
                            throw VMException(formatRuntimeDiagnostic("NX-405", "Invalid file handle for file_close: " + std::to_string(handle), sourceName, frame.funcName, currentIp));
                        }
                    } else if (calleeName == "file_write") {
                        if (inst.callArgs.size() != 2) throw std::runtime_error("VM Error: file_write expects 2 arguments.");
                        Value hVal = resolveValue(inst.callArgs[0], frame, currentIp);
                        int handle = std::holds_alternative<int>(hVal) ? std::get<int>(hVal) : static_cast<int>(std::get<float>(hVal));
                        std::string content = valueToString(resolveValue(inst.callArgs[1], frame, currentIp));
                        auto itF = fileHandles.find(handle);
                        if (itF != fileHandles.end() && itF->second) {
                            int written = std::fputs(content.c_str(), itF->second);
                            std::fflush(itF->second);
                            res = (written >= 0) ? static_cast<int>(content.length()) : -1;
                        } else {
                            throw VMException(formatRuntimeDiagnostic("NX-405", "Invalid file handle for file_write: " + std::to_string(handle), sourceName, frame.funcName, currentIp));
                        }
                    } else if (calleeName == "file_read") {
                        if (inst.callArgs.size() != 1) throw std::runtime_error("VM Error: file_read expects 1 argument.");
                        Value hVal = resolveValue(inst.callArgs[0], frame, currentIp);
                        int handle = std::holds_alternative<int>(hVal) ? std::get<int>(hVal) : static_cast<int>(std::get<float>(hVal));
                        auto itF = fileHandles.find(handle);
                        if (itF != fileHandles.end() && itF->second) {
                            char buffer[4096];
                            if (std::fgets(buffer, sizeof(buffer), itF->second)) {
                                res = std::string(buffer);
                            } else {
                                res = std::string("");
                            }
                        } else {
                            throw VMException(formatRuntimeDiagnostic("NX-405", "Invalid file handle for file_read: " + std::to_string(handle), sourceName, frame.funcName, currentIp));
                        }
                    }
 
                    if (!inst.dest.empty()) {
                        frame.registers[inst.dest] = res;
                    }
                    break;
                }
 
                auto itC = functionTable.find(calleeName);
                if (itC == functionTable.end()) {
                    throw VMException(formatRuntimeDiagnostic("NX-404", "Callee function '" + calleeName + "' not found.", sourceName, frame.funcName, currentIp));
                }
 
                IRFunction calleeFunc = itC->second;
                StackFrame calleeFrame;
                calleeFrame.funcName = calleeName;
                calleeFrame.ip = 0;
                setupLabels(calleeFunc, calleeFrame);
 
                // Map arguments
                for (size_t i = 0; i < calleeFunc.paramNames.size() && i < inst.callArgs.size(); ++i) {
                    Value val = resolveValue(inst.callArgs[i], frame, currentIp);
                    std::string regName = "%" + calleeFunc.paramNames[i];
                    calleeFrame.registers[regName] = val;
                }
 
                // Check call stack limit (recursion limit)
                if (callStack.size() >= 1000) {
                    throw VMException(formatRuntimeDiagnostic("NX-407", "Stack overflow: call stack limit (1000 frames) exceeded", sourceName, frame.funcName, currentIp));
                }

                // Push onto stack
                callStack.push_back(calleeFrame);
                break;
            }
            case IROp::RET: {
                Value retVal = resolveValue(inst.dest, frame, currentIp);
                lastReturnValue = retVal;
                
                // Pop the current frame
                if (callStack.empty()) {
                    throw VMException(formatRuntimeDiagnostic("NX-406", "Stack underflow: ret called on empty call stack", sourceName, "unknown", 0));
                }
                callStack.pop_back();
 
                // If caller exists, store return value in call instruction destination register
                if (!callStack.empty()) {
                    StackFrame& callerFrame = callStack.back();
                    IRFunction& callerFunc = functionTable[callerFrame.funcName];
                    // The instruction that made the call was at callerFrame.ip - 1
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
                Value val = resolveValue(inst.arg1, frame, currentIp);
                std::cout << valueToString(val);
                break;
            }
        }
    }
 
    return lastReturnValue;
}
