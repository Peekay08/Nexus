#include "ir_loader.hpp"
#include "compiler/diagnostics.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <set>

static std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

static std::vector<std::string> splitByComma(const std::string& s) {
    std::vector<std::string> result;
    std::string current = "";
    bool inQuotes = false;
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (c == '"') {
            if (i > 0 && s[i - 1] == '\\') {
                // escaped quote, keep it
            } else {
                inQuotes = !inQuotes;
            }
            current += c;
        } else if (c == ',' && !inQuotes) {
            result.push_back(trim(current));
            current = "";
        } else {
            current += c;
        }
    }
    result.push_back(trim(current));

    std::vector<std::string> filtered;
    for (const auto& item : result) {
        if (!item.empty()) {
            filtered.push_back(item);
        }
    }
    return filtered;
}

static bool isValidRegisterFormat(const std::string& reg) {
    if (reg.empty() || reg[0] != '%') return false;
    if (reg.length() < 2) return false;
    return true; // Simple prefix check
}

IRProgram IRLoader::loadFromString(const std::string& irText, const std::string& filePath) {
    IRProgram program;
    std::stringstream ss(irText);
    std::string line;
    IRFunction* currentFunc = nullptr;
    int nxsLineNum = 0;
    bool checkedHeader = false;

    while (std::getline(ss, line)) {
        nxsLineNum++;
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty()) {
            continue;
        }

        if (!checkedHeader) {
            if (trimmedLine[0] == ';' && trimmedLine != "; NEXUS IR v1.0") {
                throw std::runtime_error(formatDiagnostic("NX-301", "IR Loader Error", "Invalid version header comment: " + trimmedLine + ". Expected '; NEXUS IR v1.0'", filePath, nxsLineNum, 1));
            }
            if (trimmedLine[0] != ';') {
                throw std::runtime_error(formatDiagnostic("NX-301", "IR Loader Error", "Missing version header comment. Expected '; NEXUS IR v1.0' as first statement", filePath, nxsLineNum, 1));
            }
            checkedHeader = true;
            continue;
        }

        if (trimmedLine[0] == ';') {
            continue; // skip other comments
        }

        if (trimmedLine.rfind("define ", 0) == 0) {
            // Function definition: define <returnType> @<funcName>(<params>) {
            size_t defineLen = 7; // length of "define "
            size_t atPos = trimmedLine.find('@');
            if (atPos == std::string::npos) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid function definition (missing '@'): " + trimmedLine, filePath, nxsLineNum, 1));
            }

            std::string returnType = trim(trimmedLine.substr(defineLen, atPos - defineLen));
            size_t lparenPos = trimmedLine.find('(', atPos);
            if (lparenPos == std::string::npos) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid function definition (missing '('): " + trimmedLine, filePath, nxsLineNum, 1));
            }

            std::string funcName = trim(trimmedLine.substr(atPos + 1, lparenPos - (atPos + 1)));
            
            size_t rparenPos = trimmedLine.find(')', lparenPos);
            if (rparenPos == std::string::npos) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid function definition (missing ')'): " + trimmedLine, filePath, nxsLineNum, 1));
            }

            std::string paramsStr = trimmedLine.substr(lparenPos + 1, rparenPos - lparenPos - 1);
            std::vector<std::string> params;
            if (!trim(paramsStr).empty()) {
                std::vector<std::string> rawParams = splitByComma(paramsStr);
                for (auto& p : rawParams) {
                    p = trim(p);
                    if (!p.empty() && p[0] == '%') {
                        p = p.substr(1); // strip leading '%'
                    }
                    params.push_back(p);
                }
            }

            // Create function
            IRFunction fn;
            fn.name = funcName;
            fn.returnType = returnType;
            fn.paramNames = params;
            
            program.functions.push_back(fn);
            currentFunc = &program.functions.back();
            continue;
        }

        if (trimmedLine == "}") {
            currentFunc = nullptr;
            continue;
        }

        if (currentFunc == nullptr) {
            continue; // ignore code outside functions
        }

        // Parse instruction
        IRInstruction inst;
        
        // 1. Label definition?
        if (trimmedLine.back() == ':') {
            inst.op = IROp::LABEL;
            inst.dest = trim(trimmedLine.substr(0, trimmedLine.length() - 1));
            currentFunc->instructions.push_back(inst);
            continue;
        }

        // 2. Branch, store, print, or ret?
        if (trimmedLine.rfind("br label %", 0) == 0) {
            inst.op = IROp::BR;
            inst.dest = trim(trimmedLine.substr(10));
            currentFunc->instructions.push_back(inst);
            continue;
        }

        if (trimmedLine.rfind("cbr ", 0) == 0) {
            inst.op = IROp::CBR;
            std::string rest = trim(trimmedLine.substr(4)); // after "cbr "
            std::vector<std::string> parts = splitByComma(rest);
            if (parts.size() < 3) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid cbr instruction: " + trimmedLine, filePath, nxsLineNum, 1));
            }
            inst.dest = parts[0]; // cond register
            if (!isValidRegisterFormat(inst.dest)) {
                throw std::runtime_error(formatDiagnostic("NX-303", "IR Loader Error", "Invalid condition register format in cbr: " + inst.dest, filePath, nxsLineNum, 1));
            }
            
            std::string thenStr = parts[1];
            size_t labelPos1 = thenStr.find("label %");
            if (labelPos1 != std::string::npos) {
                inst.arg1 = trim(thenStr.substr(labelPos1 + 7));
            } else {
                inst.arg1 = thenStr;
            }

            std::string elseStr = parts[2];
            size_t labelPos2 = elseStr.find("label %");
            if (labelPos2 != std::string::npos) {
                inst.arg2 = trim(elseStr.substr(labelPos2 + 7));
            } else {
                inst.arg2 = elseStr;
            }

            currentFunc->instructions.push_back(inst);
            continue;
        }

        if (trimmedLine.rfind("store ", 0) == 0) {
            inst.op = IROp::STORE;
            std::string rest = trim(trimmedLine.substr(6)); // after "store "
            std::vector<std::string> parts = splitByComma(rest);
            if (parts.size() < 2) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid store instruction: " + trimmedLine, filePath, nxsLineNum, 1));
            }
            inst.dest = parts[0]; // val register or constant
            inst.arg1 = parts[1]; // addr register
            if (!isValidRegisterFormat(inst.arg1)) {
                throw std::runtime_error(formatDiagnostic("NX-303", "IR Loader Error", "Invalid address register format in store: " + inst.arg1, filePath, nxsLineNum, 1));
            }
            currentFunc->instructions.push_back(inst);
            continue;
        }

        if (trimmedLine.rfind("print ", 0) == 0) {
            inst.op = IROp::PRINT;
            inst.arg1 = trim(trimmedLine.substr(6));
            currentFunc->instructions.push_back(inst);
            continue;
        }

        if (trimmedLine.rfind("ret", 0) == 0) {
            inst.op = IROp::RET;
            if (trimmedLine.length() > 3 && trimmedLine[3] == ' ') {
                inst.dest = trim(trimmedLine.substr(4));
            } else {
                inst.dest = "";
            }
            currentFunc->instructions.push_back(inst);
            continue;
        }

        // 3. Assignment: <dest> = <op> <args>
        size_t eqPos = trimmedLine.find('=');
        if (eqPos == std::string::npos) {
            throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Unknown or malformed IR instruction: " + trimmedLine, filePath, nxsLineNum, 1));
        }

        inst.dest = trim(trimmedLine.substr(0, eqPos));
        if (!isValidRegisterFormat(inst.dest)) {
            throw std::runtime_error(formatDiagnostic("NX-303", "IR Loader Error", "Invalid destination register format: " + inst.dest, filePath, nxsLineNum, 1));
        }

        std::string rightHand = trim(trimmedLine.substr(eqPos + 1));

        if (rightHand.rfind("alloca ", 0) == 0) {
            inst.op = IROp::ALLOCA;
            inst.arg1 = trim(rightHand.substr(7));
        } else if (rightHand.rfind("load ", 0) == 0) {
            inst.op = IROp::LOAD;
            inst.arg1 = trim(rightHand.substr(5));
            if (!isValidRegisterFormat(inst.arg1)) {
                throw std::runtime_error(formatDiagnostic("NX-303", "IR Loader Error", "Invalid source address register format in load: " + inst.arg1, filePath, nxsLineNum, 1));
            }
        } else if (rightHand.rfind("const ", 0) == 0) {
            inst.op = IROp::CONST;
            inst.arg1 = trim(rightHand.substr(6));
        } else if (rightHand.rfind("call @", 0) == 0) {
            inst.op = IROp::CALL;
            size_t atPos = rightHand.find('@');
            size_t lparenPos = rightHand.find('(', atPos);
            if (atPos == std::string::npos || lparenPos == std::string::npos) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid call instruction syntax: " + trimmedLine, filePath, nxsLineNum, 1));
            }
            inst.arg1 = trim(rightHand.substr(atPos + 1, lparenPos - (atPos + 1)));

            size_t rparenPos = rightHand.find_last_of(')');
            if (rparenPos == std::string::npos || rparenPos < lparenPos) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid call instruction (missing closing parenthesis): " + trimmedLine, filePath, nxsLineNum, 1));
            }

            std::string argsStr = rightHand.substr(lparenPos + 1, rparenPos - lparenPos - 1);
            if (!trim(argsStr).empty()) {
                std::vector<std::string> rawArgs = splitByComma(argsStr);
                for (const auto& a : rawArgs) {
                    inst.callArgs.push_back(trim(a));
                }
            }
        } else {
            // Binary operations: <opName> <arg1>, <arg2>
            size_t spacePos = rightHand.find(' ');
            if (spacePos == std::string::npos) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Invalid binary operation syntax: " + trimmedLine, filePath, nxsLineNum, 1));
            }

            std::string opName = trim(rightHand.substr(0, spacePos));
            std::string argsStr = trim(rightHand.substr(spacePos + 1));

            if (opName == "add") inst.op = IROp::ADD;
            else if (opName == "sub") inst.op = IROp::SUB;
            else if (opName == "mul") inst.op = IROp::MUL;
            else if (opName == "div") inst.op = IROp::DIV;
            else if (opName == "mod") inst.op = IROp::MOD;
            else if (opName == "eq") inst.op = IROp::EQ;
            else if (opName == "ne") inst.op = IROp::NE;
            else if (opName == "lt") inst.op = IROp::LT;
            else if (opName == "le") inst.op = IROp::LE;
            else if (opName == "gt") inst.op = IROp::GT;
            else if (opName == "ge") inst.op = IROp::GE;
            else {
                throw std::runtime_error(formatDiagnostic("NX-303", "IR Loader Error", "Unknown instruction operator: '" + opName + "'", filePath, nxsLineNum, 1));
            }

            std::vector<std::string> parts = splitByComma(argsStr);
            if (parts.size() < 2) {
                throw std::runtime_error(formatDiagnostic("NX-302", "IR Loader Error", "Binary operation expects 2 arguments: " + trimmedLine, filePath, nxsLineNum, 1));
            }
            inst.arg1 = parts[0];
            inst.arg2 = parts[1];
        }

        currentFunc->instructions.push_back(inst);
    }

    // Semantic verification: check label references
    for (const auto& func : program.functions) {
        std::set<std::string> definedLabels;
        for (const auto& inst : func.instructions) {
            if (inst.op == IROp::LABEL) {
                if (definedLabels.count(inst.dest)) {
                    throw std::runtime_error(formatDiagnostic("NX-305", "IR Loader Error", "Duplicate label '" + inst.dest + "' definition in function '" + func.name + "'", filePath, 1, 1));
                }
                definedLabels.insert(inst.dest);
            }
        }

        for (const auto& inst : func.instructions) {
            if (inst.op == IROp::BR) {
                if (!definedLabels.count(inst.dest)) {
                    throw std::runtime_error(formatDiagnostic("NX-304", "IR Loader Error", "Undefined jump target label '" + inst.dest + "' in function '" + func.name + "'", filePath, 1, 1));
                }
            } else if (inst.op == IROp::CBR) {
                if (!definedLabels.count(inst.arg1)) {
                    throw std::runtime_error(formatDiagnostic("NX-304", "IR Loader Error", "Undefined jump target label '" + inst.arg1 + "' in function '" + func.name + "'", filePath, 1, 1));
                }
                if (!definedLabels.count(inst.arg2)) {
                    throw std::runtime_error(formatDiagnostic("NX-304", "IR Loader Error", "Undefined jump target label '" + inst.arg2 + "' in function '" + func.name + "'", filePath, 1, 1));
                }
            }
        }
    }

    return program;
}

IRProgram IRLoader::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error(formatDiagnostic("NX-306", "IR Loader Error", "Could not open IR file: " + filePath, filePath, 0, 0));
    }
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    return loadFromString(ss.str(), filePath);
}
