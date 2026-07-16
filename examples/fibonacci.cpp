#include "nexus_builtins.h"
#include <iostream>

int main() {
    int limit = 10;
    int a = 0;
    int b = 1;
    int i = 0;
    
    std::cout << "Fibonacci sequence up to term 10:" << std::endl;
    while (i < limit) {
        std::cout << a << std::endl;
        int temp = a + b;
        a = b;
        b = temp;
        i = i + 1;
    }
    return a;
}
