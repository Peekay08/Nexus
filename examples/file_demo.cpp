#include "nexus_builtins.h"
#include <iostream>

int main() {
    std::cout << "Opening demo.txt for writing..." << std::endl;
    int handle = file_open("demo.txt", "w");
    if (handle < 0) {
        std::cout << "Failed to open demo.txt" << std::endl;
        return 1;
    }
    
    std::cout << "Writing to demo.txt..." << std::endl;
    file_write(handle, "NEXUS 1.0 File IO Built-ins work perfectly!\n");
    file_close(handle);
    
    std::cout << "Opening demo.txt for reading..." << std::endl;
    int readHandle = file_open("demo.txt", "r");
    if (readHandle < 0) {
        std::cout << "Failed to open demo.txt for reading" << std::endl;
        return 1;
    }
    
    std::cout << "File contents:" << std::endl;
    std::string content = file_read(readHandle);
    std::cout << content;
    file_close(readHandle);
    
    return 0;
}
