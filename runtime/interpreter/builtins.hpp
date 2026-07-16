#ifndef BUILTINS_HPP
#define BUILTINS_HPP

#include "frame.hpp"
#include <map>
#include <cstdio>

// Intercepts and executes standard builtins. Returns true if function was a builtin, setting result out param.
bool executeBuiltin(const std::string& calleeName, 
                    const std::vector<std::string>& callArgs, 
                    StackFrame& frame, 
                    int currentIp,
                    const std::string& sourceName,
                    std::map<int, FILE*>& fileHandles,
                    int& nextFileHandle,
                    Value& result);

#endif // BUILTINS_HPP
