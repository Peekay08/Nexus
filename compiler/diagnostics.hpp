#ifndef DIAGNOSTICS_HPP
#define DIAGNOSTICS_HPP

#include <string>
#include <sstream>

inline std::string formatDiagnostic(
    const std::string& code,
    const std::string& errorType,
    const std::string& message,
    const std::string& filename,
    int line,
    int column
) {
    std::stringstream ss;
    ss << "[" << code << "] " << errorType << "\n\n"
       << message << "\n\n"
       << "--> " << filename << ":" << line << ":" << column;
    return ss.str();
}

inline std::string formatRuntimeDiagnostic(
    const std::string& code,
    const std::string& message,
    const std::string& filename,
    const std::string& functionName,
    int instructionIndex
) {
    std::stringstream ss;
    ss << "[" << code << "] Runtime Error\n\n"
       << message << "\n\n"
       << "--> " << filename << ":" << functionName << ":" << instructionIndex;
    return ss.str();
}

#endif // DIAGNOSTICS_HPP
