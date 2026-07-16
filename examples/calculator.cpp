#include "nexus_builtins.h"
#include <iostream>

int calculate(int op, int a, int b) {
    if (op == 1) { // Add
        return a + b;
    }
    if (op == 2) { // Subtract
        return a - b;
    }
    if (op == 3) { // Multiply
        return a * b;
    }
    if (op == 4) { // Divide
        if (b == 0) {
            std::cout << "Error: Division by zero" << std::endl;
            return 0;
        }
        return a / b;
    }
    return 0;
}

int main() {
    int op = 3; // Multiply
    int a = 6;
    int b = 7;
    int result = calculate(op, a, b);
    std::cout << "Calculator result: " << result << std::endl;
    return result;
}
