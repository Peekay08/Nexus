#ifndef FRAME_HPP
#define FRAME_HPP

#include <variant>
#include <vector>
#include <map>
#include <string>

using Value = std::variant<int, float, std::string>;

struct StackFrame {
    std::string funcName;
    std::map<std::string, Value> registers;
    std::map<std::string, Value> variables;
    std::map<std::string, int> labels;
    int ip;
};

inline std::string valueToString(const Value& val) {
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

// Global helper to resolve a operand string (register, string literal, float, integer)
Value resolveValue(const std::string& valStr, const StackFrame& frame, int ip, const std::string& sourceName);

#endif // FRAME_HPP
