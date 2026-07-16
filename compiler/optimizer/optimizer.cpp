#include "optimizer.hpp"
#include <map>
#include <set>

// Helper to check if a value is a float
static bool isFloatValue(const std::string& val) {
    return val.find('.') != std::string::npos;
}

void IROptimizer::optimize(IRProgram& program) {
    for (auto& func : program.functions) {
        optimizeFunction(func);
    }
}

void IROptimizer::optimizeFunction(IRFunction& func) {
    bool changed = true;
    int iterations = 0;
    // Run optimization passes in a loop until convergence (max 10 iterations to prevent infinite loops)
    while (changed && iterations < 10) {
        changed = false;
        changed |= runConstantFolding(func);
        changed |= runDeadCodeElimination(func);
        iterations++;
    }
}

bool IROptimizer::runConstantFolding(IRFunction& func) {
    bool changed = false;
    // Map register name -> constant value string (e.g. "%0" -> "5" or "3.14" or "\"hello\"")
    std::map<std::string, std::string> constRegs;
    // Map variable address -> constant value string (e.g. "%addr_a" -> "50")
    std::map<std::string, std::string> varValues;

    for (auto& inst : func.instructions) {
        if (inst.op == IROp::LABEL) {
            // Control flow boundary: clear variable constants tracking to be safe
            varValues.clear();
        } else if (inst.op == IROp::ALLOCA) {
            varValues.erase(inst.dest);
        } else if (inst.op == IROp::STORE) {
            // Check if stored value is a known constant register
            auto it = constRegs.find(inst.dest);
            if (it != constRegs.end()) {
                varValues[inst.arg1] = it->second;
            } else {
                // If it's a direct constant literal (like "5" or "3.14" or string)
                if (!inst.dest.empty() && inst.dest[0] != '%') {
                    varValues[inst.arg1] = inst.dest;
                } else {
                    // Stored value is dynamic, invalidate constant for this address
                    varValues.erase(inst.arg1);
                }
            }
        } else if (inst.op == IROp::LOAD) {
            // Check if loading from an address that has a known constant
            auto it = varValues.find(inst.arg1);
            if (it != varValues.end()) {
                // Replace LOAD with CONST
                inst.op = IROp::CONST;
                inst.arg1 = it->second;
                constRegs[inst.dest] = inst.arg1;
                changed = true;
            }
        } else if (inst.op == IROp::CONST) {
            constRegs[inst.dest] = inst.arg1;
        } else if (inst.op == IROp::ADD || inst.op == IROp::SUB || inst.op == IROp::MUL || inst.op == IROp::DIV || inst.op == IROp::MOD ||
                   inst.op == IROp::EQ || inst.op == IROp::NE || inst.op == IROp::LT || inst.op == IROp::LE || inst.op == IROp::GT || inst.op == IROp::GE) {
            // Check if both operands are known constants
            auto it1 = constRegs.find(inst.arg1);
            auto it2 = constRegs.find(inst.arg2);
            if (it1 != constRegs.end() && it2 != constRegs.end()) {
                std::string val1 = it1->second;
                std::string val2 = it2->second;

                // Check if string operand
                bool isStr1 = (val1[0] == '"');
                bool isStr2 = (val2[0] == '"');

                if (isStr1 || isStr2) {
                    if (inst.op == IROp::ADD && isStr1 && isStr2) {
                        // Fold string concatenation
                        std::string s1 = val1.substr(1, val1.length() - 2);
                        std::string s2 = val2.substr(1, val2.length() - 2);
                        inst.op = IROp::CONST;
                        inst.arg1 = "\"" + s1 + s2 + "\"";
                        inst.arg2 = "";
                        constRegs[inst.dest] = inst.arg1;
                        changed = true;
                    }
                } else {
                    bool isF1 = isFloatValue(val1);
                    bool isF2 = isFloatValue(val2);

                    if (isF1 || isF2) {
                        float f1 = std::stof(val1);
                        float f2 = std::stof(val2);
                        float res = 0.0f;
                        bool folded = true;
                        bool isComparison = false;
                        bool compRes = false;

                        switch (inst.op) {
                            case IROp::ADD: res = f1 + f2; break;
                            case IROp::SUB: res = f1 - f2; break;
                            case IROp::MUL: res = f1 * f2; break;
                            case IROp::DIV: 
                                if (f2 != 0.0f) res = f1 / f2; 
                                else folded = false; // Don't fold division by zero
                                break;
                            case IROp::EQ: compRes = (f1 == f2); isComparison = true; break;
                            case IROp::NE: compRes = (f1 != f2); isComparison = true; break;
                            case IROp::LT: compRes = (f1 < f2); isComparison = true; break;
                            case IROp::LE: compRes = (f1 <= f2); isComparison = true; break;
                            case IROp::GT: compRes = (f1 > f2); isComparison = true; break;
                            case IROp::GE: compRes = (f1 >= f2); isComparison = true; break;
                            default: folded = false; break;
                        }

                        if (folded) {
                            inst.op = IROp::CONST;
                            if (isComparison) {
                                inst.arg1 = compRes ? "1" : "0";
                            } else {
                                std::string s = std::to_string(res);
                                // clean trailing zeros
                                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                                if (s.back() == '.') s.pop_back();
                                inst.arg1 = s;
                            }
                            inst.arg2 = "";
                            constRegs[inst.dest] = inst.arg1;
                            changed = true;
                        }
                    } else {
                        int i1 = std::stoi(val1);
                        int i2 = std::stoi(val2);
                        int res = 0;
                        bool folded = true;
                        bool isComparison = false;
                        bool compRes = false;

                        switch (inst.op) {
                            case IROp::ADD: res = i1 + i2; break;
                            case IROp::SUB: res = i1 - i2; break;
                            case IROp::MUL: res = i1 * i2; break;
                            case IROp::DIV: 
                                if (i2 != 0) res = i1 / i2; 
                                else folded = false; 
                                break;
                            case IROp::MOD:
                                if (i2 != 0) res = i1 % i2;
                                else folded = false;
                                break;
                            case IROp::EQ: compRes = (i1 == i2); isComparison = true; break;
                            case IROp::NE: compRes = (i1 != i2); isComparison = true; break;
                            case IROp::LT: compRes = (i1 < i2); isComparison = true; break;
                            case IROp::LE: compRes = (i1 <= i2); isComparison = true; break;
                            case IROp::GT: compRes = (i1 > i2); isComparison = true; break;
                            case IROp::GE: compRes = (i1 >= i2); isComparison = true; break;
                            default: folded = false; break;
                        }

                        if (folded) {
                            inst.op = IROp::CONST;
                            if (isComparison) {
                                inst.arg1 = compRes ? "1" : "0";
                            } else {
                                inst.arg1 = std::to_string(res);
                            }
                            inst.arg2 = "";
                            constRegs[inst.dest] = inst.arg1;
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return changed;
}

bool IROptimizer::runDeadCodeElimination(IRFunction& func) {
    std::set<std::string> loadedAddrs;
    for (const auto& inst : func.instructions) {
        if (inst.op == IROp::LOAD) {
            loadedAddrs.insert(inst.arg1);
        }
    }

    std::set<std::string> usedRegs;
    // Collect all registers used in instructions
    for (const auto& inst : func.instructions) {
        if (inst.op == IROp::STORE) {
            // Uses source register and destination address register
            // Only count as used if the variable address is ever loaded
            if (loadedAddrs.find(inst.arg1) != loadedAddrs.end()) {
                if (!inst.dest.empty() && inst.dest[0] == '%') usedRegs.insert(inst.dest);
                if (!inst.arg1.empty() && inst.arg1[0] == '%') usedRegs.insert(inst.arg1);
            }
        } else if (inst.op == IROp::LOAD) {
            if (!inst.arg1.empty() && inst.arg1[0] == '%') usedRegs.insert(inst.arg1);
        } else if (inst.op == IROp::CBR) {
            if (!inst.dest.empty() && inst.dest[0] == '%') usedRegs.insert(inst.dest);
        } else if (inst.op == IROp::RET) {
            if (!inst.dest.empty() && inst.dest[0] == '%') usedRegs.insert(inst.dest);
        } else if (inst.op == IROp::PRINT) {
            if (!inst.arg1.empty() && inst.arg1[0] == '%') usedRegs.insert(inst.arg1);
        } else if (inst.op == IROp::CALL) {
            // CALL uses its arguments
            for (const auto& arg : inst.callArgs) {
                if (!arg.empty() && arg[0] == '%') {
                    usedRegs.insert(arg);
                }
            }
        } else {
            // For binary ops or const: they use their arguments
            if (!inst.arg1.empty() && inst.arg1[0] == '%') usedRegs.insert(inst.arg1);
            if (!inst.arg2.empty() && inst.arg2[0] == '%') usedRegs.insert(inst.arg2);
        }
    }

    // Identify and remove dead code
    std::vector<IRInstruction> newInsts;
    bool changed = false;

    for (const auto& inst : func.instructions) {
        bool shouldRemove = false;
        if (inst.op == IROp::STORE) {
            // Remove stores to variables that are never loaded
            if (loadedAddrs.find(inst.arg1) == loadedAddrs.end()) {
                shouldRemove = true;
            }
        } else if (inst.op == IROp::ALLOCA) {
            // Remove allocas of variables that are never loaded
            if (loadedAddrs.find(inst.dest) == loadedAddrs.end()) {
                shouldRemove = true;
            }
        } else {
            bool removable = false;
            if (inst.op == IROp::CONST || inst.op == IROp::LOAD ||
                inst.op == IROp::ADD || inst.op == IROp::SUB || inst.op == IROp::MUL || inst.op == IROp::DIV || inst.op == IROp::MOD ||
                inst.op == IROp::EQ || inst.op == IROp::NE || inst.op == IROp::LT || inst.op == IROp::LE || inst.op == IROp::GT || inst.op == IROp::GE) {
                removable = true;
            }

            if (removable && !inst.dest.empty() && usedRegs.find(inst.dest) == usedRegs.end()) {
                // Register is defined but never used, eliminate it!
                shouldRemove = true;
            }
        }

        if (shouldRemove) {
            changed = true;
        } else {
            newInsts.push_back(inst);
        }
    }

    if (changed) {
        func.instructions = newInsts;
    }
    return changed;
}
