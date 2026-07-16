#include "builtins.hpp"
#include "vm.hpp"
#include "compiler/diagnostics.hpp"
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>

Value resolveValue(const std::string& valStr, const StackFrame& frame, int ip, const std::string& sourceName) {
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

bool executeBuiltin(const std::string& calleeName, 
                    const std::vector<std::string>& callArgs, 
                    StackFrame& frame, 
                    int currentIp,
                    const std::string& sourceName,
                    std::map<int, FILE*>& fileHandles,
                    int& nextFileHandle,
                    Value& result) {
    if (calleeName != "sqrt" && calleeName != "pow" && calleeName != "abs" &&
        calleeName != "sin" && calleeName != "cos" && calleeName != "system" &&
        calleeName != "sleep" && calleeName != "file_open" && calleeName != "file_close" &&
        calleeName != "file_write" && calleeName != "file_read") {
        return false;
    }

    if (calleeName == "sqrt") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: sqrt expects 1 argument.");
        Value arg = resolveValue(callArgs[0], frame, currentIp, sourceName);
        float val = std::holds_alternative<float>(arg) ? std::get<float>(arg) : static_cast<float>(std::get<int>(arg));
        result = std::sqrt(val);
    } else if (calleeName == "pow") {
        if (callArgs.size() != 2) throw std::runtime_error("VM Error: pow expects 2 arguments.");
        Value base = resolveValue(callArgs[0], frame, currentIp, sourceName);
        Value exp = resolveValue(callArgs[1], frame, currentIp, sourceName);
        float b = std::holds_alternative<float>(base) ? std::get<float>(base) : static_cast<float>(std::get<int>(base));
        float e = std::holds_alternative<float>(exp) ? std::get<float>(exp) : static_cast<float>(std::get<int>(exp));
        result = std::pow(b, e);
    } else if (calleeName == "abs") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: abs expects 1 argument.");
        Value arg = resolveValue(callArgs[0], frame, currentIp, sourceName);
        if (std::holds_alternative<float>(arg)) {
            result = std::abs(std::get<float>(arg));
        } else {
            result = std::abs(std::get<int>(arg));
        }
    } else if (calleeName == "sin") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: sin expects 1 argument.");
        Value arg = resolveValue(callArgs[0], frame, currentIp, sourceName);
        float val = std::holds_alternative<float>(arg) ? std::get<float>(arg) : static_cast<float>(std::get<int>(arg));
        result = std::sin(val);
    } else if (calleeName == "cos") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: cos expects 1 argument.");
        Value arg = resolveValue(callArgs[0], frame, currentIp, sourceName);
        float val = std::holds_alternative<float>(arg) ? std::get<float>(arg) : static_cast<float>(std::get<int>(arg));
        result = std::cos(val);
    } else if (calleeName == "system") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: system expects 1 argument.");
        Value arg = resolveValue(callArgs[0], frame, currentIp, sourceName);
        std::string cmd = valueToString(arg);
        result = std::system(cmd.c_str());
    } else if (calleeName == "sleep") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: sleep expects 1 argument.");
        Value arg = resolveValue(callArgs[0], frame, currentIp, sourceName);
        int ms = std::holds_alternative<int>(arg) ? std::get<int>(arg) : static_cast<int>(std::get<float>(arg));
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        result = 0;
    } else if (calleeName == "file_open") {
        if (callArgs.size() != 2) throw std::runtime_error("VM Error: file_open expects 2 arguments.");
        std::string filename = valueToString(resolveValue(callArgs[0], frame, currentIp, sourceName));
        std::string mode = valueToString(resolveValue(callArgs[1], frame, currentIp, sourceName));
        FILE* f = std::fopen(filename.c_str(), mode.c_str());
        if (f) {
            int handle = nextFileHandle++;
            fileHandles[handle] = f;
            result = handle;
        } else {
            result = -1;
        }
    } else if (calleeName == "file_close") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: file_close expects 1 argument.");
        Value hVal = resolveValue(callArgs[0], frame, currentIp, sourceName);
        int handle = std::holds_alternative<int>(hVal) ? std::get<int>(hVal) : static_cast<int>(std::get<float>(hVal));
        auto itF = fileHandles.find(handle);
        if (itF != fileHandles.end()) {
            if (itF->second) std::fclose(itF->second);
            fileHandles.erase(itF);
            result = 0;
        } else {
            throw VMException(formatRuntimeDiagnostic("NX-405", "Invalid file handle for file_close: " + std::to_string(handle), sourceName, frame.funcName, currentIp));
        }
    } else if (calleeName == "file_write") {
        if (callArgs.size() != 2) throw std::runtime_error("VM Error: file_write expects 2 arguments.");
        Value hVal = resolveValue(callArgs[0], frame, currentIp, sourceName);
        int handle = std::holds_alternative<int>(hVal) ? std::get<int>(hVal) : static_cast<int>(std::get<float>(hVal));
        std::string content = valueToString(resolveValue(callArgs[1], frame, currentIp, sourceName));
        auto itF = fileHandles.find(handle);
        if (itF != fileHandles.end() && itF->second) {
            int written = std::fputs(content.c_str(), itF->second);
            std::fflush(itF->second);
            result = (written >= 0) ? static_cast<int>(content.length()) : -1;
        } else {
            throw VMException(formatRuntimeDiagnostic("NX-405", "Invalid file handle for file_write: " + std::to_string(handle), sourceName, frame.funcName, currentIp));
        }
    } else if (calleeName == "file_read") {
        if (callArgs.size() != 1) throw std::runtime_error("VM Error: file_read expects 1 argument.");
        Value hVal = resolveValue(callArgs[0], frame, currentIp, sourceName);
        int handle = std::holds_alternative<int>(hVal) ? std::get<int>(hVal) : static_cast<int>(std::get<float>(hVal));
        auto itF = fileHandles.find(handle);
        if (itF != fileHandles.end() && itF->second) {
            char buffer[4096];
            if (std::fgets(buffer, sizeof(buffer), itF->second)) {
                result = std::string(buffer);
            } else {
                result = std::string("");
            }
        } else {
            throw VMException(formatRuntimeDiagnostic("NX-405", "Invalid file handle for file_read: " + std::to_string(handle), sourceName, frame.funcName, currentIp));
        }
    }

    return true;
}
