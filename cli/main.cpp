#include "commands/help.hpp"
#include "commands/build.hpp"
#include "commands/run.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
        return 0;
    }

    if (command == "version" || command == "--version" || command == "-v") {
        std::cout << "NEXUS Compiler Version 1.0.0\n";
        return 0;
    }

    if (command == "target") {
        if (argc >= 3 && std::string(argv[2]) == "list") {
            std::cout << "Available targets:\n";
            std::cout << "  nexus-ir            NEXUS Intermediate Representation (VM Interpreter) [default]\n";
            std::cout << "  x86_64-windows      Windows x64 Native Executable [under development]\n";
            std::cout << "  x86_64-linux        Linux ELF Native Binary [under development]\n";
            std::cout << "  aarch64-android     Android ARM64 Native Binary [under development]\n";
            return 0;
        }
        std::cerr << "Error: Unknown target command. Did you mean 'nxs target list'?\n";
        return 1;
    }

    if (command == "build") {
        if (argc < 3) {
            std::cerr << "Error: 'build' command requires a file path.\n";
            std::cerr << "Usage: nxs build <file.cpp> [--target=<arch>]\n";
            return 1;
        }
        std::string filePath = argv[2];
        std::string target = "nexus-ir";

        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--target=", 0) == 0) {
                target = arg.substr(9);
            }
        }

        bool success = runBuildCommand(filePath, target);
        return success ? 0 : 1;
    }

    if (command == "run") {
        if (argc < 3) {
            std::cerr << "Error: 'run' command requires a file path.\n";
            std::cerr << "Usage: nxs run <file.cpp|file.nxs>\n";
            return 1;
        }
        std::string filePath = argv[2];
        bool success = runRunCommand(filePath);
        return success ? 0 : 1;
    }

    std::cerr << "Error: Unknown command '" << command << "'. Type 'nxs help' for usage.\n";
    return 1;
}
