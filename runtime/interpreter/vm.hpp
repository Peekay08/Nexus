#ifndef VM_HPP
#define VM_HPP

#include "frame.hpp"
#include "../../compiler/ir/ir.hpp"
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>

class VMException : public std::runtime_error {
public:
    explicit VMException(const std::string& message) : std::runtime_error(message) {}
};

class VM {
public:
    explicit VM(const IRProgram& program, const std::string& sourceName = "runtime");
    ~VM();
    
    // Execute VM program. Set traceMode to true to log each executed instruction.
    Value execute(const std::string& entryFunction = "main", const std::vector<Value>& args = {}, bool traceMode = false);

private:
    IRProgram program;
    std::string sourceName;
    std::map<std::string, IRFunction> functionTable;
    std::vector<StackFrame> callStack;

    std::map<int, FILE*> fileHandles;
    int nextFileHandle = 1;

    // Helper functions
    bool isTrue(const Value& val);
    void setupLabels(IRFunction& func, StackFrame& frame);
};

#endif // VM_HPP
