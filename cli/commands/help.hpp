#ifndef HELP_HPP
#define HELP_HPP

#include <iostream>

inline void printHelp() {
    std::cout << "=========================================================\n";
    std::cout << " 🚀  NEXUS: Universal Compilation & Execution Platform   \n";
    std::cout << "=========================================================\n\n";
    std::cout << "Usage: nxs <command> [arguments] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  build <file.cpp>           Compile C++ code into NEXUS IR (.nxs)\n";
    std::cout << "  run <file.cpp|file.nxs>    Execute program within NEXUS VM\n";
    std::cout << "  doctor                     Verify development environment and compiler setup\n";
    std::cout << "  dump-ir <file.cpp>         Compile C++ to IR and print to stdout\n";
    std::cout << "  stats <file.cpp>           Measure compile time, execution time, and memory\n";
    std::cout << "  trace <file.cpp|file.nxs>  Execute program with verbose register tracing\n";
    std::cout << "  debug <file.cpp|file.nxs>  Launch interactive instruction-level step debugger\n";
    std::cout << "  target list                List all compilation and hardware targets\n";
    std::cout << "  help                       Display help and usage instructions\n";
    std::cout << "  version                    Display CLI compiler version\n\n";
    std::cout << "Options:\n";
    std::cout << "  --target=<arch>            Specify destination compilation target\n\n";
}

#endif // HELP_HPP
