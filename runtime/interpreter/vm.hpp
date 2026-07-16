#ifndef VM_HPP
#define VM_HPP

#include "../../compiler/ir/ir.hpp"
#include <variant>
#include <vector>
#include <map>
#include <string>
#include <cstdio>
#include <stdexcept>

using Value = std::variant<int, float, std::string>;

std::string valueToString(const Value& val);

struct StackFrame {
    std::string funcName;
    std::map<std::string, Value> registers;  // registers like %0, %1
    std::map<std::string, Value> variables;  // allocated stack memory variables like %addr_x
    std::map<std::string, int> labels;       // maps label names to instruction indices
    int ip;                                  // instruction pointer
};

class VMException : public std::runtime_error {
public:
    explicit VMException(const std::string& message) : std::runtime_error(message) {}
};

class VM {
public:
    explicit VM(const IRProgram& program, const std::string& sourceName = "runtime");
    ~VM();
    Value execute(const std::string& entryFunction = "main", const std::vector<Value>& args = {});

private:
    IRProgram program;
    std::string sourceName;
    std::map<std::string, IRFunction> functionTable;
    std::vector<StackFrame> callStack;

    std::map<int, FILE*> fileHandles;
    int nextFileHandle = 1;

    // Helper functions
    Value resolveValue(const std::string& valStr, const StackFrame& frame, int ip);
    bool isTrue(const Value& val);
    void setupLabels(IRFunction& func, StackFrame& frame);
};

#endif // VM_HPP
