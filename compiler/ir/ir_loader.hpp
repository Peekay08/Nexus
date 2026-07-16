#ifndef IR_LOADER_HPP
#define IR_LOADER_HPP

#include "ir.hpp"
#include <string>

class IRLoader {
public:
    static IRProgram loadFromString(const std::string& irText, const std::string& filePath = "inline_ir");
    static IRProgram loadFromFile(const std::string& filePath);
};

#endif // IR_LOADER_HPP
