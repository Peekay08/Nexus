#ifndef NEXUS_BUILTINS_H
#define NEXUS_BUILTINS_H

#include <string>
#include <cmath>
#include <cstdlib>

// Dummy declarations for C++ compilers and IntelliSense to resolve NEXUS VM built-in functions
using std::sqrt;
using std::pow;
using std::abs;

inline int file_open(const std::string& /*filename*/, const std::string& /*mode*/) { return 0; }
inline int file_write(int /*handle*/, const std::string& /*content*/) { return 0; }
inline std::string file_read(int /*handle*/) { return ""; }
inline int file_close(int /*handle*/) { return 0; }

#endif // NEXUS_BUILTINS_H
