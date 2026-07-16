#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include "compiler/ir/ir.hpp"

class IROptimizer {
public:
    void optimize(IRProgram& program);

private:
    void optimizeFunction(IRFunction& func);
    bool runConstantFolding(IRFunction& func);
    bool runDeadCodeElimination(IRFunction& func);
};

#endif // OPTIMIZER_HPP
